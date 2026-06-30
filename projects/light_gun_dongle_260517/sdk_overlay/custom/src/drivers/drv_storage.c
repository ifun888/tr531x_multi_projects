#include "drivers/drv_storage.h"
#include <stdint.h>
#include <string.h>

#define STORAGE_BUF_SZ 256U

static int g_opened;
static int g_ready;
static uint8_t g_store[STORAGE_BUF_SZ];
static uint32_t g_store_len;

static int st_open(void *ctx)
{
    (void)ctx;
    g_opened = 1;
    g_ready = 1;
    g_store_len = 0;
    return 0;
}

static int st_close(void *ctx)
{
    (void)ctx;
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int st_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }
    n = (len < g_store_len) ? len : g_store_len;
    if (n > 0U) {
        (void)memcpy(buf, g_store, n);
    }
    *out_len = n;
    return 0;
}

static int st_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint32_t n;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0)) {
        return -1;
    }
    n = (len < STORAGE_BUF_SZ) ? len : STORAGE_BUF_SZ;
    (void)memcpy(g_store, buf, n);
    g_store_len = n;
    *out_len = n;
    return 0;
}

static int st_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = st_open,
    .close = st_close,
    .read = st_read,
    .write = st_write,
    .ioctl = st_ioctl,
};

static of_dev_t g_dev = {
    .name = "storage",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_storage_get_dev(void)
{
    return &g_dev;
}

int drv_storage_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
