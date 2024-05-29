#include <Arduino.h>
#include <Servo.h>

// Motor pins
const int MOTOR_RIGHT_BACKWARD = 4;
const int MOTOR_LEFT_BACKWARD = 7;
const int MOTOR_RIGHT_FORWARD = 5;
const int MOTOR_LEFT_FORWARD = 6;

// Sensors configuration
const int NUM_SENSORS = 8;
const int SENSOR_PINS[NUM_SENSORS] = {A6, A7, A0, A2, A1, A3, A4, A5};
const int SENSOR_THRESHOLD = 576;
const int ECHO_PIN = 12;
const int TRIGGER_PIN = 8;
const int GRIPPER_PIN = 13;

// Miscellaneous variables
Servo servoGripper;
int isInputAllBlackDetectedStart = 0;
const int blackWaitTime = 100;
bool isInputAllBlackEventActive = false;

// Functions
void readSensors(bool sensorValues[]) {
  for (int i = 0; i < NUM_SENSORS; i++) {
    sensorValues[i] = analogRead(SENSOR_PINS[i]) > SENSOR_THRESHOLD;
  }
}

int calculateLinePosition(const bool sensorValues[]) {
  int position = 0;
  int numActiveSensors = 0;

  for (int i = 0; i < NUM_SENSORS; i++) {
    if (sensorValues[i]) {
      position += (i * 1000);
      numActiveSensors++;
    }
  }

  return numActiveSensors > 0 ? position / numActiveSensors - (NUM_SENSORS - 1) * 500 : 0;
}

void controlMotors(int position) {
  int baseSpeed = 255;  
  
  if (abs(position) > 300) { // For sharp turns
    if (position < 0) {
      analogWrite(MOTOR_RIGHT_FORWARD, baseSpeed);
      analogWrite(MOTOR_LEFT_FORWARD, LOW);  
    } else {
      analogWrite(MOTOR_RIGHT_FORWARD, LOW);  
      analogWrite(MOTOR_LEFT_FORWARD, baseSpeed);
    }
  } else if (abs(position) > 150) {  // For gentler turns
    analogWrite(MOTOR_RIGHT_FORWARD, baseSpeed - position / 2);
    analogWrite(MOTOR_LEFT_FORWARD, baseSpeed + position / 2);
  } else {  // For a straight path
    analogWrite(MOTOR_RIGHT_FORWARD, baseSpeed);
    analogWrite(MOTOR_LEFT_FORWARD, baseSpeed);
  }
}

void stopMotors() {
  analogWrite(MOTOR_RIGHT_FORWARD, LOW);
  analogWrite(MOTOR_LEFT_FORWARD, LOW);
  analogWrite(MOTOR_LEFT_BACKWARD, LOW);
  analogWrite(MOTOR_RIGHT_BACKWARD, LOW);
  servoGripper.write(180); 
}

double calculateDistanceFromObject() {
  digitalWrite(TRIGGER_PIN, LOW); 
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGGER_PIN, LOW);
  
  long timeTaken = pulseIn(ECHO_PIN, HIGH);
  return timeTaken * 0.034 / 2;
}

void checkForObject() 
{
  if (calculateDistanceFromObject() <= 5) 
  {
     servoGripper.write(0);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_RIGHT_BACKWARD, OUTPUT);
  pinMode(MOTOR_LEFT_BACKWARD, OUTPUT);
  pinMode(MOTOR_RIGHT_FORWARD, OUTPUT);
  pinMode(MOTOR_LEFT_FORWARD, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TRIGGER_PIN, OUTPUT);
  servoGripper.attach(GRIPPER_PIN);
  servoGripper.write(180);
}

void loop() {
  bool sensorValues[NUM_SENSORS];
  readSensors(sensorValues);

  // Check if all sensors detect black
  bool isInputAllBlack = true;
  for (int i = 0; i < NUM_SENSORS; i++) {
    if (!sensorValues[i]) {
      isInputAllBlack = false;
      break;
    }
  }

  // Handle an all black response from line sensors
  if (isInputAllBlack) {
    if (!isInputAllBlackEventActive) {
      isInputAllBlackEventActive = true;
      isInputAllBlackDetectedStart = millis();
    } else if (millis() - isInputAllBlackDetectedStart > blackWaitTime) {
      stopMotors();
    }
  } else {
    isInputAllBlackEventActive = false; 
    int position = calculateLinePosition(sensorValues);
    controlMotors(position);
    checkForObject();
  }
}
