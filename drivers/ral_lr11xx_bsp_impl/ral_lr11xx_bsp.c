/** @file ral_lr11xx_bsp.c
 *
 * @brief BSP implementation
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <ral_lr11xx_bsp.h>

#include <zephyr/device.h>
#include <zephyr/zephyr.h>

#include "lr11xx_pa_pwr_cfg.h"

#include <smtc_modem_api.h>

#include <lr11xx_board.h>
#include <lr11xx_radio.h>

typedef enum lr11xx_pa_type_s {
	LR11XX_WITH_LF_LP_PA,
	LR11XX_WITH_LF_HP_PA,
	LR11XX_WITH_LF_LP_HP_PA,
	LR11XX_WITH_HF_PA,
} lr11xx_pa_type_t;

typedef struct lr11xx_pa_pwr_cfg_s {
	int8_t power;
	uint8_t pa_duty_cycle;
	uint8_t pa_hp_sel;
} lr11xx_pa_pwr_cfg_t;

#define LR11XX_PWR_VREG_VBAT_SWITCH 8

#define LR11XX_MIN_PWR_LP_LF -17
#define LR11XX_MAX_PWR_LP_LF 15

#define LR11XX_MIN_PWR_HP_LF -9
#define LR11XX_MAX_PWR_HP_LF 22

#define LR11XX_MIN_PWR_PA_HF -18
#define LR11XX_MAX_PWR_PA_HF 13

const lr11xx_pa_pwr_cfg_t pa_lp_cfg_table[LR11XX_MAX_PWR_LP_LF - LR11XX_MIN_PWR_LP_LF + 1] =
	LR11XX_PA_LP_LF_CFG_TABLE;
const lr11xx_pa_pwr_cfg_t pa_hp_cfg_table[LR11XX_MAX_PWR_HP_LF - LR11XX_MIN_PWR_HP_LF + 1] =
	LR11XX_PA_HP_LF_CFG_TABLE;

const lr11xx_pa_pwr_cfg_t pa_hf_cfg_table[LR11XX_MAX_PWR_PA_HF - LR11XX_MIN_PWR_PA_HF + 1] =
	LR11XX_PA_HF_CFG_TABLE;

/* NOTE: this implementation is copied from Semtech */
static void prv_lr11xx_get_tx_cfg(lr11xx_pa_type_t pa_type, int8_t expected_output_pwr_in_dbm,
				  ral_lr11xx_bsp_tx_cfg_output_params_t *output_params)
{
	int8_t power = expected_output_pwr_in_dbm;

	// Ramp time is the same for any config
	output_params->pa_ramp_time = LR11XX_RADIO_RAMP_48_US;

	switch (pa_type) {
	case LR11XX_WITH_LF_LP_PA: {
		// Check power boundaries for LP LF PA: The output power must be in range [ -17 ,
		// +15 ] dBm
		if (power < LR11XX_MIN_PWR_LP_LF) {
			power = LR11XX_MIN_PWR_LP_LF;
		} else if (power > LR11XX_MAX_PWR_LP_LF) {
			power = LR11XX_MAX_PWR_LP_LF;
		}
		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_LP;
		output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
		output_params->pa_cfg.pa_duty_cycle =
			pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel =
			pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured =
			pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		break;
	}
	case LR11XX_WITH_LF_HP_PA: {
		// Check power boundaries for HP LF PA: The output power must be in range [ -9 , +22
		// ] dBm
		if (power < LR11XX_MIN_PWR_HP_LF) {
			power = LR11XX_MIN_PWR_HP_LF;
		} else if (power > LR11XX_MAX_PWR_HP_LF) {
			power = LR11XX_MAX_PWR_HP_LF;
		}
		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HP;
		output_params->chip_output_pwr_in_dbm_expected = power;

		if (power <= LR11XX_PWR_VREG_VBAT_SWITCH) {
			// For powers below 8dBm use regulated supply for HP PA for a better
			// efficiency.
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
		} else {
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VBAT;
		}

		output_params->pa_cfg.pa_duty_cycle =
			pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel =
			pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured =
			pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].power;
		break;
	}
	case LR11XX_WITH_LF_LP_HP_PA: {
		// Check power boundaries for LP/HP LF PA: The output power must be in range [ -17 ,
		// +22 ] dBm
		if (power < LR11XX_MIN_PWR_LP_LF) {
			power = LR11XX_MIN_PWR_LP_LF;
		} else if (power > LR11XX_MAX_PWR_HP_LF) {
			power = LR11XX_MAX_PWR_HP_LF;
		}
		output_params->chip_output_pwr_in_dbm_expected = power;

		if (power <= LR11XX_MAX_PWR_LP_LF) {
			output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_LP;
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
			output_params->pa_cfg.pa_duty_cycle =
				pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].pa_duty_cycle;
			output_params->pa_cfg.pa_hp_sel =
				pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].pa_hp_sel;
			output_params->chip_output_pwr_in_dbm_configured =
				pa_lp_cfg_table[power - LR11XX_MIN_PWR_LP_LF].power;
		} else {
			output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HP;
			output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VBAT;
			output_params->pa_cfg.pa_duty_cycle =
				pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].pa_duty_cycle;
			output_params->pa_cfg.pa_hp_sel =
				pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].pa_hp_sel;
			output_params->chip_output_pwr_in_dbm_configured =
				pa_hp_cfg_table[power - LR11XX_MIN_PWR_HP_LF].power;
		}
		break;
	}
	case LR11XX_WITH_HF_PA: {
		// Check power boundaries for HF PA: The output power must be in range [ -18 , +13 ]
		// dBm
		if (power < LR11XX_MIN_PWR_PA_HF) {
			power = LR11XX_MIN_PWR_PA_HF;
		} else if (power > LR11XX_MAX_PWR_PA_HF) {
			power = LR11XX_MAX_PWR_PA_HF;
		}
		output_params->pa_cfg.pa_sel = LR11XX_RADIO_PA_SEL_HF;
		output_params->pa_cfg.pa_reg_supply = LR11XX_RADIO_PA_REG_SUPPLY_VREG;
		output_params->pa_cfg.pa_duty_cycle =
			pa_hf_cfg_table[power - LR11XX_MIN_PWR_PA_HF].pa_duty_cycle;
		output_params->pa_cfg.pa_hp_sel =
			pa_hf_cfg_table[power - LR11XX_MIN_PWR_PA_HF].pa_hp_sel;
		output_params->chip_output_pwr_in_dbm_configured =
			pa_hf_cfg_table[power - LR11XX_MIN_PWR_PA_HF].power;
		output_params->chip_output_pwr_in_dbm_expected = power;
		break;
	}
	}
}

