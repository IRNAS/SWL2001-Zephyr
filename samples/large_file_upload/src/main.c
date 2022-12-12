/** @file main.c
 *
 * @brief Large file upload example
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <smtc_modem_api.h>

#include <smtc_app.h>
#include <smtc_modem_api_str.h>

/* include hal and ralf so that initialization can be done */
#include <ralf_lr11xx.h>
#include <smtc_modem_hal.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

#include <file.h>

LOG_MODULE_REGISTER(main);

/* ---------------- Function declarations ---------------- */

static void on_modem_reset(uint16_t reset_count);
static void on_modem_network_joined(void);
static void on_modem_upload_done(smtc_modem_event_uploaddone_status_t status);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief Large file upload encryption mode
 */
#define APP_SMTC_MODEM_LFU_CIPHER_MODE SMTC_MODEM_FILE_UPLOAD_NO_CIPHER

/**
 * @brief Size in byte of the file to be uploaded (from 1 to 2048 included)
 *
 * 2048 is the limit here since that is the size of the text array.
 * smtc_modem_file_upload_init can be called with a maximum size of 8180 bytes.
 */
#define APP_SMTC_MODEM_LFU_SIZE 256

/**
 * @brief Average delay in second between two uplinks
 */
#define APP_SMTC_MODEM_LFU_AVERAGE_DELAY 10

/* ---------------- LoRaWAN Configurations ---------------- */

/* Stack id value (multistack modem is not yet available, so use 0) */
#define STACK_ID 0

/*LoRaWAN configuration */
static struct smtc_app_lorawan_cfg lorawan_cfg = {
	.use_chip_eui_as_dev_eui = true,
	.join_eui = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
	.app_key = {0x15, 0x1B, 0x76, 0x0D, 0xED, 0x74, 0x87, 0xE7, 0x8A, 0xCA, 0xE8, 0xC8, 0x74,
		    0x16, 0x31, 0x19},
	.class = SMTC_MODEM_CLASS_A,
	.region = SMTC_MODEM_REGION_EU_868,
};

static struct smtc_app_event_callbacks event_callbacks = {
	.reset = on_modem_reset,
	.joined = on_modem_network_joined,
	.upload_done = on_modem_upload_done,

};

/* lr11xx radio context and its use in the ralf layer */
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr1120)));

/* ---------------- Main ---------------- */

K_SEM_DEFINE(main_sleep_sem, 0, 1);

void main(void)
{
	/* configure LoRaWAN modem */

	/* Init the modem and use provided event callbacks.
	 * Please note that the reset callback will be called immediately after the first call to
	 * smtc_modem_run_engine because of reset detection.
	 */
	smtc_app_init(&modem_radio, &event_callbacks, NULL);
	smtc_app_display_versions();

	/* Enter main loop:
	 * The fist call to smtc_modem_run_engine will trigger the reset callback.
	 */

	while (1) {
		/* Execute modem runtime, this function must be called again in sleep_time_ms
		 * milliseconds or sooner. */
		uint32_t sleep_time_ms = smtc_modem_run_engine();

		LOG_INF("Sleeping for %d ms", sleep_time_ms);
		k_sleep(K_MSEC(sleep_time_ms));
	}
}

/**
 * @brief Reset event callback
 *
 * @param [in] reset_count reset counter from the modem
 */
static void on_modem_reset(uint16_t reset_count)
{
	/* configure lorawan parameters after reset and start join sequence */
	smtc_app_configure_lorawan_params(STACK_ID, &lorawan_cfg);
	smtc_modem_join_network(STACK_ID);
}

/**
 * @brief Network Joined event callback
 */
static void on_modem_network_joined(void)
{
	/* Successfully joined, configure start file upload
	 */

	LOG_INF("Launching a file upload session.");
	smtc_modem_return_code_t ret = smtc_modem_file_upload_init(
		STACK_ID, 0, APP_SMTC_MODEM_LFU_CIPHER_MODE, text, APP_SMTC_MODEM_LFU_SIZE,
		APP_SMTC_MODEM_LFU_AVERAGE_DELAY);

	if (ret != SMTC_MODEM_RC_OK) {
		LOG_ERR("Unable to initialize file upload, err: %d", ret);
		/* NOTE: if ret == SMTC_MODEM_RC_FAIL, you might have forgotten to set
		 * CONFIG_LORA_BASICS_MODEM_FILE_UPLOAD
		 */
	}

	smtc_modem_file_upload_start(STACK_ID);
}

void on_modem_upload_done(smtc_modem_event_uploaddone_status_t status)
{
	LOG_INF("Upload done. Status: %s", smtc_modem_event_uploaddone_status_to_str(status));
}