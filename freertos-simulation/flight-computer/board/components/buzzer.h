#ifndef BUZZER_H
#define BUZZER_H

#define FREQ 4000 //in hertz
#define COUNTER (84000000 * ((1/FREQ)/2)) //84000000 = clock frequency in hz
#define SECOND 4
//#define PIN1 GPIO_PIN_4
//#define PIN2 GPIO_PIN_5


void buzzer_init ( void );

void buzz_delay ( int milliseconds );


#endif // BUZZER_H
