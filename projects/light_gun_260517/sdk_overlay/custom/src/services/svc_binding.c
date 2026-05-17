#include "services/svc_binding.h"
#include "drivers/drv_storage.h"
#include <string.h>

#define BIND_MAGIC0 0x42U
#define BIND_MAGIC1 0x44U
#define BIND_VER    0x01U
#define BIND_SLOTS  8U

static uint8_t g_binding[BIND_SLOTS + 3U];

int svc_binding_apply_default(void)
{
    uint8_t i;
    g_binding[0] = BIND_MAGIC0;
    g_binding[1] = BIND_MAGIC1;
    g_binding[2] = BIND_VER;
    for (i = 0; i < BIND_SLOTS; i++) {
        g_binding[3U + i] = i;
    }
    return 0;
}

int svc_binding_load(void)
{
    const of_dev_t *st = drv_storage_get_dev();
    uint8_t raw[32] = {0};
    uint32_t got = 0;
    if ((st == 0) || (st->ops == 0) || (st->ops->read == 0)) {
        return -1;
    }
    if (st->ops->read(st->priv, raw, sizeof(raw), &got) != 0) {
        return -1;
    }
    if (got < sizeof(g_binding) || raw[0] != BIND_MAGIC0 || raw[1] != BIND_MAGIC1) {
        return svc_binding_apply_default();
    }
    (void)memcpy(g_binding, raw, sizeof(g_binding));
    return 0;
}

int svc_binding_save(void)
{
    const of_dev_t *st = drv_storage_get_dev();
    uint32_t out = 0;
    if ((st == 0) || (st->ops == 0) || (st->ops->write == 0)) {
        return -1;
    }
    if (st->ops->write(st->priv, g_binding, sizeof(g_binding), &out) != 0) {
        return -1;
    }
    return (out == sizeof(g_binding)) ? 0 : -1;
}

int svc_binding_set(uint8_t slot, uint8_t code)
{
    if (slot >= BIND_SLOTS) {
        return -1;
    }
    g_binding[3U + slot] = code;
    return 0;
}

int svc_binding_get(uint8_t slot, uint8_t *code)
{
    if (slot >= BIND_SLOTS || code == 0) {
        return -1;
    }
    *code = g_binding[3U + slot];
    return 0;
}

const uint8_t *svc_binding_data(uint32_t *len)
{
    if (len != 0) {
        *len = sizeof(g_binding);
    }
    return g_binding;
}
