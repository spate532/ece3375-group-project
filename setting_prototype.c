#include "address_map_arm.h"
#define TIMER_A9_BASE 0xFFFEC600
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050

volatile a9_timer * const timer_1 = ( a9_timer *) TIMER_A9_BASE ;   //hardware pointers
volatile int * SETTINGS_SWITCH = (int *) SW_BASE;   //used for entering settings
volatile int * BUTTONS = (int *) KEY_BASE;          //used for increasing/decreasing values + setting
volatile int * DISP_0_3 = (int *) HEX3_HEX0_BASE; // for minutes and seconds
volatile int * DISP_4_5 = (int *) HEX5_HEX4_BASE; //for hours

volatile int t_hr = 0, t_min = 0, t_sec = 0; //timer values
volatile int meal_frequency = 2; //twice daily (12 hours gaps) 
volatile int meal_weight = 150;  //grams

int digit_to_hex(int value){
    if      (value == 0)    return 0x3F;
    else if (value == 1)    return 0x06;
    else if (value == 2)    return 0x5B;
    else if (value == 3)    return 0x4F;
    else if (value == 4)    return 0x66;
    else if (value == 5)    return 0x6D;
    else if (value == 6)    return 0x7D;
    else if (value == 7)    return 0x07;
    else if (value == 8)    return 0x7F;
    else if (value == 9)    return 0x6F;
    else                    return 0x20; //display '-' for error
}

void change_settings(){
    unsigned int pb_value;
    char * input = (char *) BUTTONS;
    volatile int flash_state = 1;
    
    while(1){ //SET FREQUENCY --------------------------------------------------------------------
        pb_value = *input;
        pb_value |= 0b0000;
        if(pb_value == 1){ //value has been set, go next
            break;
        }
        else if (pb_value == 2){ // increase value
            if(meal_frequency < 9) //ensure there is at most 9 meals per day
                meal_frequency += 1;
        }
        else if (pb_value == 4){ //decrease value
            if(meal_frequency > 1) //ensure there is at least 1 "meal" per day
                meal_frequency -= 1;
        }
        if(flash_state == 1){
            flash_state = 0;
            *DISP_4_5 = (0x57 << 8); //F for frequency
            *DISP_0_3 = digit_to_hex(meal_frequency);
        }
        else{
            flash_state = 1;
            *DISP_4_5 = 0x0;
            *DISP_0_3 = 0x0;
        }
    }

    while(1){ //SET QUANTITY --------------------------------------------------------------------
        pb_value = *input;
        pb_value |= 0b0000;
        if(pb_value == 1){ //value has been set, go next
            break;
        }
        else if (pb_value == 2){ // increase value
            if(meal_weight < 500) //ensure the max weight is 500g
                meal_weight += 10;
        }
        else if (pb_value == 4){ //decrease value
            if(meal_weight > 10) //ensure the min weight is 10g
                meal_weight -= 10;
        }
        if(flash_state == 1){
            flash_state = 0;
            volatile int tens = meal_weight % 100;
            volatile int hundos = meal_weight % 1000;
            *DISP_4_5 = (0x73 << 8); //q for quantity
            *DISP_0_3 = (hundos << 16) + (tens << 8) + digit_to_hex(0);
        }
        else{
            flash_state = 1;
            *DISP_4_5 = 0x0;
            *DISP_0_3 = 0x0;
        }
    }

    /*-------- RESET TIMER --------*/
}

int main(void){
    while(1){
        char * lap_in = (char *) SETTINGS_SWITCH;
        unsigned int sw_value;
        sw_value = *lap_in;
        sw_value |= 0b0000;
		if(sw_value == 1){
            change_settings();
        }
    }
}