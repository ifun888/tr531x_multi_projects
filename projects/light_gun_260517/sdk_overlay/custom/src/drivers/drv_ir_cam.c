#include "drivers/drv_ir_cam.h"
#include "i2c.h"
#include "pinctrl.h"
#include "common_def.h"
#include <stdint.h>
#include <string.h>

#ifndef OF_IR_I2C_BUS
#define OF_IR_I2C_BUS 0
#endif
#ifndef OF_IR_I2C_ADDR
#define OF_IR_I2C_ADDR 0x58
#endif

static int g_opened;
static int g_ready;
static uint16_t g_last_x;
static uint16_t g_last_y;
static uint8_t g_last_valid;

static int ir_open(void *ctx)
{
    errcode_t ret;
    (void)ctx;
#if defined(CONFIG_I2C_MASTER_SCL_PIN) && defined(CONFIG_I2C_MASTER_SCL_PIN_MODE)
    (void)uapi_pin_set_mode(CONFIG_I2C_MASTER_SCL_PIN, CONFIG_I2C_MASTER_SCL_PIN_MODE);
#endif
#if defined(CONFIG_I2C_MASTER_SDA_PIN) && defined(CONFIG_I2C_MASTER_SDA_PIN_MODE)
    (void)uapi_pin_set_mode(CONFIG_I2C_MASTER_SDA_PIN, CONFIG_I2C_MASTER_SDA_PIN_MODE);
#endif
    ret = uapi_i2c_master_init((i2c_bus_t)OF_IR_I2C_BUS, 400000U, 0);
    g_opened = 1;
    g_ready = (ret == ERRCODE_SUCC);
    g_last_x = 0;
    g_last_y = 0;
    g_last_valid = 0;
    return 0;
}

static int ir_close(void *ctx)
{
    (void)ctx;
    (void)uapi_i2c_deinit((i2c_bus_t)OF_IR_I2C_BUS);
    g_opened = 0;
    g_ready = 0;
    return 0;
}

static int ir_read(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    uint8_t tx[1] = {0x00};
    uint8_t rx[5] = {0};
    i2c_data_t data;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((g_opened == 0) || (buf == 0) || (out_len == 0) || (len < 5U)) {
        return -1;
    }

    (void)memset(&data, 0, sizeof(data));
    data.send_buf = tx;
    data.send_len = sizeof(tx);
    data.receive_buf = rx;
    data.receive_len = sizeof(rx);

    if (uapi_i2c_master_writeread((i2c_bus_t)OF_IR_I2C_BUS, OF_IR_I2C_ADDR, &data) == ERRCODE_SUCC) {
        g_last_x = (uint16_t)rx[0] | ((uint16_t)rx[1] << 8);
        g_last_y = (uint16_t)rx[2] | ((uint16_t)rx[3] << 8);
        g_last_valid = rx[4] & 0x01U;
        g_ready = 1;
    } else {
        g_last_valid = 0;
        g_ready = 0;
    }

    buf[0] = (uint8_t)(g_last_x & 0xFFU);
    buf[1] = (uint8_t)((g_last_x >> 8) & 0xFFU);
    buf[2] = (uint8_t)(g_last_y & 0xFFU);
    buf[3] = (uint8_t)((g_last_y >> 8) & 0xFFU);
    buf[4] = g_last_valid;
    *out_len = 5U;
    return 0;
}

static int ir_write(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    i2c_data_t data;
    (void)ctx;
    if (out_len != 0) {
        *out_len = 0;
    }
    if ((buf == 0) || (len == 0U) || (out_len == 0)) {
        return -1;
    }
    (void)memset(&data, 0, sizeof(data));
    data.send_buf = (uint8_t *)buf;
    data.send_len = len;
    if (uapi_i2c_master_write((i2c_bus_t)OF_IR_I2C_BUS, OF_IR_I2C_ADDR, &data) == ERRCODE_SUCC) {
        *out_len = len;
        return 0;
    }
    return -1;
}

static int ir_ioctl(void *ctx, uint32_t cmd, void *arg)
{
    (void)ctx;
    (void)cmd;
    (void)arg;
    return 0;
}

static const of_fops_t g_ops = {
    .open = ir_open,
    .close = ir_close,
    .read = ir_read,
    .write = ir_write,
    .ioctl = ir_ioctl,
};

static of_dev_t g_dev = {
    .name = "ir_cam",
    .ops = &g_ops,
    .priv = 0,
};

const of_dev_t *drv_ir_cam_get_dev(void)
{
    return &g_dev;
}

int drv_ir_cam_is_ready(void)
{
    return (g_opened != 0) && (g_ready != 0);
}
