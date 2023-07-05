/** @file main.c
 *
 * @brief Simple LoRaWAN example
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
static void on_modem_alarm(void);
static void on_modem_tx_done(smtc_modem_event_txdone_status_t status);
static void on_modem_down_data(int8_t rssi, int8_t snr,
			       smtc_modem_event_downdata_window_t rx_window, uint8_t port,
			       const uint8_t *payload, uint8_t size);
static void send_frame(const uint8_t *buffer, const uint8_t length, const bool confirmed);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief Duration between uplinks
 */
#define APP_TX_DUTYCYCLE 60

/**
 * @brief Port to send uplinks on
 */
#define LORAWAN_APP_PORT 2

/**
 * @brief Should uplinks be confirmed
 */
#define LORAWAN_CONFIRMED_MSG_ON false

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
	.alarm = on_modem_alarm,
	.tx_done = on_modem_tx_done,
	.down_data = on_modem_down_data,

};

/* lr11xx radio context and its use in the ralf layer */
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr11xx)));

/* Buffer for uplinks */
static uint8_t app_data_buffer[256];

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
	/* Successfully joined, use built in alarm feature to trigger alarm calback after some delay
	 */
	smtc_modem_alarm_start_timer(APP_TX_DUTYCYCLE);
	LOG_INF("Next transmision in: %d s", APP_TX_DUTYCYCLE);

	/* adr profile MUST be set after successfully joining a network.
	 * NOTE: if SMTC_MODEM_ADR_PROFILE_CUSTOM is used, the adr_custom_data parameter must be
	 * set. if some other datarate configuration is used, the adr_custom_data parameter can be
	 * NULL */
	smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_NETWORK_CONTROLLED, NULL);
}

/**
 * @brief Alarm event callback
 *
 * This is triggered after calling smtc_modem_alarm_start_timer and the timer expires
 */
static void on_modem_alarm(void)
{
	uint32_t charge = 0;
	uint8_t i = 0;
	static uint8_t tx_cnt = 0;

	/* Schedule next packet transmission */
	smtc_modem_alarm_start_timer(APP_TX_DUTYCYCLE);
	LOG_INF("Next transmision in: %d s", APP_TX_DUTYCYCLE);

	/* Prepare packet */
	smtc_modem_get_charge(&charge);

	app_data_buffer[i++] = ++tx_cnt;
	app_data_buffer[i++] = (uint8_t)(charge);
	app_data_buffer[i++] = (uint8_t)(charge >> 8);
	app_data_buffer[i++] = (uint8_t)(charge >> 16);
	app_data_buffer[i++] = (uint8_t)(charge >> 24);

	send_frame(app_data_buffer, i, LORAWAN_CONFIRMED_MSG_ON);
}

/**
 * @brief Tx done event callback
 *
 * @param [in] status tx done status @ref smtc_modem_event_txdone_status_t
 */
static void on_modem_tx_done(smtc_modem_event_txdone_status_t status)
{
	if (status == SMTC_MODEM_EVENT_TXDONE_NOT_SENT) {
		LOG_ERR("Uplink was not sent");
	} else if (status == SMTC_MODEM_EVENT_TXDONE_SENT) {
		LOG_INF("Uplink sent (not confirmed)");
	} else if (status == SMTC_MODEM_EVENT_TXDONE_CONFIRMED) {
		LOG_INF("Uplink sent (confirmed)");
	}
}

/**
 * @brief Downlink data event callback.
 *
 * @param [in] rssi       RSSI in signed value in dBm + 64
 * @param [in] snr        SNR signed value in 0.25 dB steps
 * @param [in] rx_window  RX window
 * @param [in] port       LoRaWAN port
 * @param [in] payload    Received buffer pointer
 * @param [in] size       Received buffer size
 */
static void on_modem_down_data(int8_t rssi, int8_t snr,
			       smtc_modem_event_downdata_window_t rx_window, uint8_t port,
			       const uint8_t *payload, uint8_t size)
{
	LOG_INF("EVENT: DOWNDATA");

	LOG_INF("RSSI: %d", rssi);
	LOG_INF("SNR: %d", snr);
	LOG_INF("RX window: %s (%d)", smtc_modem_event_downdata_window_to_str(rx_window),
		rx_window);
	LOG_INF("PORT: %d", port);
	LOG_INF("Payload len: %d", size);
	LOG_HEXDUMP_INF(payload, size, "Payload buffer:");
}

/**
 * @brief   Send an application frame on LoRaWAN port defined by LORAWAN_APP_PORT
 *
 * This function checks if we are allowed to send (due to duty cycle limitations).
 * It also checks if we are allowed to send the number of bytes we are sending. If not, an empty
 * uplink is sent in order to flush mac commands.
 *
 * @param [in] buffer     Buffer containing the LoRaWAN buffer
 * @param [in] length     Payload length
 * @param [in] confirmed  Send a confirmed or unconfirmed uplink [false : unconfirmed / true :
 * confirmed]
 */
static void send_frame(const uint8_t *buffer, const uint8_t length, bool tx_confirmed)
{
	uint8_t tx_max_payload;
	int32_t duty_cycle;

	/* Check if duty cycle is available */
	smtc_modem_get_duty_cycle_status(&duty_cycle);
	if (duty_cycle < 0) {
		LOG_WRN("Duty-cycle limitation - next possible uplink in %d ms", duty_cycle);
		/* NOTE: an actual application will probably schedule a new attempt for an uplink
		 * after duty_cycle ms elapses. */
		return;
	}

	smtc_modem_get_next_tx_max_payload(STACK_ID, &tx_max_payload);

	/* NOTE: an actual application might send a shorter payload or schedule the same payload at
	 * a later time */
	if (length > tx_max_payload) {
		LOG_WRN("Not enough space in buffer - requesting empty uplink to flush MAC "
			"commands");
		smtc_modem_request_empty_uplink(STACK_ID, true, LORAWAN_APP_PORT, tx_confirmed);
	} else {
		LOG_INF("Requesting uplink");
		smtc_modem_request_uplink(STACK_ID, LORAWAN_APP_PORT, tx_confirmed, buffer, length);
	}
}
