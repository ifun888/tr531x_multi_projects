#include "services/svc_profile.h"
#include "drivers/drv_storage.h"
#include <stdint.h>
#include <string.h>

typedef struct {
    uint8_t magic[4];
    uint8_t toggles[16];
    uint8_t pins[16];
    uint8_t settings[16];
    uint8_t profile[32];
    uint8_t buttons[16];
    uint8_t usb_id[18];
} of_profile_store_t;

#define OF_PROFILE_RUN_MODE_IDX 0U
#define OF_PROFILE_CENTER_X_L_IDX  2U
#define OF_PROFILE_CENTER_X_H_IDX  3U
#define OF_PROFILE_CENTER_Y_L_IDX  4U
#define OF_PROFILE_CENTER_Y_H_IDX  5U
#define OF_PROFILE_TOP_OFF_L_IDX   6U
#define OF_PROFILE_TOP_OFF_H_IDX   7U
#define OF_PROFILE_BOTTOM_OFF_L_IDX 8U
#define OF_PROFILE_BOTTOM_OFF_H_IDX 9U
#define OF_PROFILE_LEFT_OFF_L_IDX  10U
#define OF_PROFILE_LEFT_OFF_H_IDX  11U
#define OF_PROFILE_RIGHT_OFF_L_IDX 12U
#define OF_PROFILE_RIGHT_OFF_H_IDX 13U

static of_profile_store_t g_store = {
    .magic = {'O', 'F', 'P', '2'},
};

static uint16_t profile_get_u16(uint8_t lo_idx)
{
    return (uint16_t)g_store.profile[lo_idx] |
           ((uint16_t)g_store.profile[lo_idx + 1U] << 8);
}

static void profile_set_u16(uint8_t lo_idx, uint16_t value)
{
    g_store.profile[lo_idx] = (uint8_t)(value & 0xFFU);
    g_store.profile[lo_idx + 1U] = (uint8_t)((value >> 8) & 0xFFU);
}

void svc_profile_apply_default(void)
{
    uint32_t i;
    g_store.magic[0] = 'O';
    g_store.magic[1] = 'F';
    g_store.magic[2] = 'P';
    g_store.magic[3] = '2';
    for (i = 0; i < sizeof(g_store.toggles); i++) {
        g_store.toggles[i] = 0U;
    }
    for (i = 0; i < sizeof(g_store.pins); i++) {
        g_store.pins[i] = (uint8_t)i;
    }
    for (i = 0; i < sizeof(g_store.settings); i++) {
        g_store.settings[i] = 0U;
    }
    for (i = 0; i < sizeof(g_store.profile); i++) {
        g_store.profile[i] = 0U;
    }
    g_store.profile[OF_PROFILE_RUN_MODE_IDX] = 0U;
    for (i = 0; i < sizeof(g_store.buttons); i++) {
        g_store.buttons[i] = (uint8_t)i;
    }
    for (i = 0; i < sizeof(g_store.usb_id); i++) {
        g_store.usb_id[i] = 0U;
    }
}

int svc_profile_load(void)
{
    const of_dev_t *st = drv_storage_get_dev();
    uint32_t got = 0;
    if ((st == 0) || (st->ops == 0) || (st->ops->read == 0)) {
        return -1;
    }
    if (st->ops->read(st->priv, (uint8_t *)&g_store, sizeof(g_store), &got) != 0) {
        return -1;
    }
    if (got < sizeof(g_store.magic) ||
        g_store.magic[0] != 'O' || g_store.magic[1] != 'F' ||
        g_store.magic[2] != 'P' || g_store.magic[3] != '2') {
        svc_profile_apply_default();
    } else if (g_store.profile[OF_PROFILE_RUN_MODE_IDX] > 2U) {
        g_store.profile[OF_PROFILE_RUN_MODE_IDX] = 0U;
    }
    return 0;
}

int svc_profile_save(void)
{
    const of_dev_t *st = drv_storage_get_dev();
    uint32_t out = 0;
    if ((st == 0) || (st->ops == 0) || (st->ops->write == 0)) {
        return -1;
    }
    if (st->ops->write(st->priv, (const uint8_t *)&g_store, sizeof(g_store), &out) != 0) {
        return -1;
    }
    return (out == sizeof(g_store)) ? 0 : -1;
}

const uint8_t *svc_profile_get_blob(of_cfg_type_t type, uint32_t *len)
{
    if (len != 0) {
        *len = 0U;
    }
    switch (type) {
        case OF_CFG_TOGGLES:
            if (len != 0) *len = sizeof(g_store.toggles);
            return g_store.toggles;
        case OF_CFG_PINS:
            if (len != 0) *len = sizeof(g_store.pins);
            return g_store.pins;
        case OF_CFG_SETTINGS:
            if (len != 0) *len = sizeof(g_store.settings);
            return g_store.settings;
        case OF_CFG_PROFILE:
            if (len != 0) *len = sizeof(g_store.profile);
            return g_store.profile;
        case OF_CFG_BUTTONS:
            if (len != 0) *len = sizeof(g_store.buttons);
            return g_store.buttons;
        case OF_CFG_USB_ID:
            if (len != 0) *len = sizeof(g_store.usb_id);
            return g_store.usb_id;
        default:
            return 0;
    }
}

