/** @file main.c
 *
 * @brief Almanac update example
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
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

/* ---------------- Function declarations ---------------- */

static void on_modem_reset(uint16_t reset_count);
static void on_modem_network_joined(void);
static void on_modem_almanac_update(smtc_modem_event_almanac_update_status_t status);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief LoRaWAN fport used by the DM service (default value on TTI/TTN is 199)
 */
#define APP_MODEM_DM_LORAWAN_FPORT 199

/**
 * @brief Application device management interval in minute
 *
 * The inverval between automatic uplinks of the device management (DM) payload
 *
 */
#define APP_MODEM_DM_INTERVAL 1

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
	.almanac_update = on_modem_almanac_update,

};

/* lr11xx radio context and its use in the ralf layer */
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr11xx)));

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
	/* Configure DM info to only send almanac status. We could send additional fields as well.
	 */
	const uint8_t dm_info_fields[1] = {SMTC_MODEM_DM_FIELD_ALMANAC_STATUS};
	smtc_modem_dm_set_info_fields(dm_info_fields, 1);
	smtc_modem_dm_set_info_interval(SMTC_MODEM_DM_INFO_INTERVAL_IN_MINUTE,
					APP_MODEM_DM_INTERVAL);
}

void on_modem_almanac_update(smtc_modem_event_almanac_update_status_t status)
{
	/*Request a DM uplink instead of waiting for the periodic
	 * report to share the almanac status.
	 */

	LOG_INF("Almanac has been updated: %s",
		smtc_modem_event_almanac_update_status_to_str(status));

	const uint8_t dm_info_fields[1] = {SMTC_MODEM_DM_FIELD_ALMANAC_STATUS};
	smtc_modem_dm_request_single_uplink(dm_info_fields, 1);
}