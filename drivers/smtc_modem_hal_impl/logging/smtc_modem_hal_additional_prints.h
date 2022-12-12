/** @file smtc_modem_hal_additional_prints.h
 *
 * @brief Additional prints for custom logging hooks
 *
 * @par
 * COPYRIGHT NOTICE: (c) 2022 Irnas.  All rights reserved.
 */

#ifndef SMTC_MODEM_HAL_ADDITIONAL_PRINTS_H
#define SMTC_MODEM_HAL_ADDITIONAL_PRINTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void smtc_modem_hal_print_trace(const char *fmt, ...);
void smtc_modem_hal_print_trace_inf(const char *fmt, ...);
void smtc_modem_hal_print_trace_dbg(const char *fmt, ...);
void smtc_modem_hal_print_trace_wrn(const char *fmt, ...);
void smtc_modem_hal_print_trace_err(const char *fmt, ...);
void smtc_modem_hal_print_trace_array_inf(char *msg, uint8_t *array, uint32_t len);
void smtc_modem_hal_print_trace_array_dbg(char *msg, uint8_t *array, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* SMTC_MODEM_HAL_ADDITIONAL_PRINTS_H */
