# based on makefiles from Semtech

zephyr_library()

#  ----- lr11xx.mk -----

zephyr_library_compile_definitions(LR11XX)
zephyr_library_compile_definitions(LR11XX_TRANSCEIVER)

# sources
zephyr_library_sources(
    smtc/smtc_modem_core/smtc_ral/src/ral_lr11xx.c
    smtc/smtc_modem_core/smtc_ralf/src/ralf_lr11xx.c

	smtc/smtc_modem_core/smtc_modem_crypto/smtc_modem_crypto.c
)

# USE_GNSS
zephyr_library_sources(smtc/smtc_modem_core/smtc_modem_services/src/almanac_update/almanac_update.c)
zephyr_library_compile_definitions(ENABLE_MODEM_GNSS_FEATURE)

# CRYPTO = SOFT
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_SOFT
    smtc/smtc_modem_core/smtc_modem_crypto/soft_secure_element/aes.c
    smtc/smtc_modem_core/smtc_modem_crypto/soft_secure_element/cmac.c
    smtc/smtc_modem_core/smtc_modem_crypto/soft_secure_element/soft_se.c
)
zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_SOFT smtc_modem_core/smtc_modem_crypto/soft_secure_element)

# CRYPTO = LR11XX
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX
    smtc/smtc_modem_core/smtc_modem_crypto/lr11xx_crypto_engine/lr11xx_ce.c
)
zephyr_library_include_directories_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX smtc/smtc_modem_core/smtc_modem_crypto/lr11xx_crypto_engine)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_CRYPTOGRAPHY_LR11XX USE_LR11XX_CE)
# NOTE: USE_PRE_PROVISIONED_FEATURES must not be enabled,
# since factory provisioned keys of lr11xx will be used instead
# of what is set in the application.
# zephyr_library_compile_definitions(USE_PRE_PROVISIONED_FEATURES)

#  ----- regions.mk -----

# Regional parameters version
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_RP2_101 RP2_101)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_RP2_103 RP2_103)

# enabled regions

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_AS_923 REGION_AS_923)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_AU_915 REGION_AU_915)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_CN_470 REGION_CN_470)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_CN_470_RP_1_0 REGION_CN_470_RP_1_0)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_EU_868 REGION_EU_868)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_IN_865 REGION_IN_865)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_KR_920 REGION_KR_920)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_RU_864 REGION_RU_864)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_US_915 REGION_US_915)

zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_WW_2G4 REGION_WW2G4)  # the underscore missing here is intentional!
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_WW_2G4 WW2G4_SINGLE_DATARATE)

zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_AS_923 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_as_923.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_AU_915 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_au_915.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_CN_470 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_CN_470_RP_1_0 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_cn_470_rp_1_0.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_EU_868 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_eu_868.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_IN_865 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_in_865.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_KR_920 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_kr_920.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_RU_864 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_ru_864.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_US_915 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_us_915.c)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_REGION_WW_2G4 smtc/smtc_modem_core/lr1mac/src/smtc_real/src/region_ww2g4.c)

#  ----- common.mk -----

# Some defines from common.mk that we probably don't need (they conditionally add some files and some defines)
# MODEM_TRACE, MODEM_DEEP_TRACE, PERF_TEST, MIDDLEWARE

# these sources are always included
zephyr_library_sources(
    smtc/smtc_modem_core/device_management/dm_downlink.c
    smtc/smtc_modem_core/device_management/modem_context.c

    smtc/smtc_modem_core/lorawan_api/lorawan_api.c

	smtc/smtc_modem_core/lr1mac/src/lr1_stack_mac_layer.c
	smtc/smtc_modem_core/lr1mac/src/lr1mac_core.c
	smtc/smtc_modem_core/lr1mac/src/lr1mac_utilities.c
	smtc/smtc_modem_core/lr1mac/src/smtc_real/src/smtc_real.c
	smtc/smtc_modem_core/lr1mac/src/services/smtc_duty_cycle.c
	smtc/smtc_modem_core/lr1mac/src/services/smtc_lbt.c
	smtc/smtc_modem_core/lr1mac/src/lr1mac_class_c/lr1mac_class_c.c
	smtc/smtc_modem_core/lr1mac/src/lr1mac_class_b/smtc_beacon_sniff.c
	smtc/smtc_modem_core/lr1mac/src/lr1mac_class_b/smtc_ping_slot.c

    smtc/smtc_modem_core/modem_core/smtc_modem.c
    smtc/smtc_modem_core/modem_core/smtc_modem_test.c

    smtc/smtc_modem_core/modem_services/fifo_ctrl.c
    smtc/smtc_modem_core/modem_services/modem_utilities.c
    smtc/smtc_modem_core/modem_services/smtc_modem_services_hal.c
    smtc/smtc_modem_core/modem_services/lorawan_certification.c

    smtc/smtc_modem_core/modem_supervisor/modem_supervisor.c

    smtc/smtc_modem_core/smtc_modem_crypto/smtc_modem_crypto.c

    smtc/smtc_modem_core/radio_planner/src/radio_planner.c
    smtc/smtc_modem_core/radio_planner/src/radio_planner_hal.c
)

