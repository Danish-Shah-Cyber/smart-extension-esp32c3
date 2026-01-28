#define BLYNK_PRINT Serial

#define BLYNK_TEMPLATE_ID "TMPL6urt08Gu7"
#define BLYNK_TEMPLATE_NAME "Smart Extension"
#define BLYNK_AUTH_TOKEN "FrZhXNhObwbtkdxU7k6MpyxAvl442wzr"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ------------ Blynk Credentials -------------
char auth[] = "FrZhXNhObwbtkdxU7k6MpyxAvl442wzr";
char ssid[] = "huawei-qtsb";
char pass[] = "12112121";

/************ PIN DEFINITIONS ************/
#define RELAY1 25
#define RELAY2 26
#define RELAY3 27

#define CURRENT_SENSOR 34                           // ACS712 output pin
#define TEMP_SENSOR 35                              // LM35 output pin

/************ SAFETY LIMITS ************/
float CURRENT_LIMIT = 10.0;                         // Max allowed current (A)
float TEMP_LIMIT = 60.0;                            // Max allowed temp (°C)

/************ GLOBAL VARIABLES ************/
float currentValue = 0;
float temperature = 0;
float energy_kWh = 0;
unsigned long lastMillis = 0;

/************ TIMER ************/
BlynkTimer timer;

/************************************************************
 CURRENT SENSOR FUNCTION (ACS712)
************************************************************/
float readCurrent() {
  int adc = analogRead(CURRENT_SENSOR);             // Read ADC value
  float voltage = adc * (3.3 / 4095.0);             // Convert ADC to voltage
  float current = (voltage - 2.5) / 0.100;          // ACS712-20A formula
  return abs(current);                              // Return absolute current
}

/************************************************************
 TEMPERATURE SENSOR FUNCTION (LM35)
************************************************************/
float readTemperature() {
  int adc = analogRead(TEMP_SENSOR);                // Read ADC
  float voltage = adc * (3.3 / 4095.0);              // Convert to voltage
  float temp = voltage * 100;                       // LM35: 10mV = 1°C
  return temp;
}

/************************************************************
 SAFETY CHECK FUNCTION
************************************************************/
void safetyCheck() {

  currentValue = readCurrent();                     // Read current
  temperature = readTemperature();                  // Read temperature

  Blynk.virtualWrite(V5, currentValue);             // Send current to Blynk
  Blynk.virtualWrite(V6, temperature);              // Send temperature to Blynk

  // -------- OVER CURRENT PROTECTION --------
  if (currentValue > CURRENT_LIMIT) {
    digitalWrite(RELAY1, LOW);                      // Turn OFF relays
    digitalWrite(RELAY2, LOW);
    digitalWrite(RELAY3, LOW);

    Blynk.logEvent("over_current", "⚠ Over-Current Detected!");
    Blynk.virtualWrite(V8, "⚠ Over-Current! Relays OFF");
  }

  // -------- OVER TEMPERATURE PROTECTION --------
  if (temperature > TEMP_LIMIT) {
    digitalWrite(RELAY1, LOW);
    digitalWrite(RELAY2, LOW);
    digitalWrite(RELAY3, LOW);

    Blynk.logEvent("over_temp", "🔥 Over-Temperature Detected!");
    Blynk.virtualWrite(V8, "🔥 Over-Temperature! Relays OFF");
  }

  // -------- CURRENT FLOW INDICATOR --------
  if (currentValue > 0.2) {
    Blynk.virtualWrite(V9, 255);                     // LED ON
  } else {
    Blynk.virtualWrite(V9, 0);                       // LED OFF
  }
}

/************************************************************
 ENERGY CALCULATION (kWh)
************************************************************/
void energyCalculation() {
  unsigned long now = millis();                     // Current time

  float hours = (now - lastMillis) / 3600000.0;     // Convert ms → hours
  float power = currentValue * 220;                 // Power = V × I
  energy_kWh += (power * hours) / 1000.0;           // Energy accumulation

  lastMillis = now;                                 // Update time
  Blynk.virtualWrite(V7, energy_kWh);                // Send energy to Blynk
}

/************************************************************
 BLYNK RELAY CONTROLS
************************************************************/
BLYNK_WRITE(V1) {
  if (readCurrent() < CURRENT_LIMIT)
    digitalWrite(RELAY1, param.asInt());
}

BLYNK_WRITE(V2) {
  if (readCurrent() < CURRENT_LIMIT)
    digitalWrite(RELAY2, param.asInt());
}

BLYNK_WRITE(V3) {
  if (readCurrent() < CURRENT_LIMIT)
    digitalWrite(RELAY3, param.asInt());
}

/************************************************************
 SETUP FUNCTION
************************************************************/
void setup() {

  Serial.begin(9600);                               // Start serial monitor

  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  digitalWrite(RELAY1, LOW);                        // Relays OFF at boot
  digitalWrite(RELAY2, LOW);
  digitalWrite(RELAY3, LOW);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);        // Connect to Blynk

  timer.setInterval(2000L, safetyCheck);            // Safety check every 2s
  timer.setInterval(5000L, energyCalculation);      // Energy calc every 5s
}

/************************************************************
 LOOP FUNCTION
************************************************************/
void loop() {
  Blynk.run();                                      // Run Blynk
  timer.run();                                      // Run timer
}