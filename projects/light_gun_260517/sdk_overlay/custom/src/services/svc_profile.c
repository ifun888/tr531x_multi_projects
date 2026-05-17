#include "services/svc_profile.h"
#include "drivers/drv_storage.h"
#include <stdint.h>
#include <string.h>

static uint8_t g_profile[32] = {
    0x4F, 0x46, 0x50, 0x31, 1, 0, 0, 0,
};

int svc_profile_load(void)
{
    const of_dev_t *st = drv_storage_get_dev();
    uint32_t got = 0;
    if ((st == 0) || (st->ops == 0) || (st->ops->read == 0)) {
        return -1;
    }
    if (st->ops->read(st->priv, g_profile, sizeof(g_profile), &got) != 0) {
        return -1;
    }
    if (got < 4U || g_profile[0] != 0x4F || g_profile[1] != 0x46 || g_profile[2] != 0x50) {
        g_profile[0] = 0x4F;
        g_profile[1] = 0x46;
        g_profile[2] = 0x50;
        g_profile[3] = 0x31;
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
    if (st->ops->write(st->priv, g_profile, sizeof(g_profile), &out) != 0) {
        return -1;
    }
    return (out == sizeof(g_profile)) ? 0 : -1;
}
