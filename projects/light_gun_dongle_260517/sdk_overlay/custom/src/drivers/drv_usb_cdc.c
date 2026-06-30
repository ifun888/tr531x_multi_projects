#include "drivers/drv_usb_cdc.h"
#include <stdint.h>
#include <stddef.h>

#define USB_BUF_SZ 1024U
#define USB_FLUSH_CHUNK_SZ 64U

extern int usb_serial_read(uint32_t index, char *buffer, size_t buflen) __attribute__((weak));
extern int usb_serial_write(uint32_t index, const char *buffer, size_t buflen) __attribute__((weak));

static unsigned char g_rx_buf[USB_BUF_SZ];
static unsigned char g_tx_buf[USB_BUF_SZ];
static unsigned int g_rx_r = 0;
static unsigned int g_rx_w = 0;
static unsigned int g_tx_r = 0;
static unsigned int g_tx_w = 0;
static int g_opened = 0;
static int g_ready = 0;

static uint32_t usb_rb_push(const uint8_t *buf, uint32_t len, unsigned char *ring,
    unsigned int *r, unsigned int *w)
{
    uint32_t i;
    uint32_t pushed = 0U;

    if (buf == 0) {
        return 0U;
    }
    for (i = 0; i < len; i++) {
        unsigned int next = (*w + 1U) % USB_BUF_SZ;
        if (next == *r) {
            break;
        }
        ring[*w] = buf[i];
        *w = next;
        pushed++;
    }
    return pushed;
}

static uint32_t usb_rb_contig_count(unsigned int r, unsigned int w)
{
    if (w >= r) {
        return (uint32_t)(w - r);
    }
    return (uint32_t)(USB_BUF_SZ - r);
}

static void usb_rb_consume(unsigned int *r, uint32_t len)
{
    *r = (unsigned int)((*r + len) % USB_BUF_SZ);
}

static int usb_try_flush_tx(void)
{
    while (g_tx_r != g_tx_w) {
        uint32_t chunk = usb_rb_contig_count(g_tx_r, g_tx_w);
        int wrote;

        if (chunk > USB_FLUSH_CHUNK_SZ) {
            chunk = USB_FLUSH_CHUNK_SZ;
        }

        wrote = (int)usb_serial_write(0, (const char *)&g_tx_buf[g_tx_r], (size_t)chunk);
        if (wrote > 0) {
            g_ready = 1;
            usb_rb_consume(&g_tx_r, (uint32_t)wrote);
            if ((uint32_t)wrote < chunk) {
                return 0;
            }
            continue;
        }
        if (wrote < 0) {
            g_ready = 0;
        }
        return -1;
    }

    return 0;
}

static int usb_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_ready = 0;
    g_rx_r = 0;
    g_rx_w = 0;
    g_tx_r = 0;
    g_tx_w = 0;
    return 0;
}

static int usb_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int usb_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n = 0;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }

    (void)usb_try_flush_tx();

    {
        int r = (int)usb_serial_read(0, (char *)buf, (size_t)len);
        if (r > 0) {
            g_ready = 1;
            *out_len = (uint32_t)r;
            return 0;
        }
        if (r < 0) {
            g_ready = 0;
        }
    }

    while ((n < len) && (g_rx_r != g_rx_w)) {
        buf[n++] = g_rx_buf[g_rx_r];
        g_rx_r = (g_rx_r + 1U) % USB_BUF_SZ;
    }
    *out_len = n;
    return 0;
}

static int usb_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }

    if (g_tx_r != g_tx_w) {
        (void)usb_try_flush_tx();
    }
    if (g_tx_r != g_tx_w) {
        *out_len = usb_rb_push(buf, len, g_tx_buf, &g_tx_r, &g_tx_w);
        return 0;
    }

    {
        int w = (int)usb_serial_write(0, (const char *)buf, (size_t)len);
        if (w > 0) {
            g_ready = 1;
            *out_len = (uint32_t)w;
            if (*out_len < len) {
                *out_len += usb_rb_push(&buf[*out_len], len - *out_len, g_tx_buf, &g_tx_r, &g_tx_w);
            }
            return 0;
        }
        if (w < 0) {
            g_ready = 0;
        }
    }

    *out_len = usb_rb_push(buf, len, g_tx_buf, &g_tx_r, &g_tx_w);
    return 0;
}

static int usb_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = usb_open,
    .close = usb_close,
    .read = usb_read,
    .write = usb_write,
    .ioctl = usb_ioctl,
};

static of_dev_t g_dev = {
    .name = "usb_cdc_512000",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_usb_cdc_get_dev(void)
{
    return &g_dev;
}

int drv_usb_cdc_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}

void drv_usb_cdc_push_rx(const uint8_t *buf, uint32_t len)
{
    (void)usb_rb_push(buf, len, g_rx_buf, &g_rx_r, &g_rx_w);
}

void drv_usb_cdc_on_resume(void)
{
    g_ready = 1;
    (void)usb_try_flush_tx();
}

void drv_usb_cdc_on_suspend(void)
{
    g_ready = 0;
}
