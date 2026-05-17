/*
 * Copyright (c) CompanyNameMagicTag 2021-2022. All rights reserved.
 * Description: mutex
 * Author: AuthorNameMagicTag
 * Create: 2021-12-16
 */

#include <los_mux.h>
#include <los_mux_pri.h>
#include "soc_osal.h"
#include "osal_errno.h"
#include "osal_inner.h"

#define MUTEX_MAGIC_WORD 0x800

int osal_mutex_init(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return OSAL_FAILURE;
    }

    if (((unsigned int)(UINTPTR)mutex->mutex & MUTEX_MAGIC_WORD) == MUTEX_MAGIC_WORD) {
        osal_log("The mutex has been initialized. mutex id:%d.\n",
            ((unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD));
        return OSAL_FAILURE;
    }
    if (GET_MUX_INDEX((unsigned int)(UINTPTR)mutex->mutex) >= (unsigned int)KERNEL_MUX_LIMIT) {
        osal_log("mutex id invalid!\n");
        return OSAL_FAILURE;
    }

    unsigned int ret = LOS_MuxCreate((unsigned int *)&(mutex->mutex));
    if (ret != LOS_OK) {
        osal_log("LOS_MuxCreate failed! ret = %#x.\n", ret);
    }
    mutex->mutex = (void *)(UINTPTR)((unsigned int)(UINTPTR)mutex->mutex | MUTEX_MAGIC_WORD);
    return (int)ret;
}

void osal_mutex_destroy(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    unsigned int ret = LOS_MuxDelete(mutex_tmp);
    if (ret != LOS_OK) {
        osal_log("LOS_MuxDelete failed! ret = %#x.\n", ret);
    }
    mutex->mutex = NULL;
}

int osal_mutex_lock(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return OSAL_FAILURE;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    unsigned int ret = LOS_MuxPend(mutex_tmp, LOS_WAIT_FOREVER);
    if (ret != LOS_OK) {
        osal_log("LOS_MuxPend failed! ret = %#x.\n", ret);
    }
    return (int)ret;
}

int osal_mutex_lock_timeout(osal_mutex *mutex, unsigned int timeout)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return OSAL_FAILURE;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    unsigned int tick = (timeout == OSAL_MUTEX_WAIT_FOREVER) ? LOS_WAIT_FOREVER : osal_msecs_to_jiffies(timeout);

    unsigned int ret = LOS_MuxPend(mutex_tmp, tick);
    if (ret != LOS_OK) {
        osal_log("LOS_MuxPend failed! ret = %#x.\n", ret);
    }
    return (int)ret;
}

int osal_mutex_lock_interruptible(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return OSAL_FAILURE;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    unsigned int ret = LOS_MuxPend(mutex_tmp, LOS_WAIT_FOREVER);
    if (ret == LOS_ERRNO_MUX_PEND_INTERR) {
        osal_log("The mutex is being locked during an interrupt!\n");
    } else if (ret != LOS_OK) {
        osal_log("LOS_MuxPend failed! ret = %#x.\n", ret);
    }

    return (int)ret;
}

int osal_mutex_trylock(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return FALSE;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    return (int)LOS_MuxPend(mutex_tmp, LOS_NO_WAIT);
}

void osal_mutex_unlock(osal_mutex *mutex)
{
    if (mutex == NULL) {
        osal_log("parameter invalid!\n");
        return;
    }
    unsigned int mutex_tmp = (unsigned int)(UINTPTR)mutex->mutex ^ MUTEX_MAGIC_WORD;
    unsigned int ret = LOS_MuxPost(mutex_tmp);
    if (ret != LOS_OK) {
        osal_log("LOS_MuxPost failed! ret = %#x.\n", ret);
    }
}

int osal_mutex_is_locked(osal_mutex *mutex)
{
    osal_unused(mutex);
    return -1;
}
