set(TARGET_NAME Bee_Logger)

add_executable(${TARGET_NAME}
        bee_logger.c
        mqtt_client.c
        temperature_sensors.cpp
        humidity_temp_sensors.cpp
        weight_sensor.c
        one_wire.cpp
        HTU21D.cpp
        )

target_include_directories(${TARGET_NAME} PUBLIC
        ${PORT_DIR}
        ../../libraries/CMSIS-Driver/Config
        )

target_link_libraries(${TARGET_NAME} PRIVATE
        pico_stdlib
        cmsis_core
        CMSIS_FREERTOS_FILES
        hardware_i2c
        hardware_spi
        hardware_adc
        hardware_dma
        FREERTOS_FILES
        TIMER_FILES
        WIZFI360_FILES
        IOT_SOCKET_FILES
        )

pico_enable_stdio_usb(${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)

pico_add_extra_outputs(${TARGET_NAME})
