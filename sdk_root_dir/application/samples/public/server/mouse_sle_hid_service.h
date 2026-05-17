/**
 * Copyright (c) Triductor 2024-2024. All rights reserved. \n
 *
 * Description: Mouse SLE Server Service Header\n
 * Author: Triductor \n
 * History: \n
 * 2024-08-01, Create file. \n
 */
#ifndef MOUSE_SLE_HID_SERVICE_H
#define MOUSE_SLE_HID_SERVICE_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define SLE_HID_REPORT_ID    0x01
errcode_t sle_server_add_and_start_hid_service(uint8_t server_id, uint16_t* property_ntf_hdl);
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif