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

volatile a9_timer * const timer_1 = ( a9_timer *) TIMER_A9_BASE ;   //hardware pointers
volatile int * SEC_DISPLAY = (int *) HEX3_HEX0_BASE; //for seconds and milliseconds
volatile int * MIN_DISPLAY = (int *) HEX5_HEX4_BASE;
volatile int * LAP_SWITCH = (int *) SW_BASE;
volatile int * PSH_BTNS = (int *) KEY_BASE; 
volatile int t_min = 0, t_sec = 0, t_msc = 0; //timer values
volatile int lap_min = 0, lap_sec = 0, lap_msc = 0; //lap values
volatile int display_type; //0 for show timer, 1 for show lap

void set_timer(int interval){
    //int current_count = 0;          //set count to 0
    //int last_count = interval;      //set limit of the timer to the interval
    t_min = 0;
    t_sec = 0;
    t_msc = 0;
    timer_1 -> load = interval ;    //set timer for 10ms intervals (every time the watch values should change)
}

void start_timer(){
    // start timer for continuous counting
    // 3 is ( 0b011 ) for control -> start continuous count
    // (1 << 😎 is for prescaler
	timer_1 -> status = 1;
    timer_1 -> control = 3 + (1<<8);
}

void stop_timer(){
    // start timer at current count
    // 0 is ( 0b0 ) for control -> stop counting
    // (1 << 😎 is for prescaler
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

void tick(){ //add 10 millseconds to the clock, roll over digits as needed 
    if(t_msc == 99){
        if(t_sec == 59){
            if(t_min == 59){
                t_min = 0;
				t_sec = 0;
				t_msc = 0;
            }
            else{
                t_min += 1;
            }
            t_sec = 0;
        }
        else{
            t_sec += 1;
        }
        t_msc = 0;
    }
    else{
        t_msc += 1;
    }
}

void wait_for_timer(){
    
	while(check_timer() == 1){ //loops while counter is still running
		check_count();
    }
    tick();
}

int digit_to_hex(int value){
    if(value == 0)          return 0x3F;
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

void display(int minutes , int seconds , int mseconds){
    int min_a = digit_to_hex(minutes/10); //split values into single digits for display
    int min_b = digit_to_hex(minutes%10);
    int sec_a = digit_to_hex(seconds/10);
    int sec_b = digit_to_hex(seconds%10);
    int msc_a = digit_to_hex(mseconds/10);
    int msc_b = digit_to_hex(mseconds%10);

    *SEC_DISPLAY = (sec_a << 24) + (sec_b << 16) + (msc_a << 8) + (msc_b);
    *MIN_DISPLAY = (min_a << 8) + (min_b);
}

int main(void){
    int interval =  300000;    //interval of 0.01 seconds (100000000 for 1 second)
    set_timer(interval);        //set the time using 0.01 intervals
    while(1){
        /*if start button press -> start*/
        /*if stop button press -> stop*/
        /*if lap button press -> set lap time*/
        unsigned int pb_value;
        char * input = (char *) PSH_BTNS;
        pb_value = *input;
        pb_value |= 0b0000;
        if(pb_value == 1){  //start
			display(11 , 11 , 11);
            start_timer();
        }
        else if(pb_value == 2){ //stop
            display(22 , 22 , 22);
			stop_timer();
        }
        else if(pb_value == 4){ //lap
            display(33 , 33 , 33);
			lap_min = t_min;
            lap_sec = t_sec;
            lap_msc = t_msc;
        }
        else if(pb_value == 8){ //clear
            display(44 , 44 , 44);
			t_min = 0;
            t_sec = 0;
            t_msc = 0;
			
			lap_min = 0;
            lap_sec = 0;
            lap_msc = 0;
        }
        else{}
		
		/*if lap switch up -> display lap*/
        /*if lap switch down -> display timer*/
        unsigned int sw_value;
        char * lap_in = (char *) LAP_SWITCH;
        sw_value = *lap_in;
        sw_value |= 0b0000;
		if(sw_value == 1){
            display(lap_min , lap_sec , lap_msc);
        }
        else{
            display(t_min , t_sec , t_msc);
        }
		
		/*wait for counter and "tick"*/	
		if(check_timer() == 1)
			wait_for_timer();
		else{}
    }
}