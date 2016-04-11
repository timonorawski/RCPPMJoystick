/***
 * An Arduino Leonardo/Pro Micro version of a RC PPM to USB Joystick Converter - using Arduino Joystick Library.
 * 
 * Requires no modifications to the arduino core files.
 * 
 * Remix of single joystick example at: https://github.com/MHeironimus/ArduinoJoystickLibrary
 * and: https://github.com/voroshkov/Leonardo-USB-RC-Adapter/blob/master/Leonardo-USB-RC-Adapter.ino
 * 
 * Requires installation of single joystick library from: https://github.com/MHeironimus/ArduinoJoystickLibrary
 * 
 * Wiring:
 *  - transmitter trainer port center pole: Arduino D4
 *  - transmitter trainer port barrel: Arduino GND
 * 
 * by Timon Orawski
 * 
 * 2016-04-11
 */

#include <Joystick.h>

#include <avr/interrupt.h>

// Use for Futaba transmitters (they have shifted center value and narrower range by default)
//#define FUTABA

// if you have a stick that isn't centred at 1500ppm, set your center below
#define CUSTOM_STICK_CENTER 1450

// if any of your controls are inverted, comment/uncomment the lines below
#define INVERT_THROTTLE
#define INVERT_PITCH
//#define INVERT_ROLL
//#define INVERT_YAW

// Use to enable output of PPM values to serial
// #define SERIALOUT

#define RC_CHANNELS_COUNT 6

#ifdef FUTABA
#define STICK_HALFWAY 450
#define STICK_CENTER 1530
#define THRESHOLD 200
#else
#ifdef CUSTOM_STICK_CENTER
#define STICK_CENTER CUSTOM_STICK_CENTER
#else
#define STICK_CENTER 1500
#endif
#define STICK_HALFWAY 500
#define THRESHOLD 100 // threshold is used to detect PPM values (added to range at both ends)
#endif

#define USB_STICK_VALUE_MIN -126
#define USB_STICK_VALUE_MAX 126

#define USB_STICK_ROTATION_VALUE_MIN 0
#define USB_STICK_ROTATION_VALUE_MAX 359

#define MIN_PULSE_WIDTH (STICK_CENTER - STICK_HALFWAY)
#define MAX_PULSE_WIDTH (STICK_CENTER + STICK_HALFWAY)
#define NEWFRAME_PULSE_WIDTH 3000

// timer capture ICP1 pin corresponds to Leonardo digital pin 4
#define PPM_CAPTURE_PIN 4
#define LED_PIN 13

// for timer prescaler set to 1/8 of 16MHz, counter values should be
//  divided by 2 to get the number of microseconds
#define TIMER_COUNT_DIVIDER 2

// this array contains the lengths of read PPM pulses in microseconds
volatile uint16_t rcValue[RC_CHANNELS_COUNT];

// enum defines the order of channels
enum {
  ROLL,
  PITCH,
  THROTTLE,
  YAW,
  AUX1,
  AUX2
};

void setup() {
  setupPins();
  initTimer();
  // Initialize Joystick Library
  Joystick.begin(true);
}

// Constant that maps the phyical pin to the joystick button.
const int pinToButtonMap = 9;

// Last state of the button
int lastButtonState[4] = {0,0,0,0};

void loop() {
  setControllerDataJoystick();

  //Joystick.sendState();
  delay(5);
}

void setControllerDataJoystick() {
  Joystick.setXAxis(
    #ifdef INVERT_YAW
      -1*
    #endif
    stickValue(rcValue[YAW]));
  Joystick.setYAxis(
    #ifdef INVERT_THROTTLE
      -1*
    #endif
    stickValue(rcValue[THROTTLE]));
  Joystick.setXAxisRotation(
    #ifdef INVERT_ROLL
      360 -
    #endif
    stickRotationValue(rcValue[ROLL]));
  Joystick.setYAxisRotation(
    #ifdef INVERT_PITCH
      360 -
    #endif
    stickRotationValue(rcValue[PITCH]));
  Joystick.setButton(0, rcValue[AUX1] > STICK_CENTER);
  Joystick.setButton(1, rcValue[AUX2] > STICK_CENTER);
}

void setupPins(void) {
  // Set up the Input Capture pin
  pinMode(PPM_CAPTURE_PIN, INPUT);
  digitalWrite(PPM_CAPTURE_PIN, 1); // enable the pullup
  pinMode(LED_PIN, OUTPUT);
}

void initTimer(void) {
  // Input Capture setup
  // ICNC1: =0 Disable Input Capture Noise Canceler to prevent delay in reading
  // ICES1: =1 for trigger on rising edge
  // CS11: =1 set prescaler to 1/8 system clock (F_CPU)
  TCCR1A = 0;
  TCCR1B = (0 << ICNC1) | (1 << ICES1) | (1 << CS11);
  TCCR1C = 0;

  // Interrupt setup
  // ICIE1: Input capture
  TIFR1 = (1 << ICF1); // clear pending
  TIMSK1 = (1 << ICIE1); // and enable
}

// Convert a value in the range of [Min Pulse - Max Pulse] to [0 - USB_STICK_VALUE_MAX]
char stickValue(uint16_t rcVal) {
  return (char)constrain(
           map(rcVal - MIN_PULSE_WIDTH,
               0, MAX_PULSE_WIDTH - MIN_PULSE_WIDTH,
               USB_STICK_VALUE_MIN, USB_STICK_VALUE_MAX
              ),
           USB_STICK_VALUE_MIN, USB_STICK_VALUE_MAX
         );
}
int stickRotationValue(uint16_t rcVal) {
  return (int)constrain(
           map(rcVal - MIN_PULSE_WIDTH,
               0, MAX_PULSE_WIDTH - MIN_PULSE_WIDTH,
               USB_STICK_ROTATION_VALUE_MIN, USB_STICK_ROTATION_VALUE_MAX
              ),
           USB_STICK_ROTATION_VALUE_MIN, USB_STICK_ROTATION_VALUE_MAX
         );
}

uint16_t adjust(uint16_t diff, uint8_t chan) {
  // Here you can trim your rc values (e.g. if TX has no trims).

  // switch (chan) {
  //   case THROTTLE: return diff + 50;
  //   case YAW:      return diff + 60;
  //   case PITCH:    return diff + 60;
  //   case ROLL:     return diff + 90;
  //   case AUX1:     return diff + 10;
  // }

  //convert to microseconds (because of timer prescaler usage)
  return diff / TIMER_COUNT_DIVIDER;
}
ISR(TIMER1_CAPT_vect) {
  union twoBytes {
    uint16_t word;
    uint8_t  byte[2];
  } timeValue;

  uint16_t now, diff;
  static uint16_t last = 0;
  static uint8_t chan = 0;

  timeValue.byte[0] = ICR1L;    // grab captured timer value (low byte)
  timeValue.byte[1] = ICR1H;    // grab captured timer value (high byte)

  now = timeValue.word;
  diff = now - last;
  last = now;

  //all numbers are microseconds multiplied by TIMER_COUNT_DIVIDER (as prescaler is set to 1/8 of 16 MHz)
  if (diff > (NEWFRAME_PULSE_WIDTH * TIMER_COUNT_DIVIDER)) {
    chan = 0;  // new data frame detected, start again
  }
  else {
    if (diff > (MIN_PULSE_WIDTH * TIMER_COUNT_DIVIDER - THRESHOLD)
        && diff < (MAX_PULSE_WIDTH * TIMER_COUNT_DIVIDER + THRESHOLD)
        && chan < RC_CHANNELS_COUNT)
    {
      rcValue[chan] = adjust(diff, chan); //store detected value
    }
    chan++; //no value detected within expected range, move to next channel
  }
}
