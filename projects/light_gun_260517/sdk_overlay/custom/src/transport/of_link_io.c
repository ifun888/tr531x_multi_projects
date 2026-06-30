#include "of_link_io.h"

#include "drivers/drv_sle_link.h"
#include "of_transport.h"
#include "of_wireless_pkt.h"
#include "osal_debug.h"
#include "platform/of_time.h"

#ifndef OF_ROLE_DONGLE
#define OF_ROLE_DONGLE 0
#endif

#define OF_LINK_HELLO_INTERVAL_MS 500U
#define OF_LINK_READY_TIMEOUT_MS 3000U

static const char *link_role_name(void)
{
    return OF_ROLE_DONGLE ? "dongle" : "gun";
}

static uint8_t g_link_ready = 0U;
static uint8_t g_link_hello_seen = 0U;
static uint32_t g_link_last_hello_ms = 0U;
static uint32_t g_link_wait_start_ms = 0U;

static int link_phy_active(void)
{
    return drv_sle_link_is_ready();
}

int of_link_wireless_active(void)
{
    return (of_transport_get_type() == OF_TRANSPORT_SLE) && link_phy_active();
}

int of_link_is_ready(void)
{
    return link_phy_active() && (g_link_ready != 0U);
}

static int link_raw_sle_write(const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    const of_dev_t *sle = drv_sle_link_get_dev();

    if ((sle == 0) || (sle->ops == 0) || (sle->ops->write == 0)) {
        return -1;
    }
    return sle->ops->write(sle->priv, buf, len, out_len);
}

void of_link_tick(void)
{
    uint32_t now_ms;
    uint32_t sent = 0U;

    if (!link_phy_active()) {
        g_link_ready = 0U;
        g_link_hello_seen = 0U;
        g_link_last_hello_ms = 0U;
        g_link_wait_start_ms = 0U;
        return;
    }

    if (g_link_ready != 0U) {
        g_link_wait_start_ms = 0U;
        return;
    }

    now_ms = (uint32_t)(of_time_us() / 1000U);
    if (g_link_wait_start_ms == 0U) {
        g_link_wait_start_ms = now_ms;
    } else if ((now_ms - g_link_wait_start_ms) >= OF_LINK_READY_TIMEOUT_MS) {
        osal_printk("[openfire][%s] link ready timeout, fallback to search\n", link_role_name());
        drv_sle_link_request_search();
        g_link_hello_seen = 0U;
        g_link_last_hello_ms = 0U;
        g_link_wait_start_ms = now_ms;
        return;
    }

    if ((g_link_last_hello_ms != 0U) &&
        ((now_ms - g_link_last_hello_ms) < OF_LINK_HELLO_INTERVAL_MS)) {
        return;
    }

    g_link_last_hello_ms = now_ms;
    osal_printk("[openfire][%s] send hello\n", link_role_name());
    (void)of_link_send_packet(OF_WPKT_TYPE_LINK_HELLO, 0, 0U, &sent);
}

int of_link_on_wireless_packet(uint8_t type, const uint8_t *payload, uint32_t payload_len)
{
    uint32_t sent = 0U;

    (void)payload;
    (void)payload_len;

    if (type == OF_WPKT_TYPE_LINK_HELLO) {
        g_link_hello_seen = 1U;
        osal_printk("[openfire][%s] rx hello\n", link_role_name());
        (void)of_link_send_packet(OF_WPKT_TYPE_LINK_HELLO_ACK, 0, 0U, &sent);
        return 1;
    }

    if (type == OF_WPKT_TYPE_LINK_HELLO_ACK) {
        g_link_hello_seen = 1U;
        g_link_ready = 1U;
        g_link_wait_start_ms = 0U;
        osal_printk("[openfire][%s] rx hello ack, link ready\n", link_role_name());
        return 1;
    }

    return 0;
}

int of_link_send_packet(uint8_t type, const uint8_t *payload, uint32_t payload_len, uint32_t *out_len)
{
    uint8_t frame[5U + OF_WPKT_MAX_PAYLOAD];
    uint32_t frame_len = 0U;

    if (out_len != 0) {
        *out_len = 0U;
    }
    if (!link_phy_active()) {
        return -1;
    }
    if (of_wireless_pkt_encode(type, payload, payload_len, frame, sizeof(frame), &frame_len) != 0) {
        return -1;
    }
    if (of_link_wireless_active()) {
        if (of_transport_write(frame, frame_len, out_len) != 0) {
            drv_sle_link_note_tx_fault();
            return -1;
        }
        return 0;
    }
    if ((type != OF_WPKT_TYPE_LINK_HELLO) && (type != OF_WPKT_TYPE_LINK_HELLO_ACK)) {
        return -1;
    }
    if (link_raw_sle_write(frame, frame_len, out_len) != 0) {
        drv_sle_link_note_tx_fault();
        return -1;
    }
    return 0;
}

int of_link_send_serial(const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    if (of_link_wireless_active()) {
        if (!of_link_is_ready()) {
            if (out_len != 0) {
                *out_len = 0U;
            }
            return -1;
        }
        return of_link_send_packet(OF_WPKT_TYPE_SERIAL_TUNNEL, buf, len, out_len);
    }
    return of_transport_write(buf, len, out_len);
}

int of_link_send_telemetry(const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    if (of_link_wireless_active()) {
        if (!of_link_is_ready()) {
            if (out_len != 0) {
                *out_len = 0U;
            }
            return -1;
        }
        return of_link_send_packet(OF_WPKT_TYPE_TELEMETRY, buf, len, out_len);
    }
    return of_transport_write(buf, len, out_len);
}
