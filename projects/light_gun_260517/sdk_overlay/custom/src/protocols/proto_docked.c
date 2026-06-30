#include "of_protocol.h"
#include "of_link_io.h"
#include "of_transport.h"
#include "drivers/drv_led.h"
#include "drivers/drv_feedback.h"
#include "drivers/drv_rumble.h"
#include "drivers/drv_storage.h"
#include "drivers/drv_temp_sensor.h"
#include "of_selftest.h"
#include "of_transport.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"
#include "services/svc_profile.h"

static of_mode_t g_mode = OF_MODE_RUN;

#define OF_CMD_COMMIT_START   0xAAU
#define OF_CMD_COMMIT_TOGGLES 0xABU
#define OF_CMD_COMMIT_PINS    0xACU
#define OF_CMD_COMMIT_SETTINGS 0xADU
#define OF_CMD_COMMIT_PROFILE 0xAEU
#define OF_CMD_COMMIT_BTNS    0xAFU
#define OF_CMD_COMMIT_ID      0xB0U
#define OF_CMD_GET_PINS       0xC8U
#define OF_CMD_GET_TOGGLES    0xC9U
#define OF_CMD_GET_SETTINGS   0xCAU
#define OF_CMD_GET_PROFILE    0xCBU
#define OF_CMD_GET_BTNS       0xCCU
#define OF_CMD_SAVE           0xFCU
#define OF_CMD_CLEAR_FLASH    0xFDU
#define OF_CMD_TERM           0xFEU

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
    (void)of_link_send_serial(p, n, &sent);
}

static void proto_reply_byte(uint8_t a, uint8_t b)
{
    uint8_t rsp[2] = {a, b};
    proto_reply(rsp, sizeof(rsp));
}

static void proto_reply_u32(uint8_t cmd, uint8_t index, uint32_t value)
{
    uint8_t rsp[6];
    rsp[0] = cmd;
    rsp[1] = index;
    rsp[2] = (uint8_t)(value & 0xFFU);
    rsp[3] = (uint8_t)((value >> 8) & 0xFFU);
    rsp[4] = (uint8_t)((value >> 16) & 0xFFU);
    rsp[5] = (uint8_t)((value >> 24) & 0xFFU);
    proto_reply(rsp, sizeof(rsp));
}

