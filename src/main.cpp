/*
author: Axel Schmidt, axel.sebastian.schmidt@rwth-aachen.de
*/
#include <Arduino.h>
#include <AccelStepper.h>
#include <Servo.h>

#define limitSwitch 11 // limit switch pin
// #define DRY_TESTING    // uncomment to enable dry testing without motors and switchs
// #define DISPLAY_2004A // uncomment to enable display and encoder KY040

String machineName = "Blechbieger ITA";
String version = "0.2.0";

// Define variables

int benderOffsetToSwitch = 450; // in steps

int benderMaxSpeed = 50; // in steps/s
int feederMaxSpeed = 50; // in steps/s
int bendSpeed = 50;      // in steps/s

int timeoutTime = 3000;           // in ms
int waitBeforeBendingBack = 1000; // in ms

// End of variables

// Define the stepper motors and the pins the will use
AccelStepper feederStepper(1, 5, 6); // (Type:driver, STEP, DIR)
AccelStepper benderStepper(1, 9, 10);

#ifdef DISPLAY_2004A
// LCD display
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 20 chars and 4 line display

// Initialisation of the encoder variables
int Counter = 0;
int Pin_clk_last;
int Pin_clk_current;

// Definition of the encoder pins
int pin_clk = 2;    // clock pin
int pin_dt = 3;     // data pin
int button_pin = 4; // button pin

// Definition of button
int button = 7;      // Digital Pin 7
int buttonState = 0; // 0 = button not pressed, 1 = button pressed

#endif

enum modes
{
  angle,
  steps
};
#ifdef DISPLAY_2004A
enum encodePositions
{
  length3,
  length2,
  length1,
  length0,
  length1_,
  angle3,
  angle2,
  angle1,
  angle0,
  angle1_,
};
encodePositions encodePosition;
#endif
modes mode;
String dataIn = "";
String confirmation = "";
float lengthFloat = 0.0;
float angleFloat = 0.0;
int lengthSteps = 0;
int angleSteps = 0;

int angleToSteps(float angle)
{
  // Array of angles from 0 to 90 degrees in 5 degree steps
  float angles[8] = {2, 20, 37.5, 41.3, 42.3, 70, 90};

  // Array of 19 values for steps correlated to the angles
  int steps[8] = {150, 200, 300, 324, 334, 376, 400, 500};

  for (int i = 0; i < 9; i++)
  {
    if (angle <= angles[i])
    {
      // map the angle to the steps array and return the value
      return (int)map(angle, angles[i - 1], angles[i], steps[i - 1], steps[i]);
    }
  }
  return 0;
}

int lengthToSteps(float length)
{
  // Map the length to the steps
  return map(length, 0, 100, 0, 180);
}

