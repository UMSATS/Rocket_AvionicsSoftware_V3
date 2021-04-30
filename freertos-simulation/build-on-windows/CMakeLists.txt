CMAKE_MINIMUM_REQUIRED(VERSION 3.5.1)

FIND_PACKAGE(Python 3.0 REQUIRED)
EXECUTE_PROCESS(COMMAND python ${CMAKE_SOURCE_DIR}/generator.py WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(EXISTS "${CMAKE_SOURCE_DIR}/generated/CMakeConfigFile.cmake")
    INCLUDE(${CMAKE_SOURCE_DIR}/generated/CMakeConfigFile.cmake)
else()
    MESSAGE(FATAL_ERROR "Cannot generate project: please, first run \"python generator.py\"")
endif()


OPTION(BUILD_FOR_SIM_ON_WIN_OS "Indicates whether the build needs to be built against WinOS or RTOS" OFF)

ADD_DEFINITIONS(-DPRINT_DEBUG_LOG)

IF(BUILD_FOR_SIM_ON_WIN_OS)
    MESSAGE(STATUS "Selected WinOS BUILD with MINGW build tools")

    # Add -O0 to remove optimizations when using gcc
    IF(CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_CXX_FLAGS_DEBUG       "${CMAKE_CXX_FLAGS_DEBUG} -O0" )
        SET(CMAKE_C_FLAGS_DEBUG         "${CMAKE_C_FLAGS_DEBUG} -O0"   )
    ENDIF(CMAKE_COMPILER_IS_GNUCC)

    SET(EXECUTABLE_EXTENSION            .exe                            )
    SET(CMAKE_C_COMPILER                ${ARM_TOOLS_DIR}/gcc${EXECUTABLE_EXTENSION}     )   # absolute path needs to be set accordingly
    SET(CMAKE_CXX_COMPILER              ${ARM_TOOLS_DIR}/g++${EXECUTABLE_EXTENSION}     )   # absolute path needs to be set accordingly

    INCLUDE(CMakeForceCompiler)
    SET(CMAKE_C_COMPILER_WORKS          1)
    CMAKE_FORCE_C_COMPILER  (${CMAKE_C_COMPILER}   GNU )   # absolute path needs to be set accordingly
    CMAKE_FORCE_CXX_COMPILER(${CMAKE_CXX_COMPILER} GNU )   # absolute path needs to be set accordingly

    SET(CMAKE_ASM_COMPILER              ${ARM_TOOLS_DIR}/as${EXECUTABLE_EXTENSION}       )   # absolute path needs to be set accordingly
    SET(AR                              ${ARM_TOOLS_DIR}/ar${EXECUTABLE_EXTENSION}       )   # absolute path needs to be set accordingly
    SET(OBJCOPY                         ${ARM_TOOLS_DIR}/objcopy${EXECUTABLE_EXTENSION}  )   # absolute path needs to be set accordingly
    SET(OBJDUMP                         ${ARM_TOOLS_DIR}/objdump${EXECUTABLE_EXTENSION}  )   # absolute path needs to be set accordingly
    SET(SIZE                            ${ARM_TOOLS_DIR}/size${EXECUTABLE_EXTENSION}     )   # absolute path needs to be set accordingly
    SET(GDB                             ${ARM_TOOLS_DIR}/gdb${EXECUTABLE_EXTENSION}      )   # absolute path needs to be set accordingly

    SET(CMAKE_SYSTEM_NAME               Generic)
    SET(CMAKE_SYSTEM_VERSION            1)

    PROJECT(avionics ASM C CXX RC)
    SET(CMAKE_VERBOSE_MAKEFILE ON)



    SET(CMAKE_C_FLAGS "-g -std=gnu11 -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wall -Werror -fstack-usage -fdata-sections -fdata-sections -mthreads "    )
    SET(CMAKE_CXX_FLAGS "-g -std=c++17 -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-format -Wall -Werror -fstack-usage -fdata-sections -fdata-sections -mthreads "  )

    FILE(GLOB_RECURSE SRAD_CSV_FILE_PATH "../flight-computer/sim-port/sensor-simulation/srad_flight.data.csv" )
    FILE(GLOB_RECURSE COTS_CSV_FILE_PATH "../flight-computer/sim-port/sensor-simulation/cots_flight.data.csv" )

    ADD_DEFINITIONS(-DSRAD_CSV_FILE_PATH=${SRAD_CSV_FILE_PATH})
    ADD_DEFINITIONS(-DCOTS_CSV_FILE_PATH=${COTS_CSV_FILE_PATH})
    ADD_DEFINITIONS("-D__packed=__attribute__((__packed__))")

    INCLUDE_DIRECTORIES         (../flight-computer/sim-port/os-kernel/FreeRTOS/Source/include)
    INCLUDE_DIRECTORIES         (../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/MSVC-MingW)
    INCLUDE_DIRECTORIES         (../os-kernel/FreeRTOS-Plus-CLI)

    SET(SIM_PORT_SRC        # sensor simulation
            ../flight-computer/sim-port/sensor-simulation/buzzer.c
            ../flight-computer/sim-port/sensor-simulation/imu_sensor.c
            ../flight-computer/sim-port/sensor-simulation/pressure_sensor.c
            ../flight-computer/sim-port/sensor-simulation/flash.c
            ../flight-computer/sim-port/sensor-simulation/datafeeder.cpp

            # Communication/Transmission protocols
            ../flight-computer/sim-port/transmission-protocols/SPI.c
            ../flight-computer/sim-port/transmission-protocols/UART.c
            )

    ADD_LIBRARY(RTOS_LIB
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/MSVC-MingW/port.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/portable/MemMang/heap_1.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/croutine.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/event_groups.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/list.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/queue.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/tasks.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/timers.c
            ../flight-computer/sim-port/os-kernel/FreeRTOS/Source/event_groups.c
            ../os-kernel/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c
            )

ELSE()
    MESSAGE(STATUS "Selected ARM BUILD with ARM-Build tools")

    # Add -O0 to remove optimizations when using gcc
    IF(CMAKE_COMPILER_IS_GNUCC)
        SET(CMAKE_CXX_FLAGS_DEBUG       "${CMAKE_CXX_FLAGS_DEBUG} -O0")
        SET(CMAKE_C_FLAGS_DEBUG         "${CMAKE_C_FLAGS_DEBUG} -O0")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)

    SET(EXECUTABLE_EXTENSION            .exe)
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

    SET(CMAKE_C_FLAGS                   "-g -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=gnu11 -Wno-implicit-function-declaration -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wall -Werror -specs=nano.specs -fstack-usage -fdata-sections -fdata-sections")
    SET(CMAKE_CXX_FLAGS                 "-g -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 -std=c++11 -Wno-implicit-function-declaration -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wall -Werror -specs=nano.specs -fstack-usage -fdata-sections -fdata-sections")
    set(SYS_CALLS_SRC ../sys/syscalls.c)
    SET(LINKER_SCRIPT                   ${CMAKE_HOME_DIRECTORY}/../linker-script/STM32F401RE_FLASH.ld)
    SET(CMAKE_EXE_LINKER_FLAGS          "-Wl,-gc-sections -T ${LINKER_SCRIPT}")
    SET(CMAKE_EXE_LINKER_FLAGS_INIT     "-Wmissing-prototypes -Wmissing-declarations -Wl,-gc-sections,--print-memory-usage")
    SET(CMAKE_SYSTEM_NAME               Generic)
    SET(CMAKE_SYSTEM_VERSION            1)

    PROJECT(avionics ASM C CXX RC)
    SET(CMAKE_VERBOSE_MAKEFILE ON)

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
            ../flight-computer/board/components/impl/icm20948_imu_sensor.c
            ../flight-computer/board/components/impl/pressure_sensor.c
            ../flight-computer/board/components/impl/buzzer.c
            ../flight-computer/board/components/flash.c
            )
