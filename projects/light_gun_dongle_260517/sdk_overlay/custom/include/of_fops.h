#ifndef OF_FOPS_H
#define OF_FOPS_H

#include <stdint.h>

typedef struct of_dev of_dev_t;

typedef struct {
    int (*open)(void *ctx);
    int (*close)(void *ctx);
    int (*read)(void *ctx, uint8_t *buf, uint32_t len, uint32_t *out_len);
    int (*write)(void *ctx, const uint8_t *buf, uint32_t len, uint32_t *out_len);
    int (*ioctl)(void *ctx, uint32_t cmd, void *arg);
} of_fops_t;

struct of_dev {
    const char *name;
    const of_fops_t *ops;
    void *priv;
};

#endif
