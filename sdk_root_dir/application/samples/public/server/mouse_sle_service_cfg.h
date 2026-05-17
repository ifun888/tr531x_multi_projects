/**
 * Copyright (c) Triductor 2024-2024. All rights reserved. \n
 *
 * Description: Mouse SLE Server Service Header\n
 * Author: Triductor \n
 * History: \n
 * 2024-08-01, Create file. \n
 */
#ifndef MOUSE_SLE_SERVICE_CFG_H
#define MOUSE_SLE_SERVICE_CFG_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

typedef enum {
    SLE_SERVICE_INDEX0, // dis service
    SLE_SERVICE_INDEX1, // name
    SLE_SERVICE_INDEX2, // appearance
    SLE_SERVICE_INDEX3, // pnp id
    SLE_SERVICE_INDEX4, // pnp id
    SLE_SERVICE_INDEX5, // pnp id
    SLE_SERVICE_INDEX6, // pnp id
    SLE_SERVICE_INDEX7, // pnp id
    SLE_SERVICE_INDEX8, // pnp id
} sle_service_index_t;

errcode_t sle_service_add_descriptor_interface(uint8_t server_id, uint16_t service_hdl, uint32_t property_operate, uint16_t len, uint8_t *data, uint16_t property_handle);
errcode_t sle_service_add_property_interface(uint8_t server_id, uint16_t service_hdl, const uint8_t *property_uuid, uint32_t property_operate, uint16_t len, uint8_t *data, uint16_t* property_hdl);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif