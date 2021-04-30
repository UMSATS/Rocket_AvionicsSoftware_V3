#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


#ifndef AVIONICS_SYSTEMCTL_H
#define AVIONICS_SYSTEMCTL_H

bool cli_tools_sysctl ( char * pcWriteBuffer, size_t xWriteBufferLen, const char * cmd_option, const char * str_option_arg );

#endif //AVIONICS_SYSTEMCTL_H