int svc_profile_set_blob(of_cfg_type_t type, const uint8_t *buf, uint32_t len)
{
    uint32_t n = 0U;
    uint8_t *dst = 0;
    if (buf == 0) {
        return -1;
    }
    switch (type) {
        case OF_CFG_TOGGLES:
            dst = g_store.toggles;
            n = sizeof(g_store.toggles);
            break;
        case OF_CFG_PINS:
            dst = g_store.pins;
            n = sizeof(g_store.pins);
            break;
        case OF_CFG_SETTINGS:
            dst = g_store.settings;
            n = sizeof(g_store.settings);
            break;
        case OF_CFG_PROFILE:
            dst = g_store.profile;
            n = sizeof(g_store.profile);
            break;
        case OF_CFG_BUTTONS:
            dst = g_store.buttons;
            n = sizeof(g_store.buttons);
            break;
        case OF_CFG_USB_ID:
            dst = g_store.usb_id;
            n = sizeof(g_store.usb_id);
            break;
        default:
            return -1;
    }
    if (len > n) {
        len = n;
    }
    (void)memcpy(dst, buf, len);
    if (len < n) {
        (void)memset(dst + len, 0, n - len);
    }
    return 0;
}

int svc_profile_get_u32(of_cfg_type_t type, uint8_t index, uint32_t *value)
{
    const uint8_t *src = 0;
    uint32_t len = 0U;
    uint32_t off;

    if (value == 0) {
        return -1;
    }
    src = svc_profile_get_blob(type, &len);
    off = ((uint32_t)index) * 4U;
    if ((src == 0) || (off + 4U > len)) {
        return -1;
    }
    *value = (uint32_t)src[off] |
             ((uint32_t)src[off + 1U] << 8) |
             ((uint32_t)src[off + 2U] << 16) |
             ((uint32_t)src[off + 3U] << 24);
    return 0;
}

int svc_profile_set_u32(of_cfg_type_t type, uint8_t index, uint32_t value)
{
    uint8_t *dst = 0;
    uint32_t len = 0U;
    uint32_t off;

    switch (type) {
        case OF_CFG_SETTINGS:
            dst = g_store.settings;
            len = sizeof(g_store.settings);
            break;
        case OF_CFG_PROFILE:
            dst = g_store.profile;
            len = sizeof(g_store.profile);
            break;
        default:
            return -1;
    }
    off = ((uint32_t)index) * 4U;
    if (off + 4U > len) {
        return -1;
    }
    dst[off] = (uint8_t)(value & 0xFFU);
    dst[off + 1U] = (uint8_t)((value >> 8) & 0xFFU);
    dst[off + 2U] = (uint8_t)((value >> 16) & 0xFFU);
    dst[off + 3U] = (uint8_t)((value >> 24) & 0xFFU);
    return 0;
}

uint8_t svc_profile_get_run_mode(void)
{
    return g_store.profile[OF_PROFILE_RUN_MODE_IDX];
}

void svc_profile_set_run_mode(uint8_t mode)
{
    if (mode > 2U) {
        mode = 0U;
    }
    g_store.profile[OF_PROFILE_RUN_MODE_IDX] = mode;
}

int svc_profile_get_calibration(uint16_t *cx, uint16_t *cy)
{
    return svc_profile_get_ir_center(cx, cy);
}

void svc_profile_set_calibration(uint16_t cx, uint16_t cy)
{
    svc_profile_set_ir_center(cx, cy);
}

int svc_profile_get_ir_center(uint16_t *cx, uint16_t *cy)
{
    if ((cx == 0) || (cy == 0)) {
        return -1;
    }
    *cx = profile_get_u16(OF_PROFILE_CENTER_X_L_IDX);
    *cy = profile_get_u16(OF_PROFILE_CENTER_Y_L_IDX);
    return 0;
}

void svc_profile_set_ir_center(uint16_t cx, uint16_t cy)
{
    profile_set_u16(OF_PROFILE_CENTER_X_L_IDX, cx);
    profile_set_u16(OF_PROFILE_CENTER_Y_L_IDX, cy);
}

int svc_profile_get_ir_offsets(uint16_t *top, uint16_t *bottom, uint16_t *left, uint16_t *right)
{
    if ((top == 0) || (bottom == 0) || (left == 0) || (right == 0)) {
        return -1;
    }
    *top = profile_get_u16(OF_PROFILE_TOP_OFF_L_IDX);
    *bottom = profile_get_u16(OF_PROFILE_BOTTOM_OFF_L_IDX);
    *left = profile_get_u16(OF_PROFILE_LEFT_OFF_L_IDX);
    *right = profile_get_u16(OF_PROFILE_RIGHT_OFF_L_IDX);
    return 0;
}

void svc_profile_set_ir_offsets(uint16_t top, uint16_t bottom, uint16_t left, uint16_t right)
{
    profile_set_u16(OF_PROFILE_TOP_OFF_L_IDX, top);
    profile_set_u16(OF_PROFILE_BOTTOM_OFF_L_IDX, bottom);
    profile_set_u16(OF_PROFILE_LEFT_OFF_L_IDX, left);
    profile_set_u16(OF_PROFILE_RIGHT_OFF_L_IDX, right);
}