void ral_lr11xx_bsp_get_rf_switch_cfg(const void *context,
				      lr11xx_system_rfswitch_cfg_t *rf_switch_cfg)
{
	const struct device *lr11xx = context;
	const struct lr11xx_hal_context_cfg_t *config = lr11xx->config;

	*rf_switch_cfg = config->rf_switch_cfg;
}

/* NOTE: this implementation is copied from Semtech */
void ral_lr11xx_bsp_get_tx_cfg(const void *context,
			       const ral_lr11xx_bsp_tx_cfg_input_params_t *input_params,
			       ral_lr11xx_bsp_tx_cfg_output_params_t *output_params)
{
	int8_t modem_tx_offset;

	// get modem_configured tx power offset
	if (smtc_modem_get_tx_power_offset_db(0, &modem_tx_offset) != SMTC_MODEM_RC_OK) {
		// in case rc code is not RC_OK, this function will not return the offset and we
		// need to use no offset (in test mode for example)
		modem_tx_offset = 0;
	}

	int16_t power = input_params->system_output_pwr_in_dbm + modem_tx_offset;

	lr11xx_pa_type_t pa_type;

	// check frequency band first to choose LF of HF PA
	if (input_params->freq_in_hz >= 2400000000) {
		pa_type = LR11XX_WITH_HF_PA;
	} else {
		// Modem is acting in subgig band: use LP/HP PA (both LP and HP are connected on
		// lr11xx evk board)
		pa_type = LR11XX_WITH_LF_LP_HP_PA;
	}

	// call the configuration function
	prv_lr11xx_get_tx_cfg(pa_type, power, output_params);
}

void ral_lr11xx_bsp_get_reg_mode(const void *context, lr11xx_system_reg_mode_t *reg_mode)
{
	const struct device *lr11xx = context;
	const struct lr11xx_hal_context_cfg_t *config = lr11xx->config;

	*reg_mode = config->reg_mode;
}

void ral_lr11xx_bsp_get_xosc_cfg(const void *context, bool *tcxo_is_radio_controlled,
				 lr11xx_system_tcxo_supply_voltage_t *supply_voltage,
				 uint32_t *startup_time_in_tick)
{
	const struct device *lr11xx = context;
	const struct lr11xx_hal_context_cfg_t *config = lr11xx->config;
	const struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	// Radio control TCXO 1.8V and 5 ms of startup time
	*tcxo_is_radio_controlled = tcxo_cfg.has_tcxo;
	*supply_voltage = tcxo_cfg.supply;
	*startup_time_in_tick = lr11xx_radio_convert_time_in_ms_to_rtc_step(tcxo_cfg.timeout_ms);
}

