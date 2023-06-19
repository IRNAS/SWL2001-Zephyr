/*!
 * @file      apps_modem_common.c
 *
 * @brief     Common functions shared by the examples
 *
 * @copyright
 * The Clear BSD License
 * Copyright Semtech Corporation 2021. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the disclaimer
 * below) provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Semtech corporation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 * THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SEMTECH CORPORATION BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * -----------------------------------------------------------------------------
 * --- DEPENDENCIES ------------------------------------------------------------
 */

#include <smtc_app.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <smtc_basic_modem_lr11xx_api_extension.h>
#include <smtc_modem_api.h>
#include <smtc_modem_api_str.h>
#include <smtc_modem_middleware_advanced_api.h>

#include <smtc_modem_hal_init.h>

LOG_MODULE_REGISTER(smtc_app, CONFIG_SMTC_APP_LOG_LEVEL);

/* Offset in second between GPS EPOCH and UNIX EPOCH time */
#define OFFSET_BETWEEN_GPS_EPOCH_AND_UNIX_EPOCH 315964800

/* Number of leap seconds as of September 15th 2021 */
#define OFFSET_LEAP_SECONDS 18

void prv_event_process(void);
struct smtc_app_event_callbacks *prv_callbacks;
struct smtc_app_env_callbacks *prv_env_callbacks;
struct smtc_app_lorawan_cfg *prv_cfg;

/* macro to ease repeated error checking in apps_modem_common_init */
#define SMTC_ERR_CHECK(func_name, rc)                                                              \
	do {                                                                                       \
		if (rc != SMTC_MODEM_RC_OK) {                                                      \
			LOG_ERR("%s, err: %s (%d)", func_name, smtc_modem_return_code_to_str(rc),  \
				rc);                                                               \
			return rc;                                                                 \
		}                                                                                  \
	} while (0);

/**
 * @brief Convert gps time to unix epoch time
 *
 * @param[in] gps_time_s GPS time to convert. In seconds.
 * @return uint32_t The same time in unix epoch format, in seconds.
 */
static uint32_t prv_convert_gps_to_utc_time(uint32_t gps_time_s)
{
	return gps_time_s + OFFSET_BETWEEN_GPS_EPOCH_AND_UNIX_EPOCH - OFFSET_LEAP_SECONDS;
}

/**
 * @brief Callback for modem hal
 */
static int prv_get_battery_level_cb(uint32_t *value)
{
	if (!prv_env_callbacks || !prv_env_callbacks->get_battery_level) {
		return -1;
	}

	return prv_env_callbacks->get_battery_level(value);
}

/**
 * @brief Callback for modem hal
 */
static int prv_get_temperature_cb(int32_t *value)
{
	if (!prv_env_callbacks || !prv_env_callbacks->get_temperature) {
		return -1;
	}

	return prv_env_callbacks->get_temperature(value);
}

/**
 * @brief Callback for modem hal
 */
static int prv_get_voltage_cb(uint32_t *value)
{
	if (!prv_env_callbacks || !prv_env_callbacks->get_voltage_level) {
		return -1;
	}

	return prv_env_callbacks->get_voltage_level(value);
}

void smtc_app_init(const ralf_t *radio, struct smtc_app_event_callbacks *callbacks,
		   struct smtc_app_env_callbacks *env_callbacks)
{
	__ASSERT(radio, "radio must be provided");
	__ASSERT(callbacks, "callbacks must be provided");
	/* env_callbacks can be NULL */

	prv_callbacks = callbacks;
	prv_env_callbacks = env_callbacks;

	smtc_modem_hal_init((const struct device *)radio->ral.context, prv_get_battery_level_cb,
			    prv_get_temperature_cb, prv_get_voltage_cb);

	smtc_modem_init(radio, &prv_event_process);
}

smtc_modem_return_code_t smtc_app_configure_lorawan_params(uint8_t stack_id,
							   struct smtc_app_lorawan_cfg *cfg)
{
	prv_cfg = cfg;
	smtc_modem_return_code_t rc = SMTC_MODEM_RC_OK;

	if (cfg->use_chip_eui_as_dev_eui) {
		rc = smtc_modem_get_chip_eui(stack_id, cfg->dev_eui);
		SMTC_ERR_CHECK("smtc_modem_get_chip_eui", rc);
	}

