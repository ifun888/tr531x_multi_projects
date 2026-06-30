#include "drivers/drv_usb_cdc.h"
#include <stdint.h>
#include <stddef.h>

#define USB_BUF_SZ 1024U

extern int usb_serial_read(uint32_t index, char *buffer, size_t buflen) __attribute__((weak));
extern int usb_serial_write(uint32_t index, const char *buffer, size_t buflen) __attribute__((weak));

static unsigned char g_buf[USB_BUF_SZ];
static unsigned int g_r = 0;
static unsigned int g_w = 0;
static int g_opened = 0;
static int g_ready = 0;

static void usb_rb_push(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    if (buf == 0) {
        return;
    }
    for (i = 0; i < len; i++) {
        unsigned int next = (g_w + 1U) % USB_BUF_SZ;
        if (next == g_r) {
            break;
        }
        g_buf[g_w] = buf[i];
        g_w = next;
    }
}

static int usb_open(void *ctx)
{
    int keep_ready = g_ready;
    (void)ctx;
    g_opened = 1;
    g_ready = keep_ready;
    g_r = 0;
    g_w = 0;
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

    while ((n < len) && (g_r != g_w)) {
        buf[n++] = g_buf[g_r];
        g_r = (g_r + 1U) % USB_BUF_SZ;
    }
    *out_len = n;
    return 0;
}

static int usb_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n = 0;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }

    {
        int w = (int)usb_serial_write(0, (const char *)buf, (size_t)len);
        if (w > 0) {
            g_ready = 1;
            *out_len = (uint32_t)w;
            return 0;
        }
        if (w < 0) {
            g_ready = 0;
        }
    }

    while (n < len) {
        unsigned int next = (g_w + 1U) % USB_BUF_SZ;
        if (next == g_r) {
            break;
        }
        g_buf[g_w] = buf[n++];
        g_w = next;
    }
    *out_len = n;
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
    usb_rb_push(buf, len);
}

void drv_usb_cdc_on_resume(void)
{
    g_ready = 1;
}

void drv_usb_cdc_on_suspend(void)
{
    g_ready = 0;
}
