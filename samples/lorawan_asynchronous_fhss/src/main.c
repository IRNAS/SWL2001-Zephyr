/** @file main.c
 *
 * @brief LoRaWAN asynchronous fhss sample
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
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/zephyr.h>

LOG_MODULE_REGISTER(main);

/**
 * @brief LR-FHSS workaround currently needed for US-915 and AU-915 regions
 */
#define LORAWAN_LR_FHSS_WORKAROUND_CRYSTAL_ERROR (14000)

/* ---------------- Function declarations ---------------- */

static void on_modem_reset(uint16_t reset_count);
static void on_modem_network_joined(void);

static void on_new_link_adr(void);
static void on_new_link_adr_eu_868(uint16_t datarate_mask);
static void on_new_link_adr_us_915(uint16_t datarate_mask);
static void on_new_link_adr_au_915(uint16_t datarate_mask);

static void on_modem_tx_done(smtc_modem_event_txdone_status_t status);
static void on_modem_down_data(int8_t rssi, int8_t snr,
			       smtc_modem_event_downdata_window_t rx_window, uint8_t port,
			       const uint8_t *payload, uint8_t size);
static bool is_joined(void);
static void send_frame(const uint8_t *buffer, const uint8_t length, const bool confirmed);

static void button_init(void);
static void on_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief Port to send uplinks on
 */
#define LORAWAN_APP_PORT 23

/**
 * @brief Should uplinks be confirmed
 */
#define LORAWAN_CONFIRMED_MSG_ON false

/* ---------------- LoRaWAN ---------------- */

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
	.new_link_adr = on_new_link_adr,
	.tx_done = on_modem_tx_done,
	.down_data = on_modem_down_data,

};

/* lr11xx radio context and its use in the ralf layer */
static const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr1120)));

/* ---------------- Button ---------------- */

/* sw1 ~ button 2 on nrf52840dk */
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static struct gpio_callback button_cb_data;

static bool user_button_pressed = false;
static uint8_t user_button_count = 0;

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

	/* configure button pin and interrupt */
	button_init();

	/* Enter main loop:
	 * The fist call to smtc_modem_run_engine will trigger the reset callback.
	 * We set our LoRaWAN configration then and start the join process - see reset().
	 *
	 * After it is joined, a press to button2 on the DK will send an uplink message with the
	 * number of button presses on port 102.
	 *
	 */

	while (1) {
		/* Check if the button has been pressed */
		if (user_button_pressed == true) {
			user_button_pressed = false;

			/* Check if the device has already joined a network */
			if (is_joined() == true) {
				send_frame(&user_button_count, 1, LORAWAN_CONFIRMED_MSG_ON);
			} else {
				LOG_WRN("Device has not joined a network yet, try again later.");
			}
		}

		/* Execute modem runtime, this function must be recalled in sleep_time_ms or sooner
		 */
		uint32_t sleep_time_ms = smtc_modem_run_engine();

		LOG_INF("Sleeping for %d ms", sleep_time_ms);
		k_sem_take(&main_sleep_sem, K_MSEC(sleep_time_ms));
	}
}

/**
 * @brief Called when a modem reset is detected
 *
 * @param[in] reset_count Number of resets (stored persistently)
 */
void on_modem_reset(uint16_t reset_count)
{
	LOG_INF("EVENT: RESET");
	LOG_INF("Count: %d", reset_count);
	smtc_app_configure_lorawan_params(STACK_ID, &lorawan_cfg);

	/* Apply workaround crystal error */
	smtc_modem_region_t region;
	smtc_modem_get_region(STACK_ID, &region);

	uint32_t crystal_error;
	smtc_modem_get_crystal_error_ppm(&crystal_error);
	LOG_INF("Previous crystal error setting: %d", crystal_error);

	if (region == SMTC_MODEM_REGION_US_915) {
		crystal_error = LORAWAN_LR_FHSS_WORKAROUND_CRYSTAL_ERROR;
	} else if (region == SMTC_MODEM_REGION_AU_915) {
		crystal_error = LORAWAN_LR_FHSS_WORKAROUND_CRYSTAL_ERROR;
	}

	LOG_INF("Crystal error set to: %d", crystal_error);
	smtc_modem_set_crystal_error_ppm(crystal_error);

	smtc_modem_join_network(STACK_ID);
}

/**
 * @brief Called when modem joins the network
 *
 */
void on_modem_network_joined(void)
{
	LOG_INF("JOINED!");

	/* adr profile MUST be set after successfully joining a network.
	 * NOTE: if SMTC_MODEM_ADR_PROFILE_CUSTOM is used, the adr_custom_data parameter must be
	 * set. if some other datarate configuration is used, the adr_custom_data parameter can be
	 * NULL */
	smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_NETWORK_CONTROLLED, NULL);
}

