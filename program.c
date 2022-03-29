#include "address_map_arm.h"

#define TIMER_A9_BASE 0xFFFEC600
#define HEX3_HEX0_BASE 0xFF200020
#define HEX5_HEX4_BASE 0xFF200030
#define SW_BASE 0xFF200040
#define KEY_BASE 0xFF200050

typedef struct _a9_timer {
    int load ;
    int count ;
    int control ;
    int status ;
} a9_timer ;

volatile int * SETTINGS_SWITCH = (int *) SW_BASE;   //used for entering settings
volatile int * BUTTONS = (int *) KEY_BASE;          //used for increasing/decreasing values + setting
volatile int * DISP_0_3 = (int *) HEX3_HEX0_BASE; // for minutes and seconds
volatile int * DISP_4_5 = (int *) HEX5_HEX4_BASE; //for hours

volatile int t_hr = 0, t_min = 0, t_sec = 0; //timer values
volatile int meal_frequency = 2; //twice daily (12 hours gaps) 
volatile int meal_interval
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
            if(meal_frequency < 3) //ensure there is at most 9 meals per day
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

void set_timer(int interval){
    //int current_count = 0;          //set count to 0
    //int last_count = interval;      //set limit of the timer to the interval
    t_hr = 0;
    t_min = 0;
    t_sec = 0;
    timer_1 -> load = interval ;    //set timer for 10ms intervals (every time the watch values should change)
}

void start_timer(){
    // start timer for continuous counting
    // 3 is ( 0b011 ) for control -> start continuous count
    // (1 << 8) is for prescaler
	timer_1 -> status = 1;
    timer_1 -> control = 3 + (1<<8);
}

void stop_timer(){
    // start timer at current count
    // 0 is ( 0b0 ) for control -> stop counting
    // (1 << 8) is for prescaler
    timer_1 -> control = 0; // + (1<<8);
}

int check_timer(){
    if(timer_1 -> status == 1) 
        return 1; //returns 0 if count is done
    else
        return 0; //returns 1 if counter is still counting
}

void check_count(){
	if(timer_1 -> count == 0)
		timer_1 -> status = 1;
}

void tick(){ //add 1 second to clock, roll digits if nessecary
    if(t_hr == 99){
        if(t_min == 59){
            if(t_hr == 59){
                t_hr = 0;
				t_min = 0;
				t_hr = 0;
            }
            else{
                t_hr += 1;
            }
            t_min = 0;
        }
        else{
            t_min += 1;
        }
        t_hr = 0;
    }
    else{
        t_hr += 1;
    }
}

void display(int hours , int minutes , int seconds){
    int min_a = digit_to_hex(hours/10); //split values into single digits for display
    int min_b = digit_to_hex(hours%10);
    int sec_a = digit_to_hex(minutes/10);
    int sec_b = digit_to_hex(minutes%10);
    int msc_a = digit_to_hex(seconds/10);
    int msc_b = digit_to_hex(seconds%10);

    *MS_DISPLAY = (sec_a << 24) + (sec_b << 16) + (msc_a << 8) + (msc_b);
    *HR_DISPLAY = (min_a << 8) + (min_b);
}

void wait_for_timer(){
	while(check_timer() == 1){ //loops while counter is still running
		check_count();
    }
    tick();
}

int read_settings_sw(void) { //read the state of the mode switch
  volatile int readSwitch;
  readSwitch = *SETTINGS_SWITCH & 0x1; //reads only first bit
  return readSwitch;
}

void feed_that_mf(){
    //@TODO: add this
}

int main(void){
    int interval =  100000000;    //interval of 0.01 seconds (100000000 for 1 second)
    set_timer(interval);        //set the time using 0.01 intervals

    while(1){
		if(read_settings_sw() == 1) //check if settings mode needs to entered
            change_settings();
        
        if(t_hr == meal_interval){
            feed_that_mf(0)
            set_timer();
        }

        if(check_timer() == 1)
			wait_for_timer();
		else{}
    }
}