/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: Provides v151 hal sio \n
 *
 * History: \n
 * 2023-03-07, Create file. \n
 */
#ifndef HAL_SIO_V151_H
#define HAL_SIO_V151_H

#include "hal_sio_v151_regs_op.h"
#include "hal_sio.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/**
 * @defgroup drivers_hal_sio_v151 SIO V151
 * @ingroup  drivers_hal_sio
 * @{
 */

/**
 * @if Eng
 * @brief  Initialize device for hal sio.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @retval ERRCODE_SUCC   Success.
 * @retval Other          Failure. For details, see @ref errcode_t.
 * @else
 * @brief  HAL层sio的初始化接口
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @retval ERRCODE_SUCC 成功
 * @retval Other        失败，参考 @ref errcode_t.
 * @endif
 */
errcode_t hal_sio_v151_init(sio_bus_t bus);

/**
 * @if Eng
 * @brief  Deinitialize device for hal sio.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @else
 * @brief  HAL层sio的去初始化接口
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @endif
 */
void hal_sio_v151_deinit(sio_bus_t bus);

/**
 * @if Eng
 * @brief  Configure parameters for hal sio.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  config The sio attributes. see @ref hal_sio_config_t.
 * @else
 * @brief  HAL层sio配置参数接口
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  config sio配置参数， see @ref hal_sio_config_t.
 * @endif
 */
void hal_sio_v151_set_config(sio_bus_t bus, const hal_sio_config_t *config);

/**
 * @if Eng
 * @brief  Get configuration parameters for hal sio.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  config The sio attributes. see @ref hal_sio_config_t.
 * @retval ERRCODE_SUCC   Success.
 * @retval Other          Failure. For details, see @ref errcode_t.
 * @else
 * @brief  获取HAL层sio配置参数接口
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  config sio配置参数， see @ref hal_sio_config_t.
 * @retval ERRCODE_SUCC 成功
 * @retval Other        失败，参考 @ref errcode_t.
 * @endif
 */
errcode_t hal_sio_v151_get_config(sio_bus_t bus, hal_sio_config_t *config);

/**
 * @if Eng
 * @brief  Hal sio rx config, start read data.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  en Config rx enable or not.
 * @else
 * @brief  HAL层sio rx配置，开始读数据
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  en rx是否使能。
 * @endif
 */
void hal_sio_v151_rx_enable(sio_bus_t bus, bool en);

/**
 * @if Eng
 * @brief  Hal sio tx config, start write data.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  data Config tx data. see @ref hal_sio_tx_data_t.
 * @param  [in]  mode Config sio mode(i2s or pcm). see @ref hal_sio_mode_t.
 * @else
 * @brief  HAL层sio tx配置，开始发送数据
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  data 配置的tx数据，参考 @ref hal_sio_tx_data_t.
 * @param  [in]  mode 配置sio模式(i2s or pcm)， 参考 @ref hal_sio_mode_t.
 * @endif
 */
void hal_sio_v151_write(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode);

/**
 * @if Eng
 * @brief  Get hal sio receive data.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  data rx data buffer. see @ref hal_sio_rx_data_t.
 * @else
 * @brief  获取HAL层sio接收到的数据
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  data rx数据buffer，参考 @ref hal_sio_rx_data_t.
 * @endif
 */
void hal_sio_v151_get_data(sio_bus_t bus, hal_sio_rx_data_t *data);

#if defined(CONFIG_I2S_SUPPORT_LOOPBACK)
/**
 * @if Eng
 * @brief  Set hal sio loop mode.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  en Config loop mode or not.
 * @else
 * @brief  设置HAL层sio是否为loop自测模式
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  en 是否使能loop自测模式.
 * @endif
 */
void hal_sio_v151_loop(sio_bus_t bus, bool en);

/**
 * @if Eng
 * @brief  Hal sio loop transfer data.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  data Config tx data. see @ref hal_sio_tx_data_t.
 * @param  [in]  mode Config sio mode(i2s or pcm). see @ref hal_sio_mode_t.
 * @else
 * @brief  HAL层sio loop模式传输数据
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  data 配置的tx数据，参考 @ref hal_sio_tx_data_t.
 * @param  [in]  mode 配置sio模式(i2s or pcm)， 参考 @ref hal_sio_mode_t.
 * @endif
 */
void hal_sio_v151_loop_trans(sio_bus_t bus, hal_sio_tx_data_t *data, hal_sio_mode_t mode);
#endif /* CONFIG_I2S_SUPPORT_LOOPBACK */

#if defined(CONFIG_I2S_SUPPORT_DMA)
/**
 * @if Eng
 * @brief  Set hal sio dma configuration.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  attr Config sio dma attributes. see @ref hal_i2s_dma_attr_t.
 * @else
 * @brief  设置HAL层sio dma模式相关配置参数
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  attr 配置sio dma相关配置参数， 参考 @ref hal_i2s_dma_attr_t.
 * @endif
 */
void hal_sio_v151_dma_cfg(sio_bus_t bus, hal_i2s_dma_attr_t *attr);
#endif /* CONFIG_I2S_SUPPORT_DMA */

/**
 * @if Eng
 * @brief  hal sio register interrupt handler callback.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  callback The interrupt handler callback. see @ref hal_sio_callback_t.
 * @else
 * @brief  注册HAL层sio中断处理回调函数
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  callback 中断处理回调函数，参考 @ref hal_sio_callback_t.
 * @endif
 */
void hal_sio_v151_register(sio_bus_t bus, hal_sio_callback_t callback);

/**
 * @if Eng
 * @brief  hal sio unregister interrupt handler callback.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @else
 * @brief  去注册HAL层sio中断处理回调函数
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @endif
 */
void hal_sio_v151_unregister(sio_bus_t bus);

/**
 * @if Eng
 * @brief  Handler of the sio interrupt request.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @else
 * @brief  SIO中断处理函数
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @endif
 */
void hal_sio_v151_irq_handler(sio_bus_t bus);

/**
 * @if Eng
 * @brief  sio crg clock enable.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  enable true or false.
 * @else
 * @brief  sio crg clock 使能。
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  enable true 或者 false.
 * @endif
 */
void hal_sio_v151_crg_clock_enable(sio_bus_t bus, bool enable);

/**
 * @if Eng
 * @brief  sio tx enable.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  val 0 or 1.
 * @else
 * @brief  sio tx 使能。
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  val 0 或者 1.
 * @endif
 */
void hal_sio_v151_set_tx_enable(sio_bus_t bus, uint32_t val);

/**
 * @if Eng
 * @brief  sio rx enable.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @param  [in]  val 0 or 1.
 * @else
 * @brief  sio rx 使能。
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @param  [in]  val 0 或者 1.
 * @endif
 */
void hal_sio_v151_set_rx_enable(sio_bus_t bus, uint32_t val);

/**
 * @if Eng
 * @brief  sio tx rx enable.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @else
 * @brief  sio 接收发送使能。
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @endif
 */
void hal_sio_v151_txrx_enable(sio_bus_t bus);

/**
 * @if Eng
 * @brief  sio tx rx disable.
 * @param  [in]  bus The sio bus. see @ref sio_bus_t.
 * @else
 * @brief  sio 接收发送去使能。
 * @param  [in]  bus 串口号， 参考 @ref sio_bus_t.
 * @endif
 */
void hal_sio_v151_txrx_disable(sio_bus_t bus);

/**
 * @}
 */

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif