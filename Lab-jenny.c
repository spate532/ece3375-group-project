#include "address_map_arm.h"

//create global pointers to board hardware
volatile unsigned int* const intTimer1 = (unsigned int*) TIMER_BASE; // interval timer
volatile unsigned int* const pushbutton = (unsigned int*) KEY_BASE;
volatile unsigned int* const modeSwitch = (unsigned int*) SW_BASE;
volatile unsigned int* const sevSegment1 = (unsigned int*) HEX3_HEX0_BASE;
volatile unsigned int* const sevSegment2 = (unsigned int*) HEX5_HEX4_BASE;

// general global variables
unsigned int const lookUpTable[]= {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101,0b00000111, 0b01111111, 0b01101111, 0b01110111, 0b01111100, 0b00111001, 0b01011110, 0b01111001, 0b01110001};
unsigned int minutes = 0;
unsigned int seconds = 0;
unsigned int hundredSec = 0;

int readPushButtons(void) //read the state of the pushbuttons
{
  volatile int readButton;
  readButton = *pushbutton & 0xf; // reads only first four bits
  return readButton;
}

int readModeSwitch(void) //read the state of the mode switch
{
  volatile int readSwitch;
  readSwitch = *modeSwitch & 0x1; //reads only first bit
  return readSwitch;
}

void displayHex(int value, int side) //display the hex numbers to the seven segment display
{

// for first four seven segment panels
  if(side <= 3)
  {
    *sevSegment1 &= ~(0xff << side*8); // bit mask to get address of specific seven seg display
    *sevSegment1 |= lookUpTable[value] <<8*side; //write
  }
// for remaining two seven segment panels
  else
  {
    side -= 4;
    *sevSegment2 &= ~(0xff << side*8); // bit mask to get specific seven seg display address
    *sevSegment2 |= (lookUpTable[value] << side*8);
  }

}

// taking clock number and converting to times for display
//           hundredths of a seconds          seconds
void hexNum(unsigned long counter, unsigned long counter2) //send the calculated times to display
{
  minutes = counter2/60;
  seconds = counter2 - minutes*60;
  hundredSec = counter;

  // minutes
  unsigned char seg5 = minutes/10;
  unsigned char seg4 = minutes - (seg5*10);

  // seconds
  unsigned char seg3 = seconds/10;
  unsigned char seg2 = seconds - (seg3*10);

  // hundredths
  unsigned char seg1 = hundredSec/10;
  unsigned char seg0 = hundredSec - (seg1*10);

  // minutes
  displayHex(seg5, 5);
  displayHex(seg4, 4);

  // seconds
  displayHex(seg3, 3);
  displayHex(seg2, 2);

  // hundredths
  displayHex(seg1, 1);
  displayHex(seg0, 0);
}

int main(void) {
  volatile unsigned long hunCount = 0;
  volatile unsigned long secCount = 0;
  volatile unsigned long lapSec = 0;
  volatile unsigned long lapHund = 0;

  //default status of the stopwatch
  // 1megahz/100 interval = 1000000
  *(intTimer1 + 2) = 1000000; //period start value (low)
  *(intTimer1 + 3) = 1000000 >> 16; //period start value (high)
  *(intTimer1 + 0) = 1; //reset timeout signal

  while(1)
  {
    // 
    if(*(intTimer1+0) == 0x3) 
    {
      *(intTimer1+0) = 1; // must write to timer
      hunCount++; //increment every hundredth of a second

      if(hunCount >= 100) // hundredths = one second
      {
        hunCount = 0; //reset hundredth counter
        secCount++; //increment every second
      }
    }
    //reading push buttons

    // start button pushes - first button
    if(readPushButtons() & 0b0001) 
    {
      *(intTimer1 + 1) = 0x6; //write 1 to start and cont
    }
    // stop button pushed - second button
    else if(readPushButtons() & 0b0010)
    {
      *(intTimer1 + 1) = 0xA; //write 1 to stop and cont
    }
    // lap button pushed - third button
    // also accept lap button and start button at the same time
    if(readPushButtons() & 0b0100 || readPushButtons() & 0b0101) 
    {
      // notes current time
      lapSec = secCount; //second
      lapHund = hunCount; //hundredth of second
    }
    // clear button pushed - fourth button
    else if(readPushButtons() & 0b1000) 
    {
      *(intTimer1 + 1) = 0x8;
      *(intTimer1 + 0) = 1;
      secCount = 0;
      hunCount = 0;
      lapHund = 0;
      lapSec = 0;
    }

    //read switch
    // show lap time
    if(readModeSwitch() & 1) 
    {
      hexNum(lapHund, lapSec); //hundredths and seconds
    }
    // show timer time
    else 
    {
      hexNum(hunCount, secCount); //hundreths and seconds
    }
  }
}
