#include <Wire.h>
#include "I2Cdev.h"
#include "MPU6050.h"

// object for reading accelerometer
MPU6050 accelgyro;
int16_t accelX, accelY, accelZ;
int accel;

// used for interfacing
#define SPEAKER_PIN 8
#define LED_PIN 10
const int sineTableSize = 28;

// used to account for rest acceleration
int restState;

// used to control pitch modulation
const int speed = 100;
int period = 200;
const int sensitivity = 300;
const int gyroSensitivity = 2000;

// used to construct sine waves
int sineTable[sineTableSize] = {
  127, 151, 176, 198, 219, 236, 250, 255, 
  250, 236, 219, 198, 176, 151, 127, 102, 
  77, 55, 34, 17, 3, 0, 3, 17, 
  34, 55, 77, 102
};

void setup() {
  // = allows for console output
  Serial.begin(9600);
  
  // controls IO for both speaker and accelerometer
  Wire.begin();

  // allow LED
  pinMode(LED_PIN, OUTPUT);

  // initialize accelerometer
  Serial.println("Intializing accelerometer...");
  accelgyro.initialize();
  Serial.println("Testing connection...");
  Serial.println(accelgyro.testConnection() ? "MPU6050 successfully connected." : "Failed to connect MPU6050.");

  // calculates resting acceleration for offset
  Serial.println("Calibrating accelerometer; please do not move for five seconds.");
  digitalWrite(LED_PIN, 1);
  float runningTotal;
  for(int i=0; i < 50; i++){
    // get average accelerations in each axis and find magnitude
    accelgyro.getAcceleration(&accelX, &accelY, &accelZ);
    runningTotal = (runningTotal*i + sqrt(pow(accelX, 2) + pow(accelY, 2) + pow(accelZ, 2)))/(i+1);
    delay(50);
  }
  // rest acceleration is removed from future measurements
  restState = runningTotal;
  Serial.print("Calibration complete. Resting state: ");
  Serial.println(restState);
}

void loop() {
  // get acceleration
  accelgyro.getAcceleration(&accelX, &accelY, &accelZ);
  // rest by orienting certain way
  int16_t gx, gy, gz;
  accelgyro.getAcceleration(&gx, &gy, &gz);
  //Serial.println(gx);
  bool disable = -gyroSensitivity <  gx && gx < gyroSensitivity && -gyroSensitivity < gy && gy < gyroSensitivity ? 1 : 0;
  accel = max(sqrt(pow(accelX, 2) + pow(accelY, 2) + pow(accelZ, 2)) - restState, 0);

  // gradually change frequency after minimum acceleration reached; otherwise, hold pitch
  if (accel >= sensitivity){
    period = (period*(speed - 1) + 200000/accel)/speed;
  }
  // error catching
  if (period < 0){
    period = 1;
  }
  // every main loop, constructs a sine wave using the sine table incrementally over one period
  for (int i = 0; i < sineTableSize; i++) {
    int value = sineTable[i];
    if(!disable){
      analogWrite(SPEAKER_PIN, value);
    }
    delayMicroseconds(period); // determines frequency
  }
}
