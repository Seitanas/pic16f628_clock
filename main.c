/*
 * File:   main.c
 * Author: Tadas
 *
 * Created on Penktadienis, 2017, rugpjūtis 4, 09.10
 */
#define _XTAL_FREQ 20000000

// CONFIG
#pragma config FOSC = HS        // Oscillator Selection bits (HS oscillator: High-speed crystal/resonator on RA6/OSC2/CLKOUT and RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is digital input, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#include <xc.h>


#define led1 RA0
#define led2 RA1
#define led3 RA2
#define led4 RA3
#define button RB7
#define mosfet_off_signal 1
#define mosfet_on_signal 0
#define setup_timeout 4 // how much inactivity will move to next setting
#define display_for 3 // for how long display time after button is pushed
#define setup_init_time 3 // for how long button must be pressed for setup to be initiated

int segment[10];
int timer = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;
int button_pressed = 0;
int setup = 0;
int timeout = 0;

void init_segments(void){
    // ABCDEFG
    // 3420156
    segment[0] = 0b01000000;   
    segment[1] = 0b01101011; 
    segment[2] = 0b00100100; 
    segment[3] = 0b00100010;
    segment[4] = 0b00001011;
    segment[5] = 0b00010010;
    segment[6] = 0b00010000;
    segment[7] = 0b01100011;
    segment[8] = 0b00000000;
    segment[9] = 0b00000010;
}
void reset_mosfets(void){
    led1 = mosfet_off_signal;
    led2 = mosfet_off_signal;
    led3 = mosfet_off_signal;
    led4 = mosfet_off_signal;
}
void led_wait(void){
    __delay_ms(1);    
}
void animation_wait(void){
    __delay_ms(30);
}
void play_setup(void){
    int cycle = 4;
    while (cycle){
        PORTB = 0b01110111;
        led1 = 0;
        animation_wait();
        reset_mosfets();
        PORTB = 0b01110111;
        led2 = 0;
        animation_wait();
        reset_mosfets(); 
        PORTB = 0b01110111;
        led3 = 0;
        animation_wait();
        reset_mosfets(); 
        PORTB = 0b01110111;
        led4 = 0;
        animation_wait();
        reset_mosfets();
    
        PORTB = 0b01101111;
        led4 = 0;
        animation_wait();
        reset_mosfets();
        PORTB = 0b01111011;
        led4 = 0;
        animation_wait();
        reset_mosfets();
    
        PORTB = 0b01111110;
        led4 = 0;
        animation_wait();
        reset_mosfets();
        PORTB = 0b01111110;
        led3 = 0;
        animation_wait();
        reset_mosfets();
        PORTB = 0b01111110;
        led2 = 0;
        animation_wait();
        reset_mosfets(); 
        PORTB = 0b01111110;
        led1 = 0;
        animation_wait();
        reset_mosfets();
    
        PORTB = 0b01111101;
        led1 = 0;
        animation_wait();
        reset_mosfets();    
        PORTB = 0b01011111;
        led1 = 0;
        animation_wait();
        reset_mosfets();
        --cycle;
    }
}
int display_till(void){
    int end_seconds = seconds + display_for;
    if (end_seconds > 59) // the sum above cant overlap 59 seconds
        end_seconds = end_seconds - 60;
    return end_seconds;
}

