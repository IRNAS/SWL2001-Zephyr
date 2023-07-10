/** @file smtc_modem_hal.c
 *
 * @brief Zephyr implementation of the HAL
 *
 * This HAL was written using v5 of the porting guide, which can be found here:
 * https://semtech.my.salesforce.com/sfc/p/#E0000000JelG/a/3n000000vBOS/cId_tITnWws2wVARy7NIpzXUl2MK1G7At3y_XP3kAQo
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas. All rights reserved.
 */

#include <smtc_modem_hal.h>
#include <smtc_modem_hal_init.h>

#include <zephyr/kernel.h>
#include <zephyr/random/rand32.h>
#include <zephyr/settings/settings.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/reboot.h>

#include <lr11xx_board.h>
#include <lr11xx_radio.h>
#include <lr11xx_system.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(smtc_modem_hal);

/* ------------ Local context ------------*/

/* lr11xx device pointer */
static const struct device *prv_lr11xx_dev;

/* External callbacks */
static struct smtc_modem_hal_cb *prv_hal_cb;

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
static bool prv_skip_next_radio_irq;

/* ------------ Initialization ------------
 *
 * This function is defined in smtc_modem_hal_init.h
 * and is used to set everything up in here.
 */

void smtc_modem_hal_init(const struct device *lr11xx, struct smtc_modem_hal_cb *hal_cb)
{
	__ASSERT(lr11xx, "lr11xx must be provided");
	__ASSERT_NO_MSG(hal_cb);
	__ASSERT_NO_MSG(hal_cb->get_battery_level);
	__ASSERT_NO_MSG(hal_cb->get_temperature);
	__ASSERT_NO_MSG(hal_cb->get_voltage);

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL
	__ASSERT_NO_MSG(hal_cb->context_store);
	__ASSERT_NO_MSG(hal_cb->context_restore);
#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

	prv_lr11xx_dev = lr11xx;
	prv_hal_cb = hal_cb;
}

/* ------------ Reset management ------------*/