	rc = smtc_modem_set_deveui(stack_id, cfg->dev_eui);
	SMTC_ERR_CHECK("smtc_modem_set_deveui", rc);

	rc = smtc_modem_set_joineui(stack_id, cfg->join_eui);
	SMTC_ERR_CHECK("smtc_modem_set_joineui", rc);

	rc = smtc_modem_set_nwkkey(stack_id, cfg->app_key);
	SMTC_ERR_CHECK("smtc_modem_set_nwkkey", rc);

	rc = smtc_modem_set_class(stack_id, cfg->class);
	SMTC_ERR_CHECK("smtc_modem_set_class", rc);

	rc = smtc_modem_set_region(stack_id, cfg->region);
	SMTC_ERR_CHECK("smtc_modem_set_region", rc);

	/* Print what was set */
	LOG_INF("Configured LoRaWAN parameters:");
	LOG_INF("Region: %s (%d)", smtc_modem_region_to_str(cfg->region), cfg->region);
	LOG_INF("Class: %s (%d)", smtc_modem_class_to_str(cfg->class), cfg->class);
	LOG_HEXDUMP_INF(cfg->dev_eui, 8, "DevEui: ");
	LOG_HEXDUMP_INF(cfg->join_eui, 8, "JoinEui: ");
	LOG_HEXDUMP_INF(cfg->app_key, 16, "AppKey: ");

	return SMTC_MODEM_RC_OK;
}

void smtc_app_display_versions(void)
{
	smtc_modem_return_code_t rc = SMTC_MODEM_RC_OK;
	smtc_modem_lorawan_version_t version;
	smtc_modem_version_t firmware_version;

	rc = smtc_modem_get_modem_version(&firmware_version);
	if (rc == SMTC_MODEM_RC_OK) {
		LOG_INF("LoRa Basics Modem version: %d.%d.%d", firmware_version.major,
			firmware_version.minor, firmware_version.patch);
	}

	rc = smtc_modem_get_lorawan_version(&version);
	if (rc == SMTC_MODEM_RC_OK) {
		LOG_INF("LoRaWAN version: %d.%d.%d.%d", version.major, version.minor, version.patch,
			version.revision);
	}

	rc = smtc_modem_get_regional_params_version(&version);
	if (rc == SMTC_MODEM_RC_OK) {
		LOG_INF("Regional parameters version: %d.%d.%d.%d", version.major, version.minor,
			version.patch, version.revision);
	}
}

smtc_modem_return_code_t smtc_app_get_gps_time(uint32_t *gps_time)
{
	uint32_t gps_fractional_s = 0;
	return smtc_modem_get_time(gps_time, &gps_fractional_s);
}

smtc_modem_return_code_t smtc_app_get_utc_time(uint32_t *utc_time)
{
	uint32_t gps_time_s;
	smtc_modem_return_code_t ret = smtc_app_get_gps_time(&gps_time_s);
	if (ret != SMTC_MODEM_RC_OK) {
		return ret;
	}

	*utc_time = prv_convert_gps_to_utc_time(gps_time_s);
	return SMTC_MODEM_RC_OK;
}

/* PRIVATE EVENT PROCESSOR - this calls registered callbacks from app layer */

