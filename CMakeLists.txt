cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(test_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(theremin 
            theremin.c
)

pico_enable_stdio_usb(theremin 1)
#pico_enable_stdio_uart(theremin 1)

pico_add_extra_outputs(theremin)
target_include_directories(theremin PRIVATE ${CMAKE_CURRENT_LIST_DIR} )
target_link_libraries(theremin 
                pico_stdlib
		pico_multicore
		hardware_spi
        	hardware_adc
        	hardware_pwm
		sound
		vl53l0x
		display
)

add_subdirectory(sound)
add_subdirectory(VL53L0X)
add_subdirectory(display)

#auto reboot/program
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_BAUD_RATE=1)
add_compile_definitions(PICO_STDIO_USB_ENABLE_RESET_VIA_VENDOR_INTERFACE=0)

