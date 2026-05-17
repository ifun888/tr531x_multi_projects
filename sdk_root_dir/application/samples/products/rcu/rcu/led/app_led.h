/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: APP LED. \n
 *
 * History: \n
 * 2024-05-23, Create file. \n
 */
#ifndef RCU_LED_H
#define RCU_LED_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* 指示灯时长 */
#define SHORT_BLINK_TIME  200
#define LONG_BLINK_TIME   500
 
/* 配对指示灯 */
#define SLE_RCU_PAIR_LED_BLINK_TIME     SHORT_BLINK_TIME
#define SLE_RCU_PAIR_LED_BLINK_TIMEOUT  8000
/* 配对成功指示灯 */
#define SLE_RCU_PAIR_SUCCESS_LED_BLINK_TIME    500
#define SLE_RCU_PAIR_SUCCESS_LED_BLINK_TIMEOUT 2000
/* 解配指示灯 */
#define SLE_RCU_UNPAIR_LED_BLINK_TIME    SHORT_BLINK_TIME
#define SLE_RCU_UNPAIR_LED_BLINK_TIMEOUT 8000
/* 红外学习成功指示灯 */
#define IR_STUDY_SUCCESS_LED_BLINK_TIME    SHORT_BLINK_TIME
#define IR_STUDY_SUCCESS_LED_BLINK_TIMEOUT 4000
/* 红外学习等待指示灯 */
#define IR_STUDY_WAIT_LED_BLINK_TIME    LONG_BLINK_TIME
#define IR_STUDY_WAIT_LED_BLINK_TIMEOUT 10000

#define LED_TIMER_INIT_INTERVAL 100

#define LED_OPEN_GPIO_LEVEL CONFIG_SAMPLE_LED_OPEN_LEVEL
#define LED_CLOSE_GPIO_LEVEL (!(CONFIG_SAMPLE_LED_OPEN_LEVEL))

enum LED_COLOR {
    NO_LED = 0,
    RED_LED = 1,
};

typedef enum {
    LED_STATUS_IDLE = 0,
    LED_STATUS_SINGLETONE,

    LED_STATUS_IRLEARN_SUCESS,
    LED_STATUS_IRLEARN,
    LED_STATUS_UNPAIR,
    LED_STATUS_VOICE,
    LED_STATUS_PAIRSUCESS,
    LED_STATUS_PAIR,
    LED_STATUS_SMARTCTL,
    LED_STATUS_IR_CHANGE,
    LED_STATUS_IR_STUDY_MODE_CHANGE,
    LED_STATUS_IR_STUDY_ERASE,
    LED_STATUS_LOWPOWER,
    LED_STATUS_COMBINEKEY,
    LED_STATUS_IRKEY,
    LED_STATUS_BLEKEY,
    LED_STATUS_UNKNOW,
} LED_STATUS;

void start_led_timer(uint8_t color, uint32_t led_timer_period, uint32_t led_timer_timeout, LED_STATUS ledstatus);
void stop_led_timer(void);
void app_led_init(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif