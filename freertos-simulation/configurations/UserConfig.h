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


#endif //AVIONICS_USERCONFIG_H
