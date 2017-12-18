#include <RTCconfig.h>
#include <RTCalarm.h>
#include <RTCplus.h>
#include "MspFlash.h"
#include "pitches.h"

// Arduino 7 segment display example software
// http://www.hacktronics.com/Tutorials/arduino-and-7-segment-led.html
// License: http://www.opensource.org/licenses/mit-license.php (Go crazy)
 
// Define the LED digit patters, from 0 - 9
// Note that these patterns are for common cathode displays
// For common anode displays, change the 1's to 0's and 0's to 1's
// 1 = LED on, 0 = LED off, in this order:
//                                    Arduino pin: 2,3,4,5,6,7,8
byte seven_seg_digits[11][8] = { { 0,0,0,1,0,0,1,0 },  // = 0
                               { 1,1,0,1,0,1,1,1 },  // = 1
                               { 0,0,1,1,0,0,0,1 },  // = 2
                               { 1,0,0,1,0,0,0,1 },  // = 3
                               { 1,1,0,1,0,1,0,0 },  // = 4
                               { 1,0,0,1,1,0,0,0 },  // = 5
                               { 0,0,0,1,1,0,0,0 },  // = 6
                               { 1,1,0,1,0,0,1,1 },  // = 7
                               { 0,0,0,1,0,0,0,0 },  // = 8
                               { 1,0,0,1,0,0,0,0 },  // = 9
                               { 1,1,1,1,1,1,1,1 }   // = blank (10)
                               };
                               
boolean digit_blink_state[3] = {0,0,0};
RealTimeClock display_timer;
RealTimeClock countdown_timer;
enum { ONES, TENS, HUNDREDS };
enum { COUNTDOWN, SET_TIMER, EASY_HOLD };
#define BLANK 10
#define BUTTON1 19
#define BUTTON2 18
#define EASY_BUTTON 11
#define flash SEGMENT_D

//button states and debouncing
//int last_button1_state = HIGH;
//int last_button2_state = HIGH;
//int last_easy_button_state = HIGH;
//int button1_state = 0;
//int button2_state = 0;
//int easy_button_state = 0;
//int last_button1_debounce_time = 0;
//int last_button2_debounce_time = 0;
//int last_easy_button_debounce_time = 0;
//int debounce_delay = 50;

int melody[] = {
  NOTE_A5, NOTE_A5, NOTE_G4, NOTE_F4, NOTE_E4, NOTE_D4, NOTE_CS4};
  
int timing[] = {4,8,8,8,8,8,8};

volatile int easy_button_hold_time = 0;
volatile int display_number = 0;
volatile byte mode;
volatile byte selected_digit;
volatile byte hundreds, tens, ones;
volatile boolean easy_button_pressed = false;
volatile boolean button1_pressed = false;
volatile boolean button2_pressed = false;
volatile boolean mode_toggled = false;

int countdown_time = 0;

const int hold_time_threshold = 256;//in seconds / 256
const int blink_rate = 128; //blink rate will be ((RTC_chunk % blink_rate) / 2) / 256 Hz;

void setup() {
  mode = COUNTDOWN;
  Flash.read(flash, (unsigned char *)(&countdown_time), sizeof(int));
  display_timer.begin();
  countdown_timer.begin();  
  pinMode(2, OUTPUT); //7 Segment display (TDSO1150) pin 1  
  pinMode(3, OUTPUT); //pin2
  pinMode(4, OUTPUT); //pin4
  pinMode(5, OUTPUT); //pin5
  pinMode(6, OUTPUT); //pin6
  pinMode(7, OUTPUT); //pin7
  pinMode(8, OUTPUT); //pin9
  pinMode(9, OUTPUT); //7 Segment display pin 10
  pinMode(10, OUTPUT); //speaker
  pinMode(EASY_BUTTON, INPUT_PULLUP); //EASY button
  attachInterrupt(EASY_BUTTON, easy_interrupt, FALLING);
  //attachInterrupt(EASY_BUTTON, easy_keyup, RISING);
  pinMode(13, OUTPUT); //7 segment display-1 control
  pinMode(14, OUTPUT); //display-2 control
  pinMode(15, OUTPUT); //display-3 control
  pinMode(BUTTON2, INPUT_PULLUP); //SET_TIMER control
  pinMode(BUTTON1, INPUT_PULLUP); //SET_TIMER control
  attachInterrupt(BUTTON2, button2_interrupt, FALLING);
  attachInterrupt(BUTTON1, button1_interrupt, FALLING);
  writeDot(1);  // start with the "dot" off
  for (byte i = 2; i <=9; i++){
    digitalWrite(i, HIGH);
  }
  digitalWrite(13, HIGH); // 1
  digitalWrite(14, HIGH); // 2
  digitalWrite(15, HIGH); // 3
  set_display_number(countdown_time);
//  Serial.begin(9600);
}

void easy_interrupt(){
  //rising
  if(digitalRead(EASY_BUTTON) == HIGH && easy_button_pressed){
//    Serial.println("keyup");
    mode_toggled = false;
    easy_button_pressed = false;
  }
  //falling
  else if(digitalRead(EASY_BUTTON) == LOW && easy_button_pressed == false){
//    Serial.println("keydown");
    countdown_timer.Set_Time( 0, 0, 0 );
    easy_button_hold_time = 0;
    easy_button_pressed = true; 
  }    
  
  //toggles the interrupt mode - clever trick from http://forum.43oh.com/topic/3657-interrupts-on-the-energia/
  P2IES=(P2IES & ~B00001000) | (P2IN & B00001000);
}

void button1_interrupt(){
  //rising
  if(digitalRead(BUTTON1) == HIGH && button1_pressed){
    button1_pressed = false;
  }
  //falling
  else if(digitalRead(BUTTON1) == LOW && button1_pressed == false){
    if(mode == SET_TIMER)
      increment_digit(selected_digit);
    button1_pressed = true; 
  }  
  
  P2IES=(P2IES & ~B01000000) | (P2IN & B01000000);    
}


void button2_interrupt(){
  //rising
  if(digitalRead(BUTTON2) == HIGH && button2_pressed){
//    Serial.println("button2_keyup");
    button2_pressed = false;}
//  }
  //falling
  else if(digitalRead(BUTTON2) == LOW && button2_pressed == false){
//    Serial.println("button2_keydown");
    if(mode == SET_TIMER){
      set_blink(selected_digit, 0);
      selected_digit = (selected_digit + 1) % 3;
      set_blink(selected_digit, 1);
    }
    button2_pressed = true;
  }
  P2IES=(P2IES & ~B10000000) | (P2IN & B10000000);
}

void toggle_mode(){
  if(mode == COUNTDOWN)
    mode = SET_TIMER;
  else
    mode = COUNTDOWN;
}

void writeDot(byte dot) {
  digitalWrite(5, dot);
}
    
void sevenSegWrite(byte digit_value) {
  byte pin = 2;
  for (byte segCount = 0; segCount < 8; ++segCount) {
      digitalWrite(pin, seven_seg_digits[digit_value][segCount]);
    ++pin;
  }
}

void clear_segments(){
  for (byte i = 2; i <=9; i++){
    digitalWrite(i, HIGH);
  }
}

void set_countdown_time(){//sets countdown time to current display number
  countdown_time = get_display_number();
  Flash.erase(flash);
  Flash.write(flash, (unsigned char *)(&countdown_time), sizeof(int));
//save to flash once you figure out how to do that
}

int get_display_number(){
  int display_number = ones + tens*10 + hundreds*100;
  return display_number;
}

void write_display(byte digit){
  if(digit == ONES){
    digitalWrite(13, HIGH);
    digitalWrite(14, HIGH);
    digitalWrite(15, LOW);
    if(digit_blink_state[ONES] == true && display_timer.RTC_chunk % blink_rate < (blink_rate/2))
      sevenSegWrite(BLANK);
    else
      sevenSegWrite(ones);    
  }
  else if(digit == TENS){
    digitalWrite(13, HIGH);
    digitalWrite(15, HIGH);
    digitalWrite(14, LOW);
    if(digit_blink_state[TENS] == true && display_timer.RTC_chunk % blink_rate < (blink_rate/2))
      sevenSegWrite(BLANK);
    else    
      sevenSegWrite(tens);    
  }
  else if(digit == HUNDREDS){
    digitalWrite(15, HIGH);
    digitalWrite(14, HIGH);
    digitalWrite(13, LOW);
    if(digit_blink_state[HUNDREDS] == true && display_timer.RTC_chunk % blink_rate < (blink_rate/2))
      sevenSegWrite(BLANK);
    else    
      sevenSegWrite(hundreds);
  }
  else return;
}



RTC_ISR(void){
   display_timer.Inc();
   countdown_timer.Inc();
   easy_button_hold_time++;
   if(easy_button_hold_time > hold_time_threshold && digitalRead(EASY_BUTTON) == LOW && mode_toggled == false){
     toggle_mode();
     mode_toggled = true;
   }
   int time = display_timer.RTC_chunk;
   byte digit = time % 3; // there are 3 7 segment LEDs and I want them to fire in sequence
   write_display(digit);
}

void increment_digit(byte digit){
      switch(digit)
      {
        case ONES:    
          ones = (ones + 1) % 10;
          break;
        case TENS:
          tens = (tens + 1) % 10;
          break;
        case HUNDREDS:
          hundreds = (hundreds + 1) % 10;
          break;
      }
}

void modify_display_number(int amount){
//  if(amount > 999 || amount < 1)
//    return;
//  else{
    int display_number = (ones + tens*10 + hundreds*100 + amount) % 1000;
    hundreds = (display_number % 1000) / 100;
    tens = (display_number % 100) / 10;
    ones = display_number % 10;
//  }
}

void set_display_number(int amount){
  if(amount > 999 || amount < 0)
    return;
  else{
    hundreds = (amount % 1000) / 100;
    tens = (amount % 100) / 10;
    ones = amount % 10;
  }
}

void buzzer(){
  int length = sizeof(melody)/sizeof(melody[0]);
  for (int thisNote = 0; thisNote < length; thisNote++ ){
    int noteDuration = 1000/timing[thisNote];
    tone(10, melody[thisNote],noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);    
    noTone(10);
  }
}

void set_timer(){
  set_blink(selected_digit, 1);
  set_display_number(countdown_time);  
  while(mode == SET_TIMER){
//    modify_digit(digit);
  }
  set_blink(selected_digit, 0);  
  set_countdown_time();
}

void set_blink(byte digit, byte state){
  digit_blink_state[digit] = state;
}

void countdown(){
//  Serial.println("Countdown begin.");
  countdown_timer.Set_Time(0, 0, 0);
  while(mode == COUNTDOWN){
    while(countdown_time >= countdown_timer.RTC_sec && mode == COUNTDOWN){
      set_display_number( countdown_time - countdown_timer.RTC_sec);     
//      Serial.println(countdown_timer.RTC_sec);
    }
    if(mode == COUNTDOWN)
      buzzer();
    //buzzer
    while(digitalRead(EASY_BUTTON) == HIGH && mode == COUNTDOWN){
      //waiting for button press
    }
  }
}

void loop() {
  switch(mode){
    case SET_TIMER:
      set_timer();
      break;
    case COUNTDOWN:
      countdown();
      break;
  }
}


