#include "of_protocol.h"
#include "of_transport.h"
#include "drivers/drv_led.h"
#include "drivers/drv_feedback.h"
#include "drivers/drv_storage.h"
#include "drivers/drv_temp_sensor.h"
#include "of_selftest.h"
#include "of_transport.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"

static of_mode_t g_mode = OF_MODE_RUN;
static uint8_t g_cfg_blob[16] = {0};

void of_proto_set_mode(of_mode_t mode)
{
    g_mode = mode;
}

of_mode_t of_proto_get_mode(void)
{
    return g_mode;
}

static void proto_reply(const uint8_t *p, uint32_t n)
{
    uint32_t sent = 0;
    (void)of_transport_write(p, n, &sent);
}

void of_proto_docked_process(const uint8_t *buf, uint32_t len)
{
    if ((buf == 0) || (len == 0U)) {
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x01U) && (buf[1] == 0x02U)) {
        static const uint8_t ack[] = {0x81, 0x02, 0x00};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x10U)) {
        g_mode = (of_mode_t)buf[1];
        {
            uint8_t ack[] = {0x90, (uint8_t)g_mode, 0x00};
            proto_reply(ack, sizeof(ack));
        }
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x11U)) {
        uint8_t rsp[] = {0x91, (uint8_t)g_mode};
        proto_reply(rsp, sizeof(rsp));
        return;
    }

    if ((len >= 3U) && (buf[0] == 0x20U)) {
        if (buf[1] < sizeof(g_cfg_blob)) {
            g_cfg_blob[buf[1]] = buf[2];
            {
                uint8_t ack[] = {0xA0, buf[1], g_cfg_blob[buf[1]], 0x00};
                proto_reply(ack, sizeof(ack));
            }
        }
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x21U)) {
        if (buf[1] < sizeof(g_cfg_blob)) {
            uint8_t rsp[] = {0xA1, buf[1], g_cfg_blob[buf[1]]};
            proto_reply(rsp, sizeof(rsp));
        }
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x30U)) {
        uint8_t rsp[] = {0xB0, (uint8_t)g_mode, 0x01};
        proto_reply(rsp, sizeof(rsp));
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x31U)) {
        const of_dev_t *led = drv_led_get_dev();
        uint8_t v = buf[1];
        uint32_t out = 0;
        if ((led != 0) && (led->ops != 0) && (led->ops->write != 0)) {
            (void)led->ops->write(led->priv, &v, 1U, &out);
        }
        {
            uint8_t ack[] = {0xB1, v, 0x00};
            proto_reply(ack, sizeof(ack));
        }
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x32U)) {
        const of_dev_t *fb = drv_feedback_get_dev();
        uint8_t v = buf[1];
        uint32_t out = 0;
        if ((fb != 0) && (fb->ops != 0) && (fb->ops->write != 0)) {
            (void)fb->ops->write(fb->priv, &v, 1U, &out);
        }
        {
            uint8_t ack[] = {0xB2, v, 0x00};
            proto_reply(ack, sizeof(ack));
        }
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x33U)) {
        const of_dev_t *st = drv_storage_get_dev();
        uint8_t rsp[20] = {0xB3};
        uint32_t out = 0;
        if ((st != 0) && (st->ops != 0) && (st->ops->read != 0)) {
            (void)st->ops->read(st->priv, &rsp[1], (uint32_t)(sizeof(rsp) - 1U), &out);
        }
        proto_reply(rsp, out + 1U);
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x34U)) {
        const of_dev_t *temp = drv_temp_sensor_get_dev();
        uint8_t tbuf[2] = {0};
        uint32_t out = 0;
        uint32_t st_map = of_selftest_bitmap();
        uint8_t rsp[10];
        rsp[0] = 0xB4;
        rsp[1] = (uint8_t)(st_map & 0xFFU);
        rsp[2] = (uint8_t)((st_map >> 8) & 0xFFU);
        rsp[3] = (uint8_t)((st_map >> 16) & 0xFFU);
        rsp[4] = (uint8_t)((st_map >> 24) & 0xFFU);
        if ((temp != 0) && (temp->ops != 0) && (temp->ops->read != 0)) {
            (void)temp->ops->read(temp->priv, tbuf, sizeof(tbuf), &out);
        }
        rsp[5] = tbuf[0];
        rsp[6] = tbuf[1];
        rsp[7] = (uint8_t)of_transport_get_type();
        proto_reply(rsp, 8U);
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x40U)) {
        uint32_t n = 0;
        const uint8_t *d = svc_binding_data(&n);
        uint8_t rsp[20] = {0xC0};
        uint32_t i;
        if (d != 0) {
            for (i = 0; i < n && i < (sizeof(rsp) - 1U); i++) {
                rsp[i + 1U] = d[i];
            }
            proto_reply(rsp, i + 1U);
        }
        return;
    }

    if ((len >= 3U) && (buf[0] == 0x41U)) {
        int rc = svc_binding_set(buf[1], buf[2]);
        uint8_t ack[] = {0xC1, buf[1], buf[2], (uint8_t)((rc == 0) ? 0 : 1)};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x42U)) {
        int rc = svc_binding_save();
        uint8_t ack[] = {0xC2, (uint8_t)((rc == 0) ? 0 : 1)};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x50U)) {
        int rc = svc_calibration_enter();
        uint8_t ack[] = {0xD0, (uint8_t)((rc == 0) ? 0 : 1)};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 5U) && (buf[0] == 0x51U)) {
        uint16_t x = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
        uint16_t y = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8);
        int rc = svc_calibration_push_sample(x, y);
        uint8_t ack[] = {0xD1, (uint8_t)((rc == 0) ? 0 : 1), (uint8_t)svc_calibration_get_state()};
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x52U)) {
        uint16_t cx = 0;
        uint16_t cy = 0;
        int rc1 = svc_calibration_get_result(&cx, &cy);
        int rc2 = svc_calibration_commit();
        uint8_t ack[] = {
            0xD2, (uint8_t)((rc1 == 0 && rc2 == 0) ? 0 : 1),
            (uint8_t)(cx & 0xFFU), (uint8_t)((cx >> 8) & 0xFFU),
            (uint8_t)(cy & 0xFFU), (uint8_t)((cy >> 8) & 0xFFU)
        };
        proto_reply(ack, sizeof(ack));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x53U)) {
        uint16_t cx = 0;
        uint16_t cy = 0;
        (void)svc_calibration_get_result(&cx, &cy);
        {
            uint8_t rsp[] = {
                0xD3, (uint8_t)svc_calibration_get_state(),
                (uint8_t)(cx & 0xFFU), (uint8_t)((cx >> 8) & 0xFFU),
                (uint8_t)(cy & 0xFFU), (uint8_t)((cy >> 8) & 0xFFU)
            };
            proto_reply(rsp, sizeof(rsp));
        }
    }
}
