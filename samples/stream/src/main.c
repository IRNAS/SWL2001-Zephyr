/** @file main.c
 *
 * @brief LoRaWAN stream example
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
static void on_modem_stream_done(void);

static void get_next_chunk_from_pangram(uint8_t *buffer, uint8_t len);
static void add_chunk_to_stream_buffer(uint8_t len);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief LoRaWAN fport used by the stream service
 */
#define APP_SMTC_MODEM_STREAM_LORAWAN_FPORT 199

/**
 * @brief Streaming encryption mode
 */
#define APP_SMTC_MODEM_STREAM_CIPHER_MODE SMTC_MODEM_STREAM_NO_CIPHER

/**
 * @brief Streaming redundancy ratio (in percent)
 */
#define APP_SMTC_MODEM_STREAM_REDUNDANCY_RATIO 110

/**
 * @brief Size in byte of a chunk added to the stream
 *
 * @remark This value has to be in the range [1:254]
 */
#define APP_STREAM_CHUNK_SIZE 40

/**
 * @brief Period in second between two chunks being added to the stream buffer
 */
#define APP_STREAM_CHUNK_PERIOD 20

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
	.stream_done = on_modem_stream_done,

};

/* lr11xx radio context and its use in the ralf layer */
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr11xx)));

/* ---------------- Main ---------------- */

const char pangram[] = "The quick brown fox jumps over the lazy dog.";
static uint8_t pangram_index = 0;

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
	smtc_modem_alarm_start_timer(APP_STREAM_CHUNK_PERIOD);
	LOG_INF("Next alarm in: %d s", APP_STREAM_CHUNK_PERIOD);

	/* initialize modem stream */
	smtc_modem_return_code_t ret = smtc_modem_stream_init(
		STACK_ID, APP_SMTC_MODEM_STREAM_LORAWAN_FPORT, APP_SMTC_MODEM_STREAM_CIPHER_MODE,
		APP_SMTC_MODEM_STREAM_REDUNDANCY_RATIO);

	if (ret != SMTC_MODEM_RC_OK) {
		LOG_ERR("Unable to initialize stream, err: %d", ret);
		/* NOTE: if ret == SMTC_MODEM_RC_FAIL, you might have forgotten to set
		 * CONFIG_LORA_BASICS_MODEM_STREAM
		 */
	}
}

/**
 * @brief Alarm event callback
 *
 * This is triggered after calling smtc_modem_alarm_start_timer and the timer expires
 */
static void on_modem_alarm(void)
{
	/* Schedule next alarm */
	smtc_modem_alarm_start_timer(APP_STREAM_CHUNK_PERIOD);
	LOG_INF("Next transmision in: %d s", APP_STREAM_CHUNK_PERIOD);

	/* Add data to stream */
	add_chunk_to_stream_buffer(APP_STREAM_CHUNK_SIZE);
}

void on_modem_stream_done(void)
{
	LOG_INF("Stream buffer is now empty.");
}

void get_next_chunk_from_pangram(uint8_t *buffer, uint8_t len)
{
	for (int i = 0; i < len; i++) {
		buffer[i] = pangram[(pangram_index + i) % strlen(pangram)];
	}

	pangram_index = (pangram_index + len) % strlen(pangram);

	buffer[len] = 0; // Done to make the buffer a null-terminated string
	LOG_INF("Chunk to be added is: %s (%d byte(s) long)", buffer, len);
}

void add_chunk_to_stream_buffer(uint8_t len)
{
	uint16_t pending;
	uint16_t free;
	uint8_t buffer[255];

	/* Check how much free space is available in the stream */
	smtc_modem_stream_status(STACK_ID, &pending, &free);

	if (free >= len) {
		get_next_chunk_from_pangram(buffer, len);
		smtc_modem_stream_add_data(STACK_ID, buffer, len);
	} else {
		LOG_WRN("Not enough free space.");
	}
}