static void proto_reply_cal_status(uint8_t cmd, uint8_t result)
{
    uint16_t cx = 0U;
    uint16_t cy = 0U;
    uint16_t top = 0U;
    uint16_t bottom = 0U;
    uint16_t left = 0U;
    uint16_t right = 0U;
    uint8_t rsp[12];

    (void)svc_calibration_get_result(&cx, &cy);
    (void)svc_calibration_get_offsets(&top, &bottom, &left, &right);

    rsp[0] = cmd;
    rsp[1] = result;
    rsp[2] = (uint8_t)svc_calibration_get_state();
    rsp[3] = (uint8_t)(cx & 0xFFU);
    rsp[4] = (uint8_t)((cx >> 8) & 0xFFU);
    rsp[5] = (uint8_t)(cy & 0xFFU);
    rsp[6] = (uint8_t)((cy >> 8) & 0xFFU);
    rsp[7] = (uint8_t)(top & 0xFFU);
    rsp[8] = (uint8_t)(bottom & 0xFFU);
    rsp[9] = (uint8_t)(left & 0xFFU);
    rsp[10] = (uint8_t)(right & 0xFFU);
    rsp[11] = 0x00U;
    proto_reply(rsp, sizeof(rsp));
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
        uint32_t n = 0U;
        const uint8_t *cur = svc_profile_get_blob(OF_CFG_SETTINGS, &n);
        uint8_t tmp[16];
        uint8_t ack[4];
        uint32_t i;
        for (i = 0; i < sizeof(tmp); i++) {
            tmp[i] = (cur != 0 && i < n) ? cur[i] : 0U;
        }
        if (buf[1] < sizeof(tmp)) {
            tmp[buf[1]] = buf[2];
            (void)svc_profile_set_blob(OF_CFG_SETTINGS, tmp, sizeof(tmp));
            ack[0] = 0xA0;
            ack[1] = buf[1];
            ack[2] = tmp[buf[1]];
            ack[3] = 0x00;
            proto_reply(ack, sizeof(ack));
        }
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x21U)) {
        uint32_t n = 0U;
        const uint8_t *cur = svc_profile_get_blob(OF_CFG_SETTINGS, &n);
        if ((cur != 0) && (buf[1] < n)) {
            uint8_t rsp[] = {0xA1, buf[1], cur[buf[1]]};
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
        uint8_t v = buf[1];
        (void)drv_rumble_set_level(v);
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
    if ((len >= 1U) && (buf[0] == OF_CMD_GET_PINS || buf[0] == OF_CMD_GET_BTNS)) {
        uint32_t n = 0;
        const uint8_t *d = svc_binding_data(&n);
        uint8_t rsp[40] = {buf[0]};
        uint32_t i;
        if (d != 0) {
            for (i = 0; i < n && i < (sizeof(rsp) - 1U); i++) {
                rsp[i + 1U] = d[i];
            }
            proto_reply(rsp, i + 1U);
        }
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_GET_TOGGLES || buf[0] == OF_CMD_GET_SETTINGS)) {
        if ((buf[0] == OF_CMD_GET_SETTINGS) && (len >= 2U)) {
            uint32_t v = 0U;
            if (svc_profile_get_u32(OF_CFG_SETTINGS, buf[1], &v) == 0) {
                proto_reply_u32(buf[0], buf[1], v);
            }
            return;
        }
        uint8_t rsp[40] = {buf[0]};
        of_cfg_type_t type = (buf[0] == OF_CMD_GET_TOGGLES) ? OF_CFG_TOGGLES : OF_CFG_SETTINGS;
        uint32_t n = 0U;
        const uint8_t *src = svc_profile_get_blob(type, &n);
        uint32_t i;
        for (i = 0; (src != 0) && i < n && i < (sizeof(rsp) - 1U); i++) {
            rsp[i + 1U] = src[i];
        }
        proto_reply(rsp, i + 1U);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_GET_PROFILE)) {
        if (len >= 2U) {
            uint32_t v = 0U;
            if (svc_profile_get_u32(OF_CFG_PROFILE, buf[1], &v) == 0) {
                proto_reply_u32(buf[0], buf[1], v);
            }
            return;
        }
        uint8_t rsp[40] = {buf[0]};
        uint32_t n = 0U;
        const uint8_t *src = svc_profile_get_blob(OF_CFG_PROFILE, &n);
        uint32_t i;
        for (i = 0; (src != 0) && i < n && i < (sizeof(rsp) - 1U); i++) {
            rsp[i + 1U] = src[i];
        }
        proto_reply(rsp, i + 1U);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_COMMIT_START)) {
        proto_reply_byte(OF_CMD_COMMIT_START, 0x00U);
        return;
    }
    if ((len >= 2U) && (buf[0] == OF_CMD_COMMIT_TOGGLES || buf[0] == OF_CMD_COMMIT_SETTINGS)) {
        of_cfg_type_t type = (buf[0] == OF_CMD_COMMIT_TOGGLES) ? OF_CFG_TOGGLES : OF_CFG_SETTINGS;
        if ((buf[0] == OF_CMD_COMMIT_SETTINGS) && (len >= 6U)) {
            uint32_t v = (uint32_t)buf[2] |
                         ((uint32_t)buf[3] << 8) |
                         ((uint32_t)buf[4] << 16) |
                         ((uint32_t)buf[5] << 24);
            (void)svc_profile_set_u32(OF_CFG_SETTINGS, buf[1], v);
            proto_reply_byte(buf[0], 0x00U);
            return;
        }
        (void)svc_profile_set_blob(type, &buf[1], len - 1U);
        proto_reply_byte(buf[0], 0x00U);
        return;
    }
    if ((len >= 3U) && (buf[0] == OF_CMD_COMMIT_PINS || buf[0] == OF_CMD_COMMIT_BTNS)) {
        (void)svc_binding_set(buf[1], buf[2]);
        proto_reply_byte(buf[0], 0x00U);
        return;
    }
    if ((len >= 5U) && (buf[0] == OF_CMD_COMMIT_PROFILE)) {
        if (len >= 6U) {
            uint32_t v = (uint32_t)buf[2] |
                         ((uint32_t)buf[3] << 8) |
                         ((uint32_t)buf[4] << 16) |
                         ((uint32_t)buf[5] << 24);
            (void)svc_profile_set_u32(OF_CFG_PROFILE, buf[1], v);
            (void)svc_calibration_load_profile();
            proto_reply_byte(buf[0], 0x00U);
            return;
        }
        (void)svc_profile_set_blob(OF_CFG_PROFILE, &buf[1], len - 1U);
        (void)svc_calibration_load_profile();
        proto_reply_byte(buf[0], 0x00U);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_COMMIT_ID)) {
        if (len > 1U) {
            (void)svc_profile_set_blob(OF_CFG_USB_ID, &buf[1], len - 1U);
        }
        proto_reply_byte(buf[0], 0x00U);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_SAVE)) {
        int rc = 0;
        if (svc_binding_save() != 0) {
            rc = 1;
        }
        if (svc_profile_save() != 0) {
            rc = 1;
        }
        proto_reply_byte(OF_CMD_SAVE, (uint8_t)rc);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_CLEAR_FLASH)) {
        (void)svc_binding_apply_default();
        (void)svc_calibration_exit();
        svc_profile_apply_default();
        (void)svc_calibration_load_profile();
        proto_reply_byte(OF_CMD_CLEAR_FLASH, 0x00U);
        return;
    }
    if ((len >= 1U) && (buf[0] == OF_CMD_TERM)) {
        g_mode = OF_MODE_RUN;
        proto_reply_byte(OF_CMD_TERM, 0x00U);
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x40U)) {
        uint32_t n = 0;
        const uint8_t *d = svc_binding_data(&n);
        uint8_t rsp[40] = {0xC0};
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
        proto_reply_cal_status(0xD0U, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 2U) && (buf[0] == 0x54U)) {
        int rc = svc_calibration_set_stage((of_cal_state_t)buf[1]);
        proto_reply_cal_status(0xD4U, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x55U)) {
        int rc = svc_calibration_exit();
        proto_reply_cal_status(0xD5U, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 5U) && (buf[0] == 0x51U)) {
        uint16_t x = 0U;
        uint16_t y = 0U;
        int rc = 0;

        if (len >= 6U) {
            int rc_stage = svc_calibration_set_stage((of_cal_state_t)buf[1]);
            x = (uint16_t)buf[2] | ((uint16_t)buf[3] << 8);
            y = (uint16_t)buf[4] | ((uint16_t)buf[5] << 8);
            if (rc_stage != 0) {
                proto_reply_cal_status(0xD1U, 1U);
                return;
            }
        } else {
            x = (uint16_t)buf[1] | ((uint16_t)buf[2] << 8);
            y = (uint16_t)buf[3] | ((uint16_t)buf[4] << 8);
        }

        rc = svc_calibration_push_sample(x, y);
        proto_reply_cal_status(0xD1U, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x52U)) {
        int rc = svc_calibration_commit();
        proto_reply_cal_status(0xD2U, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x53U)) {
        proto_reply_cal_status(0xD3U, 0U);
        return;
    }
}
