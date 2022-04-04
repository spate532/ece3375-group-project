#include "address_map_arm.h"
#define TIMER_A9_BASE 0xFFFEC600
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050

volatile int * SETTINGS_SWITCH = (int *) SW_BASE;   //used for entering settings
volatile int * BUTTONS = (int *) KEY_BASE;          //used for increasing/decreasing values + setting
volatile int * DISP_0_3 = (int *) HEX3_HEX0_BASE; // for minutes and seconds
volatile int * DISP_4_5 = (int *) HEX5_HEX4_BASE; //for hours

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
    else                    return 0x40; //display '-' for error
}

void wait_for_release(){ //loop until all buttons are released
	char * sw_in = (char *) BUTTONS;
    unsigned int input;
    input = *sw_in;
    input |= 0b0000;
    volatile int pressed = 1;

    while(pressed){
		printf("\n loopy");
        if(input == 0){
            pressed = 0;
			break;
        }
		
		input = *sw_in;
		input |= 0b0000;
    }
}

void change_settings(){
    unsigned int pb_value;
    char * input = (char *) BUTTONS;
	unsigned int go_next = 1;
    
    while(go_next){ //SET FREQUENCY --------------------------------------------------------------------
        pb_value = *input;
        pb_value |= 0b0000;
        if(pb_value == 1){ //value has been set, go next
            go_next = 0;
			wait_for_release();
			break;
        }
        else if (pb_value == 2){ //increase value
            wait_for_release();
			if(meal_frequency < 4) //ensure there is at most 4 meals per day
                meal_frequency += 1;
        }
        else if (pb_value == 4){ //decrease value
            wait_for_release();
			if(meal_frequency > 1) //ensure there is at least 1 "meal" per day
                meal_frequency -= 1;
        }
		
        *DISP_4_5 = (0x71 << 8); //F for frequency
        *DISP_0_3 = digit_to_hex(meal_frequency);
    }
	
	//printf("next setting");
	go_next = 1;
	
    while(go_next){ //SET QUANTITY --------------------------------------------------------------------
        pb_value = *input;
        pb_value |= 0b0000;
        if(pb_value == 1){ //value has been set, go next
            go_next = 0;
			wait_for_release();
			break;
        }
        else if (pb_value == 2){ // increase value
            wait_for_release();
			if(meal_weight < 500) //ensure the max weight is 500g
                meal_weight += 10;
        }
        else if (pb_value == 4){ //decrease value
			wait_for_release();
			if(meal_weight > 10) //ensure the min weight is 10g
                meal_weight -= 10;
        }

        volatile int tens = meal_weight % 100;
		tens /= 10;
        volatile int hundos = meal_weight /100;
        *DISP_4_5 = (0x67 << 8); //q for quantity
        *DISP_0_3 = (digit_to_hex(hundos) << 16) + (digit_to_hex(tens) << 8) + digit_to_hex(0);
    }

    /*-------- RESET TIMER --------*/
}

int get_meal_frequency(){
	return meal_frequency;
}

int get_meal_weight(){
	return meal_weight;
}