# private includes

zephyr_library_include_directories(
    smtc/smtc_modem_api
    smtc/smtc_modem_core
    # we don't include modem_config since we provide our own implementation
    # smtc/smtc_modem_core/modem_config
    smtc/smtc_modem_core/modem_core
    smtc/smtc_modem_core/modem_supervisor
    smtc/smtc_modem_core/device_management
    smtc/smtc_modem_core/modem_services
    smtc/smtc_modem_core/lorawan_api
    smtc/smtc_modem_core/smtc_modem_services/headers
    smtc/smtc_modem_core/smtc_modem_services/src
    smtc/smtc_modem_core/smtc_modem_services/src/stream
    smtc/smtc_modem_core/smtc_modem_services/src/file_upload
    smtc/smtc_modem_core/smtc_modem_services/src/alc_sync
    smtc/smtc_modem_core/smtc_modem_services
    smtc/smtc_modem_core/smtc_ral/src
    smtc/smtc_modem_core/smtc_ralf/src
    smtc/smtc_modem_core/lr1mac
    smtc/smtc_modem_core/lr1mac/src
    smtc/smtc_modem_core/lr1mac/src/services
    smtc/smtc_modem_core/lr1mac/src/lr1mac_class_c
    smtc/smtc_modem_core/lr1mac/src/lr1mac_class_b
    smtc/smtc_modem_core/radio_planner/src
    smtc/smtc_modem_core/smtc_modem_crypto
    smtc/smtc_modem_core/smtc_modem_crypto/smtc_secure_element
    smtc/smtc_modem_core/lorawan_api
    smtc/smtc_modem_core/lr1mac/src/smtc_real/src
    smtc/smtc_modem_hal
    smtc/
)

# ADD_D2D
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_D2D SMTC_D2D)
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_D2D ADD_D2D)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_D2D smtc/smtc_modem_core/lr1mac/src/lr1mac_class_b/smtc_d2d.c)

# ADD_MULTICAST
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_MULTICAST SMTC_MULTICAST)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_MULTICAST smtc/smtc_modem_core/lr1mac/src/services/smtc_multicast.c)

# ADD_SMTC_STREAM
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_STREAM ADD_SMTC_STREAM)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_STREAM
    smtc/smtc_modem_core/smtc_modem_services/src/stream/stream.c
    smtc/smtc_modem_core/smtc_modem_services/src/stream/rose.c
)

# ADD_SMTC_FILE_UPLOAD
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_FILE_UPLOAD ADD_SMTC_FILE_UPLOAD)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_FILE_UPLOAD smtc/smtc_modem_core/smtc_modem_services/src/file_upload/file_upload.c)

# ADD_SMTC_ALC_SYNC
zephyr_library_compile_definitions_ifdef(CONFIG_LORA_BASICS_MODEM_TIME_SYNC ADD_SMTC_ALC_SYNC)
zephyr_library_sources_ifdef(CONFIG_LORA_BASICS_MODEM_TIME_SYNC
    smtc/smtc_modem_core/modem_services/smtc_clock_sync.c
    smtc/smtc_modem_core/smtc_modem_services/src/alc_sync/alc_sync.c
)

# ----- public includes (visible outside of the library) -----
zephyr_include_directories(
    smtc/smtc_modem_api

    # this is required since smtc_modem_utilities.h includes ralf.h
    # also cause ralf_lr11xx or some other ralf_ has to be included from the app.
    smtc/smtc_modem_core/smtc_ralf/src
    smtc/smtc_modem_core/smtc_ral/src

    smtc/smtc_modem_hal
)

#  ----- added by us -----

# Disable all warnings for Semtech code.
#
# Zephyr is compiled with a lot more warnings enabled than the basics modem.
# Since we copy the Basics Modem directly with no modifications, the warnings clog up
# the build output in our applications.
#
# To disable the warnings from the Basics Modem we need to pass "-w" to the compiler, this is done with below line.
zephyr_library_compile_options(-w)

# start new zephyr library for our code (warnings are enabled again for our hal-impl code)
zephyr_library_named(smtc_hal)

# hal implementation
zephyr_library_sources(smtc_modem_hal_impl/smtc_modem_hal.c)
# hal init
zephyr_include_directories(smtc_modem_hal_impl)

# radio abstraction implementation
zephyr_library_sources(ral_lr11xx_bsp_impl/ral_lr11xx_bsp.c)
zephyr_library_include_directories(ral_lr11xx_bsp_impl)

# custom logging to hook into zephyr LOG correctly
zephyr_library_sources(smtc_modem_hal_impl/logging/smtc_modem_hal_additional_prints.c)
zephyr_include_directories(smtc_modem_hal_impl/logging)

# common layer
add_subdirectory(apps_common)
