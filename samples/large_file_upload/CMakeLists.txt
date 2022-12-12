# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

# create compile_commands.json for clang
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
project(sample_lorawan)

zephyr_compile_options(-fdiagnostics-color=always)

target_sources(app PRIVATE src/main.c)

target_include_directories(app PRIVATE src)