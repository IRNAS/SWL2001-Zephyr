/** @file smtc_modem_hal.c
 *
 * @brief Zephyr implementation of the HAL
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <smtc_modem_hal.h>
#include <smtc_modem_hal_init.h>

#include <zephyr/random/rand32.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/zephyr.h>

#include <lr11xx_board.h>
#include <lr11xx_radio.h>
#include <lr11xx_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smtc_modem_hal);

/* ------------ Local context ------------*/

/* lr11xx device pointer */
const struct device *prv_lr11xx_dev;

/* environment getters */
static get_battery_level_cb_t prv_get_battery_level_cb;
static get_temperature_cb_t prv_get_temperature_cb;
static get_voltage_cb_t prv_get_voltage_cb;

/* context and callback for modem_hal_timer */
static void *prv_smtc_modem_hal_timer_context;
static void (*prv_smtc_modem_hal_timer_callback)(void *context);

/* flag for enabling/disabling timer interrupt. This is set by the libraray during "critical"
 * sections */
static bool prv_modem_irq_enabled;

/* The timer and work used for the modem_hal_timer */
static void prv_smtc_modem_hal_timer_handler(struct k_timer *timer);
K_TIMER_DEFINE(prv_smtc_modem_hal_timer, prv_smtc_modem_hal_timer_handler, NULL);

static void prv_smtc_modem_hal_timer_work_handler(struct k_work *work);
K_WORK_DEFINE(prv_smtc_modem_hal_timer_work, prv_smtc_modem_hal_timer_work_handler);

/* context and callback for the event pin interrupt */
static void *prv_smtc_modem_hal_radio_irq_context;
static void (*prv_smtc_modem_hal_radio_irq_callback)(void *context);

/* Context for loading from persistant storage */
static uint8_t *prv_load_into;
static uint32_t prv_load_size;

static int prv_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
static struct settings_handler prv_sh = {
	.name = "smtc_modem_hal",
	.h_set = prv_set,
};

/* ------------ Initialization ------------
 *
 * This function is defined in smtc_modem_hal_init.h
 * and is used to set everything up in here.
 */

void smtc_modem_hal_init(const struct device *lr11xx, get_battery_level_cb_t get_battery,
			 get_temperature_cb_t get_temperature, get_voltage_cb_t get_voltage)
{
	__ASSERT(lr11xx, "lr11xx must be provided");
	__ASSERT(get_battery, "get_battery must be provided");
	__ASSERT(get_temperature, "get_temperature must be provided");
	__ASSERT(get_voltage, "get_voltage must be provided");

	prv_lr11xx_dev = lr11xx;
	prv_get_battery_level_cb = get_battery;
	prv_get_temperature_cb = get_temperature;
	prv_get_voltage_cb = get_voltage;
}

/* ------------ Reset management ------------*/

void smtc_modem_hal_reset_mcu(void)
{
	sys_reboot(SYS_REBOOT_COLD);
}

/* ------------ Watchdog management ------------*/

void smtc_modem_hal_reload_wdog(void)
{
	/* This is not called anywhere from the smtc modem stack, so I am unsure why this is
	 * required */
}

/* ------------ Time management ------------*/

uint32_t smtc_modem_hal_get_time_in_s(void)
{
	int64_t time_ms = k_uptime_get();

	return (uint32_t)(time_ms / 1000);
}

/* There is no difference in zephyr between "uncompensated" and "compensated" time,
 * so we just use the same implementation.
 * Semtech does something similar in their HAl for stm
 */
uint32_t smtc_modem_hal_get_compensated_time_in_s(void)
{
	return smtc_modem_hal_get_time_in_s();
}

int32_t smtc_modem_hal_get_time_compensation_in_s(void)
{
	/* There is no compensation going on */
	return 0;
}

uint32_t smtc_modem_hal_get_time_in_ms(void)
{
	/* The wrapping every 49 days is expected by the modem lib */
	return k_uptime_get_32();
}

uint32_t smtc_modem_hal_get_time_in_100us(void)
{
	int64_t current_ticks = k_uptime_ticks();

	/*
	 * ticks_per_100us = ticks_per_second / 10000.
	 *
	 * time_in_100us = current_ticks / ticks_per_100us
	 *
	 * Since we do not want to lose precision by dividing
	 * in the ticks_per_100us calculation, we move the division up
	 * in the double fraction to make it a multiplication.
	 *
	 *    cur_ticks        ticks_per_s * 10000
	 *   -----------  =  ----------------------
	 *   ticks_per_s           cur_ticks
	 *   -----------
	 *      10000
	 *
	 * The wrapping every 4.9 days is expected by the modem lib
	 */
	return (current_ticks * 10000) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
}

