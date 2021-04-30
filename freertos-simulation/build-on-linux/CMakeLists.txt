cmake_minimum_required(VERSION 3.5.1)

ADD_DEFINITIONS(-DPRINT_DEBUG_LOG)

execute_process(COMMAND python3 ${CMAKE_SOURCE_DIR}/generator.py WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})
if(EXISTS "${CMAKE_SOURCE_DIR}/generated/CMakeConfigFile.cmake")
    include(${CMAKE_SOURCE_DIR}/generated/CMakeConfigFile.cmake)
    message(STATUS ARM_TOOLS_DIR=${ARM_TOOLS_DIR})
    message(STATUS EABI_PREFIX=${EABI_PREFIX})

else()
    message(FATAL_ERROR "Cannot generate project: please, first run \"python process-user-configurations.py\"")
endif()

OPTION(BUILD_FOR_SIM_ON_UNIX_OS "Indicates whether the build needs to be built against WinOS or RTOS" ON)

IF(BUILD_FOR_SIM_ON_UNIX_OS)

    MESSAGE(STATUS "Selected WinOS BUILD with MINGW build tools")

    PROJECT(avionics ASM C CXX)
    MESSAGE(STATUS "Selected ARM BUILD with ARM-Build tools")

    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_VERBOSE_MAKEFILE ON)

    # Add -O0 to remove optimizations when using gcc
    IF(CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_CXX_FLAGS_DEBUG       "${CMAKE_CXX_FLAGS_DEBUG} -O0" )
        SET(CMAKE_C_FLAGS_DEBUG         "${CMAKE_C_FLAGS_DEBUG} -O0"   )
    ENDIF(CMAKE_COMPILER_IS_GNUCC)

    set (CMAKE_IGNORE_PATH  ${CMAKE_SOURCE_DIR}/../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/MSVC-MingW)

    SET(CMAKE_C_FLAGS "-g -std=gnu11 -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wall -Werror -fstack-usage -fdata-sections -fdata-sections  "   )
    SET(CMAKE_CXX_FLAGS "-g -std=c++17 -Wno-unused-function-Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wall -Werror -fstack-usage -fdata-sections -fdata-sections "  )

    FILE(GLOB_RECURSE SRAD_CSV_FILE_PATH "../flight-computer/sim-port/sensor-simulation/srad_flight.data.csv" )
    FILE(GLOB_RECURSE COTS_CSV_FILE_PATH "../flight-computer/sim-port/sensor-simulation/cots_flight.data.csv" )

    get_filename_component(SRAD_CSV_FILE_ABS_PATH "${SRAD_CSV_FILE_PATH}" ABSOLUTE)
    get_filename_component(COTS_CSV_FILE_ABS_PATH "${COTS_CSV_FILE_PATH}" ABSOLUTE)


    ADD_DEFINITIONS(-DSRAD_CSV_FILE_PATH=${SRAD_CSV_FILE_ABS_PATH})
    ADD_DEFINITIONS(-DCOTS_CSV_FILE_PATH=${COTS_CSV_FILE_ABS_PATH})

    INCLUDE_DIRECTORIES         (../flight-computer/sim-port/os-kernel/FreeRTOS/Source/include)
    INCLUDE_DIRECTORIES         (../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/POSIX)
    INCLUDE_DIRECTORIES         (../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/POSIX/utils)

    INCLUDE_DIRECTORIES         (../os-kernel/FreeRTOS-Plus-CLI)

    SET(SIM_PORT_SRC        # sensor simulation
            ../flight-computer/sim-port/sensor-simulation/buzzer.c
            ../flight-computer/sim-port/sensor-simulation/imu_sensor.c
            ../flight-computer/sim-port/sensor-simulation/pressure_sensor.c
            ../flight-computer/sim-port/sensor-simulation/datafeeder.cpp

            # Communication/Transmission protocols
            ../flight-computer/sim-port/transmission-protocols/SPI.c
            ../flight-computer/sim-port/transmission-protocols/UART.c
            )

    ADD_LIBRARY(RTOS_LIB
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/POSIX/port.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/MemMang/heap_1.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/croutine.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/event_groups.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/list.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/queue.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/tasks.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/timers.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/event_groups.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/POSIX/utils/wait_for_event.c
            ../os-kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c
            )



