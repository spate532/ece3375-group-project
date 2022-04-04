#include "address_map_arm.h"
#include "settings_input.h"

// structure for the GPIO
typedef struct _GPIO
{
  unsigned int data;
  unsigned int control;
} GPIO;

// structure for the ADC
typedef struct _ADC
{
    unsigned int ch0 ;
    unsigned int ch1 ;
    unsigned int ch2 ;
    unsigned int ch3 ;
    unsigned int ch4 ;
    unsigned int ch5 ;
    unsigned int ch6 ;
    unsigned int ch7 ;
} ADC ;

// pointers to hardware
volatile ADC * const adc_ptr = ( ADC *) ADC_BASE;
volatile GPIO* const gpio = (unsigned int*) JP1_BASE;
volatile unsigned int* const intTimer1 = (unsigned int*) TIMER_BASE;
volatile unsigned int* const pushbutton = (unsigned int*) KEY_BASE;

// global variables
unsigned int startWeight;
unsigned int currentTime=0;

// arbitrary inputs
unsigned int mealWeight=100;
unsigned int lowWater=100;
unsigned int fullWater=499;
unsigned int feedTime=2000;

//reverse polarity to retract actuator
void retractMotor(motor) {
    gpio->data = (0b10 << 2*motor);
}

// forward polarity to extend actuator
void extendMotor(motor) {
    gpio->data = (0b01 << 2*motor);
}

void foodStartWeight() {  // globally stores start weight
    startWeight = weightCalc(0);
}

void dispenseFood(foodWeight) {
    while(1) {
        // difference equal or greater than desired meal size - stop feeding
        if((startWeight-weightCalc(0))>=foodWeight) {
            extendMotor(0);
            break;
        }
        // keep motor up to release food
        else {
            retractMotor(0);
            extendMotor(1); //make sure water stays closed
        }
    }
}

void dispenseWater() {
    while(1) {
        if(weightCalc(1)>=fullWater) {
            extendMotor(1);
            break;
        }
        // keep motor up to release water
        else {
            retractMotor(1);
            extendMotor(0); //make sure food stays closed
        }
    }
}

void lowFoodCheck() {
    if (weightCalc(0)<=mealWeight) {
        gpio->data = (1 << 9);
    }
}

int weightCalc(potNumber) {
    // assuming max weight is 500, converting voltage to weight value
    int weight;
    weight = (readADC(potNumber)*500)/4096;
    return weight;
}

int readADC(potNum) {
    int voltage;
    if (potNum == 0){
        voltage = adc_ptr->ch0 & 0xfff;
    }
    else if (potNum == 1) {
        voltage = adc_ptr->ch1 & 0xfff;
    }
    return voltage;
}

void initializeTimer() {
  //default status of the stopwatch
  // 1megahz/100 interval = 1000000
  *(intTimer1 + 2) = 100000000; //period start value (low)
  *(intTimer1 + 3) = 100000000 >> 16; //period start value (high)
  *(intTimer1 + 0) = 1; //reset timeout signal
  *(intTimer1 + 1) = 0x6; //write 1 to start and cont
}

void continueTimer() {
    if(*(intTimer1+0) == 0x3) 
    {
      *(intTimer1+0) = 1; // must write to timer
      currentTime++;
    }
}

int readPushButtons(void) //read the state of the pushbuttons
{
  volatile int readButton;
  readButton = *pushbutton;
  return readButton;
}

int main(void) {
    gpio->control = 0x3FF;
    extendMotor(0);
    extendMotor(1);

    initializeTimer();

    while(1) {
        if(readPushButtons() & 0b1000) {
            change_settings();
        }
        continueTimer();
        feedTime = 36/get_meal_frequency();
        mealWeight = get_meal_weight();
        if (currentTime>=feedTime) {
            foodStartWeight();
            dispenseFood(mealWeight);
            currentTime=0;
        }
        else {
            extendMotor(0);
        }
        if (weightCalc(1)<=lowWater) {
            dispenseWater();
        }
        else {
            extendMotor(1);
        }
        lowFoodCheck();
    }
}