/* Semtechs HAL for STM uses the same implemetation for mtc_modem_hal_get_time_in_100us
 * and smtc_modem_hal_get_radio_irq_timestamp_in_100us.
 *
 * This does not make sense from just reading the doscstring in smtc_modem_hal.h,
 * where we would assume smtc_modem_hal_get_radio_irq_timestamp_in_100us
 * should return a time difference from some "event".
 *
 * This should be checked with Semtech
 */
uint32_t smtc_modem_hal_get_radio_irq_timestamp_in_100us(void)
{
	return smtc_modem_hal_get_time_in_100us();
}

/* ------------ Timer management ------------*/

static void prv_smtc_modem_hal_timer_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	prv_smtc_modem_hal_timer_callback(prv_smtc_modem_hal_timer_context);
}

static void prv_smtc_modem_hal_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (prv_modem_irq_enabled) {
		/* Semtech is a bad boy :(
		 * We have to execute this from a thread and not from irq.
		 */
		k_work_submit(&prv_smtc_modem_hal_timer_work);
	}
};

void smtc_modem_hal_start_timer(const uint32_t milliseconds, void (*callback)(void *context),
				void *context)
{
	prv_smtc_modem_hal_timer_callback = callback;
	prv_smtc_modem_hal_timer_context = context;

	/* start one-shot timer */
	k_timer_start(&prv_smtc_modem_hal_timer, K_MSEC(milliseconds), K_NO_WAIT);
}

void smtc_modem_hal_stop_timer(void)
{
	k_timer_stop(&prv_smtc_modem_hal_timer);
}

/* ------------ IRQ management ------------*/

void smtc_modem_hal_disable_modem_irq(void)
{
	prv_modem_irq_enabled = false;
	lr11xx_board_disable_interrupt(prv_lr11xx_dev);
}

void smtc_modem_hal_enable_modem_irq(void)
{
	prv_modem_irq_enabled = true;
	lr11xx_board_enable_interrupt(prv_lr11xx_dev);
}

/* ------------ Context saving management ------------*/

static int prv_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	/* We always load only 1 value via settings_load_subtree,
	 * so there is no need to check the name
	 */

	int err = read_cb(cb_arg, prv_load_into, prv_load_size);
	if (err < 0) {
		LOG_ERR("Unable to load %s", name);
		/* TODO: We do no error checking here. This never seems to fail in the way we are
		 * currently using the settings module. If this start to fail in some project,
		 * consider adding some error handling. */
	}
	LOG_INF("Loaded %s", name);

	return 0;
}

void prv_settings_init(void)
{
	static bool is_init = false;

	if (!is_init) {
		is_init = true;

		settings_subsys_init();
		settings_register(&prv_sh);
	}
}

static void prv_store(char *path, const uint8_t *buffer, const uint32_t size)
{
	prv_settings_init();

	settings_save_one(path, buffer, size);
}

static void prv_load(char *path, uint8_t *buffer, const uint32_t size)
{

	prv_settings_init();

	prv_load_into = buffer;
	prv_load_size = size;
	settings_load_subtree(path);
}

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint8_t *buffer,
				    const uint32_t size)
{

	char path[30];
	snprintk(path, sizeof(path), "smtc_modem_hal/context/%d", ctx_type);
	prv_load(path, buffer, size);
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, const uint8_t *buffer,
				  const uint32_t size)
{

	char path[30];
	snprintk(path, sizeof(path), "smtc_modem_hal/context/%d", ctx_type);
	prv_store(path, buffer, size);
}

