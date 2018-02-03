/* vim: set ai et ts=4 sw=4: */

#define F_CPU 1000000

#include <avr/io.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <string.h>
#include <stdbool.h>

void inc_time(int8_t* time) {
    for(uint8_t i = 0; i < 4; i++) {
        time[i]++;
        if(time[i] == 10) {
            // digit overflowed, go to the next one
            time[i] = 0;
        } else {
            // done!
            break;
        }
    }
}

void dec_time(int8_t* time) {
    for(uint8_t i = 0; i < 4; i++) {
        time[i]--;
        if(time[i] == -1) {
            // digit overflowed, go to the next one
            time[i] = 9;
        } else {
            // done!
            break;
        }
    }
}

bool time_is_zero(int8_t* time) {
    return (!time[0]) && (!time[1]) && (!time[2]) && (!time[3]);
}

void relay(bool on) {
    if(on) {
        PORTB |= (1 << 5);
    } else {
        PORTB &= ~(1 << 5);
    }
}

void seg_g(bool on) {
    if(on) {
        PORTD |= (1 << 0);
    } else {
        PORTD &= ~(1 << 0);
    }
}

void seg_c(bool on) {
    if(on) {
        PORTD |= (1 << 1);
    } else {
        PORTD &= ~(1 << 1);
    }
}

void seg_h(bool on) {
    if(on) {
        PORTD |= (1 << 2);
    } else {
        PORTD &= ~(1 << 2);
    }
}

void seg_d(bool on) {
    if(on) {
        PORTD |= (1 << 3);
    } else {
        PORTD &= ~(1 << 3);
    }
}

void seg_e(bool on) {
    if(on) {
        PORTD |= (1 << 4);
    } else {
        PORTD &= ~(1 << 4);
    }
}

void digit0(bool on) {
    // _low_ voltage enables the digit
    if(on) {
        PORTB &= ~(1 << 6);
    } else {
        PORTB |= (1 << 6);
    }
}

void seg_a(bool on) {
    if(on) {
        PORTB |= (1 << 7);
    } else {
        PORTB &= ~(1 << 7);
    }
}

void seg_f(bool on) {
    if(on) {
        PORTD |= (1 << 5);
    } else {
        PORTD &= ~(1 << 5);
    }
}

void digit1(bool on) {
    // _low_ voltage enables the digit
    if(on) {
        PORTD &= ~(1 << 6);
    } else {
        PORTD |= (1 << 6);
    }
}

void digit2(bool on) {
    // _low_ voltage enables the digit
    if(on) {
        PORTD &= ~(1 << 7);
    } else {
        PORTD |= (1 << 7);
    }
}

void seg_b(bool on) {
    if(on) {
        PORTB |= (1 << 0);
    } else {
        PORTB &= ~(1 << 0);
    }
}

void digit3(bool on) {
    // _low_ voltage enables the digit
    if(on) {
        PORTB &= ~(1 << 1);
    } else {
        PORTB |= (1 << 1);
    }
}

void digit(int8_t num, bool on) {
    switch(num) {
        case 0:
            digit0(on);
            break;
        case 1:
            digit1(on);
            break;
        case 2:
            digit2(on);
            break;
        case 3:
            digit3(on);
            break;
        default:
            break;
            // do nothing; should never happen
    }
}

inline bool inc_pressed() {
    // the button is pulled-up to VCC, the button is conneted to GND
    // i.e. low voltage means that the button is pressed
    return !(PINC & (1 << 0));
}

inline bool dec_pressed() {
    // the button is pulled-up to VCC, the button is conneted to GND
    // i.e. low voltage means that the button is pressed
    return !(PINC & (1 << 1));
}

inline bool start_pressed() {
    // the button is pulled-up to VCC, the button is conneted to GND
    // i.e. low voltage means that the button is pressed
    return !(PINC & (1 << 2));
}

inline bool save_pressed() {
    // the button is pulled-up to VCC, the button is conneted to GND
    // i.e. low voltage means that the button is pressed
    return !(PINC & (1 << 3));
}

void encode_digit(int8_t d) {
    seg_a(d == 0 || d == 2 || d == 3 || ((d >= 5) && (d <= 9)));
    seg_b(((d >= 0) && (d <= 4)) || ((d >= 7) && (d <= 9)));
    seg_c(d == 0 || d == 1 || d == 3 || ((d >= 4) && (d <= 9)));
    seg_d(d == 0 || d == 2 || d == 3 || d == 5 || d == 6 || d == 8 || d == 9);
    seg_e(d == 0 || d == 2 || d == 6 || d == 8);
    seg_f(d == 0 || ((d >= 4) && (d <= 6)) || d == 8 || d == 9);
    seg_g(d == 2 || d == 3 || ((d >= 4) && (d <= 6)) || d == 8 || d == 9);
}


int8_t saved_timeout[4] EEMEM = {0, 2, 0, 0}; // 20 seconds
int8_t timeout[4];
int8_t countdown[4];
uint16_t countdown_ms;
bool countdown_active = false;
bool inc_was_released = true;
bool dec_was_released = true;
bool start_was_released = true;
bool save_was_released = true;

void setup() {
    // load timeout from the EEPROM
    eeprom_read_block(timeout, saved_timeout, sizeof(timeout));

    DDRB = 0;
    // PB5 = relay control
    DDRB |= (1 << 5);
    relay(false);
    // PB0, PB1, PB6 and PB7 are used to drive LED indicator
    DDRB |= (1 << 0) | (1 << 1) | (1 << 6) | (1 << 7);
    //PD0-PD8 are used to drive LED indicator as well
    DDRD = 0xFF; 

    DDRC = 0; // PC0-PC3 = buttons
    PORTC = 0xFF; // enable pull-up resistors

    // initially none of the numbers is chosen
    digit0(false);
    digit1(false);
    digit2(false);
    digit3(false);
}

void loop() {
    bool old_countdown_active = countdown_active;
    
    for(int8_t i = 0; i < 4; i++) {
        encode_digit(countdown_active ? countdown[3-i] : timeout[3-i]);
        digit(i, true);
        _delay_ms(5);
        countdown_ms += countdown_active ? 5 : 0;
        digit(i, false);
    }

    if(!countdown_active && inc_pressed()) {
        if(inc_was_released) { // debounce
            inc_time(timeout);
            inc_was_released = false;
        }
    } else
        inc_was_released = true;

    if(!countdown_active && dec_pressed()) {
        if(dec_was_released) { // debounce
            dec_time(timeout);
            dec_was_released = false;
        }
    } else
        dec_was_released = true;

    if(!countdown_active && save_pressed()) {
        if(save_was_released) { // debounce
            // save current timeout to the EEPROM
            eeprom_update_block(timeout, saved_timeout, sizeof(timeout));
            save_was_released = false;
        }
    } else
        save_was_released = true;

    if(start_pressed()) {
        if(start_was_released) { // debounce
            if(countdown_active) {
                // stop the countdown
                countdown_active = false;
            } else {
                if(!time_is_zero(timeout)) {
                    // start the countdown
                    memcpy(countdown, timeout, sizeof(timeout));
                    countdown_ms = 0;
                    countdown_active = true;
                }
            }
            start_was_released = false;
        }
    } else
        start_was_released = true;

    if(countdown_active && (countdown_ms >= 1000)) {
        // one second has passed
        dec_time(countdown);
        countdown_ms -= 1000;
        if(time_is_zero(countdown)) {
            // stop the countdown
            countdown_active = false;
        }
    }

    if(old_countdown_active != countdown_active) {
        // change the port state only if necessary
        relay(countdown_active);
    }
}

int main() {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
