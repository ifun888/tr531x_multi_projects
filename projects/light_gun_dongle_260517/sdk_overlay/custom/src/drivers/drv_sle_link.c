#include "drivers/drv_sle_link.h"
#include "osal_debug.h"
#include <stdint.h>

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif

#define SLE_BUF_SZ 512U

static const char *sle_role_name(void)
{
    return OF_ROLE_DONGLE ? "dongle" : "gun";
}

/*
 * 由sample侧可选实现并导出：
 * int of_sle_sdk_send(const uint8_t *buf, uint32_t len);
 */
extern int of_sle_sdk_send(const uint8_t *buf, uint32_t len) __attribute__((weak));
extern void of_sle_sdk_enter_search(void) __attribute__((weak));
extern void of_sle_sdk_force_reconnect(void) __attribute__((weak));

static unsigned char g_buf[SLE_BUF_SZ];
static unsigned int g_r = 0;
static unsigned int g_w = 0;
static int g_opened = 0;
static uint8_t g_open_ref = 0U;
static int g_connected = 0;
static int g_searching = 0;
static uint8_t g_tx_fault_cnt = 0U;

static void sle_rb_reset(void)
{
    g_r = 0;
    g_w = 0;
}

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
    if (g_opened != 0) {
        if (g_open_ref < 0xFFU) {
            g_open_ref++;
        }
        return 0;
    }

    g_opened = 1;
    g_open_ref = 1U;
    g_connected = 0;
    g_searching = 1;
    g_tx_fault_cnt = 0U;
    sle_rb_reset();
    osal_printk("[openfire][%s] sle open, enter search\n", sle_role_name());
    return 0;
}

static int sle_close(void *ctx)
{
    (void)ctx;
    if (g_opened == 0) {
        return 0;
    }
    if (g_open_ref > 1U) {
        g_open_ref--;
        return 0;
    }

    g_opened = 0;
    g_open_ref = 0U;
    g_connected = 0;
    g_searching = 0;
    g_tx_fault_cnt = 0U;
    sle_rb_reset();
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
            g_tx_fault_cnt = 0U;
            return 0;
        }
        return -1;
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

int drv_sle_link_is_searching(void)
{
    return (g_opened != 0) && (g_searching != 0);
}

void drv_sle_link_push_rx(const uint8_t *buf, uint32_t len)
{
    sle_rb_push(buf, len);
}

void drv_sle_link_set_connected(int connected)
{
    int next_connected = (connected != 0);

    if (g_connected == next_connected) {
        return;
    }

    g_connected = (connected != 0);
    g_searching = (connected == 0);
    g_tx_fault_cnt = 0U;
    osal_printk("[openfire][%s] sle %s\n", sle_role_name(),
        g_connected ? "connected" : "disconnected");
    if (connected == 0) {
        sle_rb_reset();
    }
}

void drv_sle_link_request_search(void)
{
    if (g_opened == 0) {
        return;
    }

    g_connected = 0;
    g_searching = 1;
    g_tx_fault_cnt = 0U;
    sle_rb_reset();
    osal_printk("[openfire][%s] sle search requested\n", sle_role_name());

    if (of_sle_sdk_force_reconnect != 0) {
        of_sle_sdk_force_reconnect();
    } else if (of_sle_sdk_enter_search != 0) {
        of_sle_sdk_enter_search();
    }
}

void drv_sle_link_note_tx_fault(void)
{
    if ((g_opened == 0) || (g_connected == 0)) {
        return;
    }
    if (g_tx_fault_cnt < 0xFFU) {
        g_tx_fault_cnt++;
    }
    if (g_tx_fault_cnt >= 3U) {
        osal_printk("[openfire][%s] sle tx fault threshold=%u\n", sle_role_name(),
            (unsigned int)g_tx_fault_cnt);
        drv_sle_link_request_search();
    }
}
