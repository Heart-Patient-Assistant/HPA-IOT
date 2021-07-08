#include <Wire.h>
#include "MAX30105.h" 
#include "heartRate.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> // ### json library


#include "MAX30100_PulseOximeter.h"


PulseOximeter pox;


MAX30105 particleSensor;



//==========================================
const char* ssid = "*******";  // Enter SSID here
const char* password = "*******";  //Enter Password here
//==========================================
String userId = "number"; // Fixed user id 
String serverName = "http://IP:port/users/"+userId+"/profile/sensor/data/"; // Localhost IP address, port number and the EndPoint

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

int beeep = 0;

float beatsPerMinute;
int beatAvg;
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void setup()
{
Serial.begin(115200);
Serial.println("Initializing...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //Start the OLED display
  display.display();
   delay(2000);
//==========================================
WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
//==========================================
// Initialize sensor
if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
{
Serial.println("MAX30105 was not found. Please check wiring/power. ");

while (1);
}
Serial.println("Place your index finger on the sensor with steady pressure.");
 
particleSensor.setup(); //Configure sensor with default settings
particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
}
 
void loop()
{
pox.update();
long irValue = particleSensor.getIR();

StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
JsonObject& JSONencoder = JSONbuffer.createObject();
String type1 = "Heart Rate";
String type2 = "SpO2";
int SpO2 = pox.getSpO2();
if (checkForBeat(irValue) == true)
{
//We sensed a beat!
long delta = millis() - lastBeat;
lastBeat = millis();

beatsPerMinute = 60 / (delta / 1000.0);
 
if (beatsPerMinute < 255 && beatsPerMinute > 20)
{
rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
rateSpot %= RATE_SIZE; //Wrap variable

//Take average of readings
beatAvg = 0;
for (byte x = 0 ; x < RATE_SIZE ; x++)
beatAvg += rates[x];
beatAvg /= RATE_SIZE;
}
}

    display.clearDisplay();                                //Clear the display
    display.setTextSize(2);                                //And still displays the average BPM
    display.setTextColor(WHITE);             
    display.setCursor(50,0);                
    display.println("BPM");             
    display.setCursor(50,18);                
    display.println(beatAvg); 
     display.println("SpO2");             
    display.setCursor(50,18);                
    display.println(SpO2);
    display.display();
Serial.print("IR=");
Serial.print(irValue);
Serial.print(", BPM=");
Serial.print(beatsPerMinute);
Serial.print(", Avg BPM=");
Serial.print(beatAvg);
Serial.print(", SpO2=");
Serial.print(SpO2);
Serial.print("%");
Serial.println();
//========================================== Put all collected data in json format and send it to the server
JSONencoder["user"] = userId;
JSONencoder["sensorType1"] = type1;
JSONencoder["reading"] = String(beatAvg);
JSONencoder["sensorType2"] = type2;
JSONencoder["SpO2"] = SpO2;
char JSONmessageBuffer[300];
JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
Serial.println(JSONmessageBuffer);
WiFiClient client;
HTTPClient http;
http.begin(client, serverName);
http.addHeader ("Content-Type", "application/json"); // type of connect


beeep = beeep + 1;
// Changing remainder changes rate the server receives data
if(beeep%200 == 0){
  if (beatAvg < 50 && beatAvg > 0 ){
    Serial.println("Fix finger pressure.");
  } else {
    int httpCode = http.POST(JSONmessageBuffer);
    if (httpCode >0 ) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpCode);
      }
    else {
      Serial.print("Error code: ");
      Serial.println(httpCode);
    }
    // Free resources
    http.end();
  }
}

if (irValue < 50000)
Serial.print(" No finger?");
Serial.println();
}
