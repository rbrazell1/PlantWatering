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

#include "key.h"
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
void dustSenor();
#line 22 "c:/Users/russe/Desktop/IoT/projects/PlantWatering/PlantWatering/src/PlantWatering.ino"
const int SOIL_PIN = A0;
const int PUMP_PIN = A4;
const int DUST_PIN = A1;
const int BUTTON_PIN = D5;
const int WATER_SENSOR_PIN = A3;

// Moisture
const int wetSoil = 2150;
const int moistSoil = 2420;
const int drySoil = 2750;
const int emptyTank = 1100;
const int halfTank = 1700;
const int fullTank = 2000;

// OLED
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 64;
const int SCREEN_ADDRESS = 0x3C;

int moisture;
int remotePump;
int buttonRead;
int waterLevel;

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

// Air sensors
int airQuality;
int dustCount;

// Dust sensor
unsigned long sampletime_ms = 30000;
unsigned long duration;
unsigned long starttime;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

TCPClient TheClient;

Adafruit_SSD1306 display(-1);

Adafruit_MQTT_SPARK mqtt(&TheClient, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Publish
Adafruit_MQTT_Publish mqttPubMoisture = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/plant-moisture");
Adafruit_MQTT_Publish mqttPubTemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/plant-temp");
Adafruit_MQTT_Publish mqttPubHum = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/plant-hum");
Adafruit_MQTT_Publish mqttPubPressure = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/plant-pressure");
Adafruit_MQTT_Publish mqttPubAQ = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/aq-level");
Adafruit_MQTT_Publish mqttPubDust = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/dust-count");
Adafruit_MQTT_Publish mqttPubTank = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME
"/feeds/water-level");

// Subscribe
Adafruit_MQTT_Subscribe mqttSubPump = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME
"/feeds/pump-on-slash-off");

Adafruit_BME280 bme;

IOTTimer pumpTimer;
IOTTimer soakTimer;
IOTTimer connectTimer;

AirQualitySensor airQualitySensor(A2);

// setup() runs once, when the device is first turned on.
void setup() {
    Serial.begin(115200);
    waitFor(Serial.isConnected, 10000); //wait for Serial Monitor to startup
    WiFi.connect();
    mqtt.subscribe(&mqttSubPump);
    pinSetUp();
    OLEDSetUp();
    bme.begin();
    airQualitySensor.init();
    soakTimer.startTimer(5000); // TODO reset to 120000
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    moistureDisplay();
    publishReadings();
}

void pinSetUp() {
    pinMode(SOIL_PIN, INPUT);
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(DUST_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(WATER_SENSOR_PIN, INPUT);
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
    waterLevel = analogRead(WATER_SENSOR_PIN);
    onlyDate = Time.timeStr().substring(0, 10);
    onlyTime = Time.timeStr().substring(11, 19);
    display.setCursor(0, 0);
    display.printf("Date: %s\nTime: %s\nMoisture: %i\n Water Level: %i\n", onlyDate.c_str(), onlyTime.c_str(), moisture, waterLevel);
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
        if (!mqtt.ping()) {
            Serial.printf("Disconnecting \n");
            mqtt.disconnect();
        }
        last = millis();
    }
    if ((millis() - lastTime > 30000)) {
        bmeRead();
        airQuality = airQualitySensor.getValue();
        dustSenor();
        if (mqtt.Update()) {
            mqttPubTemp.publish(bmeTemp);
            mqttPubHum.publish(bmeHumidity);
            mqttPubPressure.publish(bmePressure);
            mqttPubMoisture.publish(moisture);
            mqttPubAQ.publish(airQuality);
            mqttPubDust.publish(ratio);
            mqttPubTank.publish(waterLevel);
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
        Serial.printf("%s\n", (char *) mqtt.connectErrorString(ret));
        Serial.printf("Retrying MQTT connection in 5 seconds..\n");
        mqtt.disconnect();
        connectTimer.startTimer(5000);
        while (!connectTimer.isTimerReady());
    }
    Serial.printf("MQTT Connected!\n");
}

void runPump() {
    buttonRead = digitalRead(BUTTON_PIN);
    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription(100))) {
        if (subscription == &mqttSubPump) {
            remotePump = atoi((char *) mqttSubPump.lastread);
        }
    }
    if ((moisture > drySoil || remotePump > 0 || buttonRead == HIGH) && soakTimer.isTimerReady()) {
        digitalWrite(PUMP_PIN, HIGH);
        pumpTimer.startTimer(250);
        while (!pumpTimer.isTimerReady());
        digitalWrite(PUMP_PIN, LOW);
        Serial.printf("Pump ran\n");
        remotePump = 0;
        Serial.printf("starting soak timer\n");
        soakTimer.startTimer(5000); // TODO reset to 120000
    }
    // TODO send text to say it watered
}

void dustSenor() {
    duration = pulseIn(DUST_PIN, LOW);
    lowpulseoccupancy = lowpulseoccupancy + duration;
    if ((millis() - starttime) > sampletime_ms) {
        ratio = lowpulseoccupancy / (sampletime_ms * 10.0);  // Integer percentage 0=>100
        concentration = 1.1 * pow(ratio, 3) - 3.8 * pow(ratio, 2) + 520 * ratio + 0.62; // using spec sheet curve
        Serial.printf("Dust Concentration: %f\n", concentration);
        lowpulseoccupancy = 0;
        starttime = millis();
    }
}