void display_done(void){
    int end_seconds = display_till();
    while (end_seconds != seconds){
        // ABCDEFG
        // 3420156
        PORTB = 0b00101000;
        led1 = 0;
        led_wait();
        reset_mosfets();
        PORTB = 0b00111000;
        led2 = 0;
        led_wait();
        reset_mosfets();
        PORTB = 0b00111001;
        led3 = 0;
        led_wait();
        reset_mosfets();
        PORTB = 0b00010100;
        led4 = 0;
        led_wait();
        reset_mosfets();
    }
}
void display_time(int hours, int minutes, int display){ /*
                                                         display = 0 - display hours and minutes
                                                         display = 1 - display hours
                                                         display = 2 display minutes
                                                         */
    if (!display || display == 1){
        // display most significant hour digit:
        PORTB = segment[hours/10];
        led1 = 0;
        led_wait();
        reset_mosfets();
        // display least significant hour digit:
        PORTB = segment[hours%10];
        led2 = 0;
        led_wait();
        reset_mosfets();
    }
    if (!display || display == 2){
        // display most significant minute digit:
        PORTB = segment[minutes/10];
        led3 = 0;
        led_wait();
        reset_mosfets();
        // display least significant minute digit:
        PORTB = segment[minutes%10];
        led4 = 0;
        led_wait();
        reset_mosfets();    
    }
}
void interrupt timer_overflow(){
    TMR2IF = 0; 
    ++timer;
    if (timer == 500){ /*we are getting 500hz interrupts at 20mhz crystal. 
                        *We need to run routine only every 500th interrupt to get 1s pulse
                        */
        ++seconds;
        if (seconds == 60){
            seconds = 0;
            ++minutes;
        }
        if (minutes == 60){
            minutes = 0;
            ++hours;
        }
        if (hours == 24)
            hours = 0;
        if (!button && !setup){
            ++button_pressed;          
        }
        if (setup && timeout) /* if setup is enabled, decrease timeout each second
                                for setup to jump from minutes to hours after defined
                               inactivity time*/
            --timeout;
        timer=0;
    }    
}
void time_setup(void){
    play_setup();
    timeout = setup_timeout;
    button_pressed = 0; 
    while (!button) // wait for button to be released, initiate minute config after
        display_time(hours, minutes, 2);
    while (timeout){ // run minute configuration fill button is not pushed for `setup_timeout` seconds.
        display_time(hours, minutes, 2);
        if (!button){
            ++minutes;
            if (minutes == 60)
                minutes = 0;
            timeout = setup_timeout; //if button was pushed, refresh timeout
        }
        while (!button){// wait for button to be released
            display_time(hours, minutes, 2);
        }
        seconds = 0;
    }
    timeout = setup_timeout;
    while (timeout){ // run minute configuration fill button is not pushed for `setup_timeout` seconds.
        display_time(hours, minutes, 1);
        if (!button){
            ++hours;
            if (hours == 24)
                hours = 0;
            timeout = setup_timeout; //if button was pushed, refresh timeout
        }
        while (!button){// wait for button to be released
            display_time(hours, minutes, 1);
        }
        seconds = 0;
    }
    setup = 0;
    timeout = 0;
    display_done();
}

void main(void) {
    CMCON = 0x07; // comparators off
    TRISB = 0x80; // RB7 input, all other pins - output.
    TRISA = 0x00;
    reset_mosfets();
    init_segments();    
    
    //Timer2 Registers Prescaler= 4 - TMR2 PostScaler = 10 - PR2 = 250 - Freq = 500.00 Hz - Period = 0.002000 seconds
    T2CON |= 72;        // bits 6-3 Post scaler 1:1 thru 1:16
    TMR2ON = 1;  // bit 2 turn timer2 on;
    T2CKPS1 = 0; // bits 1-0  Prescaler Rate Select bits
    T2CKPS0 = 1;
    PR2 = 250;         // PR2 (Timer2 Match value)
    // Interrupt Registers
    INTCON = 0;// clear the interrpt control register
    TMR0IE = 0;// bit5 TMR0 Overflow Interrupt Enable bit...0 = Disables the TMR0 interrupt
    TMR2IF = 0;// clear timer1 interupt flag TMR1IF
    TMR2IE = 1;// enable Timer2 interrupts
    TMR0IF = 0;// bit2 clear timer 0 interrupt flag
    GIE = 1;// bit7 global interrupt enable
    PEIE = 1;// bit6 Peripheral Interrupt Enable bit...1 = Enables all unmasked peripheral interrupts
    while (1){
        if(!button){
            int end_seconds = display_till();
            while (end_seconds != seconds){
                display_time(hours, minutes, 0);                
            }
            if (button_pressed >= setup_init_time && !setup){
                setup = 1;
                time_setup();
            }    
        }
    }
}