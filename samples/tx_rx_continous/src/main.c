/** @file main.c
 *
 * @brief rx_rx_continuous sample
 *
 * Use this for radio rf testing
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <smtc_modem_api.h>
#include <smtc_modem_test_api.h>

#include <smtc_app.h>
#include <smtc_modem_api_str.h>

/* include hal and ralf so that initialization can be done */
#include <ralf_lr11xx.h>
#include <smtc_modem_hal.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(main);

/* ---------------- SAMPLE CONFIGURATION ---------------- */

/**
 * @brief TX continuous
 *
 * @note only on modulated
 */
#define TX_CONTINUOUS true

/**
 * @brief RX continuous
 */
#define RX_CONTINUOUS false

/**
 * @brief TX continuous or modulated
 */
#define TX_MODULATED true

/**
 * @brief Delay in ms between two TX single
 */
#define TX_SINGLE_INTER_DELAY 1000

/**
 * @brief Tx Power used during test, in dBm.
 */
#define TX_POWER_USED 14

/**
 * @brief Tx Power offset used during test
 */
#define TX_POWER_OFFSET 0

/**
 * @brief Preamble size in symbol \note only on modulated
 */
#define PREAMBLE_SIZE 8

/**
 * @brief Tx payload len \note only on modulated
 */
#define TX_PAYLOAD_LEN 51

/**
 * @brief LoRaWAN regulatory region \ref smtc_modem_region_t
 */
#define LORAWAN_REGION_USED SMTC_MODEM_REGION_EU_868

/**
 * @brief Frequency used during test, \note the frequency SHALL be allowed by the lorawan region
 *
 *       For example, setting 915MHz with EU868 region will not work.
 */
#define FREQUENCY 868100000

/**
 * @brief Spreading factor for test mode \see smtc_modem_test_sf_t
 */
#define SPREADING_FACTOR_USED SMTC_MODEM_TEST_LORA_SF7

/**
 * @brief bandwidth for test mode \see smtc_modem_test_bw_t
 */
#define BANDWIDTH_USED SMTC_MODEM_TEST_BW_125_KHZ

/**
 * @brief Coding rate for test mode \see smtc_modem_test_cr_t
 */
#define CODING_RATE_USED SMTC_MODEM_TEST_CR_4_5
/* ---------------- LoRaWAN Configurations ---------------- */

/* Stack id value (multistack modem is not yet available, so use 0) */
#define STACK_ID 0

/* No callbacks */
static struct smtc_app_event_callbacks event_callbacks = {0};

/* lr11xx radio context and its use in the ralf layer */
const ralf_t modem_radio = RALF_LR11XX_INSTANTIATE(DEVICE_DT_GET(DT_NODELABEL(lr11xx)));

/* ---------------- Main ---------------- */

void main(void)
{
	/* configure LoRaWAN modem */

	/* Init the modem and use provided event callbacks.
	 * Please note that the reset callback will be called immediately after the first call to
	 * smtc_modem_run_engine because of reset detection.
	 */
	smtc_app_init(&modem_radio, &event_callbacks, NULL);
	smtc_app_display_versions();

	if (TX_CONTINUOUS == RX_CONTINUOUS) {
		LOG_ERR("Select between TX Continuous or RX Continuous");
		return;
	}

	smtc_modem_set_region(STACK_ID, LORAWAN_REGION_USED);
	smtc_modem_set_tx_power_offset_db(STACK_ID, TX_POWER_OFFSET);
	smtc_modem_test_start();

	if (TX_CONTINUOUS) {
		LOG_INF("TX PARAM");
		LOG_INF("FREQ         : %d MHz", FREQUENCY);
		LOG_INF("REGION       : %d", LORAWAN_REGION_USED);
		LOG_INF("TX POWER     : %d dBm", TX_POWER_USED);
		if (TX_MODULATED) {
			LOG_INF("TX           : MODULATED");
			if (SPREADING_FACTOR_USED != SMTC_MODEM_TEST_FSK) {
				LOG_INF("MODULATION   : LORA");
				LOG_INF("SF           : %d", SPREADING_FACTOR_USED + 6);
				LOG_INF("CR           : 4/%d", CODING_RATE_USED + 5);
			} else {
				LOG_INF("MODULATION   : FSK");
			}
			LOG_INF("BW           : %d", BANDWIDTH_USED);
		} else {
			LOG_INF("TX           : CONTINUOUS");
		}

		if (TX_MODULATED) {
			if (TX_CONTINUOUS == true) {
				smtc_modem_test_tx(NULL, TX_PAYLOAD_LEN, FREQUENCY, TX_POWER_USED,
						   SPREADING_FACTOR_USED, BANDWIDTH_USED,
						   CODING_RATE_USED, PREAMBLE_SIZE, true);
			} else {
				while (1) {
					LOG_INF("TX");
					smtc_modem_test_tx(NULL, TX_PAYLOAD_LEN, FREQUENCY,
							   TX_POWER_USED, SPREADING_FACTOR_USED,
							   BANDWIDTH_USED, CODING_RATE_USED,
							   PREAMBLE_SIZE, false);
					k_sleep(K_MSEC(TX_SINGLE_INTER_DELAY));
				}
			}
		} else {
			smtc_modem_test_tx_cw(FREQUENCY, TX_POWER_USED);
		}
	} else {
		LOG_INF("RX PARAM");
		LOG_INF("FREQ         : %d MHz", FREQUENCY);
		LOG_INF("REGION       : %d", LORAWAN_REGION_USED);

		LOG_INF("TX           : MODULATED");
		if (SPREADING_FACTOR_USED != SMTC_MODEM_TEST_FSK) {
			LOG_INF("MODULATION   : LORA");
			LOG_INF("SF           : %d", SPREADING_FACTOR_USED + 6);
			LOG_INF("CR           : 4/%d", CODING_RATE_USED + 5);
		} else {
			LOG_INF("MODULATION   : FSK");
		}
		LOG_INF("BW           : %d", BANDWIDTH_USED);

		smtc_modem_test_rx_continuous(FREQUENCY, SPREADING_FACTOR_USED, BANDWIDTH_USED,
					      CODING_RATE_USED);
	}

	LOG_INF("Test done");
	k_sleep(K_FOREVER);
}