ELSE()
    SET(CMAKE_C_COMPILER                ${EABI_PREFIX}-gcc${EXECUTABLE_EXTENSION}      )   # absolute path needs to be set accordingly
    SET(CMAKE_CXX_COMPILER              ${EABI_PREFIX}-g++${EXECUTABLE_EXTENSION}      )   # absolute path needs to be set accordingly

    INCLUDE(CMakeForceCompiler)
    SET(CMAKE_C_COMPILER_WORKS          1)
    CMAKE_FORCE_C_COMPILER  (${ARM_TOOLS_DIR}/${CMAKE_C_COMPILER}   GNU                )   # absolute path needs to be set accordingly
    CMAKE_FORCE_CXX_COMPILER(${ARM_TOOLS_DIR}/${CMAKE_CXX_COMPILER} GNU                )   # absolute path needs to be set accordingly

    SET(AS                              ${EABI_PREFIX}-gcc-as${EXECUTABLE_EXTENSION}   )   # absolute path needs to be set accordingly
    SET(AR                              ${EABI_PREFIX}-gcc-ar${EXECUTABLE_EXTENSION}   )   # absolute path needs to be set accordingly
    SET(OBJCOPY                         ${EABI_PREFIX}-objcopy${EXECUTABLE_EXTENSION}  )   # absolute path needs to be set accordingly
    SET(OBJDUMP                         ${EABI_PREFIX}-objdump${EXECUTABLE_EXTENSION}  )   # absolute path needs to be set accordingly
    SET(SIZE                            ${EABI_PREFIX}-size${EXECUTABLE_EXTENSION}     )   # absolute path needs to be set accordingly
    SET(GDB                             ${EABI_PREFIX}-gdb${EXECUTABLE_EXTENSION}      )   # absolute path needs to be set accordingly

    PROJECT(avionics ASM C CXX)
    MESSAGE(STATUS "Selected ARM BUILD with ARM-Build tools")

    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_VERSION 1)
    set(CMAKE_VERBOSE_MAKEFILE ON)



    # Add -O0 to remove optimizations when using gcc
    IF(CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_CXX_FLAGS_DEBUG       "${CMAKE_CXX_FLAGS_DEBUG} -O0")
        SET(CMAKE_C_FLAGS_DEBUG         "${CMAKE_C_FLAGS_DEBUG} -O0")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)


    SET(CMAKE_C_FLAGS                   "-g -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11 -specs=nano.specs -Wno-implicit-function-declaration -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wall -Werror -fstack-usage -fdata-sections -fdata-sections")
    SET(CMAKE_CXX_FLAGS                 "-g -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c++11 -specs=nano.specs -Wno-implicit-function-declaration -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wall -Werror -fstack-usage -fdata-sections -fdata-sections")
    set(SYS_CALLS_SRC                   ../sys/syscalls.c)
    SET(LINKER_SCRIPT                   ${CMAKE_HOME_DIRECTORY}/../linker-script/STM32F401RE_FLASH.ld)
    SET(CMAKE_EXE_LINKER_FLAGS          "-Wl,-gc-sections -T ${LINKER_SCRIPT}")
    SET(CMAKE_EXE_LINKER_FLAGS_INIT     "-Wmissing-prototypes -Wmissing-declarations -Wl,-gc-sections,--print-memory-usage")
    SET(CMAKE_SYSTEM_NAME               Generic)
    SET(CMAKE_SYSTEM_VERSION            1)



    ADD_DEFINITIONS(-DSTM32F401xE)
    ADD_DEFINITIONS(-DUSE_HAL_DRIVER)
    ADD_DEFINITIONS("-D__weak=__attribute__((weak))")
    ADD_DEFINITIONS("-D__packed=__attribute__((__packed__))")


    INCLUDE_DIRECTORIES (../board-hardware-drivers/BMI08x-Sensor-API)
    INCLUDE_DIRECTORIES (../board-hardware-drivers/BMI08x-Sensor-API/Inc)
    INCLUDE_DIRECTORIES (../board-hardware-drivers/CMSIS/Include)
    INCLUDE_DIRECTORIES (../board-hardware-drivers/STM32F4xx_HAL_Driver/Inc)
    INCLUDE_DIRECTORIES (../board-hardware-drivers/STM32F4xx_HAL_Driver/Inc/Custom)
    INCLUDE_DIRECTORIES (../board-hardware-drivers/CMSIS/Device/ST/STM32F4xx/Include)
    INCLUDE_DIRECTORIES (../os-kernel/FreeRTOS/Source/portable/GCC/ARM_CM4F)
    INCLUDE_DIRECTORIES (../os-kernel/FreeRTOS/Source/CMSIS_RTOS)
    INCLUDE_DIRECTORIES (../os-kernel/FreeRTOS/Source/include)
    INCLUDE_DIRECTORIES (../os-kernel/FreeRTOS-Plus-CLI)

    ADD_LIBRARY(CMSIS_LIB
            ../board-hardware-drivers/CMSIS/Device/ST/STM32F4xx/Source/system_stm32f4xx.c
            ../Startup/startup_stm32f401xe.s
            )

    FILE(GLOB_RECURSE HAL_SRC "../board-hardware-drivers/STM32F4xx_HAL_Driver/Src/*.c")
    FILE(GLOB_RECURSE BOSCH_API_SRC "../board-hardware-drivers/BMI08x-Sensor-API/Src/*.c")

    ADD_LIBRARY(RTOS_LIB
            ../os-kernel/FreeRTOS/Source/portable/GCC/ARM_CM4F/port.c
            ../os-kernel/FreeRTOS/Source/portable/MemMang/heap_1.c
            ../os-kernel/FreeRTOS/Source/croutine.c
            ../os-kernel/FreeRTOS/Source/event_groups.c
            ../os-kernel/FreeRTOS/Source/list.c
            ../os-kernel/FreeRTOS/Source/queue.c
            ../os-kernel/FreeRTOS/Source/tasks.c
            ../os-kernel/FreeRTOS/Source/timers.c
            ../os-kernel/FreeRTOS/Source/event_groups.c
            ../os-kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c
            )

    SET(IMPL_FOLDERS_SRC  # Communication/Transmission protocols
            ../flight-computer/protocols/impl/UART.c
            ../flight-computer/protocols/impl/SPI.c

            # sensors
            ../flight-computer/board/components/impl/buzzer.c
            ../flight-computer/board/components/impl/imu_sensor.c
            ../flight-computer/board/components/impl/pressure_sensor.c
            )
