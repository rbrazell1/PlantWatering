/*
 * Project PlantWatering
 * Description: Code to ensure my plant stays watered
 * Author: Russell Brazell
 * Date: 7-12-2021
 */

#include <IOTTimer.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Grove_Air_quality_Sensor.h>
#include <Wire.h>
#include <Adafruit_BME280.h>

// Pins
const int PUMP_PIN = A4;
const int 

// setup() runs once, when the device is first turned on.
void setup() {
	Serial.begin(115200);

}

// loop() runs over and over again, as quickly as it can execute.
void loop() {

}