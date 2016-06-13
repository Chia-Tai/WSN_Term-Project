#include "Barometer.h"
#include <Wire.h>
#include "DHT.h"
#define DHTPIN 3     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

#include <HttpClient.h>
#include <LAudio.h>
#include <LTask.h>
#include <LWiFi.h>
#include <LWiFiClient.h>
#include <LDateTime.h>
#define WIFI_AP "Barry"
#define WIFI_PASSWORD "0937630846"
#define WIFI_AUTH LWIFI_WPA  // choose from LWIFI_OPEN, LWIFI_WPA, or LWIFI_WEP.
#define per 50
#define per1 3
#define DEVICEID "DAFiUiC0" // Input your deviceId
#define DEVICEKEY "uEIY05NnxM0p2fBk" // Input your deviceKey
#define SITE_URL "api.mediatek.com"
LWiFiClient content;
HttpClient http(content);
float t = 0.0;
float h = 0.0;
void sendValueToMCS(float value, String channelId);

const int clockPin = 12;  // Red  line
const int latchPin = 11;  // Yellow line
const int dataPin = 10;   // Blue line

int Durt_pin = 2;
unsigned long duration;
unsigned long starttime;
unsigned long sampletime_ms = 1000;//sampe 5s ;
unsigned long lowpulseoccupancy = 0;
float ratio = 0;
float concentration = 0;

float temperature;
float pressure;
float atm;
float altitude;
Barometer myBarometer;

int light_Pin = A0;    // select the input pin for the potentiometer
int LightValue = 0;  // variable to store the value coming from the sensor
int UV_Pin = A1;
int UVValue = 0;

DHT dht(DHTPIN, DHTTYPE);
 
void setup() {
  Serial.begin(9600);
  pinMode(8,INPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  starttime = millis();//get the current time;
  myBarometer.init();
  dht.begin();

  LAudio.begin();
  LAudio.setVolume(8);
  Serial.println("Connecting to AP");
  while (0 == LWiFi.connect(WIFI_AP, LWiFiLoginInfo(WIFI_AUTH, WIFI_PASSWORD))){
    delay(1000);
    Serial.println(".");
  }
  Serial.println("AP Connected successfully");
  
}

int Light_V = 0;
float Humidity_V = 0;

void loop(){
  Serial.println("------------------------------");
  durt_func();
  barometer_func();
  light_func();
  UV_func();
  TempHumi_func();
  if (Light_V > 600){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 0);
    digitalWrite(latchPin, HIGH);
  }
  if (Light_V > 400 && Light_V < 600 ){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 2);
    digitalWrite(latchPin, HIGH);
  }
  if (Light_V < 400){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 6);
    digitalWrite(latchPin, HIGH);
  }
  if(Light_V < 200 && Humidity_V > 87){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 14);
    digitalWrite(latchPin, HIGH);
  }
  if(concentration>15000){
    digitalWrite(latchPin, LOW);
    shiftOut(dataPin, clockPin, MSBFIRST, 14);
    digitalWrite(latchPin, HIGH);
  }

  sendValueToMCS(Light_V,"Light");
  sendValueToMCS(Humidity_V,"Humidity");
  sendValueToMCS(concentration, "Durt");
  
  Serial.println();
  delay(2000);
}

boolean disconnectedMsg = false;

void sendValueToMCS(float value, String channelId) {

  Serial.println("Connecting to WebSite");
  while (0 == content.connect(SITE_URL, 80))
  {
    Serial.println("Re-Connecting to WebSite");
    delay(1000);
  }

  Serial.println("send HTTP GET request");
  String action = "POST /mcs/v2/devices/";
  action += DEVICEID ;
  action += "/datapoints.csv HTTP/1.1";
  content.println(action);

  String data = channelId + ",,";
  data += value;
  Serial.println("send Data"  + data);
  int dataLength = data.length();

  content.println("Host: api.mediatek.com");
  content.print("deviceKey: ");
  content.println(DEVICEKEY);
  content.print("Content-Length: ");
  content.println(dataLength);
  content.println("Content-Type: text/csv");
  content.println("Connection: close");
  content.println();
  content.println(data);

  // waiting for server response
  Serial.println("waiting HTTP response:");
  while (!content.available())
  {
    delay(100);
  }

  // Make sure we are connected, and dump the response content to Serial
  while (content)
  {
    int v = content.read();
    if (v != -1)
    {
      Serial.print((char)v);
    }
    else
    {
      Serial.println("no more content, disconnect");
      content.stop();
      while (1)
      {
        delay(1);
      }
    }

  }

  if (!disconnectedMsg)
  {
    Serial.println("disconnected by server");
    disconnectedMsg = true;
  }
  delay(500);
} 

void durt_func() {
  duration = pulseIn(Durt_pin, LOW);
  lowpulseoccupancy = lowpulseoccupancy+duration;
 
  if ((millis()-starttime) >= sampletime_ms)//if the sampel time == 5s
  {
    ratio = lowpulseoccupancy/(sampletime_ms*10.0);  // Integer percentage 0=&gt;100
    concentration = 1.1*pow(ratio,3)-3.8*pow(ratio,2)+520*ratio+0.62; // using spec sheet curve
    Serial.print("concentration = ");
    Serial.print(concentration);
    Serial.println(" pcs/0.01cf");
    lowpulseoccupancy = 0;
    starttime = millis();
  }
}


void barometer_func(){
     temperature = myBarometer.bmp085GetTemperature(myBarometer.bmp085ReadUT()); //Get the temperature, bmp085ReadUT MUST be called first
     pressure = myBarometer.bmp085GetPressure(myBarometer.bmp085ReadUP());//Get the temperature
     altitude = myBarometer.calcAltitude(pressure); //Uncompensated caculation - in Meters
     atm = pressure / 101325;
  
    // Serial.print("Pressure: ");
    // Serial.print(pressure, 0); //whole number only.
    // Serial.println(" Pa");
  
    Serial.print("Ralated Atmosphere: ");
    Serial.println(atm, 4); //display 4 decimal places
  
    delay(100); //wait a second and get values again.
}

void light_func(){
    LightValue = analogRead(light_Pin);
    Light_V = LightValue;
    // LightValue = (float)(1023-LightValue)*10/LightValue;
    Serial.print("Light Value: ");
    Serial.println(LightValue);
    delay(100);
}

void UV_func(){
    UVValue = analogRead(UV_Pin);
    Serial.print("UV Value: ");
    Serial.println(UVValue);
    delay(100);
}

void TempHumi_func(){
    float t = 0.0;
    float h = 0.0;
    
    if(dht.readHT(&t, &h))
    {
        Serial.print("temperature = ");
        Serial.println(t);
        
        Serial.print("humidity = ");
        Humidity_V = h;
        Serial.println(h);
    }
    delay(100);
}

