//
// Created by vasil on 12/11/2020.
//

#ifndef AVIONICS_INTERRUPT_HANDLERS_H
#define AVIONICS_INTERRUPT_HANDLERS_H


void NMI_Handler ( void );
void HardFault_Handler ( void );
void MemManage_Handler ( void );
void BusFault_Handler ( void );
void UsageFault_Handler ( void );
void DebugMon_Handler ( void );
void TIM1_UP_TIM10_IRQHandler ( void );


#endif //AVIONICS_INTERRUPT_HANDLERS_H