void bend(int angleSteps, int lengthSteps)
{
  if (angleSteps < 0 || lengthSteps < 0)
  {
    Serial.println("Angle or length is negative. Please check your input.");
    return;
  }
#ifdef DISPLAY_2004A
  lcd.setCursor(0, 3);
  lcd.print("Start Bending       ");

  Serial.println("Start Bending");
#endif
#ifndef DRY_TESTING
  // Move the feeder stepper motor to the length
  Serial.println("Feeding...");
  while (feederStepper.currentPosition() != lengthSteps)
  {
    feederStepper.setSpeed(bendSpeed);
    feederStepper.run();
  }
  feederStepper.setCurrentPosition(0);

  // Move the bender stepper motor to the angle
  Serial.println("Bending...");
  while (benderStepper.currentPosition() != angleSteps)
  {
    benderStepper.setSpeed(bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }

  delay(waitBeforeBendingBack); // wait before bending back

  // Move the bender stepper motor back to 0
  Serial.println("Bending back...");
  while (benderStepper.currentPosition() != 0)
  {
    benderStepper.setSpeed(-bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }
#else
#ifdef DISPLAY_2004A
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("Dry: Feeding...     ");
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("Dry: Bending...     ");
  delay(1000);
  lcd.setCursor(0, 3);
  lcd.print("Dry: Bending back...");
  delay(1000);
#endif
#endif

  Serial.println("");
}

bool angleValid(float angle)
{
  if (angle >= 0 && angle <= 90)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool lengthValid(float length)
{
  if (length >= 0)
  {
    return true;
  }
  else
  {
    return false;
  }
}

#ifdef DISPLAY_2004A
void incrementValueAngleMode()
{
  if (encodePosition == length3)
  {
    lengthFloat += 50;
  }
  else if (encodePosition == length2)
  {
    lengthFloat += 50;
  }
  else if (encodePosition == length1)
  {
    lengthFloat += 5;
  }
  else if (encodePosition == length0)
  {
    lengthFloat += 0.5;
  }
  else if (encodePosition == length1_)
  {
    lengthFloat += 0.05;
  }
  else if (encodePosition == angle1)
  {
    angleFloat += 5;
  }
  else if (encodePosition == angle0)
  {
    angleFloat += 0.5;
  }
  else if (encodePosition == angle1_)
  {
    angleFloat += 0.05;
  }
}

void decrementValueAngleMode()
{
  if (encodePosition == length3)
  {
    lengthFloat -= 50;
  }
  else if (encodePosition == length2)
  {
    lengthFloat -= 50;
  }
  else if (encodePosition == length1)
  {
    lengthFloat -= 5;
  }
  else if (encodePosition == length0)
  {
    lengthFloat -= 0.5;
  }
  else if (encodePosition == length1_)
  {
    lengthFloat -= 0.05;
  }
  else if (encodePosition == angle1)
  {
    angleFloat -= 5;
  }
  else if (encodePosition == angle0)
  {
    angleFloat -= 0.5;
  }
  else if (encodePosition == angle1_)
  {
    angleFloat -= 0.05;
  }
}

void incrementValueStepsMode()
{
  if (encodePosition == length3)
  {
    lengthSteps += 500;
  }
  else if (encodePosition == length2)
  {
    lengthSteps += 50;
  }
  else if (encodePosition == length1)
  {
    lengthSteps += 5;
  }
  else if (encodePosition == length0)
  {
    lengthSteps += 0.5;
  }
  else if (encodePosition == angle3)
  {
    angleSteps += 500;
  }
  else if (encodePosition == angle2)
  {
    angleSteps += 50;
  }
  else if (encodePosition == angle1)
  {
    angleSteps += 5;
  }
  else if (encodePosition == angle0)
  {
    angleSteps += 0.5;
  }
}

void decrementValueStepsMode()
{
  if (encodePosition == length3)
  {
    lengthSteps -= 500;
  }
  else if (encodePosition == length2)
  {
    lengthSteps -= 50;
  }
  else if (encodePosition == length1)
  {
    lengthSteps -= 5;
  }
  else if (encodePosition == length0)
  {
    lengthSteps -= 0.5;
  }
  else if (encodePosition == angle3)
  {
    angleSteps -= 500;
  }
  else if (encodePosition == angle2)
  {
    angleSteps -= 50;
  }
  else if (encodePosition == angle1)
  {
    angleSteps -= 5;
  }
  else if (encodePosition == angle0)
  {
    angleSteps -= 0.5;
  }
}

void displayAngleMode()
{
  // Clear the LCD
  lcd.clear();
  // write the current values of lengthFloat and angleFloat to LCD
  lcd.setCursor(0, 0);
  lcd.print("length: ");
  lcd.print(lengthFloat);
  lcd.print(" mm");
  lcd.setCursor(0, 1);
  lcd.print("angle:  ");
  lcd.print(angleFloat);
  lcd.print(" deg");
  lcd.setCursor(0, 3);
  lcd.print("Press A to bend");
}

void displayStepsMode()
{
  // Clear the LCD
  lcd.clear();
  // write the current values of lengthSteps and angleSteps to LCD
  lcd.setCursor(0, 0);
  lcd.print("length: ");
  lcd.print(lengthSteps);
  lcd.print(" stp");
  lcd.setCursor(0, 1);
  lcd.print("angle:  ");
  lcd.print(angleSteps);
  lcd.print(" stp");
  lcd.setCursor(0, 3);
  lcd.print("Press A to bend");
}

void changeEncodePosition()
{
  if (mode == angle)
  {
    if (encodePosition == length2 || encodePosition == length3)
    {
      encodePosition = length1;
    }
    else if (encodePosition == length1)
    {
      encodePosition = length0;
    }
    else if (encodePosition == length0)
    {
      encodePosition = length1_;
    }
    else if (encodePosition == length1_)
    {
      encodePosition = angle1;
    }
    else if (encodePosition == angle1)
    {
      encodePosition = angle0;
    }
    else if (encodePosition == angle0)
    {
      encodePosition = angle1_;
    }
    else if (encodePosition == angle1_)
    {
      encodePosition = length2;
    }
  }
  else if (mode == steps)
  {
    if (encodePosition == length3)
    {
      encodePosition = length2;
    }
    else if (encodePosition == length2)
    {
      encodePosition = length1;
    }
    else if (encodePosition == length1)
    {
      encodePosition = length0;
    }
    else if (encodePosition == length0)
    {
      encodePosition = angle3;
    }
    else if (encodePosition == angle3)
    {
      encodePosition = angle2;
    }
    else if (encodePosition == angle2)
    {
      encodePosition = angle1;
    }
    else if (encodePosition == angle1)
    {
      encodePosition = angle0;
    }
    else if (encodePosition == angle0)
    {
      encodePosition = length3;
    }
  }
}
#endif

void stepMode()
{

#ifdef DISPLAY_2004A
  // Display the current value of lengthSteps and angleSteps
  displayStepsMode();

  // read data from encoder and display it on the LCD
  while (true)
  {
    // Read the current status of the encoder
    Pin_clk_current = digitalRead(pin_clk);

    // Check if the encoder has changed
    if (Pin_clk_current != Pin_clk_last)
    {
      if (digitalRead(pin_dt) != Pin_clk_current)
      {
        // Pin_CLK has changed first
        incrementValueStepsMode();
      }
      else
      {
        // Else the Pin_DT has changed first
        decrementValueStepsMode();
      }
      // Display the new value on the LCD
      displayStepsMode();
    }
    // Prepare for the next loop:
    // The value of the current loop is the previous value at the next loop
    Pin_clk_last = Pin_clk_current;

    // change the encodePosition to the next position if the encoder is pressed
    if (!digitalRead(button_pin))
    {
      changeEncodePosition();
      delay(200);
    }
    // if button is pressed, exit value setting mode
    buttonState = digitalRead(button);
    if (buttonState == HIGH)
    {
      confirmation = "y";
      break;
    }
  }

#else
  // Read data from serial port. "lengthSteps" as int, "angleSteps" as int, separated by a comma
  // Ask user to enter data in the form of "lengthSteps,angleSteps"
  Serial.println("Enter data in the form of 'lengthSteps,angleSteps'");

  // Read the data from the serial port
  while (Serial.available() == 0)
  {
  }
  dataIn = Serial.readStringUntil('\n');

  // Split the data into two strings, separated by a comma
  String lengthString = dataIn.substring(0, dataIn.indexOf(","));
  String angleString = dataIn.substring(dataIn.indexOf(",") + 1);

  // Convert the strings to ints
  int lengthSteps = lengthString.toInt();
  int angleSteps = angleString.toInt();

#endif
  // Confirm the data is correct
  Serial.println("Length: " + String(lengthSteps) + " steps");
  Serial.println("Angle: " + String(angleSteps) + " steps");
  Serial.println("");

#ifndef DISPLAY_2004A
  // Ask user if the data is correct
  Serial.println("Is this data correct? (y/n)");
  while (Serial.available() == 0)
  {
  }
  Serial.setTimeout(100);
  confirmation = Serial.readString();
  Serial.setTimeout(timeoutTime);
  Serial.println(confirmation);
  Serial.println("");

#endif

  // If confirmation is "y" call bending function with the angle and length
  if (confirmation.startsWith("y"))
  {
    bend(angleSteps, lengthSteps);
  }
}

void angleMode()
{
#ifdef DISPLAY_2004A
  // Display the current values of lengthFloat and angleFloat to LCD
  displayAngleMode();

  // read data from encoder and display it on the LCD
  while (true)
  {
    // Read current status of encoder
    Pin_clk_current = digitalRead(pin_clk);

    // Check if the encoder has changed
    if (Pin_clk_current != Pin_clk_last)
    {
      if (digitalRead(pin_dt) != Pin_clk_current)
      {
        // Pin_CLK has changed first
        incrementValueAngleMode();
      }
      else
      {
        // Else if Pin_DT has changed first
        decrementValueAngleMode();
      }
      // Display the new value on the LCD
      displayAngleMode();
    }

    // Prepare for the next loop: The value of the current loop is the previous value
    Pin_clk_last = Pin_clk_current;

    // change the encodePosition to the next position if the encoder is pressed
    if (!digitalRead(button_pin))
    {
      changeEncodePosition();
      delay(200);
    }
    // if button is pressed, exit value setting mode
    buttonState = digitalRead(button);
    if (buttonState == HIGH)
    {
      confirmation = "y";
      break;
    }
  }

#else
  // Read data from serial port. "length" as float, "angle" as float, separated by commas.
  // Ask user to enter data in the form of "length,angle"
  Serial.println("Enter data in the form of 'length,angle'");

  // Read data from serial port.
  while (Serial.available() == 0)
  {
  }
  dataIn = Serial.readStringUntil('\n');

  // Split the data into separate variables
  String length = dataIn.substring(0, dataIn.indexOf(","));
  String angle = dataIn.substring(dataIn.indexOf(",") + 1);

  // Convert the strings to floats
  lengthFloat = length.toFloat();
  angleFloat = angle.toFloat();

#endif
  if (angleValid(angleFloat) && lengthValid(lengthFloat))
  {

    // Confirm the data is correct
    Serial.println("Length: " + String(lengthFloat) + " mm");
    Serial.println("Angle: " + String(angleFloat) + " degrees");
    Serial.println("");

#ifndef DISPLAY_2004A
    // Ask user if the data is correct
    Serial.println("Is this data correct? (y/n)");
    while (Serial.available() == 0)
    {
    }
    Serial.setTimeout(100);
    confirmation = Serial.readString();
    Serial.setTimeout(timeoutTime);
    Serial.println(confirmation);
    Serial.println("");
#endif
    // Get angleSteps from mapping the angle to the number of steps with function angleToSteps
    int angleSteps = angleToSteps(angleFloat);
    Serial.println("Angle steps: " + String(angleSteps));

    // Convert the length to steps
    int lengthSteps = lengthToSteps(lengthFloat);
    Serial.println("Length steps: " + String(lengthSteps));

    // If confirmation is "y" call bending function with the angle and length
    if (confirmation.startsWith("y"))
    {
      bend(angleSteps, lengthSteps);
    }
  }
  else
  {
#ifdef DISPLAY_2004A
    lcd.setCursor(0, 3);
    lcd.print("Invalid angle or length");
#endif
    Serial.println("Invalid angle or length");
  }
}

#ifdef DISPLAY_2004A
void lcdWait(int seconds)
{
  int time = seconds; // seconds
  while (true)
  {
    lcd.setCursor(18, 3);
    if (time < 10)
    {
      lcd.print(" ");
    }
    lcd.print(String(time));
    delay(1000);
    time--;
    if (time == 0)
    {
      break;
    };
  }
}
#endif

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(timeoutTime);
  pinMode(limitSwitch, INPUT_PULLUP);

#ifdef DISPLAY_2004A
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print(machineName);
  lcd.setCursor(0, 2);
  lcd.print("Version: " + version);
  lcdWait(2);
  lcd.clear();

  // Input-Pins are initialized...
  pinMode(pin_clk, INPUT);
  pinMode(pin_dt, INPUT);
  pinMode(button_pin, INPUT);
  pinMode(button, INPUT);

  // ...and the pull-up resistors are enabled
  digitalWrite(pin_clk, true);
  digitalWrite(pin_dt, true);
  digitalWrite(button_pin, true);

  // Initiales Auslesen des Pin_CLK
  // Initial reading of Pin_CLK
  Pin_clk_last = digitalRead(pin_clk);

#endif

  // Print Machine Name and Version
  Serial.println("");
  Serial.println("Machine Name: " + machineName);
  Serial.println("Version: " + version);
  Serial.println("");

#ifdef DISPLAY_2004A
  // Print select mode of operation to LCD
  lcd.setCursor(0, 0);
  lcd.print("Select Mode:");
  lcd.setCursor(2, 1);
  lcd.print("1. Step Mode");
  lcd.setCursor(2, 2);
  lcd.print("2. Angle Mode");

  while (true)
  {
    // Read the current status of the encoder
    Pin_clk_current = digitalRead(pin_clk);

    // check if the encoder has changed
    if (Pin_clk_current != Pin_clk_last)
    {
      if (digitalRead(pin_dt) != Pin_clk_current)
      {
        // Pin_CLK has changed first
        Counter++;
      }
      else
      {
        // else Pin_DT has changed first
        Counter--;
      }
      if ((Counter) % 4 == 0)
      {
        mode = angle;
        lcd.setCursor(0, 1);
        lcd.print(" ");
        lcd.setCursor(0, 2);
        lcd.write(3);
      }

      if ((Counter + 2) % 4 == 0)
      {
        mode = steps;
        lcd.setCursor(0, 1);
        lcd.write(3);
        lcd.setCursor(0, 2);
        lcd.print(" ");
      }
    }

    // Preparation for the next run:
    // The value of the current run is the previous value of the next run
    Pin_clk_last = Pin_clk_current;

    // reset function to save the current position
    if (!digitalRead(button_pin))
    {
      break;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);

  if (mode == angle)
  {
    Serial.println("Angle Mode selected");
    lcd.print("Angle Mode selected");
  }
  else if (mode == steps)
  {
    Serial.println("Step Mode selected");
    lcd.print("Step Mode selected");
  }
  else
  {
    Serial.println("Error");
    lcd.print("Error");
  }
  lcdWait(2);
  lcd.clear();

#else

  // Select mode of operation. default is angle mode
  Serial.println("Select mode of operation. default is angle mode");
  Serial.println("1. Angle mode (Define angle in degree and length in mm)");
  Serial.println("2. Steps mode (Define angle in motor steps and length in motor steps)");
  Serial.print("Enter mode: ");
  while (Serial.available() == 0)
  {
    delay(1);
  }
  dataIn = Serial.readStringUntil('\n');
  int val = dataIn.toInt();
  if (val == 1)
  {
    Serial.println("Angle mode selected");
    mode = angle;
  }
  else if (val == 2)
  {
    Serial.println("Steps mode selected");
    mode = steps;
  }
  else
  {
    Serial.println("Non valid input. Angle mode selected");
    mode = angle;
  }
  Serial.println("");

#endif

#ifndef DRY_TESTING

  // Stepper motors max speed
  feederStepper.setMaxSpeed(feederMaxSpeed);
  benderStepper.setMaxSpeed(benderMaxSpeed);

  // Homing
  Serial.println("Homing bender stepper motor");
  Serial.println("");
  while (digitalRead(limitSwitch) != 0)
  {
    benderStepper.setSpeed(bendSpeed);
    benderStepper.runSpeed();
    benderStepper.setCurrentPosition(0); // When limit switch pressed set position to 0 steps
  }
  Serial.println("Endstop pressed.");
  delay(40);

  // Move number of steps from the limit switch to starting position
  while (benderStepper.currentPosition() != -benderOffsetToSwitch)
  {
    benderStepper.setSpeed(-bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }
  benderStepper.setCurrentPosition(0);
  feederStepper.setCurrentPosition(0);
  Serial.println("Homing complete");
#endif
}

void loop()
{
  // call function depending on mode of operation
  if (mode == angle)
  {
    Serial.println("Angle mode");
    angleMode();
  }
  else if (mode == steps)
  {
    Serial.println("Steps mode");
    stepMode();
  }
  else
  {
    Serial.println("Error: mode not set");
  }
}