void smtc_modem_hal_store_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
	prv_store("smtc_modem_hal/crashlog", crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_restore_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
	prv_load("smtc_modem_hal/crashlog", crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_set_crashlog_status(bool available)
{
	prv_store("smtc_modem_hal/crashlog_status", (uint8_t *)&available, sizeof(available));
}

bool smtc_modem_hal_get_crashlog_status(void)
{
	bool available;
	prv_load("smtc_modem_hal/crashlog_status", (uint8_t *)&available, sizeof(available));
	return available;
}

/* ------------ assert management ------------*/

void smtc_modem_hal_assert_fail(uint8_t *func, uint32_t line)
{

	/* NOTE: uint8_t *func parameter is actually __func__ casted to uint8_t*,
	 * so it can be safely printed with %s
	 */
	smtc_modem_hal_store_crashlog((uint8_t *)func);
	smtc_modem_hal_set_crashlog_status(true);
	LOG_ERR("Assert triggered. Crash log: %s:%u", func, line);

	/* calling assert here will halt execution if asserts are enabled and we are debugging.
	 * Otherwise, a reset of the mcu is performed */
	__ASSERT(false, "smtc_modem_hal_assert triggered.");

	smtc_modem_hal_reset_mcu();
}

/* ------------ Random management ------------*/

uint32_t smtc_modem_hal_get_random_nb(void)
{
	return sys_rand32_get();
}

uint32_t smtc_modem_hal_get_random_nb_in_range(const uint32_t val_1, const uint32_t val_2)
{
	/* Copy from Semtech since wether val_2 is included in the range or or not is unclear */
	if (val_1 <= val_2) {
		return (uint32_t)((smtc_modem_hal_get_random_nb() % (val_2 - val_1 + 1)) + val_1);
	} else {
		return (uint32_t)((smtc_modem_hal_get_random_nb() % (val_1 - val_2 + 1)) + val_2);
	}
}

int32_t smtc_modem_hal_get_signed_random_nb_in_range(const int32_t val_1, const int32_t val_2)
{
	/* Copy from Semtech since wether val_2 is included in the range or or not is unclear */
	uint32_t tmp_range = 0;

	if (val_1 <= val_2) {
		tmp_range = (val_2 - val_1);
		return (int32_t)((val_1 + smtc_modem_hal_get_random_nb_in_range(0, tmp_range)));
	} else {
		tmp_range = (val_1 - val_2);
		return (int32_t)((val_2 + smtc_modem_hal_get_random_nb_in_range(0, tmp_range)));
	}
}

/* ------------ Radio env management ------------*/

void lr11xx_event_cb(const struct device *dev)
{
	prv_smtc_modem_hal_radio_irq_callback(prv_smtc_modem_hal_radio_irq_context);
}

void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void *context), void *context)
{
	/* Unsure if this is correct:
	 *
	 * We are using lr11xx here, since the pin callback is implemented there
	 */
	prv_smtc_modem_hal_radio_irq_context = context;
	prv_smtc_modem_hal_radio_irq_callback = callback;

	/* enable callback */
	lr11xx_board_attach_interrupt(prv_lr11xx_dev, lr11xx_event_cb);
	lr11xx_board_enable_interrupt(prv_lr11xx_dev);
}

void smtc_modem_hal_radio_irq_clear_pending(void)
{
	/* Nothing to do here, this is handled by Zephyr */
}

static void prv_tcxo_set(bool enable)
{
	const struct lr11xx_hal_context_cfg_t *config = prv_lr11xx_dev->config;
	const struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	/* if ther is no tcxo, exit early */
	if (!tcxo_cfg.has_tcxo) {
		return;
	}

	/* a timeout of 0 means "disable" */
	uint32_t timeout_rtc_step = 0;
	if (enable) {
		timeout_rtc_step = lr11xx_radio_convert_time_in_ms_to_rtc_step(tcxo_cfg.timeout_ms);
	}

	int ret = lr11xx_system_set_tcxo_mode(prv_lr11xx_dev, tcxo_cfg.supply, timeout_rtc_step);
	if (ret) {
		LOG_ERR("Failed to configure TCXO.");
	}
}

/* NOTE: since not all lora radios support tcxo, this must be handled in the modem HAL, and not in
 * the lr11xx hal */
void smtc_modem_hal_start_radio_tcxo(void)
{
	prv_tcxo_set(true);
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
	prv_tcxo_set(false);
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
	const struct lr11xx_hal_context_cfg_t *config = prv_lr11xx_dev->config;
	const struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	return tcxo_cfg.timeout_ms;
}

/* ------------ Environment management ------------*/

/* This is called when LoRa Cloud sends a device status request */
uint8_t smtc_modem_hal_get_battery_level(void)
{
	uint8_t battery;
	int ret = prv_get_battery_level_cb(&battery);

	if (ret) {
		return 0;
	}

	return battery;
}

/* This is called when DM info required is */
int8_t smtc_modem_hal_get_temperature(void)
{
	int32_t temperature;
	int ret = prv_get_temperature_cb(&temperature);

	if (ret) {
		return -128;
	}

	if (temperature < -128) {
		return -128;
	} else if (temperature > 127) {
		return 127;
	} else {
		return (int8_t)temperature;
	}
}

/* This is called when DM info required is */
uint8_t smtc_modem_hal_get_voltage(void)
{

	uint32_t voltage;
	int ret = prv_get_voltage_cb(&voltage);

	if (ret) {
		return 0;
	}

	/* Step used in Semtechs hal is 1/50V == 20 mV */
	uint32_t converted = voltage / 20;

	if (converted > 255) {
		return 255;
	} else {
		return (uint8_t)converted;
	}
}

/* ------------ Misc ------------*/

int8_t smtc_modem_hal_get_board_delay_ms(void)
{
	/* the wakeup time is probably closer to 0ms then 1ms,
	 * but just to be safe: */
	return 1;
}
