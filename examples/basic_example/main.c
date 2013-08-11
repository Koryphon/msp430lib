
#include <stdint.h>

#include <msp430_xc.h>
#include <clock_sys.h>
#include <event_queue.h>
#include <button.h>
#include <timer.h>

timer_t Timer1;
timer_t Timer2;
timer_t Timer3;

void OnTimerExpire1(void *data){
    P2OUT ^= BIT2;
}

void OnTimerExpire2(void *data){
    P2OUT ^= BIT1;
}

void OnTimerExpire3(void *data){
    P5OUT ^= BIT1;
}


int main(void){
    WDTCTL = WDTPW + WDTHOLD; // Stop the Watchdog Timer
    __disable_interrupt(); // Disable Interrupts
    
    // Setup LEDs
    P2OUT &= ~(BIT1 | BIT2);
    P2DIR |= BIT1 | BIT2;
    
    P5OUT &= ~(BIT1);
    P5DIR |= BIT1;
    
    clock_init();
    event_init();
    button_init();
    timer_init();
    button_SetupPort(BIT0 | BIT1, BIT0 | BIT1, 1);
    
    struct timerctl timer_settings;
    
    timer_settings.interval_ms = 400;
    timer_settings.repeat = true;
    timer_settings.fptr = OnTimerExpire1;
    timer_settings.ev_data = NULL;
    timer_start(&Timer1,&timer_settings);
    
    timer_settings.interval_ms = 500;
    timer_settings.repeat = true;
    timer_settings.fptr = OnTimerExpire2;
    timer_settings.ev_data = NULL;
    timer_start(&Timer2,&timer_settings);
    
    timer_settings.interval_ms = 2000;
    timer_settings.repeat = true;
    timer_settings.fptr = OnTimerExpire3;
    timer_settings.ev_data = NULL;
    timer_start(&Timer3,&timer_settings);
    
    __enable_interrupt();
    event_StartHandler();
    
    return(0);
}

void onIdle(void){
    
}

void onButtonDown(uint8_t port, uint8_t b){
    if(b & BIT0){
        // Right button
        timer_start(&Timer1,NULL);
    }else if(b & BIT1){
        // Left button
        timer_stop(&Timer1);
    }
}

void onButtonUp(uint8_t port, uint8_t b){
    
}

void onButtonHold(uint8_t port, uint8_t b){
    
}
