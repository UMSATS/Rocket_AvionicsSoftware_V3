#ifndef COMMAND_CONTROLLER_H
#define COMMAND_CONTROLLER_H

#include "board/components/flash.h"
#include "protocols/UART.h"
#include "core/system_configuration.h"


void command_line_interface_start ( void * pvParameters );
bool command_line_interface_is_running ( );

#endif // COMMAND_CONTROLLER_H
