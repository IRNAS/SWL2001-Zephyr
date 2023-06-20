# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [Unreleased]

### Fixed

- `smtc_modem_hal_print_trace_array_*` functions incorrectly trimming immutable strings and causing a hard fault.

## [1.1.0] - 2023-06-19

## Changed

-   Update to NCS 2.2.0.
-   Disable compiler warnings for Semtech's code.

## [1.0.1] - 2023-06-16

### Fixed

-   Log level for printing trace array (set to debug).
-   Battery level to use correct scaling.

### Changed

-   Refactor HAL time management to use compensation (Note that the compensation value is still 0 by default).

## [1.0.0] - 2022-12-12

### Added

-   Semtech's original lora basics modem (SWL2001) at v3.2.4.
-   Zephyr HAL implementation.
-   Zephyr RAL for LR11XX implementation using SWDR001-Zephyr.
-   smtc_app wrapper for easier use of the library
-   Opinionated own implementation of logging by creating own `smtc_modem_hal_dbg_trace.h`.
-   The following samples:
    -   almanac_update
    -   large_file_upload
    -   lorawan_asynchronous
    -   lorawan_class_b
    -   lorawan_multicast_class_c
    -   time_sync
    -   dm_info
    -   lorawan
    -   lorawan_asynchronous_fhss
    -   lorawan_multicast_class_b
    -   stream
    -   tx_rx_continous

[Unreleased]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.1.0...HEAD

[1.1.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.0.1...v1.1.0

[1.0.1]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.0.0...v1.0.1

[1.0.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/52a1e1e0301ef9fc9b7c1418cee0aed9ef185e0d...v1.0.0