ENDIF()


IF(BUILD_FOR_SIM_ON_WIN_OS)
ELSE() # BUILD_FOR_SIM_ON_WIN_OS
ENDIF() # BUILD_FOR_SIM_ON_WIN_OS

INCLUDE_DIRECTORIES (../flight-computer)
INCLUDE_DIRECTORIES (../configurations)
INCLUDE_DIRECTORIES (../.)

SET(USER_SRC                # Board
        ../flight-computer/board/board.c
        ../flight-computer/board/components/recovery.c
        ../flight-computer/board/interrupt_handlers.c

        # Core
        ../flight-computer/core/flight_controller.c
        ../flight-computer/core/system_configuration.c

        # Memory Management
        ../flight-computer/memory-management/memory_manager.c

        # Event Detection
        ../flight-computer/event-detection/event_detector.c

        # CLI
        ../flight-computer/command-line-interface/controller.c
        ../flight-computer/command-line-interface/tools/read.c
        ../flight-computer/command-line-interface/tools/configure.c
        ../flight-computer/command-line-interface/tools/mem.c
        ../flight-computer/command-line-interface/tools/sysctl.c

        ../flight-computer/core/system_configuration.c)




IF(BUILD_FOR_SIM_ON_WIN_OS)
    SET(EXECUTABLE_EXTENSION exe)
    ADD_EXECUTABLE(${PROJECT_NAME}.${EXECUTABLE_EXTENSION} ../flight-computer/main.c ${USER_SRC} ${SIM_PORT_SRC})
    TARGET_LINK_LIBRARIES(RTOS_LIB pthread winmm ws2_32)
ELSE()
    SET(EXECUTABLE_EXTENSION elf)
    ADD_EXECUTABLE(${PROJECT_NAME}.${EXECUTABLE_EXTENSION} ../flight-computer/main.c ${USER_SRC} ${HAL_SRC} ${BOSCH_API_SRC} ${SYS_CALLS_SRC} ${IMPL_FOLDERS_SRC} ${LINKER_SCRIPT})
    TARGET_LINK_LIBRARIES(${PROJECT_NAME}.${EXECUTABLE_EXTENSION} CMSIS_LIB -lm)
ENDIF()

TARGET_LINK_LIBRARIES(${PROJECT_NAME}.${EXECUTABLE_EXTENSION} RTOS_LIB)

SET(HEX_FILE ${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.hex)
SET(BIN_FILE ${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.bin)

SET_TARGET_PROPERTIES(${CMAKE_PROJECT_NAME}.${EXECUTABLE_EXTENSION} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME}.${EXECUTABLE_EXTENSION} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -Oihex $<TARGET_FILE:${PROJECT_NAME}.${EXECUTABLE_EXTENSION}> ${HEX_FILE}
        COMMAND ${CMAKE_OBJCOPY} -Obinary $<TARGET_FILE:${PROJECT_NAME}.${EXECUTABLE_EXTENSION}> ${BIN_FILE}
        COMMENT "Building ${HEX_FILE} \nBuilding ${BIN_FILE}")


IF(NOT BUILD_FOR_SIM_ON_WIN_OS)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${PROJECT_SOURCE_DIR}/cmake-build-debug/${PROJECT_NAME}.map")

    ADD_CUSTOM_TARGET(upload
            ${GDB} -iex "target remote tcp:localhost:3333"
            -iex "monitor program $<TARGET_FILE:${PROJECT_NAME}.${EXECUTABLE_EXTENSION}>"
            -iex "monitor reset init"
            -iex "disconnect" -iex "quit")
ENDIF()