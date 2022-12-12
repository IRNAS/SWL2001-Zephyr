/** @file smtc_modem_hal_init.h
 *
 * @brief Initializer of the smtc modem hal implementation
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas.  All rights reserved.
 */

#ifndef SMTC_MODEM_HAL_INIT_H
#define SMTC_MODEM_HAL_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <zephyr/device.h>

/**
 * @brief Get battery level callback
 *
 * @param [out] value The battery level (in %, where 100% means full battery). The application
 * should set this to the most recent battery level it has avaliable
 *
 * @return int 0 if the battery level was set, or a negative error code if no valid battery
 * level is available
 */
typedef int (*get_battery_level_cb_t)(uint8_t *value);

/**
 * @brief Get temperature callback
 *
 * @param [out] value The temperature (in deg C). The application should set this to
 * the most recent temperature value it has avaliable
 *
 * @return int 0 if the temperature was set, or a negative error code if no valid
 * temperature is available
 */
typedef int (*get_temperature_cb_t)(int32_t *value);

/**
 * @brief Get voltage level callback
 *
 * @param [out] value The voltage level (in mV). The application should set this to
 * the most recent voltage level it has avaliable
 *
 * @return int 0 if the voltage level was set, or a negative error code if no valid voltage
 * level is available
 */
typedef int (*get_voltage_cb_t)(uint32_t *value);

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] lr11xx The device pointer of the lr11xx instance that will be used.
 * @param[in] get_battery The battery level getter callback. Must not be NULL.
 * @param[in] get_temperature The temperature getter callback. Must not be NULL.
 * @param[in] get_voltage The battery voltage level callback. Must not be NULL.
 */
void smtc_modem_hal_init(const struct device *lr11xx, get_battery_level_cb_t get_battery,
			 get_temperature_cb_t get_temperature, get_voltage_cb_t get_voltage);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_INIT_H */