void prv_event_process(void)
{
	smtc_modem_event_t current_event;
	smtc_modem_return_code_t return_code = SMTC_MODEM_RC_OK;
	uint8_t event_pending_count;

	do {
		/* Read modem event */
		return_code = smtc_modem_get_event(&current_event, &event_pending_count);

		if (return_code == SMTC_MODEM_RC_OK) {
			if (prv_callbacks != NULL) {
				switch (current_event.event_type) {
				case SMTC_MODEM_EVENT_RESET:
					LOG_DBG("RESET EVENT");
					LOG_DBG("Reset count: %u",
						current_event.event_data.reset.count);
					if (prv_callbacks->reset != NULL) {
						prv_callbacks->reset(
							current_event.event_data.reset.count);
					}
					break;
				case SMTC_MODEM_EVENT_ALARM:
					LOG_DBG("ALARM EVENT");
					if (prv_callbacks->alarm != NULL) {
						prv_callbacks->alarm();
					}
					break;
				case SMTC_MODEM_EVENT_JOINED:
					LOG_DBG("JOINED EVENT");
					if (prv_callbacks->joined != NULL) {
						prv_callbacks->joined();
					}
					break;
				case SMTC_MODEM_EVENT_JOINFAIL:
					LOG_DBG("JOIN FAILED EVENT");
					if (prv_callbacks->join_fail != NULL) {
						prv_callbacks->join_fail();
					}
					break;
				case SMTC_MODEM_EVENT_TXDONE:
					LOG_DBG("TX DONE EVENT");
					LOG_DBG("TX DONE status: %d",
						current_event.event_data.txdone.status);
					if (prv_callbacks->tx_done != NULL) {
						prv_callbacks->tx_done(
							current_event.event_data.txdone.status);
					}
					break;
				case SMTC_MODEM_EVENT_DOWNDATA:
					LOG_DBG("DOWNLINK EVENT");
					LOG_DBG("Rx window: %s (%d)",
						smtc_modem_event_downdata_window_to_str(
							current_event.event_data.downdata.window),
						current_event.event_data.downdata.window);
					LOG_DBG("Rx port: %d",
						current_event.event_data.downdata.fport);
					LOG_DBG("Rx RSSI: %d",
						current_event.event_data.downdata.rssi - 64);
					LOG_DBG("Rx SNR: %d",
						current_event.event_data.downdata.snr / 4);

					if (prv_callbacks->down_data != NULL) {
						prv_callbacks->down_data(
							current_event.event_data.downdata.rssi,
							current_event.event_data.downdata.snr,
							current_event.event_data.downdata.window,
							current_event.event_data.downdata.fport,
							current_event.event_data.downdata.data,
							current_event.event_data.downdata.length);
					}
					break;
				case SMTC_MODEM_EVENT_UPLOADDONE:
					LOG_DBG("UPLOAD DONE EVENT");
					LOG_DBG("Upload status: %s (%d)",
						smtc_modem_event_uploaddone_status_to_str(
							current_event.event_data.uploaddone.status),
						current_event.event_data.uploaddone.status);
					if (prv_callbacks->upload_done != NULL) {
						prv_callbacks->upload_done(
							current_event.event_data.uploaddone.status);
					}
					break;
				case SMTC_MODEM_EVENT_SETCONF:
					LOG_DBG("SET CONF EVENT");
					LOG_DBG("Tag: %s (%d)",
						smtc_modem_event_setconf_tag_to_str(
							current_event.event_data.setconf.tag),
						current_event.event_data.setconf.tag);
					if (prv_callbacks->set_conf != NULL) {
						prv_callbacks->set_conf(
							current_event.event_data.setconf.tag);
					}
					break;
				case SMTC_MODEM_EVENT_MUTE:
					LOG_DBG("MUTE EVENT");
					LOG_DBG("Mute: %s (%d)",
						smtc_modem_event_mute_status_to_str(
							current_event.event_data.mute.status),
						current_event.event_data.mute.status);
					if (prv_callbacks->mute != NULL) {
						prv_callbacks->mute(
							current_event.event_data.mute.status);
					}
					break;
				case SMTC_MODEM_EVENT_STREAMDONE:
					LOG_DBG("STREAM DONE EVENT");
					if (prv_callbacks->stream_done != NULL) {
						prv_callbacks->stream_done();
					}
					break;
				case SMTC_MODEM_EVENT_TIME:
					LOG_DBG("TIME EVENT");
					LOG_DBG("Time: %s (%d)",
						smtc_modem_event_time_status_to_str(
							current_event.event_data.time.status),
						current_event.event_data.time.status);
					if (prv_callbacks->time_updated_alc_sync != NULL) {
						prv_callbacks->time_updated_alc_sync(
							current_event.event_data.time.status);
					}
					break;
				case SMTC_MODEM_EVENT_TIMEOUT_ADR_CHANGED:
					LOG_DBG("ADR CHANGED EVENT");
					if (prv_callbacks->adr_mobile_to_static != NULL) {
						prv_callbacks->adr_mobile_to_static();
					}
					break;
				case SMTC_MODEM_EVENT_NEW_LINK_ADR:
					LOG_DBG("NEW LINK ADR EVENT");
					if (prv_callbacks->new_link_adr != NULL) {
						prv_callbacks->new_link_adr();
					}
					break;
				case SMTC_MODEM_EVENT_LINK_CHECK:
					LOG_DBG("LINK CHECK EVENT");
					LOG_DBG("Link status: %s (%d)",
						smtc_modem_event_link_check_status_to_str(
							current_event.event_data.link_check.status),
						current_event.event_data.link_check.status);
					LOG_DBG("Margin: %d dB",
						current_event.event_data.link_check.margin);
					LOG_DBG("Number of gateways: %d",
						current_event.event_data.link_check.gw_cnt);
					if (prv_callbacks->link_status != NULL) {
						prv_callbacks->link_status(
							current_event.event_data.link_check.status,
							current_event.event_data.link_check.margin,
							current_event.event_data.link_check.gw_cnt);
					}
					break;
				case SMTC_MODEM_EVENT_ALMANAC_UPDATE:
					LOG_DBG("ALMANAC UPDATE EVENT");
					LOG_DBG("Almanac update status: %s (%d)",
						smtc_modem_event_almanac_update_status_to_str(
							current_event.event_data.almanac_update
								.status),
						current_event.event_data.almanac_update.status);
					if (prv_callbacks->almanac_update != NULL) {
						prv_callbacks->almanac_update(
							current_event.event_data.almanac_update
								.status);
					}
					break;
				case SMTC_MODEM_EVENT_USER_RADIO_ACCESS:
					LOG_DBG("USER RADIO ACCESS EVENT");
					if (prv_callbacks->user_radio_access != NULL) {
						prv_callbacks->user_radio_access(
							current_event.event_data.user_radio_access
								.timestamp_ms,
							current_event.event_data.user_radio_access
								.status);
					}
					break;
				case SMTC_MODEM_EVENT_CLASS_B_PING_SLOT_INFO:
					LOG_DBG("CLASS B PING SLOT EVENT");
					LOG_DBG("Class B ping slot status: %s (%d)",
						smtc_modem_event_class_b_ping_slot_status_to_str(
							current_event.event_data
								.class_b_ping_slot_info.status),
						current_event.event_data.class_b_ping_slot_info
							.status);
					if (prv_callbacks->class_b_ping_slot_info != NULL) {
						prv_callbacks->class_b_ping_slot_info(
							current_event.event_data
								.class_b_ping_slot_info.status);
					}
					break;
				case SMTC_MODEM_EVENT_CLASS_B_STATUS:
					LOG_DBG("CLASS B STATUS EVENT");
					LOG_DBG("Class B status: %s (%d)",
						smtc_modem_event_class_b_status_to_str(
							current_event.event_data.class_b_status
								.status),
						current_event.event_data.class_b_status.status);
					if (prv_callbacks->class_b_status != NULL) {
						prv_callbacks->class_b_status(
							current_event.event_data.class_b_status
								.status);
					}
					break;
				case SMTC_MODEM_EVENT_MIDDLEWARE_1:
					LOG_DBG("MIDDLEWARE_1 EVENT");
					if (prv_callbacks->middleware_1 != NULL) {
						prv_callbacks->middleware_1(
							current_event.event_data
								.middleware_event_status.status);
					}
					break;
				case SMTC_MODEM_EVENT_MIDDLEWARE_2:
					LOG_DBG("MIDDLEWARE_2 EVENT");
					if (prv_callbacks->middleware_2 != NULL) {
						prv_callbacks->middleware_2(
							current_event.event_data
								.middleware_event_status.status);
					}
					break;
				case SMTC_MODEM_EVENT_MIDDLEWARE_3:
					LOG_DBG("MIDDLEWARE_3 EVENT");
					if (prv_callbacks->middleware_3 != NULL) {
						prv_callbacks->middleware_3(
							current_event.event_data
								.middleware_event_status.status);
					}
					break;
				case SMTC_MODEM_EVENT_NONE:
					break;
				default:
					LOG_DBG("UNKNOWN EVENT");
					break;
				}
			}
		} else {
			LOG_ERR("smtc_modem_get_event, err: %d", return_code);
		}
	} while ((return_code == SMTC_MODEM_RC_OK) && (event_pending_count > 0));
}
