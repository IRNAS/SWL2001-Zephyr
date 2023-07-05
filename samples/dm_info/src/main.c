/** @file main.c
 *
 * @brief DM info example
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

static int get_battery_level(uint32_t *battery_level);
static int get_temperature(int32_t *battery_level);
static int get_voltage_level(uint32_t *battery_level);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief LoRaWAN fport used by the DM service
 */
#define APP_MODEM_DM_LORAWAN_FPORT 199

/**
 * @brief Application device management interval (minutes)
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
};

static struct smtc_app_env_callbacks environment_callbacks = {
	.get_battery_level = get_battery_level,
	.get_temperature = get_temperature,
	.get_voltage_level = get_voltage_level,
};

/* ---------------- DM info fields configuration ---------------- */

/**
 * @brief Device management fields to be reported with a call to @ref smtc_modem_dm_set_info_fields
 *
 * @see @ref SMTC_MODEM_DM_INFO_DEF
 */
const uint8_t app_modem_dm_fields[] = {
	SMTC_MODEM_DM_FIELD_STATUS,
	SMTC_MODEM_DM_FIELD_CHARGE,
	SMTC_MODEM_DM_FIELD_VOLTAGE,
	SMTC_MODEM_DM_FIELD_TEMPERATURE,
};

/**
 * @brief Device management fields to be reported with a call to @ref
 * smtc_modem_dm_request_single_uplink
 *
 * @see @ref SMTC_MODEM_DM_INFO_DEF
 */
const uint8_t app_modem_dm_fields_single[] = {
	SMTC_MODEM_DM_FIELD_STATUS,   SMTC_MODEM_DM_FIELD_CHARGE,
	SMTC_MODEM_DM_FIELD_VOLTAGE,  SMTC_MODEM_DM_FIELD_TEMPERATURE,
	SMTC_MODEM_DM_FIELD_SIGNAL,   SMTC_MODEM_DM_FIELD_UP_TIME,
	SMTC_MODEM_DM_FIELD_RX_TIME,  SMTC_MODEM_DM_FIELD_ADR_MODE,
	SMTC_MODEM_DM_FIELD_JOIN_EUI, SMTC_MODEM_DM_FIELD_INTERVAL,
	SMTC_MODEM_DM_FIELD_REGION,   SMTC_MODEM_DM_FIELD_RST_COUNT,
	SMTC_MODEM_DM_FIELD_DEV_EUI,  SMTC_MODEM_DM_FIELD_SESSION,
	SMTC_MODEM_DM_FIELD_CHIP_EUI, SMTC_MODEM_DM_FIELD_APP_STATUS,
};

/**
 * @brief Application device management user-defined data
 */
const uint8_t app_modem_dm_fields_user_data[8] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

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
	smtc_app_init(&modem_radio, &event_callbacks, &environment_callbacks);
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
	/* Successfully joined, configure DM info.
	 * This DM message will be sent every APP_MODEM_DM_INTERVAL
	 */
	smtc_modem_dm_set_fport(APP_MODEM_DM_LORAWAN_FPORT);
	smtc_modem_dm_set_info_fields(app_modem_dm_fields, ARRAY_SIZE(app_modem_dm_fields));
	smtc_modem_dm_set_user_data(app_modem_dm_fields_user_data);
	smtc_modem_dm_set_info_interval(SMTC_MODEM_DM_INFO_INTERVAL_IN_MINUTE,
					APP_MODEM_DM_INTERVAL);

	/* Request one time custom DM message */
	smtc_modem_dm_request_single_uplink(app_modem_dm_fields_single,
					    ARRAY_SIZE(app_modem_dm_fields_single));
}

/**
 * @brief Get the battery level
 *
 * The app is expected to provide a valid environmental value for DM info purposes.
 * Return a negative error code if no valid value is available.
 *
 * These callbacks are only called if coresponding the DM info field is set:
 * SMTC_MODEM_DM_FIELD_CHARGE
 * SMTC_MODEM_DM_FIELD_VOLTAGE
 * SMTC_MODEM_DM_FIELD_TEMPERATURE
 *
 * if the callbacks are not set, default (minumum) values will be used.
 *
 */
static int get_battery_level(uint32_t *battery_level)
{
	/* we simulate battery draining here */
	static uint32_t battery_promiles = 1000;
	*battery_level = battery_promiles;
	battery_promiles -= 10;

	return 0;
}

static int get_temperature(int32_t *temperature)
{
	*temperature = 22;
	return 0;
}

static int get_voltage_level(uint32_t *voltage_level)
{
	/* we simulate battery draining here */
	static uint32_t battery_level_mv = 3300;
	*voltage_level = battery_level_mv;
	battery_level_mv -= 10;

	return 0;
}
