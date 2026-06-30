#include "of_transport.h"
#include "of_fops.h"
#include "drivers/drv_usb_cdc.h"
#include "drivers/drv_sle_link.h"

static const of_dev_t *g_dev = 0;
static of_transport_type_t g_type = OF_TRANSPORT_SLE;

int of_transport_init(of_transport_type_t type)
{
    g_type = type;
    g_dev = (type == OF_TRANSPORT_USB_CDC) ? drv_usb_cdc_get_dev() : drv_sle_link_get_dev();
    if ((g_dev == 0) || (g_dev->ops == 0) || (g_dev->ops->open == 0)) {
        return -1;
    }
    return g_dev->ops->open(g_dev->priv);
}

int of_transport_read(uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    if ((g_dev == 0) || (g_dev->ops == 0) || (g_dev->ops->read == 0)) {
        return -1;
    }
    return g_dev->ops->read(g_dev->priv, buf, len, out_len);
}

int of_transport_write(const uint8_t *buf, uint32_t len, uint32_t *out_len)
{
    if ((g_dev == 0) || (g_dev->ops == 0) || (g_dev->ops->write == 0)) {
        return -1;
    }
    return g_dev->ops->write(g_dev->priv, buf, len, out_len);
}

int of_transport_deinit(void)
{
    if ((g_dev == 0) || (g_dev->ops == 0) || (g_dev->ops->close == 0)) {
        return -1;
    }
    return g_dev->ops->close(g_dev->priv);
}

of_transport_type_t of_transport_get_type(void)
{
    return g_type;
}