ENDIF()

INCLUDE_DIRECTORIES (../flight-computer)
INCLUDE_DIRECTORIES (../configurations)
INCLUDE_DIRECTORIES (${CMAKE_SOURCE_DIR}/../)


SET(USER_SRC                # Board
        ../flight-computer/board/board.c
        ../flight-computer/board/components/flash.c
        ../flight-computer/board/components/recovery.c
        ../flight-computer/board/interrupt_handlers.c

        # Core
        ../flight-computer/core/flight_controller.c

        # Memory Management
        ../flight-computer/memory-management/memory_manager.c
        ../flight-computer/memory-management/queue.c

        # Event Detection
        ../flight-computer/event-detection/event_detector.c

        # CLI
        ../flight-computer/command-line-interface/controller.c
        ../flight-computer/command-line-interface/tools/read.c
        ../flight-computer/command-line-interface/tools/configure.c
        ../flight-computer/command-line-interface/tools/mem.c
        ../flight-computer/command-line-interface/tools/sysctl.c
        )




IF(BUILD_FOR_SIM_ON_UNIX_OS)
    ADD_EXECUTABLE(${PROJECT_NAME}.elf ../flight-computer/main.c ${USER_SRC} ${SIM_PORT_SRC})
    TARGET_LINK_LIBRARIES(RTOS_LIB pthread)
ELSE()
    ADD_EXECUTABLE(${PROJECT_NAME}.elf ../flight-computer/main.c ${USER_SRC} ${HAL_SRC} ${BOSCH_API_SRC} ${SYS_CALLS_SRC} ${IMPL_FOLDERS_SRC} ${LINKER_SCRIPT})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}.elf CMSIS_LIB -lm)
ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME}.elf RTOS_LIB)

SET(HEX_FILE ${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.hex)
SET(BIN_FILE ${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.bin)

SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME}.elf PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME}.elf POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.elf> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")


IF(NOT BUILD_FOR_SIM_ON_UNIX_OS)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.map")

    ADD_CUSTOM_TARGET(upload
            ${GDB} -iex "target remote tcp:localhost:3333"
            -iex "monitor program $<TARGET_FILE:${PROJECT_NAME}.elf>"
            -iex "monitor reset init"
            -iex "disconnect" -iex "quit")
ENDIF()
