# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [Unreleased]

## [1.4.2] - 2024-06-19

## [1.4.1] - 2024-06-19

### Changed

-   Update header inclusions for NCS 3.5.0 and above compatibility.

## [1.4.0] - 2023-08-01

### Added

-   Option to disable default context storage implementation in the HAL and provide one's own implementation.
-   `custom_context_storage` sample to demonstrate the use of custom context storage implementation.
-   Function to re-attach radio irq callbacks if changed by the user during direct radio access.

## [1.3.0] - 2023-07-05

### Changed

-   Update lr11xx driver to v1.5.1 and update all lr11xx DTS bindings accordingly.

### Fixed

-   Bad implementation of smtc_modem_hal_\*\_radio_tcxo functions.
-   The implementation of smtc_modem_hal_radio_irq_clear_pending based on our understanding of the porting guide.
-   `smtc_app` using the printer functions when printers are disabled. Empty strings are now printed instead.

## [1.2.0] - 2023-06-21

### Added

-   Correct LF-TX path configuration based on DTS.

## [1.1.1] - 2023-06-20

### Fixed

-   `smtc_modem_hal_print_trace_array_*` functions incorrectly trimming immutable strings and causing a hard fault.

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

[Unreleased]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.4.2...HEAD

[1.4.2]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.4.1...v1.4.2

[1.4.1]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.4.0...v1.4.1

[1.4.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.3.0...v1.4.0

[1.3.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.2.0...v1.3.0

[1.2.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.1.1...v1.2.0

[1.1.1]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.1.0...v1.1.1

[1.1.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.0.1...v1.1.0

[1.0.1]: https://github.com/IRNAS/SWL2001-Zephyr/compare/v1.0.0...v1.0.1

[1.0.0]: https://github.com/IRNAS/SWL2001-Zephyr/compare/52a1e1e0301ef9fc9b7c1418cee0aed9ef185e0d...v1.0.0
