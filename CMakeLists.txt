cmake_minimum_required(VERSION 3.15)
project(stm32_bootloader C)


set(CMAKE_C_STANDARD 11)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/linker/STM32F411RE.ld)

add_executable(bootloader.elf
    src/startup.c
    src/main.c
    src/flash.c
    src/uart.c
    src/crc32.c
)

target_include_directories(bootloader.elf PRIVATE include)

target_compile_options(bootloader.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -Os
    -Wall
    -Wextra
    -fdata-sections
    -ffunction-sections
    -ffreestanding
)

target_link_options(bootloader.elf PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard
    -T${LINKER_SCRIPT}
    -Wl,--gc-sections
    -Wl,-Map=bootloader.map
    -nostdlib
    -lc
    -lgcc
)

add_custom_command(TARGET bootloader.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary bootloader.elf bootloader.bin
    COMMAND arm-none-eabi-size bootloader.elf
    COMMENT "Generating bootloader.bin"
)
