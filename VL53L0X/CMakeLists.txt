cmake_minimum_required(VERSION 3.18)
project(test_project C CXX ASM)

project(vl53l0x)

add_library(vl53l0x INTERFACE)

target_include_directories(vl53l0x INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(vl53l0x INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/vl53l0x.c
	${CMAKE_CURRENT_LIST_DIR}/tof.c
)

target_link_libraries(vl53l0x INTERFACE
	hardware_i2c
        pico_multicore
)


