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

struct smtc_modem_hal_cb {
	/**
	 * @brief Get battery level callback
	 *
	 * @param [out] value The battery level (in permille (‰) , where 1000 ‰ means full battery).
	 * The application should set this to the most recent battery level it has avaliable
	 *
	 * @return int 0 if the battery level was set, or a negative error code if no valid battery
	 * level is available
	 */
	int (*get_battery_level)(uint32_t *value);

	/**
	 * @brief Get temperature callback
	 *
	 * @param [out] value The temperature (in deg C). The application should set this to
	 * the most recent temperature value it has avaliable
	 *
	 * @return int 0 if the temperature was set, or a negative error code if no valid
	 * temperature is available
	 */
	int (*get_temperature)(int32_t *value);

	/**
	 * @brief Get voltage level callback
	 *
	 * @param [out] value The voltage level (in mV). The application should set this to
	 * the most recent voltage level it has avaliable
	 *
	 * @return int 0 if the voltage level was set, or a negative error code if no valid voltage
	 * level is available
	 */
	int (*get_voltage)(uint32_t *value);

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

	/**
	 * @brief Persistently store context from the lora basics modem
	 *
	 * The application should use some persistent storage to store the context.
	 *
	 * @param[in] ctx_id The ID of the context to store. Each ID must be stored separately.
	 * @param[in] buffer The buffer to store.
	 * @param[in] size The size of the buffer to store, in bytes.
	 */
	void (*context_store)(const uint8_t ctx_id, const uint8_t *buffer, const uint32_t size);

	/**
	 * @brief Restore context to the lora basics modem
	 *
	 * The application should load the context from the persistent storage used in
	 * context_store.
	 *
	 * @param[in] ctx_id The ID of the context to restore.
	 * @param[in] buffer The buffer to read into.
	 * @param[in] size The size of the buffer, in bytes.
	 */
	void (*context_restore)(const uint8_t ctx_id, uint8_t *buffer, const uint32_t size);

#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */
};

/**
 * @brief Initialization of the hal implementation.
 *
 * This must be called before smtc_modem_init
 *
 * @param[in] lr11xx The device pointer of the lr11xx instance that will be used.
 * @param[in] hal_cb The callbacks to use for the hal implementation. Mus not be NULL. All of the
 * callbacks must be set.
 */
void smtc_modem_hal_init(const struct device *lr11xx, struct smtc_modem_hal_cb *hal_cb);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_INIT_H */
