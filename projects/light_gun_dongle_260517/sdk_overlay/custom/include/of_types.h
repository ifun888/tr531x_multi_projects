#ifndef OF_TYPES_H
#define OF_TYPES_H

typedef enum {
    OF_BOOT_EARLY = 0,
    OF_BOOT_DRIVER_INIT,
    OF_BOOT_CONFIG_LOAD,
    OF_BOOT_SELFTEST,
    OF_BOOT_LINK_SELECT,
    OF_BOOT_READY,
    OF_BOOT_FAIL,
} of_boot_state_t;

typedef enum {
    OF_MODE_RUN = 0,
    OF_MODE_PAUSE,
    OF_MODE_CALIB,
    OF_MODE_DOCKED,
    OF_MODE_FAULT,
} of_mode_t;

typedef enum {
    OF_LINK_NONE = 0,
    OF_LINK_USB_CDC,
    OF_LINK_SLE,
    OF_LINK_DUAL,
} of_link_type_t;

typedef enum {
    OF_USB_DETACHED = 0,
    OF_USB_ENUMING,
    OF_USB_MOUNTED,
    OF_USB_SUSPENDED,
    OF_USB_ERROR,
} of_usb_state_t;

typedef enum {
    OF_SLE_OFF = 0,
    OF_SLE_INIT,
    OF_SLE_SCANNING,
    OF_SLE_PAIRING,
    OF_SLE_CONNECTED,
    OF_SLE_DEGRADED,
    OF_SLE_RECOVERING,
    OF_SLE_ERROR,
} of_sle_state_t;

typedef enum {
    OF_ST_IR = 0,
    OF_ST_INPUT,
    OF_ST_USB,
    OF_ST_SLE,
    OF_ST_STORAGE,
    OF_ST_FEEDBACK,
    OF_ST_TEMP,
    OF_ST_MAX,
} of_selftest_item_t;

typedef enum {
    OF_ST_RES_NOT_RUN = 0,
    OF_ST_RES_PASS,
    OF_ST_RES_WARN,
    OF_ST_RES_FAIL,
} of_selftest_result_t;

#endif
