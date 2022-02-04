#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal.h>
#include <SparkFunCCS811.h>
#include <Wire.h>

// LCD pins to Arduino.
const int pin_RS = 8; 
const int pin_EN = 9; 
const int pin_d4 = 4; 
const int pin_d5 = 5; 
const int pin_d6 = 6; 
const int pin_d7 = 7; 
const int pin_BL = 10; 
// Other arduino pins
const int pin_FAN = 11; 
const int pin_LED = 12; 
const int pin_MIST = 3;
LiquidCrystal lcd( pin_RS, pin_EN, pin_d4, pin_d5, pin_d6, pin_d7);

// Default I2C address set on the board for the BME280 temperature/humidity/pressure sensor.
const int BME_I2C_ADDRESS = 0x76;
Adafruit_BME280 weatherSensor;

// Default I2C Address for the CCS811 eCO2/tVOC sensor
const int CCS811_ADDR = 0x5A; 
CCS811 co2sensor(CCS811_ADDR);

// Rate at which the loop for the display and devices runs. 
const int REFRESH_RATE_MS = 1000; 

// Fan status
bool fans = false;
// CO2 ppm at which the fans starts.
const int MAX_CO2 = 800;
// C02 ppm at which the fan closes.
const int MIN_CO2 = 600;

// Misting status
bool mister = false;
// % humidify at which the mister starts
const int MIN_HUMIDITY = 40;
// % humidify at which the mister closes.
const int MAX_HUMIDITY = 50;

void setup() {
   Serial.begin(115200);
   Serial.println("Initialization starts");
   Wire.begin();

   // CO2 sensor setup, prints error if any.
   CCS811Core::CCS811_Status_e returnCode = co2sensor.beginWithStatus(Wire);
   Serial.println("CCS811 begin exited with: ");
   Serial.println(co2sensor.statusString(returnCode));

   // Temperature and humidity sensor setup, print error if it fails.
   if (!weatherSensor.begin(BME_I2C_ADDRESS)) {
      Serial.println("Could not find the BME280 sensor, check wiring!");
   }
   
   // Light is always on in this version, so start now.
   pinMode(pin_LED, OUTPUT); 
   digitalWrite(pin_LED, HIGH);

   // Set other control pins as output and set initial values
   pinMode(pin_FAN, OUTPUT); 
   pinMode(pin_MIST, OUTPUT); 
   int co2Value = getCO2Value(&co2sensor);
   if (co2Value > MAX_CO2 && co2Value > MIN_CO2) {
      fans = true;
      digitalWrite(pin_FAN, HIGH);
   }
   else {
      fans = false;
      digitalWrite(pin_FAN, LOW);
   }

   int humidityValue = getHumidityValue(&weatherSensor);
   if (humidityValue < MIN_HUMIDITY && humidityValue < MAX_HUMIDITY) {
      mister = true;
      digitalWrite(pin_MIST, HIGH);
   }
   else {
      mister = false;
      digitalWrite(pin_MIST, LOW);
   }

   // Sets the LCD backlight at 20% brightness (analog ranges from 0 to 255).
   pinMode(pin_BL, OUTPUT);               
   analogWrite(pin_BL, 50);     
   Serial.println("Initialized");         
}

void loop() {
   // LED always on
   digitalWrite(pin_LED, HIGH);
   // First LCD line display, current sensor values.
   lcd.begin(16, 2);
   lcd.clear();
   lcd.setCursor(0,0);
   const String temperatureHumidity = getTemperatureHumidity(&weatherSensor);
   const int co2Value = getCO2Value(&co2sensor);
   const String lcdFirstLine = temperatureHumidity + " " + co2Value + "ppm";
   lcd.print(lcdFirstLine);
   Serial.println(lcdFirstLine);
   // Second LCD line display, current devices.
   lcd.setCursor(0,1);

   if (co2Value > MAX_CO2) {
      fans = true;
      digitalWrite(pin_FAN, HIGH);
   }
   else if (co2Value < MIN_CO2) {
      fans = false;
      digitalWrite(pin_FAN, LOW);
   }  

   int humidityValue = getHumidityValue(&weatherSensor);
   if (humidityValue < MIN_HUMIDITY) {
      mister = true;
      digitalWrite(pin_MIST, HIGH);
   }
   else if (humidityValue > MAX_HUMIDITY) {
      mister = false;
      digitalWrite(pin_MIST, LOW);
   }
   
   String fanLcd = "FAN";
   if (!fans) {
      fanLcd = "";
   }
   String misterLcd = "MIST";
   if (!mister) {
      misterLcd = "";
   }
   
   const String lightLcd = "LIGHT";
   String lcdSecondLine = fanLcd + " " + misterLcd + " " + lightLcd;
   lcd.print(lcdSecondLine);
   Serial.println(lcdSecondLine);

   delay(REFRESH_RATE_MS);
} 

int getHumidityValue(Adafruit_BME280* sensor) {
   float humidity = sensor->readHumidity();
   return humidity;
}


String getTemperatureHumidity(Adafruit_BME280* sensor) {
   float temperature = sensor->readTemperature();
   float humidity = sensor->readHumidity();
    
   String temperatureHumidity = String(temperature, 0);
          temperatureHumidity += "C ";
          temperatureHumidity += String(humidity, 0);
          temperatureHumidity += "%";

   return (temperatureHumidity);
}

int getCO2Value(CCS811* sensor) {
   if (sensor->dataAvailable()) {
      sensor->readAlgorithmResults();
      return sensor->getCO2();
   }
   Serial.println("No data available for CO2.");
   return 0;
}