static void on_new_link_adr(void)
{
	uint16_t datarate_mask;
	/* Each bit of datarate_mask tells us if that datarate is available.
	 * bit 0 corresponds to DR0, bit 1 to DR1, ..., bit 15 to DR15
	 */
	smtc_modem_get_available_datarates(STACK_ID, &datarate_mask);

	smtc_modem_region_t region;
	smtc_modem_get_region(STACK_ID, &region);

	if (region == SMTC_MODEM_REGION_EU_868) {
		on_new_link_adr_eu_868(datarate_mask);
	} else if (region == SMTC_MODEM_REGION_US_915) {
		on_new_link_adr_us_915(datarate_mask);
	} else if (region == SMTC_MODEM_REGION_AU_915) {
		on_new_link_adr_au_915(datarate_mask);
	} else {
		LOG_WRN("Unsupported region for FHSS sample");
	}
}

static void on_new_link_adr_eu_868(uint16_t datarate_mask)
{
	// Get mask for rates 8 to 11
	datarate_mask = (datarate_mask >> 8) & 0x0F;

	// For simplicity, this demo application only supports the cases where datarates 8-9, 10-11,
	// or 8-11 are supported by the gateway.
	if (datarate_mask == 0xF) {
		LOG_INF("LR-FHSS datarates 8-11 are available");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else if (datarate_mask == 0xC) {
		LOG_INF("LR-FHSS datarates 10-11 are available\n");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else if (datarate_mask == 0x3) {
		LOG_INF("LR-FHSS datarates 8-9 are available\n");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else if (datarate_mask == 0) {
		LOG_WRN("No LR-FHSS datarates are available");
	} else {
		LOG_WRN("Unsupported LR-FHSS datarates: 0x%x", datarate_mask);
	}
}

static void on_new_link_adr_us_915(uint16_t datarate_mask)
{
	// Get mask for rates 5 and 6
	datarate_mask = (datarate_mask >> 5) & 0x03;

	if (datarate_mask == 0x3) {
		LOG_INF("LR-FHSS datarates 5-6 are available");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else if (datarate_mask == 0x2) {
		LOG_INF("LR-FHSS datarate 6 is available");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else if (datarate_mask == 0x1) {
		LOG_INF("LR-FHSS datarate 5 is available");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else {
		LOG_WRN("No LR-FHSS datarates are available");
	}
}

static void on_new_link_adr_au_915(uint16_t datarate_mask)
{
	// Get mask for rate 7
	datarate_mask = (datarate_mask >> 7) & 0x01;

	if (datarate_mask == 0x01) {
		LOG_INF("LR-FHSS datarate 7 is available");
		uint8_t custom_datarate[SMTC_MODEM_CUSTOM_ADR_DATA_LENGTH] = {
			7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
		smtc_modem_adr_set_profile(STACK_ID, SMTC_MODEM_ADR_PROFILE_CUSTOM,
					   custom_datarate);
	} else {
		LOG_WRN("No LR-FHSS datarates are available");
	}
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
 * @brief Called when dowlink data has been received
 *
 */
void on_modem_down_data(int8_t rssi, int8_t snr, smtc_modem_event_downdata_window_t rx_window,
			uint8_t port, const uint8_t *payload, uint8_t size)
{
	LOG_INF("EVENT: DOWNDATA");

	LOG_INF("RSSI: %d", rssi);
	LOG_INF("SNR: %d", snr);
	LOG_INF("RX window: %s (%d)", smtc_modem_event_downdata_window_to_str(rx_window),
		rx_window);
	LOG_INF("PORT: %d", port);
	LOG_INF("Payload len: %d", size);
	LOG_HEXDUMP_INF(payload, size, "Payload buffer:");

	/* NOTE: copy the payload to some other buffer if further use iss required */
}

/**
 * @brief Check if device has joined the LoRaWAN network
 *
 * @retval true If joined
 * @retval false If not joined
 */
static bool is_joined(void)
{
	uint32_t status = 0;
	smtc_modem_get_status(STACK_ID, &status);
	return (status & SMTC_MODEM_STATUS_JOINED) == SMTC_MODEM_STATUS_JOINED;
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

/**
 * @brief Initialize button pin and interrupt callback
 *
 */
void button_init(void)
{
	int ret;
	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, button.port->name,
			button.pin);
		return;
	}

	ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d", ret,
			button.port->name, button.pin);
		return;
	}

	gpio_init_callback(&button_cb_data, on_button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);
}

/**
 * @brief Called on each press of the user button
 *
 */
void on_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	user_button_pressed = true;
	user_button_count++;
	LOG_INF("Button pressed (count: %d)", user_button_count);
	k_sem_give(&main_sleep_sem);
}
