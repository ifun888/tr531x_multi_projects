/**
 * Copyright (c) Triductor. 2023-2023. All rights reserved.
 *
 * Description: Calendar Sample Source. \n
 *
 * History: \n
 * 2023-09-18, Create file. \n
 */
#include "calendar.h"
#include "common_def.h"
#include "soc_osal.h"
#include "app_init.h"

#define CALENDAR_DATE_AFTER_SEC         30
#define CALENDAR_DATE_AFTER_MIN         30
#define CALENDAR_DATE_AFTER_HOUR        1
#define CALENDAR_DATE_AFTER_DAY         1
#define CALENDAR_DATE_AFTER_MON         1
#define CALENDAR_DATE_AFTER_YEAR        2000
#define CALENDAR_TIMESTAM_AFTER         1694766630

#define CALENDAR_TASK_PRIO              24
#define CALENDAR_TASK_STACK_SIZE        0x1000

static void *calendar_task(const char *arg)
{
    unused(arg);
    calendar_t date_base;
    calendar_t date_current;
    calendar_t date_after = { CALENDAR_DATE_AFTER_SEC, CALENDAR_DATE_AFTER_MIN, CALENDAR_DATE_AFTER_HOUR,
                              CALENDAR_DATE_AFTER_DAY, CALENDAR_DATE_AFTER_MON, CALENDAR_DATE_AFTER_YEAR };
    uint64_t timestamp = CALENDAR_TIMESTAM_AFTER; /* This value is a timestamp in milliseconds */
    uint64_t timestamp_current;

    uapi_calendar_init();

    if (uapi_calendar_get_datetime(&date_base) == ERRCODE_SUCC) {
        /* Get the current time of the calendar, the default value is 1970-1-1 0:0:0 */
        osal_printk("get date_base success: %d-%d-%d %d:%d:%d\r\n", date_base.year, date_base.mon,
                    date_base.day, date_base.hour, date_base.min, date_base.sec);
    }

    if (uapi_calendar_set_timestamp(timestamp) == ERRCODE_SUCC) {
        osal_printk("set timestamp success: %llu\r\n", timestamp);
    }

    if (uapi_calendar_get_datetime(&date_current) == ERRCODE_SUCC) {
        osal_printk("get date_current success: %d-%d-%d %d:%d:%d\r\n", date_current.year, date_current.mon,
                    date_current.day, date_current.hour, date_current.min, date_current.sec);
    }

    if (uapi_calendar_get_timestamp(&timestamp_current) == ERRCODE_SUCC) {
        osal_printk("get timestamp_current success: %llu\r\n", timestamp_current);
    }

    if (uapi_calendar_set_datetime(&date_after) == ERRCODE_SUCC) {
        osal_printk("set date_after success: %d-%d-%d %d:%d:%d\r\n", date_after.year, date_after.mon, date_after.day,
                    date_after.hour, date_after.min, date_after.sec);
    }

    return NULL;
}

static void calendar_entry(void)
{
    osal_task *task_handle = NULL;
    osal_kthread_lock();
    task_handle = osal_kthread_create((osal_kthread_handler)calendar_task, 0, "CalendarTask", CALENDAR_TASK_STACK_SIZE);
    if (task_handle != NULL) {
        osal_kthread_set_priority(task_handle, CALENDAR_TASK_PRIO);
    }
    osal_kthread_unlock();
}

/* Run the calendar_entry. */
app_run(calendar_entry);