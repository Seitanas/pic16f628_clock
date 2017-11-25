/*
 * File:   main.c
 * Author: Tadas Ustinavičius
 * https://github.com/Seitanas/pic16f628_clock
 * 
 */
#define _XTAL_FREQ 4000000

// CONFIG
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is digital input, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)

#include <xc.h>


#define setup_timeout 4 // how much inactivity will move to next setting
#define display_for 4 // for how long display time after button is pushed
#define display_greeting_for 1  // for how long display greeting on startup
#define setup_init_time 7 // for how long button must be pressed for setup to be initiated
/* You may need to fine-tune 'hi' and 'lo' for your crystal:
 * Basicaly it's a 16 bit register divided into two 8 bit registers.
 * If hi = 0x7F and lo = 0xFF, then real value would be 0x7FF. Which is
 * 32767 in decimal.
 * This means that 0xFFFF-0x7FFF=0x8000 or 32768.
 * Counter will count 32768 times to overflow and generate interrupt.
 * Decrease these values to slow clock down, or increase them
 * if you want it to go faster.
 */
#define hi 0x7F  // TMR1H
#define lo 0xFF  // TMR1L


#define led1 RA0
#define led2 RA1
#define led3 RA2
#define led4 RA3
#define button RA6
#define mosfet_off_signal 1
#define mosfet_on_signal 0

int hours = 0;
int minutes = 0;
int seconds=0;
int timeout = 0;
int button_pressed = 0;
int setup = 0;