void smtc_modem_hal_reset_mcu(void)
{
	LOG_WRN("Resetting the MCU");
	k_sleep(K_SECONDS(1)); /* Sleep a bit so logs are printed. */
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

int32_t smtc_modem_hal_get_time_compensation_in_s(void)
{
	/* There is no compensation going on */
	return 0;
}

/* There is no difference in zephyr between "uncompensated" and "compensated" time,
 * so we just use the same implementation.
 * Semtech does something similar in their HAl for stm
 */
uint32_t smtc_modem_hal_get_compensated_time_in_s(void)
{
	return smtc_modem_hal_get_time_in_s() + smtc_modem_hal_get_time_compensation_in_s();
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

/* Semtech's HAL for STM uses the same implementation for mtc_modem_hal_get_time_in_100us
 * and smtc_modem_hal_get_radio_irq_timestamp_in_100us.
 *
 * This does not make sense from just reading the doscstring in smtc_modem_hal.h or the porting
 * guide, where we would assume smtc_modem_hal_get_radio_irq_timestamp_in_100us should return a time
 * difference from some "event".
 *
 * This should be checked with Semtech
 */
uint32_t smtc_modem_hal_get_radio_irq_timestamp_in_100us(void)
{
	return smtc_modem_hal_get_time_in_100us();
}

/* ------------ Timer management ------------*/

/**
 * @brief Called when the prv_smtc_modem_hal_timer_work is submitted.
 *
 * Actually calls the callback that was requested by the smtc modem stack.
 */
static void prv_smtc_modem_hal_timer_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	prv_smtc_modem_hal_timer_callback(prv_smtc_modem_hal_timer_context);
}

/**
 * @brief Called when the prv_smtc_modem_hal_timer expires.
 *
 * Submits the prv_smtc_modem_hal_timer_work to be handle the callback.
 */
static void prv_smtc_modem_hal_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	if (prv_modem_irq_enabled) {
		/* We have to execute this from a thread and not from irq since SPI transactions to
		 * lr11xx may be performed. */
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

#ifdef CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL

void smtc_modem_hal_context_restore(const modem_context_type_t ctx_type, uint8_t *buffer,
				    const uint32_t size)
{
	prv_hal_cb->context_restore(ctx_type, buffer, size);
}

void smtc_modem_hal_context_store(const modem_context_type_t ctx_type, const uint8_t *buffer,
				  const uint32_t size)
{
	prv_hal_cb->context_store(ctx_type, buffer, size);
}

void smtc_modem_hal_store_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
	/* We use MODEM_CONTEXT_TYPE_SIZE as the ID so we do not overwrite any of the contexts */
	prv_hal_cb->context_store(MODEM_CONTEXT_TYPE_SIZE, crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_restore_crashlog(uint8_t crashlog[CRASH_LOG_SIZE])
{
	prv_hal_cb->context_restore(MODEM_CONTEXT_TYPE_SIZE, crashlog, CRASH_LOG_SIZE);
}

void smtc_modem_hal_set_crashlog_status(bool available)
{
	prv_hal_cb->context_store(MODEM_CONTEXT_TYPE_SIZE + 1, (uint8_t *)&available,
				  sizeof(available));
}

bool smtc_modem_hal_get_crashlog_status(void)
{
	bool available;
	prv_hal_cb->context_restore(MODEM_CONTEXT_TYPE_SIZE + 1, (uint8_t *)&available,
				    sizeof(available));
	return available;
}

#else

/* Context for loading from persistant storage */
static uint8_t *prv_load_into;
static uint32_t prv_load_size;

static int prv_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg);
static struct settings_handler prv_sh = {
	.name = "smtc_modem_hal",
	.h_set = prv_set,
};

/**
 * @brief Called when settings_load_subtree in prv_load is called.
 *
 * Reads the data from NVS into prv_load_into.
 */
static int prv_set(const char *name, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	/* We always load only 1 value via settings_load_subtree,
	 * so there is no need to check the name */

	int err = read_cb(cb_arg, prv_load_into, prv_load_size);
	if (err < 0) {
		LOG_ERR("Unable to load %s", name);
		/* NOTE: We do no error checking here. This never seems to fail in the way we are
		 * currently using the settings module. If this start to fail in some project,
		 * consider adding some error handling. */
	}
	LOG_INF("Loaded %s", name);

	return 0;
}

/**
 * @brief If not already done, initializes the settings subsystem and registers the smtc_modem_hal
 * settings.
 */
void prv_settings_init(void)
{
	static bool is_init = false;

	if (!is_init) {
		is_init = true;

		settings_subsys_init();
		settings_register(&prv_sh);
	}
}

/**
 * @brief Stores the data in buffer to settings.
 *
 * @param[in] path The settings path to store the data to.
 * @param[in] buffer The data to store.
 * @param[in] size The size of the data, in bytes.
 */
static void prv_store(char *path, const uint8_t *buffer, const uint32_t size)
{
	prv_settings_init();

	settings_save_one(path, buffer, size);
}

/**
 * @brief Load the data into a buffer from settings.
 *
 * @param[in] path The settings path to read the data from.
 * @param[in] buffer The buffer to read into.
 * @param[in] size The number of bytes to read.
 */
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

#endif /* CONFIG_LORA_BASICS_MODEM_USER_STORAGE_IMPL */

/* ------------ assert management ------------*/

void smtc_modem_hal_assert_fail(uint8_t *func, uint32_t line)
{

	/* NOTE: uint8_t *func parameter is actually __func__ casted to uint8_t*,
	 * so it can be safely printed with %s */
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
	/* Implementation copied from the porting guide section 5.21 */
	if (val_1 <= val_2) {
		return (uint32_t)((smtc_modem_hal_get_random_nb() % (val_2 - val_1 + 1)) + val_1);
	} else {
		return (uint32_t)((smtc_modem_hal_get_random_nb() % (val_1 - val_2 + 1)) + val_2);
	}
}

int32_t smtc_modem_hal_get_signed_random_nb_in_range(const int32_t val_1, const int32_t val_2)
{
	/* Implementation copied from the porting guide section 5.22 */
	uint32_t tmp_range = 0; // ( val_1 <= val_2 ) ? ( val_2 - val_1 ) : ( val_1 - val_2 );
	if (val_1 <= val_2) {
		tmp_range = (val_2 - val_1);
		return (int32_t)((val_1 + smtc_modem_hal_get_random_nb_in_range(0, tmp_range)));
	} else {
		tmp_range = (val_1 - val_2);
		return (int32_t)((val_2 + smtc_modem_hal_get_random_nb_in_range(0, tmp_range)));
	}
}

/* ------------ Radio env management ------------*/

/**
 * @brief Called when the lr11xx event pin interrupt is triggered.
 *
 * If CONFIG_LR11XX_EVENT_TRIGGER_GLOBAL_THREAD=y, this is called in the system workq.
 * If CONFIG_LR11XX_EVENT_TRIGGER_OWN_THREAD=y, this is called in the lr11xx event thread.
 *
 * @param[in] dev The lr11xx device.
 */
void prv_lr11xx_event_cb(const struct device *dev)
{
	/* This logic is based on our understanding of section 5.24 of the porting guide.
	 * NOTE:
	 * In simple (init, join, uplink) tests smtc_modem_hal_radio_irq_clear_pending is
	 * never called. This means that prv_skip_next_radio_irq is never true and no callbacks are
	 * ever skipped.
	 * This is why the LOG bellow is a warning. If it gets printed and you are encountering
	 * issues, this might be the culprit. */
	if (prv_skip_next_radio_irq) {
		LOG_WRN("Skipping radio irq");
		prv_skip_next_radio_irq = false;
		return;
	}

	/* Due to the way the lr11xx driver is implemented, this is called from the system workq. */
	prv_smtc_modem_hal_radio_irq_callback(prv_smtc_modem_hal_radio_irq_context);
}

void smtc_modem_hal_irq_config_radio_irq(void (*callback)(void *context), void *context)
{
	/* save callback function and context */
	prv_smtc_modem_hal_radio_irq_context = context;
	prv_smtc_modem_hal_radio_irq_callback = callback;

	/* enable callback via lr11xx driver */
	lr11xx_board_attach_interrupt(prv_lr11xx_dev, prv_lr11xx_event_cb);
	lr11xx_board_enable_interrupt(prv_lr11xx_dev);
}

void smtc_modem_hal_irq_reset_radio_irq(void)
{
	lr11xx_board_disable_interrupt(prv_lr11xx_dev);
	lr11xx_board_attach_interrupt(prv_lr11xx_dev, prv_lr11xx_event_cb);
	lr11xx_board_enable_interrupt(prv_lr11xx_dev);
}

void smtc_modem_hal_radio_irq_clear_pending(void)
{
	LOG_DBG("Clear pending radio irq");
	prv_skip_next_radio_irq = true;
}

void smtc_modem_hal_start_radio_tcxo(void)
{
	/* We only support TCXO's that are wired to the LR11XX. In such cases, this function must be
	 * empty. See 5.25 of the porting guide. */
}

void smtc_modem_hal_stop_radio_tcxo(void)
{
	/* We only support TCXO's that are wired to the LR11XX. In such cases, this function must be
	 * empty. See 5.26 of the porting guide. */
}

uint32_t smtc_modem_hal_get_radio_tcxo_startup_delay_ms(void)
{
	/* From the porting guide:
	 * If the TCXO is configured by the RAL BSP to start up automatically, then the value used
	 * here should be the same as the startup delay used in the RAL BSP.
	 */
	const struct lr11xx_hal_context_cfg_t *config = prv_lr11xx_dev->config;
	const struct lr11xx_hal_context_tcxo_cfg_t tcxo_cfg = config->tcxo_cfg;

	return tcxo_cfg.timeout_ms;
}

/* ------------ Environment management ------------*/

/* This is called when LoRa Cloud sends a device status request */
uint8_t smtc_modem_hal_get_battery_level(void)
{
	uint32_t battery;
	int ret = prv_hal_cb->get_battery_level(&battery);

	if (ret) {
		return 0;
	}

	if (battery > 1000) {
		return 255;
	}

	/* scale 0-1000 to 0-255 */
	return (uint8_t)((float)battery * 0.255);
}

/* This is called when DM info required is */
int8_t smtc_modem_hal_get_temperature(void)
{
	int32_t temperature;
	int ret = prv_hal_cb->get_temperature(&temperature);

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
	int ret = prv_hal_cb->get_voltage(&voltage);

	if (ret) {
		return 0;
	}

	/* Step used in Semtech's hal is 1/50V == 20 mV */
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

/*
 * NOTE: smtc_modem_hal_print_trace() is implemented in ./logging/smtc_modem_hal_additional_prints.c
 */