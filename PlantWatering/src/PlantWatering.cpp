/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/Users/russe/Desktop/IoT/projects/PlantWatering/PlantWatering/src/PlantWatering.ino"
/*
 * Project PlantWatering
 * Description: Code to ensure my plant stays watered
 * Author: Russell Brazell
 * Date: 7-12-2021
 */

#include "key.h""
#include "IOTTimer.h"

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_SSD1306.h>
#include <Grove_Air_quality_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT.h" 
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h" 

// Pins
void setup();
void loop();
void pinSetUp();
void OLEDSetUp();
void moistureDisplay();
void bmeRead();
void publishReadings();
void MQTT_connect();
void runPump();
#line 22 "c:/Users/russe/Desktop/IoT/projects/PlantWatering/PlantWatering/src/PlantWatering.ino"
const int SOIL_PIN = A0;
const int PUMP_PIN = A4;

// Moisture
const int wetSoil = 2150;
const int moistSoil = 2420;
const int drySoil = 2750;

// OLED
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int SCREEN_ADDRESS = 0x3C;

int moisture;
int remotePump;

// BME
int bmeHumidity;
int bmeTemp;

float bmePressure;

String tempString;
String humString;
String pressureString;

// MQTT 
unsigned long last;
unsigned long lastTime;

String onlyDate;
String onlyTime;


TCPClient TheClient; 

Adafruit_SSD1306 display(-1);

Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 
Adafruit_MQTT_Publish mqttPubMoisture = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/plant-moisture");
Adafruit_MQTT_Publish mqttPubTemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/plant-temp");
Adafruit_MQTT_Publish mqttPubHum = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/plant-hum");
Adafruit_MQTT_Publish mqttPubPressure = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/plant-pressure");
Adafruit_MQTT_Subscribe mqttSubPump = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/pump-on-slash-off");

Adafruit_BME280 bme;

IOTTimer pumpTimer;
IOTTimer soakTimer;
IOTTimer connectTimer;

// setup() runs once, when the device is first turned on.
void setup() {
	Serial.begin(115200);
	 waitFor(Serial.isConnected, 10000); //wait for Serial Monitor to startup
	 WiFi.connect();
	 mqtt.subscribe(&mqttSubPump);
	 pinSetUp();
	 OLEDSetUp();
	 bme.begin();
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
	moistureDisplay();
	publishReadings();
}

void pinSetUp() {
	pinMode(SOIL_PIN, INPUT);
	pinMode(PUMP_PIN, OUTPUT);
}

void OLEDSetUp() {
	display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
	display.clearDisplay();
	display.setTextSize(1);
	display.setRotation(2);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.display();
	Time.zone(-6);
	Particle.syncTime();
}

void moistureDisplay() {
	moisture = analogRead(SOIL_PIN);
	onlyDate = Time.timeStr().substring(0, 10);
	onlyTime = Time.timeStr().substring(11, 19);
	display.setCursor(0, 0);
	display.printf("Date: %s\nTime: %s\nMoisture: %i\n", onlyDate.c_str(), onlyTime.c_str(), moisture);
	display.display();
	display.clearDisplay();
	runPump();
	Time.zone(-6);
	Particle.syncTime();
}

void bmeRead() {
	bmeTemp = bme.readTemperature();
	bmeHumidity = bme.readHumidity();
	bmePressure = bme.readPressure();
}

void publishReadings() {
	MQTT_connect();
	if ((millis() - last) > 120000) {
		Serial.printf("Pinging MQTT \n");
		if (!mqtt.ping())
		{
			Serial.printf("Disconnecting \n");
			mqtt.disconnect();
		}
		last = millis();
  }
  if ((millis() - lastTime > 30000)) {
  bmeRead();
	  if (mqtt.Update())
	  {
		  mqttPubTemp.publish(bmeTemp);
		  mqttPubHum.publish(bmeHumidity);
		  mqttPubPressure.publish(bmePressure);
		  mqttPubMoisture.publish(moisture);
	  } 
    lastTime = millis();
  }
}

void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.print("Connecting to MQTT... ");
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.printf("%s\n",(char *)mqtt.connectErrorString(ret));
       Serial.printf("Retrying MQTT connection in 5 seconds..\n");
       mqtt.disconnect();
	   connectTimer.startTimer(5000);
	   while (!connectTimer.isTimerReady());	   
  }
  Serial.printf("MQTT Connected!\n");
}

void runPump() {
	Serial.printf("Pump about to run\n");
	remotePump = atoi((char *)mqttSubPump.lastread);
	Serial.printf("Pump checked remote: %i\n", remotePump);
	if (moisture > drySoil || remotePump > 0  ) {	
		soakTimer.startTimer(5000); // TODO reset to 120000
		if (soakTimer.isTimerReady()){
			digitalWrite(PUMP_PIN, HIGH);
			pumpTimer.startTimer(250);
			while (!pumpTimer.isTimerReady());
			digitalWrite(PUMP_PIN, LOW);
			Serial.printf("Pump ran\n");
		}
	// TODO send text to say it watered
	}
}