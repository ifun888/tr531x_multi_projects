/*
 * Copyright (c) Triductor. 2021-2023. All rights reserved.
 * Description: transmit
 * This file should be changed only infrequently and with great care.
 */
#include "transmit_write_read.h"
#include "securec.h"
#include "dfx_feature_config.h"
#if defined(CONFIG_DFX_SUPPORT_FILE_SYSTEM) && (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
#include "dfx_file_operation.h"
#endif
#include "transmit_debug.h"
#include "dfx_adapt_layer.h"

int32_t file_read_data(uintptr_t usr_data, uint32_t offset, uint8_t *buf, uint32_t len)
{
    dfx_assert(buf);
    uint32_t read_offset = offset;
    transmit_item_t *item = (transmit_item_t *)usr_data;
    dfx_log_debug("[WR][file_read_data][file_name=%s][offset=0x%x][len=0x%x]\r\n", item->local_file_name, offset, len);
#if defined(CONFIG_DFX_SUPPORT_FILE_SYSTEM) && (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
    return dfx_file_read_fd(item->file_fd, read_offset, buf, len, false);
#else
    uint8_t opt_type;
    switch (item->transmit_type) {
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
            memcpy_s((void *)(uintptr_t)buf, len, (void *)(uintptr_t)(item->local_bus_addr + read_offset), len);
            return (int32_t)len;

        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
            opt_type = FLASH_OP_TYPE_OTA;
            break;
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
            opt_type = FLASH_OP_TYPE_FLASH_DATA;
            read_offset += item->local_bus_addr;
            break;
        default:
            return -1;
    }
    return dfx_flash_read(opt_type, read_offset, buf, len);
#endif
}

int32_t file_write_data(uintptr_t usr_data, uint32_t offset, uint8_t *buf, uint32_t len)
{
    dfx_assert(buf);
    uint32_t write_offset = offset;
    transmit_item_t *item = (transmit_item_t *)usr_data;
    dfx_log_debug("[WR][file_write_data][file_name=%s][offset=0x%x][len=0x%x]\r\n", item->local_file_name, offset, len);
#if defined(CONFIG_DFX_SUPPORT_FILE_SYSTEM) && (CONFIG_DFX_SUPPORT_FILE_SYSTEM == DFX_YES)
    return dfx_file_write_fd(item->file_fd, write_offset, buf, len);
#else
    unused(item);
    uint8_t opt_type;
    switch (item->transmit_type) {
        case TRANSMIT_TYPE_MEMORY_UPSTREAM:
        case TRANSMIT_TYPE_MEMORY_DOWNSTREAM:
            memcpy_s((void *)(uintptr_t)(item->local_bus_addr + write_offset), len, (void *)(uintptr_t)buf, len);
            return (int32_t)len;

        case TRANSMIT_TYPE_OTA_IMG_DOWNSTREAM:
        case TRANSMIT_TYPE_OTA_IMG_UPSTREAM:
            opt_type = FLASH_OP_TYPE_OTA;
            break;
        case TRANSMIT_TYPE_FLASH_UPSTREAM:
        case TRANSMIT_TYPE_FLASH_DOWNSTREAM:
            opt_type = FLASH_OP_TYPE_FLASH_DATA;
            write_offset += item->local_bus_addr;
            break;
        default:
            return -1;
    }
    return dfx_flash_write(opt_type, write_offset, buf, len, false);
#endif
}