void light_segment(int letter){
    //    ABCDEF  G
    //RB: 342015 RA:7
    switch (letter){
        case 0:
            //ABCDEF
            PORTB = 0b00000000;
            RA7 = 1;
            break;
        case 1:
            //BC
            PORTB = 0b00101011;
            RA7 = 1;
            break;
        case 2:
            //ABDEG
            PORTB = 0b00100100;
            RA7 = 0;
            break;
        case 3:
            //ABCDG
            PORTB = 0b00100010;
            RA7 = 0;
            break;
        case 4:
            //FGBC
            PORTB = 0b00001011;
            RA7 = 0;
            break;
        case 5:
            //AFGCD
            PORTB = 0b00010010;
            RA7 = 0;
            break;
        case 6:
            //AFGCDE
            PORTB = 0b00010000;
            RA7 = 0;
            break;
        case 7:
            //ABC
            PORTB = 0b00100011;
            RA7 = 1;
            break;
        case 8:
            //ABCDEFG
            PORTB = 0b00000000;
            RA7 = 0;
            break;
        case 9:
            //ABCDFG
            PORTB = 0b00000010;
            RA7 = 0;
            break;
        //symbols:
        case 10:
            PORTB = 0b00110111;
            RA7 = 1;
            break;
        case 11:
            PORTB = 0b00101111;
            RA7 = 1;
            break;
        case 12:
            PORTB = 0b00111011;
            RA7 = 1;
            break;
        case 13:
            PORTB = 0b00111110;
            RA7 = 1;
            break;
        case 14:
            PORTB = 0b00111101;
            RA7 = 1;
            break;            
        case 15:
            PORTB = 0b00011111;
            RA7 = 1;
            break;
        case 16: // E
            PORTB = 0b00010100;
            RA7 = 0;
            break;
        case 17: // n
            PORTB = 0b00111001;
            RA7 = 0;
            break;
        case 18: // o
            PORTB = 0b00111000;
            RA7 = 0;
            break;            
        case 19: // d
            PORTB = 0b00101000;
            RA7 = 0;
            break;   
            //    ABCDEF  G
            //RB: 342015 RA:7
        case 20: // h
            PORTB = 0b00011001;
            RA7 = 0;
            break;
        case 21: // L
            PORTB = 0b00011100;
            RA7 = 1;
            break; 
        case 22: // i
            PORTB = 0b00101011;
            RA7 = 1;
            break;
        case 23: // A
            PORTB = 0b00000001;
            RA7 = 0;
            break; 
    }
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

void led_player(void(*delay_function)(void), int segments, int led_pos){ /* delay_function
                                                                          * is a pointer to one
                                                                          * of delay functions above*/
    reset_mosfets();
    light_segment(segments);
    switch (led_pos){
        case 1:
            led1 = 0;
            break;
        case 2:
            led2 = 0;
            break;
        case 3:
            led3 = 0;
            break;
        case 4:
            led4 = 0;
            break;
    } 
    delay_function();
    reset_mosfets();
}

int display_till(int display_sec){
    int end_seconds = seconds + display_sec;
    if (end_seconds > 59) // the sum above cant overlap 59 seconds
        end_seconds = end_seconds - 60;
    return end_seconds;
}

void display_done(void){
    int end_seconds = display_till(display_for);
    while (end_seconds != seconds){
        led_player(led_wait, 16, 1); // display 'E'
        led_player(led_wait, 17, 2); // display 'n'
        led_player(led_wait, 18, 3); // display 'o'
        led_player(led_wait, 19, 4); // display 'd'
    }
}

void display_hi(void){
    int end_seconds = display_till(display_greeting_for);
    while (end_seconds != seconds){
        led_player(led_wait, 22, 3); // display 'i'
        led_player(led_wait, 20, 4); // display 'h'
    }    
    
}

void play_setup(void){
    int cycle = 4;
    while (cycle){
        //    ABCDEF  G
        //RB: 342015 RA:7
        led_player(animation_wait, 10, 1);
        led_player(animation_wait, 10, 2);
        led_player(animation_wait, 10, 3);
        led_player(animation_wait, 10, 4);        
        led_player(animation_wait, 14, 4);
        led_player(animation_wait, 15, 4);    
        led_player(animation_wait, 13, 4);
        led_player(animation_wait, 13, 3);
        led_player(animation_wait, 13, 2);
        led_player(animation_wait, 13, 1);    
        led_player(animation_wait, 11, 1);
        led_player(animation_wait, 12, 1);
        --cycle;
    }
}

void display_time(int hours, int minutes, int display){ /*
                                                         display = 0 - display hours and minutes
                                                         display = 1 - display hours
                                                         display = 2 display minutes
                                                         */
    if (!display || display == 1){
        // display most significant hour digit:
        led_player(led_wait, hours/10, 4);
        // display least significant hour digit:
        led_player(led_wait, hours%10, 3);
    }
    if (!display || display == 2){
        // display most significant minute digit:
        led_player(led_wait, minutes/10, 2);
        // display least significant minute digit:
        led_player(led_wait, minutes%10, 1);
    }
}

void interrupt isr(void){
    if(TMR1IF){
        TMR1ON = 0;
        TMR1H = hi;
        TMR1L = lo;
        TMR1IF = 0; 
        TMR1ON = 1;
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
        if (!button && !setup)
            ++button_pressed;          
        if (setup && timeout) /* if setup is enabled, decrease timeout each second
                                for setup to jump from minutes to hours after defined
                               inactivity time*/
            --timeout;  
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
            timeout = setup_timeout; // if button was pushed, refresh timeout
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
            timeout = setup_timeout; // if button was pushed, refresh timeout
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
    TRISA=0b01000000;    
    TRISB=0b00000000;

    PORTB=0x00111111;
    reset_mosfets();
    /* MUST HAVE!!!! If no delay is made at the startup,
     * TMR1 is initialized too soon and re-programming
     * of PIC is no longer possible:
     */
    __delay_ms(2000);

    T1CKPS0 = 0;
    T1CKPS1 = 0;
    nT1SYNC = 1;
    TMR1CS=1;
    TMR1H = hi;
    TMR1L = lo;
    T1OSCEN = 1;     
    TMR1IE = 1;
    GIE=1;          //Enable Global Interrupt
    PEIE=1; 
    TMR1ON = 1;
    display_hi();
    while(1){
        if(!button){
            int end_seconds = display_till(display_for);
            while (end_seconds != seconds){
                display_time(hours, minutes, 0);
            }
            if (button_pressed >= setup_init_time && !setup){
                setup = 1;
                time_setup();
            }    
        }
        else {/* Put CPU to sleep. Only TMR1 is still counting.
              * Interrupt will wake CPU. (Low power mode) */
            button_pressed = 0; // reset global variable
            SLEEP();
        }
    }
}