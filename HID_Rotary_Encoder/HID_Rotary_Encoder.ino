/* Code for turning a rotary encoder with an Arduino Pro Micro into a HID device.
 * Based on this project by Prusa: https://blog.prusa3d.com/3d-print-an-oversized-media-control-volume-knob-arduino-basics_30184/
 *
 * Features:
 * - Seperate actions for rotating with and without the button held
 * - Seperate actions for clicking, double clicking and holding the button
 * - Acceleration increases the response per click when spinning faster
 * - Handles encoders with multiple steps per click
 * 
 * Default mapping:
 * - Rotation without holding changes volume
 * - Rotating while holding steps frame-by-frame in a paused YouTube video
 * - Clicking pauses media, double-clicking skips media, holding returns to previous media
 *
 * By Marro64, 2021-2024
 */

#include <ClickEncoder.h>
#include <TimerOne.h>
#include <HID-Project.h>

// Settings
#define ENCODER_CLK A1
#define ENCODER_DT A2
#define ENCODER_SW A3
const bool Acceleration = HIGH;
const uint8_t stepsPerNotch = 4;

ClickEncoder *encoder; // variable representing the rotary encoder
int16_t valueOpen, valueClosed, change; // variables for current and last rotation value
bool buttonState; // The current state of the button, LOW = released and HIGH = pressed
ClickEncoder::Button bOld; // Store the old state of the button to check if it changed
bool ignoreButton; // Ignore updates in the button state if we hold and turn

void timerIsr() {
  encoder->service(); // Callback for timer to regularly check for encoder changes
}

void setup() {
  Serial.begin(9600); // Opens the serial connection used for communication with the PC. 
  Consumer.begin(); // Initializes the media keyboard
  encoder = new ClickEncoder(ENCODER_DT, ENCODER_CLK, ENCODER_SW, stepsPerNotch); // Initializes the rotary encoder with the mentioned pins
  encoder->setAccelerationEnabled(Acceleration); //Sets acceleration
  

  Timer1.initialize(1000); // Initializes the timer, which the rotary encoder uses to detect rotation
  Timer1.attachInterrupt(timerIsr);
  bOld = encoder->getButton(); // Store the initial state of the button so we can compare for changes
  valueClosed = 0;
  valueOpen = 0;
  ignoreButton = false;
  buttonState = LOW;
} 

void loop() {  
  ClickEncoder::Button b = encoder->getButton(); // Asking the button for its current state
  buttonState = digitalRead(ENCODER_SW); // Also reading the button separately for hold+twist
  change = encoder->getValue(); // Asking the encoder for any changes in rotation

  if(buttonState) { // Logging the change in rotation to a variable depending on if the button is held
    valueOpen += change; // valueOpen represents the rotation yet to be applied while the button is unpressed (aka open-ciruit)
  } else {
    valueClosed += change; // ValueClosed represents the rotation yet to be applied while the button is pressed (aka closed-ciruit)
  }

  if(valueClosed > 0) {
    ignoreButton = true; // If rotation happened for this button press, do not execute any other actions mapped to the button
    Serial.print("valueClosed: ");
    Serial.println(valueClosed);
  }

  // Iterate over the valueClosed variable, increasing or decreasing the value until it is 0 executing the mapped command for every iteration
  while(valueClosed != 0) {
    if(valueClosed < 0) {
      valueClosed += 1;
      Keyboard.write(',');
    } else {
      valueClosed -= 1;
      Keyboard.write('.');
    }
  }

  if(valueOpen != 0) {
    Serial.print("valueOpen: ");
    Serial.println(valueOpen);
  }
  // Iterate over the valueOpen variable, increasing or decreasing the value until it is 0 executing the mapped command for every iteration
  while(valueOpen != 0) {
    if(valueOpen < 0) {
      valueOpen += 1;
      Consumer.write(MEDIA_VOLUME_DOWN);
    } else {
      valueOpen -= 1;
      Consumer.write(MEDIA_VOLUME_UP);
    }
  }
  
  // If the button has been released, no longer ignore it
  if(b != bOld && b == ClickEncoder::Open && buttonState == HIGH) {
    ignoreButton = false;
  }

  // Process changes in the buttonState if it is not ignored
  if(b != bOld && ignoreButton == false) {
    switch (b) {
        case ClickEncoder::Clicked: // Button was clicked once
          Serial.println("Button state changed to clicked");
          Consumer.write(MEDIA_PLAY_PAUSE);
        break;      
        
        case ClickEncoder::DoubleClicked: // Button was double clicked
          Serial.println("Button state changed to double clicked");
          Consumer.write(MEDIA_NEXT);
        break;      
  
        case ClickEncoder::Held: // Button is being held
          Serial.println("Button state changed to held");
          Consumer.write(MEDIA_PREVIOUS);
        break;
       
        case ClickEncoder::Released: // Button has been released
          Serial.println("Button state changed to released");
        break;

        case ClickEncoder::Open:
          Serial.println("Button state changed to open");
        break;  

        case ClickEncoder::Closed:
          Serial.println("Button state changed to closed");
        break;          

        case ClickEncoder::Pressed:
          Serial.println("Button state changed to pressed");
        break;  
     }
  }

  if(b!= bOld) {
    bOld = b;
  }
  
  delay(10);
}
