#include "of_protocol.h"
#include "of_transport.h"
#include "services/svc_binding.h"
#include "services/svc_calibration.h"
#include "drivers/drv_storage.h"
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
    (void)of_transport_write(p, n, &sent);
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
        proto_reply_byte(0xC2, (uint8_t)((rc == 0) ? 0 : 1));
        return;
    }

    if ((len >= 1U) && (buf[0] == 0x50U)) {
        int rc = svc_calibration_enter();
        proto_reply_byte(0xD0, (uint8_t)((rc == 0) ? 0 : 1));
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
        uint8_t rsp[6];
        (void)svc_calibration_get_result(&cx, &cy);
        rsp[0] = 0xD3;
        rsp[1] = (uint8_t)svc_calibration_get_state();
        rsp[2] = (uint8_t)(cx & 0xFFU);
        rsp[3] = (uint8_t)((cx >> 8) & 0xFFU);
        rsp[4] = (uint8_t)(cy & 0xFFU);
        rsp[5] = (uint8_t)((cy >> 8) & 0xFFU);
        proto_reply(rsp, sizeof(rsp));
    }
}
