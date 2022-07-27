/*
author: Axel Schmidt, axel.sebastian.schmidt@rwth-aachen.de
*/
#include <Arduino.h>
#include <AccelStepper.h>
#include <Servo.h>

#define limitSwitch 11 // limit switch pin
#define DRY_TESTING    // uncomment to enable dry testing without motors and switchs
// #define DISPLAY_2004A     // uncomment to enable display and encoder KY040

String machineName = "Blechbieger ITA";
String version = "0.1.0";

// Define variables

int benderOffsetToSwitch = 1000; // in steps

int benderMaxSpeed = 200; // in steps/s
int feederMaxSpeed = 200; // in steps/s
int bendSpeed = 200;      // in steps/s

int timeoutTime = 3000;           // in ms
int waitBeforeBendingBack = 1000; // in ms

// End of variables

// Define the stepper motors and the pins the will use
AccelStepper feederStepper(1, 5, 6); // (Type:driver, STEP, DIR)
AccelStepper benderStepper(1, 9, 10);

#ifndef DISPLAY_2004A
// LCD display
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4); // set the LCD address to 0x27 for a 20 chars and 4 line display

// Initialisierung benötigter Variablen
int Counter = 0;
int Pin_clk_Letzter;
int Pin_clk_Aktuell;

// Definition der Eingangs-Pins
int pin_clk = 3;
int pin_dt = 4;
int button_pin = 5;
#endif

enum modes
{
  angle,
  steps
};
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
  int angles[19] = {0, 5, 10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90};

  // Array of 19 values for steps correlated to the angles
  int steps[19] = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160, 170, 180};

  for (int i = 0; i < 20; i++)
  {
    if (angle <= angles[i])
    {
      // map the angle to the steps array and return the value
      return map(angle, angles[i - 1], angles[i], steps[i - 1], steps[i]);
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

  Serial.println("Start Bending");
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

void stepMode()
{
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

  // Confirm the data is correct
  Serial.println("Length: " + String(lengthSteps) + " steps");
  Serial.println("Angle: " + String(angleSteps) + " steps");
  Serial.println("");

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

  // If confirmation is "y" call bending function with the angle and length
  if (confirmation.startsWith("y"))
  {
    bend(angleSteps, lengthSteps);
  }
}

void angleMode()
{
#ifndef DISPLAY_2004A
  // read data from encoder and display it on the LCD

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
  if (angleValid(angleFloat))
  {

    // Confirm the data is correct
    Serial.println("Length: " + String(lengthFloat) + " mm");
    Serial.println("Angle: " + String(angleFloat) + " degrees");
    Serial.println("");

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

    // Get angleSteps from mapping the angle to the number of steps with function angleToSteps
    int angleSteps = angleToSteps(angleFloat);

    // Convert the length to steps
    int lengthSteps = lengthToSteps(lengthFloat);

    // If confirmation is "y" call bending function with the angle and length
    if (confirmation.startsWith("y"))
    {
      bend(angleSteps, lengthSteps);
    }
  }
  else
  {
    Serial.println("Invalid angle");
  }
}

void lcdWait(int seconds)
{
  int time = seconds; // seconds
  while (true)
  {
    lcd.setCursor(18, 3);
    lcd.print(String(time));
    delay(1000);
    time--;
    if (time == 0)
    {
      break;
    };
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(timeoutTime);
  pinMode(limitSwitch, INPUT_PULLUP);

#ifndef DISPLAY_2004A
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print(machineName);
  lcd.setCursor(0, 2);
  lcd.print("Version: " + version);
  lcdWait(10);
  lcd.clear();

  // Eingangs-Pins werden initialisiert...
  pinMode(pin_clk, INPUT);
  pinMode(pin_dt, INPUT);
  pinMode(button_pin, INPUT);

  // ...und deren Pull-Up Widerstände aktiviert
  digitalWrite(pin_clk, true);
  digitalWrite(pin_dt, true);
  digitalWrite(button_pin, true);

  // Initiales Auslesen des Pin_CLK
  Pin_clk_Letzter = digitalRead(pin_clk);

#endif

  // Print Machine Name and Version
  Serial.println("");
  Serial.println("Machine Name: " + machineName);
  Serial.println("Version: " + version);
  Serial.println("");

#ifndef DISPLAY_2004A
  // Print select mode of operation to LCD
  lcd.setCursor(0, 0);
  lcd.print("Select Mode:");
  lcd.setCursor(2, 1);
  lcd.print("1. Step Mode");
  lcd.setCursor(2, 2);
  lcd.print("2. Angle Mode");

  while (true)
  {
    // Auslesen des aktuellen Statuses
    Pin_clk_Aktuell = digitalRead(pin_clk);
    // Überprüfung auf Änderung
    if (Pin_clk_Aktuell != Pin_clk_Letzter)
    {
      if (digitalRead(pin_dt) != Pin_clk_Aktuell)
      {
        // Pin_CLK hat sich zuerst verändert
        Counter++;
      }
      else
      { // Andernfalls hat sich Pin_DT zuerst verändert
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
    // Vorbereitung für den nächsten Druchlauf:
    // Der Wert des aktuellen Durchlaufs ist beim nächsten Druchlauf der vorherige Wert
    Pin_clk_Letzter = Pin_clk_Aktuell;

    // Reset-Funktion um aktuelle Position zu speichern
    if (!digitalRead(button_pin) && Counter != 0)
    {
      Counter = 0;
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
  lcdWait(3);
  lcd.clear();

#endif

#ifdef DISPLAY_2004A
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
  delay(40);

  // Move number of steps from the limit switch to starting position
  while (benderStepper.currentPosition() != -benderOffsetToSwitch)
  {
    benderStepper.setSpeed(-bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }
  benderStepper.setCurrentPosition(0);
  feederStepper.setCurrentPosition(0);
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
