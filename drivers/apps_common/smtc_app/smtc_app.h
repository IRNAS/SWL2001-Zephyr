/**
 * @file      smtc_app.h
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

#ifndef SMTC_APP_H
#define SMTC_APP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <smtc_modem_api.h>
#include <smtc_modem_utilities.h>

/**
 * @brief Configuration structure for initializing common LoRaWAN parameters
 */
struct smtc_app_lorawan_cfg {
	/* If true, lr11xx chip eui will be used as dev_eui, and bellow dev_eui value will be
	 * ignored. */
	bool use_chip_eui_as_dev_eui;

	/* DevEUI to use (if use_chip_eui_as_dev_eui is set to false) */
	uint8_t dev_eui[8];

	/* JoinEUI to use */
	uint8_t join_eui[8];

	/* AppKey/NwKey to use */
	uint8_t app_key[16];

	/* LoRaWAN class to use */
	smtc_modem_class_t class;

	/* Regional parameters to use */
	smtc_modem_region_t region;
};

/**
 * @brief Lora Basics Modem callbacks
 */
struct smtc_app_event_callbacks {
	/**
	 * @brief  Modem reset
	 *
	 * @param [in] reset_count The number of reset (persistently stored)
	 */
	void (*reset)(uint16_t reset_count);

	/**
	 * @brief  Alarm timer expired
	 */
	void (*alarm)(void);

	/**
	 * @brief  Attempt to join network succeeded
	 */
	void (*joined)(void);

	/**
	 * @brief  Attempt to join network failed
	 */
	void (*join_fail)(void);

	/**
	 * @brief  Tx done
	 *
	 * @param [in] status Status of transmision
	 */
	void (*tx_done)(smtc_modem_event_txdone_status_t status);

	/**
	 * @brief Downlink data received
	 *
	 * @param [in] rssi	rssi in signed value in dBm + 64
	 * @param [in] snr     snr signed value in 0.25 dB steps
	 * @param [in] rx_window The RX window used for the downlink
	 * @param [in] port    LoRaWAN port
	 * @param [in] payload Received buffer
	 * @param [in] size    Received buffer size
	 */
	void (*down_data)(int8_t rssi, int8_t snr, smtc_modem_event_downdata_window_t rx_window,
			  uint8_t port, const uint8_t *payload, uint8_t size);

	/**
	 * @brief  File upload completed
	 *
	 * @param [in] status \see smtc_modem_event_uploaddone_status_t
	 */
	void (*upload_done)(smtc_modem_event_uploaddone_status_t status);

	/**
	 * @brief  Set conf changed by DM
	 *
	 * @param [in] tag \see smtc_modem_event_setconf_tag_t
	 */
	void (*set_conf)(smtc_modem_event_setconf_tag_t tag);

	/**
	 * @brief  Mute callback
	 *
	 * @param [in] status \see smtc_modem_event_mute_status_t
	 */
	void (*mute)(smtc_modem_event_mute_status_t status);

	/**
	 * @brief  Data stream fragments sent
	 */
	void (*stream_done)(void);

	/**
	 * @brief  Time updated
	 *
	 * @param [in] status \see smtc_modem_event_time_status_t
	 */
	void (*time_updated_alc_sync)(smtc_modem_event_time_status_t status);

	/**
	 * @brief  Automatic switch from mobile to static ADR when connection timeout occurs
	 */
	void (*adr_mobile_to_static)(void);

	/**
	 * @brief  New link ADR request
	 */
	void (*new_link_adr)(void);

	/**
	 * @brief  Link Status (result of link check)
	 *
	 * @param [in] status \see smtc_modem_event_link_check_status_t
	 * @param [in] margin The demodulation margin in dB
	 * @param [in] gw_cnt number of gateways that received the most recent LinkCheckReq command
	 */
	void (*link_status)(smtc_modem_event_link_check_status_t status, uint8_t margin,
			    uint8_t gw_cnt);

	/**
	 * @brief  Almanac update
	 *
	 * @param [in] status \see smtc_modem_event_almanac_update_status_t
	 */
	void (*almanac_update)(smtc_modem_event_almanac_update_status_t status);

	/**
	 * @brief  User radio access
	 *
	 * @param [in] timestamp_ms timestamp in ms of the radio irq
	 * @param [in] status Interrupt status
	 */
	void (*user_radio_access)(uint32_t timestamp_ms,
				  smtc_modem_event_user_radio_access_status_t status);

	/**
	 * @brief  Class B ping slot status
	 *
	 * @param [in] status Class B ping slot status. \see
	 * smtc_modem_event_class_b_ping_slot_status_t
	 */
	void (*class_b_ping_slot_info)(smtc_modem_event_class_b_ping_slot_status_t status);

