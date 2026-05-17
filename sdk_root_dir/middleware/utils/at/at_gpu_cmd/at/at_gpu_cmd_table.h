/**
 * Copyright (c) Triductor. 2024-2024. All rights reserved.
 *
 * Description: At gpu cmd table \n
 *
 */

#if !defined(AT_GPU_CMD_TALBE_H)
#define AT_GPU_CMD_TALBE_H

#include "at.h"

typedef struct {
    uint32_t             para_map;
    char                 *para1;
    char                 *para2;
    char                 *para3;
    char                 *para4;
    char                 *para5;
    char                 *para6;
    char                 *para7;
    char                 *para8;
    char                 *para9;
    char                 *para10;
    char                 *para11;
    char                 *para12;
    char                 *para13;
    char                 *para14;
    char                 *para15;
    char                 *para16;
    char                 *para17;
    char                 *para18;
    char                 *para19;
    char                 *para20;
} gpu_args_t;

/* AT+GPU */
at_ret_t at_gpu_sample_process(const gpu_args_t *args);

const at_para_parse_syntax_t g_gpu_parse_syntax[] = {
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para1)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para2)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para3)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para4)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para5)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para6)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para7)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para8)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para9)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para10)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para11)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para12)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para13)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para14)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para15)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para16)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para17)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para18)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .offset = offsetof(gpu_args_t, para19)
    },
    {
        .type = AT_SYNTAX_TYPE_STRING,
        .attribute = AT_SYNTAX_ATTR_OPTIONAL| AT_SYNTAX_ATTR_FIX_CASE | AT_SYNTAX_ATTR_MAX_LENGTH,
        .entry.string.max_length = 100,
        .last = true,
        .offset = offsetof(gpu_args_t, para20)
    },
};

const at_cmd_entry_t g_at_gpu_cmd_parse_table[] = {
    {
        "GPU",                             // name
        1,                                 // ID
        0,                                 // ATTRIBUTE
        g_gpu_parse_syntax,                   // SYNTAX
        NULL,                              // cmd with no args, eg: AT+GPU
        (at_set_func_t)at_gpu_sample_process, // cmd with args, eg: AT+GPU=XX,XX,XX,XX
        NULL,                              // read cmd, eg: AT+GPU?
        NULL,                              // test cmd, eg: AT+GPU=?
    },
};

#endif  /* AT_GPU_CMD_AT_CMD_TALBE_H */