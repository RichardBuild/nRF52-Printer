#include "usb_msc.h"

#include <zephyr/device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(app_usbd);

/*
 * Instantiate a context named app_usbd using the default USB device
 * controller, the Zephyr project vendor ID, and the sample product ID.
 * Zephyr project vendor ID must not be used outside of Zephyr samples.
 */

// app_usbd is a usbd_context variable.
USBD_DEVICE_DEFINE(app_usbd,
				   DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)),
				   CONFIG_SAMPLE_USBD_VID, CONFIG_SAMPLE_USBD_PID);

USBD_DESC_LANG_DEFINE(app_lang);
USBD_DESC_MANUFACTURER_DEFINE(app_mfr, CONFIG_SAMPLE_USBD_MANUFACTURER);
USBD_DESC_PRODUCT_DEFINE(app_product, CONFIG_SAMPLE_USBD_PRODUCT);
IF_ENABLED(CONFIG_HWINFO, (USBD_DESC_SERIAL_NUMBER_DEFINE(app_sn)));

USBD_DESC_CONFIG_DEFINE(fs_cfg_desc, "FS Configuration");

static const uint8_t attributes = 0; // our device as no attributes

/* Full speed configuration */
USBD_CONFIGURATION_DEFINE(app_fs_config,
						  attributes,
						  CONFIG_SAMPLE_USBD_MAX_POWER, &fs_cfg_desc);

static struct usbd_context *app_usbd_setup_device(usbd_msg_cb_t msg_cb)
{
	int err;

	err = usbd_add_descriptor(&app_usbd, &app_lang);
	if (err)
	{
		LOG_ERR("Failed to initialize language descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&app_usbd, &app_mfr);
	if (err)
	{
		LOG_ERR("Failed to initialize manufacturer descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_descriptor(&app_usbd, &app_product);
	if (err)
	{
		LOG_ERR("Failed to initialize product descriptor (%d)", err);
		return NULL;
	}

	IF_ENABLED(CONFIG_HWINFO, (
								  err = usbd_add_descriptor(&app_usbd, &app_sn);))
	if (err)
	{
		LOG_ERR("Failed to initialize SN descriptor (%d)", err);
		return NULL;
	}

	err = usbd_add_configuration(&app_usbd, USBD_SPEED_FS,
								 &app_fs_config);
	if (err)
	{
		LOG_ERR("Failed to add Full-Speed configuration");
		return NULL;
	}

	err = usbd_register_all_classes(&app_usbd, USBD_SPEED_FS, 1, NULL);
	if (err)
	{
		LOG_ERR("Failed to add register classes");
		return NULL;
	}

	if (msg_cb != NULL)
	{
		err = usbd_msg_register_cb(&app_usbd, msg_cb);
		if (err)
		{
			LOG_ERR("Failed to register message callback");
			return NULL;
		}
	}

	return &app_usbd;
}

struct usbd_context *app_usbd_init_device(usbd_msg_cb_t msg_cb)
{
	int err;

	if (app_usbd_setup_device(msg_cb) == NULL)
	{
		return NULL;
	}

	err = usbd_init(&app_usbd);
	if (err)
	{
		LOG_ERR("Failed to initialize device support");
		return NULL;
	}

	return &app_usbd;
}
