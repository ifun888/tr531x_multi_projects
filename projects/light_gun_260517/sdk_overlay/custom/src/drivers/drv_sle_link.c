#include "drivers/drv_sle_link.h"
#include "osal_debug.h"
#include <stdint.h>

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif

#define SLE_BUF_SZ 512U

/*
 * 由sample侧可选实现并导出：
 * int of_sle_sdk_send(const uint8_t *buf, uint32_t len);
 */
extern int of_sle_sdk_send(const uint8_t *buf, uint32_t len) __attribute__((weak));

static unsigned char g_buf[SLE_BUF_SZ];
static unsigned int g_r = 0;
static unsigned int g_w = 0;
static int g_opened = 0;
static int g_connected = 0;

static void sle_rb_push(const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    if (buf == 0) {
        return;
    }
    for (i = 0; i < len; i++) {
        unsigned int next = (g_w + 1U) % SLE_BUF_SZ;
        if (next == g_r) {
            break;
        }
        g_buf[g_w] = buf[i];
        g_w = next;
    }
}

static int sle_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_connected = 0;
    g_r = 0;
    g_w = 0;
    osal_printk("[openfire] sle link open role=%d\n", OF_ROLE_DONGLE);
    return 0;
}

static int sle_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    return 0;
}

static int sle_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n = 0;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }
    while ((n < len) && (g_r != g_w)) {
        buf[n++] = g_buf[g_r];
        g_r = (g_r + 1U) % SLE_BUF_SZ;
    }
    *out_len = n;
    return 0;
}

static int sle_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n = 0;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }

    if (of_sle_sdk_send != 0) {
        if (of_sle_sdk_send(buf, len) == 0) {
            *out_len = len;
            return 0;
        }
    }

    while (n < len) {
        unsigned int next = (g_w + 1U) % SLE_BUF_SZ;
        if (next == g_r) {
            break;
        }
        g_buf[g_w] = buf[n++];
        g_w = next;
    }
    *out_len = n;
    return 0;
}

static int sle_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = sle_open,
    .close = sle_close,
    .read = sle_read,
    .write = sle_write,
    .ioctl = sle_ioctl,
};

static of_dev_t g_dev = {
    .name = "sle_link",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_sle_link_get_dev(void)
{
    return &g_dev;
}

int drv_sle_link_is_ready(void)
{
    return (g_opened != 0) && (g_connected != 0);
}

void drv_sle_link_push_rx(const uint8_t *buf, uint32_t len)
{
    sle_rb_push(buf, len);
}

void drv_sle_link_set_connected(int connected)
{
    g_connected = (connected != 0);
}