void ral_lr11xx_bsp_get_crc_state(const void *context, bool *crc_is_activated)
{
#ifdef CONFIG_LR11XX_USE_CRC_OVER_SPI
	*crc_is_activated = true;
#else
	*crc_is_activated = false;
#endif
}

/* NOTE: this is a copy from Semtech */
void ral_lr11xx_bsp_get_rssi_calibration_table(
	const void *context, const uint32_t freq_in_hz,
	lr11xx_radio_rssi_calibration_table_t *rssi_calibration_table)
{
	if (freq_in_hz <= 600000000) {
		rssi_calibration_table->gain_offset = 0;
		rssi_calibration_table->gain_tune.g4 = 12;
		rssi_calibration_table->gain_tune.g5 = 12;
		rssi_calibration_table->gain_tune.g6 = 14;
		rssi_calibration_table->gain_tune.g7 = 0;
		rssi_calibration_table->gain_tune.g8 = 1;
		rssi_calibration_table->gain_tune.g9 = 3;
		rssi_calibration_table->gain_tune.g10 = 4;
		rssi_calibration_table->gain_tune.g11 = 4;
		rssi_calibration_table->gain_tune.g12 = 3;
		rssi_calibration_table->gain_tune.g13 = 6;
		rssi_calibration_table->gain_tune.g13hp1 = 6;
		rssi_calibration_table->gain_tune.g13hp2 = 6;
		rssi_calibration_table->gain_tune.g13hp3 = 6;
		rssi_calibration_table->gain_tune.g13hp4 = 6;
		rssi_calibration_table->gain_tune.g13hp5 = 6;
		rssi_calibration_table->gain_tune.g13hp6 = 6;
		rssi_calibration_table->gain_tune.g13hp7 = 6;
	} else if ((600000000 <= freq_in_hz) && (freq_in_hz <= 2000000000)) {
		rssi_calibration_table->gain_offset = 0;
		rssi_calibration_table->gain_tune.g4 = 2;
		rssi_calibration_table->gain_tune.g5 = 2;
		rssi_calibration_table->gain_tune.g6 = 2;
		rssi_calibration_table->gain_tune.g7 = 3;
		rssi_calibration_table->gain_tune.g8 = 3;
		rssi_calibration_table->gain_tune.g9 = 4;
		rssi_calibration_table->gain_tune.g10 = 5;
		rssi_calibration_table->gain_tune.g11 = 4;
		rssi_calibration_table->gain_tune.g12 = 4;
		rssi_calibration_table->gain_tune.g13 = 6;
		rssi_calibration_table->gain_tune.g13hp1 = 5;
		rssi_calibration_table->gain_tune.g13hp2 = 5;
		rssi_calibration_table->gain_tune.g13hp3 = 6;
		rssi_calibration_table->gain_tune.g13hp4 = 6;
		rssi_calibration_table->gain_tune.g13hp5 = 6;
		rssi_calibration_table->gain_tune.g13hp6 = 7;
		rssi_calibration_table->gain_tune.g13hp7 = 6;
	} else // freq_in_hz > 2000000000
	{
		rssi_calibration_table->gain_offset = 2030;
		rssi_calibration_table->gain_tune.g4 = 6;
		rssi_calibration_table->gain_tune.g5 = 7;
		rssi_calibration_table->gain_tune.g6 = 6;
		rssi_calibration_table->gain_tune.g7 = 4;
		rssi_calibration_table->gain_tune.g8 = 3;
		rssi_calibration_table->gain_tune.g9 = 4;
		rssi_calibration_table->gain_tune.g10 = 14;
		rssi_calibration_table->gain_tune.g11 = 12;
		rssi_calibration_table->gain_tune.g12 = 14;
		rssi_calibration_table->gain_tune.g13 = 12;
		rssi_calibration_table->gain_tune.g13hp1 = 12;
		rssi_calibration_table->gain_tune.g13hp2 = 12;
		rssi_calibration_table->gain_tune.g13hp3 = 12;
		rssi_calibration_table->gain_tune.g13hp4 = 8;
		rssi_calibration_table->gain_tune.g13hp5 = 8;
		rssi_calibration_table->gain_tune.g13hp6 = 9;
		rssi_calibration_table->gain_tune.g13hp7 = 9;
	}
}
