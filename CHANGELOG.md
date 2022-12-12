# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [Unreleased]

### Added

- Semtech's original lora basics modem (SWL2001) at v3.2.4.
- Zephyr HAL implementation.
- Zephyr RAL for LR11XX implementation using SWDR001-Zephyr.
- smtc_app wrapper for easier use of the library
- Opinionated own implementation of logging by creating own `smtc_modem_hal_dbg_trace.h`.
- The following samples:
  - almanac_update
  - large_file_upload
  - lorawan_asynchronous
  - lorawan_class_b
  - lorawan_multicast_class_c
  - time_sync
  - dm_info
  - lorawan
  - lorawan_asynchronous_fhss
  - lorawan_multicast_class_b
  - stream
  - tx_rx_continous
