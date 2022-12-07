# Almanac update

This sample is based on `SWSD001/apps/examples/almanac_update`.

All configuration of the sample is done at the top of `main.c`, where LoRaWAN keys and other settings can be set.

## Sample behaviour

The almanac update service is enabled by sending an almanac status to LoRa Cloud via a DM message. Based on this status, LoRa Cloud will determine if the almanac is up-to-date and, if not, perform an incremental update by sending a downlink. It is possible that several incremental updates are needed to perform a full update.

There are 2 ways to trigger an incremental update:

- Periodic DM message configured with `smtc_modem_dm_set_info_fields()` and `smtc_modem_dm_set_info_interval()` including the almanac status among other fields
- Single DM message requested with `smtc_modem_dm_request_single_uplink()` and including the almanac status among other fields

In this example, only `SMTC_MODEM_DM_FIELD_ALMANAC_STATUS` field is sent through DM messages for clarity. It is possible to reach the same beahvior with additional fields sent at the same time.

This example speeds up the process by requesting a DM message each time a partial almanac update is done (thanks to `SMTC_MODEM_EVENT_ALMANAC_UPDATE` event) instead of relying on the periodic DM report. This possibility is added for demonstration purposes.

Note that this behaviour is expected in the case where the almanac of the device are outdated.
You may consider using the *almanac_update* from `SWDR001-ZEPHYR` sample to load an outdated almanac into the device.
