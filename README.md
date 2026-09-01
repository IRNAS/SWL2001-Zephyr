# SWL2001-Zephyr

LoRa Basics Modem port for Zephyr.

## Project development setup

To work on this repository directly, first install
[west](https://developer.nordicsemi.com/nRF_Connect_SDK/doc/latest/nrf/gs_installing.html#install-west)
and [east](https://github.com/IRNAS/irnas-east-software). Then initialize a workspace and install
the toolchain used by the project:

```shell
east init -m https://github.com/IRNAS/SWL2001-Zephyr SWL2001-Zephyr
cd SWL2001-Zephyr/project

# Set up east globally (this only needs to be done once on each machine)
east install nrfutil-toolchain-manager
# Install the toolchain for the NCS version used by this project
east install toolchain

# Run `west update` via east to fetch the workspace modules
east update
```

## Adding to your existing project as a module

1. Update your `west.yml`. First in the `remotes` section add:

   ```yaml
    - name: irnas
      url-base: https://github.com/irnas
   ```

2. Then in the `projects` section add at the bottom:

    ```yaml
    - name: SWL2001-Zephyr
      repo-path: SWL2001-Zephyr
      path: irnas/SWL2001-Zephyr
      remote: irnas
      revision: <release-tag | branch | commit hash>
    ```

3. Then run `west update` from your project directory. `West` will download the repository and add it
  to `../irnas/SWL2001-Zephyr/`.

## Configuration

Please see `drivers/Kconfig` for all configuration options.

For basic usage, the default options should be sufficient.

By default, all regions are enabled.
If only some regions should be compiled in, set `CONFIG_LORA_BASICS_MODEM_ENABLE_ALL_REGIONS=n`
and enable only the regions needed.

The following additional features are disables by default and should be enabled only if required by
the application.

```Kconfig
LORA_BASICS_MODEM_D2D
LORA_BASICS_MODEM_FILE_UPLOAD
LORA_BASICS_MODEM_MULTICAST
LORA_BASICS_MODEM_PERSISTENT_JOIN_SESSION
LORA_BASICS_MODEM_STREAM
LORA_BASICS_MODEM_TIME_SYNC
LORA_BASICS_MODEM_USE_GNSS
```

Due to how Semtech has implemented the modem lib, you will get a runtime error when using a feature
that is not enabled (a compile-time error would be preferred, but alas).

Additional logging from the `smtc_app` module can be adjusted with the
`CONFIG_SMTC_APP_LOG_LEVEL_*` config.
The `printers` are enabled by default if either `LOG` or `PRINTK` are enabled. If they are not
needed, set `CONFIG_LORA_BASICS_MODEM_PRINTERS=n`.

## SWL2001 Development instructions

This section describes how to update this repository when Semtech updates.

### How to update when Semtech updates

copy `smtc_modem_api`, `smtc_modem_core`, `smtc_modem_hal` from SWL2001 to the `smtc` folder.
Go over the makefiles from the `makefiles` folder and compare to `smtc/CMakeLists.txt` from this repository.

Check the changelog in the `smtc_modem_core/radio_drivers/lr11xx_driver` folder for the version. That
exact version is required to be included via `west.yml`.
None of the files from `smtc_modem_core/radio_drivers` are actually included in the build. They are
there since we copy all files from Semtech's SWL2001 repository.

Check `smtc_modem_hal_impl/smtc_modem_hal.c` and implement any new hal functions (from `smtc_modem_hal.h`).

Check `ral_lr11xx_bsp_impl/ral_lr11xx_bsf.c` and implement any new ral functions (from `ral_lr11xx_bsp.h`)

### RAL implementation

Our implementation of `ral_lr11xx_bsf.c` is based on
`SWL2001/utilities/user_app/radio_hal/ral_lr11xx_bsp.c` but uses our `lr11xx` driver in the
implementation.

### smtc_app

The `apps_common` folder contains helpers and wrappers that ease sample and application development.
The code is based on `SWSD001/apps/common` and `SWSD001/lora_basics_modem/printers`.

If any further "common" functionality is discovered when using this lib in our projects, it should
be added to `apps_common/smtc_app`, so it becomes available for use in other projects.