	/**
	 * @brief  Class B status
	 *
	 * @param [in] status Class B status. \see smtc_modem_event_class_b_status_t
	 */
	void (*class_b_status)(smtc_modem_event_class_b_status_t status);

	/**
	 * @brief  Middleware 1 callback
	 *
	 * @param [in] status Interrupt status
	 */
	void (*middleware_1)(uint8_t status);

	/**
	 * @brief  Middleware 2 callback
	 *
	 * @param [in] status Interrupt status
	 */
	void (*middleware_2)(uint8_t status);

	/**
	 * @brief  Middleware 3 callback
	 *
	 * @param [in] status Interrupt status
	 */
	void (*middleware_3)(uint8_t status);
};

/**
 * @brief Function callbacks for providing environmental sensor values.
 *
 * These callbacks are used by the device management (DM) subsystem of the lora basics modem stack.
 * They will be called from the modem stack whenever a new DM info message is being prepared, but
 * only if that DM info field is configrued to be sent.
 *
 * if the app does not provide the callbacks, a default (minimum) will be sent by the DM subsystem.
 */
struct smtc_app_env_callbacks {
	/**
	 * @brief Get battery level callback
	 *
	 * @param [out] battery_level The battery level in permilles (‰) (where 1000‰ means full
	 * battery). The application should set this to the most recent battery level it has
	 * avaliable
	 *
	 * @return int 0 if the battery level was set, or a negative error code if no valid battery
	 * level is available
	 */
	int (*get_battery_level)(uint32_t *battery_level);

	/**
	 * @brief Get temperature callback
	 *
	 * @param [out] temperature The temperature (in deg C). The application should set this to
	 * the most recent temperature value it has avaliable
	 *
	 * @return int 0 if the temperature was set, or a negative error code if no valid
	 * temperature is available
	 */
	int (*get_temperature)(int32_t *temperature);

	/**
	 * @brief Get voltage level callback
	 *
	 * @param [out] voltage_level The voltage level (in mV). The application should set this to
	 * the most recent voltage level it has avaliable
	 *
	 * @return int 0 if the voltage level was set, or a negative error code if no valid voltage
	 * level is available
	 */
	int (*get_voltage_level)(uint32_t *voltage_level);
};

/**
 * @brief Initialize modem with the provided radio and callbacks
 *
 * Internally, this registers a private "raw" event handler to the modem stack.
 * Within the handler, callbacks provided in @p callback are called when coresponding events are
 * generated by the modem stack.
 *
 * @param[in] radio A specific RALF implementation
 * @param[in] callback Desired callbacks. All callbacks that are not required by the application can
 * be set to NULL.
 * @param[in] env_callbacks Desired environment callbacks. Can be NULL. All callbacks that are not
 * required by the application can be set to NULL.
 * @return SMTC_MODEM_RC_OK if successfully, or other error code if unsuccessful
 */
void smtc_app_init(const ralf_t *radio, struct smtc_app_event_callbacks *callbacks,
		   struct smtc_app_env_callbacks *env_callbacks);

/**
 * @brief Configure common LoRaWAN parameters
 *
 * Sets LoRaWAN parameters in the modem.
 * If cfg->use_chip_eui_as_dev_eui is true, this function will save the chip eui into cfg->dev_eui,
 * so the cfg struct must not be const.
 *
 * @param[in] stack_id The stack ID to configure.
 * @param[in] cfg The configuration to apply. The config struct must live for as long as the modem
 * engine is in use.
 * @return SMTC_MODEM_RC_OK if successfully, or other error code if unsuccessful
 */
smtc_modem_return_code_t smtc_app_configure_lorawan_params(uint8_t stack_id,
							   struct smtc_app_lorawan_cfg *cfg);

/**
 * @brief Display version information
 *
 * Displays LoRa Basics Modem, LoRaWAN and Regional parameters versions.
 * This is especially usefull to correctly setup a device server side.
 *
 */
void smtc_app_display_versions(void);

/**
 * @brief Get time in gps epoch
 *
 * @param[out] gps_time The current gps time (number of seconds since the GPS epoch)
 * @return smtc_modem_return_code_t SMTC_MODEM_RC_OK if time was read successfully, or other error
 * code if unsuccessful
 */
smtc_modem_return_code_t smtc_app_get_gps_time(uint32_t *gps_time);

/**
 * @brief Get utc time
 *
 * @param[out] gps_time The current unix time (number of seconds since the unix epoch)
 * @return SMTC_MODEM_RC_OK if time was read successfully, or other error
 * code if unsuccessful
 */
smtc_modem_return_code_t smtc_app_get_utc_time(uint32_t *utc_time);

#ifdef __cplusplus
}
#endif

#endif // SMTC_APP_H
