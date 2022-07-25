#include <Arduino.h>
#include <AccelStepper.h>
#include <Servo.h>

#define limitSwitch 11
#define DRY_TESTING

String machineName = "Blechbieger ITA";
String version = "0.1.0";

// Define the stepper motors and the pins the will use
AccelStepper feederStepper(1, 5, 6); // (Type:driver, STEP, DIR)
AccelStepper benderStepper(1, 9, 10);

enum modes
{
  angle,
  steps
};
modes mode;
String dataIn = "";
String confirmation = "";

// Define bend speed
int bendSpeed = 1200;

int timeoutTime = 3000;

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

void bend(float angleSteps, float lengthSteps)
{
  Serial.println("Bending...");
  Serial.println("");
#ifndef DRY_TESTING
  // Move the feeder stepper motor to the length
  while (feederStepper.currentPosition() != lengthSteps)
  {
    feederStepper.setSpeed(bendSpeed);
    feederStepper.run();
  }
  feederStepper.setCurrentPosition(0);

  // Move the bender stepper motor to the angle
  while (benderStepper.currentPosition() != angleSteps)
  {
    benderStepper.setSpeed(bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }

  delay(400);

  // Move the bender stepper motor back to 0
  while (benderStepper.currentPosition() != 0)
  {
    benderStepper.setSpeed(-bendSpeed); // if negative rotates anti-clockwise
    benderStepper.run();
  }
#endif
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
  if (confirmation == "y")
  {
    bend(angleSteps, lengthSteps);
  }
}

void angleMode()
{
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
  float lengthFloat = length.toFloat();
  float angleFloat = angle.toFloat();

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
    if (confirmation == "y")
    {
      bend(angleSteps, lengthSteps);
    }
  }
  else
  {
    Serial.println("Invalid angle");
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.setTimeout(timeoutTime);
  pinMode(limitSwitch, INPUT_PULLUP);

  // Print Machine Name and Version
  Serial.println("");
  Serial.println("Machine Name: " + machineName);
  Serial.println("Version: " + version);
  Serial.println("");

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

#ifndef DRY_TESTING

  // Stepper motors max speed
  feederStepper.setMaxSpeed(2000);
  benderStepper.setMaxSpeed(2000);

  // Homing
  while (digitalRead(limitSwitch) != 0)
  {
    benderStepper.setSpeed(bendSpeed);
    benderStepper.runSpeed();
    benderStepper.setCurrentPosition(0); // When limit switch pressed set position to 0 steps
  }
  delay(40);

  // Move 1400 steps from the limit switch to starting position
  while (benderStepper.currentPosition() != -1400)
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
