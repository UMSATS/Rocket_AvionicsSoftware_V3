#ifndef AVIONICS_USERCONFIG_H
#define AVIONICS_USERCONFIG_H

#define userconf_EVENT_DETECTION_AVERAGING_SUPPORT_ON       1

// Simulation
#define userconf_FREE_RTOS_SIMULATOR_MODE_ON                0

#if (userconf_FREE_RTOS_SIMULATOR_MODE_ON == 1)
    #define userconf_FLASH_DISK_SIMULATION_ON               1
    #define userconf_USE_COTS_DATA                          1
#else

#endif

//Software Unit Tests
#define pressTemp_SW_UNIT_TEST                              1
#define imu_SW_UNIT_TEST                                    0
#define flash_SW_UNIT_TEST                                  0

#if (pressTemp_SW_UNIT_TEST || imu_SW_UNIT_TEST || flash_SW_UNIT_TEST)
    #define SW_UNIT_TEST_MODE_ON                            1
#else
    #define SW_UNIT_TEST_MODE_ON                            0
#endif

#endif //AVIONICS_USERCONFIG_H
