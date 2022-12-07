zephyr_library()

zephyr_library_sources(smtc_app/smtc_app.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_PRINTERS printers/smtc_modem_api_str.c)
zephyr_include_directories(
    smtc_app
    printers
)
