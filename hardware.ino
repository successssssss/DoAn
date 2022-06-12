#include <DHT.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiUdp.h>
#include <NTPtimeESP.h>
#include <ArduinoJson.h>
#include <Ticker.h>
#include <time.h>
#include "EEPROM.h"
#include <string.h>
/************************* WiFi Access Point *********************************/
String WLAN_SSID;                        //string variable to store ssid
String WLAN_PASS; 
/************************* cloudmqtt Setup *********************************/

#define serveruri "driver.cloudmqtt.com"
#define port       18643
#define username  "cqbfckol"
#define password  "mpSkyZ4D1N6f"



#define LENGTH(x) (strlen(x) + 1)   // length of char string
#define EEPROM_SIZE 200
/*************************pin Setup *********************************/
const int DHTpin = 4; //đọc data từ chân  gpio4
const int RL1pin = 14;
const int RL2pin = 12;
const int RL3pin = 13;
const int RL4pin = 15;

const int ON = HIGH;
const int OFF = LOW;

#define PIN_BUTTON 0
#define PIN_LED 16
#define LED_ON() digitalWrite(PIN_LED, HIGH)
#define LED_OFF() digitalWrite(PIN_LED, LOW)
#define LED_TOGGLE() digitalWrite(PIN_LED, digitalRead(PIN_LED) ^ 0x01)
/*************************Do am dat Setup ******************************/
int value_soil, realvalue;
/*************************DHT Setup ************************************/
const int DHTtype = DHT11; //khai báo loại cảm biến
DHT dht(DHTpin, DHTtype);  //khởi tạo dht
float humidity;
float temperature;
unsigned long readTime;
unsigned long feedBackTime;
unsigned long alarmTime;
unsigned long confirmTime;
/*************************Instance Setup ************************************/
//tạo 1 client
WiFiClient myClient;
//**************************** Server NTP *************************************
NTPtime NTPch("vn.pool.ntp.org");
strDateTime dateTime;
byte nowHour = 0;     // Gio
byte nowMinute = 0;   // Phut
byte onHour = 0;     // Gio On
byte onMinute = 0;   // Phut On
byte offHour = 0;     // Gio Off
byte offMinute = 0;   // Phut Off

/*************************** Sketch Code ************************************/
void callback(char *tp, byte *message, unsigned int length);
void sensorRead();
void reconnect();
void feedBack();
/*************************** Smartconfig ************************************/
Ticker ticker;
String MacAddress;
String RL1; 
String RL2;
String RL3;
String RL4;
// nhap nhay LED
void tick()
{
  //toggle state
  int state = digitalRead(PIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(PIN_LED, !state);     // set pin to the opposite state
}

bool in_smartconfig = false;
void enter_smartconfig()
{
  if (in_smartconfig == false) {
    in_smartconfig = true;
    ticker.attach(0.1, tick);
    WiFi.beginSmartConfig();
  }
}

void exit_smart()
{
  ticker.detach();
  LED_ON();
  in_smartconfig = false;
}
/******************************************************************/
//*****************khởi tạo pubsubclient***************************
PubSubClient mqtt(serveruri, port, callback, myClient);

void setup()
{
  WiFi.disconnect();
  Serial.begin(9600);
  dht.begin(); //khởi động cảm biến
  //  Connect to WiFi access point.
   WiFi.disconnect();
  EEPROM.begin(EEPROM_SIZE);
  WLAN_SSID = readStringFromFlash(0); // Read SSID stored at address 0
  Serial.print("SSID = ");
  Serial.println(WLAN_SSID);
  WLAN_PASS = readStringFromFlash(40); // Read Password stored at address 40
  Serial.print("psss = ");
  Serial.println(WLAN_PASS);
  WiFi.begin(WLAN_SSID.c_str(), WLAN_PASS.c_str());
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  delay(5000);
  while (WiFi.status() != WL_CONNECTED)
  {
    WiFi.mode(WIFI_STA);
    WiFi.beginSmartConfig();
     
    //Wait for SmartConfig packet from mobile
    Serial.println("Waiting for SmartConfig.");
    while (!WiFi.smartConfigDone()) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("SmartConfig received.");

    //Wait for WiFi to connect to AP
    Serial.println("Waiting for WiFi");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("WiFi Connected.");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // read the connected WiFi SSID and password
    WLAN_SSID = WiFi.SSID();
    WLAN_PASS = WiFi.psk();
    Serial.print("SSID:");
    Serial.println(WLAN_SSID);
    Serial.print("PSS:");
    Serial.println(WLAN_PASS);
    Serial.println("Store SSID & PSS in Flash");
    writeStringToFlash(WLAN_SSID.c_str(), 0); // storing ssid at address 0
    writeStringToFlash(WLAN_PASS.c_str(), 40); // storing pss at address 40
    WiFi.begin(WLAN_SSID.c_str(), WLAN_PASS.c_str());
    delay(5000);
  }

   // kết nối với mqtt server
  while (1)
  {
    delay(500);
    if (mqtt.connect("ESP8266_10", username, password))
      break;
  }
  Serial.println("connected to MQTT server.....");

  // Get MacAddress and remove ":"
  MacAddress = WiFi.macAddress();
  MacAddress.remove(2,1);
  MacAddress.remove(4,1);
  MacAddress.remove(6,1);
  MacAddress.remove(8,1);
  MacAddress.remove(10,1);
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(MacAddress);
  //nhận dữ liệu có topic "ESPn" từ server
  RL1 = "ESPn/RL1"+MacAddress;
  RL2 = "ESPn/RL2"+MacAddress;
  RL3 = "ESPn/RL3"+MacAddress;;
  RL4 = "ESPn/RL4"+MacAddress;;
  mqtt.subscribe(RL1.c_str());
  mqtt.subscribe(RL2.c_str());
  mqtt.subscribe(RL3.c_str());
  mqtt.subscribe(RL4.c_str());
  Serial.println("Subcribe:");
  Serial.println(RL1.c_str());
  Serial.println(RL2.c_str());
  Serial.println(RL3.c_str());
  Serial.println(RL4.c_str());
  //set mode
  pinMode(DHTpin, INPUT);
  pinMode(A0, INPUT);
  pinMode(RL1pin, OUTPUT);
  pinMode(RL2pin, OUTPUT);
  pinMode(RL3pin, OUTPUT);
  pinMode(RL4pin, OUTPUT);

  //set bit first time
  digitalWrite(RL1pin, OFF);
  digitalWrite(RL2pin, OFF);
  digitalWrite(RL3pin, OFF);
  digitalWrite(RL4pin, OFF);
}

void loop()
{

 

  //làm mqtt luôn sống
  mqtt.loop();

  //phản hồi trạng thái relay lên server
  if (mqtt.connected())
  {
//    if (millis() > feedBackTime + 1000)
//    {
//      feedBack();
//    }
    if (WiFi.status() == WL_DISCONNECTED||!mqtt.connected())
        {
          reconnect();
        } 
    //check if 5 seconds has elapsed since the last time we read the sensors.
    if (millis() > readTime + 10000)
    {
      sensorRead(); //day nay
      
    }

//    if (millis() > alarmTime + 5000)
//    {
//      alarm();
//    }
  }

  //nhan thoi gian tu NTp server
  // Tham so dau tien la Time zone duoi dang floating point (7.0 la VN);
  // Tham so thu hai la DayLightSaving: 1 voi thoi gian; 2 la thoi gian US (o VN khong su dung)
  dateTime = NTPch.getNTPtime(7.0, 0);
  if (dateTime.valid) {
    nowHour = dateTime.hour;      // Gio
    nowMinute = dateTime.minute;  // Phut
  }

  //
}

//********************* hàm trả dữ liệu về *********************************
void callback(char *tp, byte *message, unsigned int length)
{
  //bien nhan gio Bat, Tat tu client
  //Hien tai neu khong cap nhat, day se la thoi gian hang ngay


  String topic(tp);
  String content = String((char *)message);
  content.remove(length);
  Serial.print("Topic:");
  Serial.println(topic);
  //điều khiển relay 1
  if (topic == RL1.c_str())
  {
    if (content == "1")
    {
      digitalWrite(RL1pin, ON);
      Serial.println("relay 1 ON");
    }
    if (content == "0")
    {
      digitalWrite(RL1pin, OFF);
      Serial.println("relay 1 OFF");
    }
  }

  //điều khiển relay 2
  if (topic == RL2.c_str())
  {
    if (content == "1")
    {
      digitalWrite(RL2pin, ON);
      Serial.println("relay 2 ON");
    }
    if (content == "0")
    {
      digitalWrite(RL2pin, OFF);
      Serial.println("relay 2 OFF");
    }
  }

  //điều khiển relay 3
  if (topic == RL3.c_str())
  {
    if (content == "1")
    {
      digitalWrite(RL3pin, ON);
      Serial.println("relay 3 ON");
    }
    if (content == "0")
    {
      digitalWrite(RL3pin, OFF);
      Serial.println("relay 3 OFF");
    }
  }

  //điều khiển relay 4
  if (topic == RL4.c_str())
  {
    if (content == "1")
    {
      digitalWrite(RL4pin, ON);
      Serial.println("relay 4 ON");
    }
    if (content == "0")
    {
      digitalWrite(RL4pin, OFF);
      Serial.println("relay 4 OFF");
    }
  }
  feedBack();


}

//************************* hàm đọc giá trị sensor *******************
void sensorRead()
{
  readTime = millis();
  humidity = dht.readHumidity();       //đọc nhiệt độ
  temperature = dht.readTemperature(); // đọc độ ẩm
  float real = analogRead(A0);
  Serial.print("real");
  Serial.println(real);
  int percent_soil = 100- 100*real/1024;
  Serial.print("Do am dat:");
  Serial.println(percent_soil);
  delay(2000);

  // Get MacAddress and remove ":"
  String MacAddress = WiFi.macAddress();
  MacAddress.remove(2,1);
  MacAddress.remove(4,1);
  MacAddress.remove(6,1);
  MacAddress.remove(8,1);
  MacAddress.remove(10,1);
  Serial.print("ESP8266 Board MAC Address:  ");
  Serial.println(MacAddress);

  // init mqttChannel
  String mqttMainChannel = "ESPs/enviroment/";
  String mqttChannel = mqttMainChannel + MacAddress;

  // convert data to JSON
  StaticJsonDocument<200> doc;
  JsonObject content  = doc.to<JsonObject>();
  doc["temperature"] = temperature;
  doc["humidity"]   = humidity;
  doc["soil_humidity"]   = percent_soil;

  char content_string[256];
  serializeJson(content, content_string);

  // push data to mqtt
  mqtt.publish(mqttChannel.c_str(), content_string);

  char buffer[10];
  dtostrf(percent_soil, 0, 0, buffer);
  mqtt.publish(("ESP/percent_soil"+MacAddress).c_str(), buffer);
  //đẩy data nhiet do do am lên server

  dtostrf(temperature, 0, 0, buffer);
  mqtt.publish(("ESP/temperature"+MacAddress).c_str(), buffer);

  dtostrf(humidity, 0, 0, buffer);
  mqtt.publish(("ESP/humidity"+MacAddress).c_str(), buffer);
}

//*********************** hàm reconnect ************************
void reconnect()
{
  // lặp đến khi kết nối lại
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Attempting connection...Wifi");
    WiFi.reconnect();
    mqtt.connect("ESP8266", username, password);
    delay(5000);
    // chờ để kết nối lại
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("reconnected");
      return;
    }
    else
    {
      Serial.print("failed to connect WiFi!!");
      Serial.println(" try again in 5 seconds...");
      // chờ 5s
      delay(5000);
    }
  }

  while (!mqtt.connected())
  {
    Serial.println("Attempting connection...Mqtt");
    mqtt.connect("ESP8266", username, password);
    delay(1000);
    // chờ để kết nối lại
    if (mqtt.connected())
    {
      Serial.println("reconnected");
      mqtt.subscribe(RL1.c_str());
      mqtt.subscribe(RL2.c_str());
      mqtt.subscribe(RL3.c_str());
      mqtt.subscribe(RL4.c_str());
      mqtt.subscribe("APPgH1/RL1");
      mqtt.subscribe("APPgM1/RL1");
      mqtt.subscribe("APPgH2/RL1");
      mqtt.subscribe("APPgM2/RL1");
      mqtt.loop();
      return;
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // chờ 5s
      delay(5000);
    }
  }
}

void feedBack() {
//  feedBackTime = millis();
//
//  // Get MacAddress and remove ":"
//  String MacAddress = WiFi.macAddress();
//  MacAddress.remove(2,1);
//  MacAddress.remove(4,1);
//  MacAddress.remove(6,1);
//  MacAddress.remove(8,1);
//  MacAddress.remove(10,1);
//  Serial.println(MacAddress);
//  // init mqttChannel
//  String mqttMainChannel = "ESPs/status/realtime/";
//  String mqttChannel = mqttMainChannel + MacAddress;
//
//  // convert data to JSON
//  StaticJsonDocument<200> doc;
//  JsonObject content_1  = doc.to<JsonObject>();
//  doc["relay_id"] = 1;
//  doc["status"]   = String(digitalRead(RL1pin));
//  char content_string_1[256];
//  serializeJson(content_1, content_string_1);
//  Serial.println(content_string_1);
//
//  JsonObject content_2  = doc.to<JsonObject>();
//  doc["relay_id"] = 2;
//  doc["status"]   = String(digitalRead(RL2pin));
//  char content_string_2[256];
//  serializeJson(content_2, content_string_2);
//
//  JsonObject content_3  = doc.to<JsonObject>();  
//  doc["relay_id"] = 3;
//  doc["status"]   = String(digitalRead(RL3pin));;
//  char content_string_3[256];
//  serializeJson(content_3, content_string_3);
//
//  JsonObject content_4  = doc.to<JsonObject>();
//  doc["relay_id"] = 4;
//  doc["status"]   = String(digitalRead(RL4pin));
//  char content_string_4[256];
//  serializeJson(content_4, content_string_4);
//
//  // push data to mqtt
//  mqtt.publish(mqttChannel.c_str(), content_string_1);
//  mqtt.publish(mqttChannel.c_str(), content_string_2);
//  mqtt.publish(mqttChannel.c_str(), content_string_3);
//  mqtt.publish(mqttChannel.c_str(), content_string_4);

//  mqtt.publish("ESPg/RL1", String(digitalRead(RL1pin)).c_str());
//  mqtt.publish("ESPg/RL2", String(digitalRead(RL2pin)).c_str());
//  mqtt.publish("ESPg/RL3", String(digitalRead(RL3pin)).c_str());
//  mqtt.publish("ESPg/RL4", String(digitalRead(RL4pin)).c_str());
}
// cap nhat tgian bat tat khi reload

void confirmAlarm() {
  confirmTime = millis();
  mqtt.publish("ESPgH1/RL1", String(onHour).c_str());
  mqtt.publish("ESPgM1/RL1", String(onMinute).c_str());
  mqtt.publish("ESPgH2/RL1", String(offHour).c_str());
  mqtt.publish("ESPgM2/RL1", String(offMinute).c_str());
}



void alarm() {
  alarmTime = millis();
  if (nowHour == onHour && nowMinute == onMinute) {
    digitalWrite(RL1pin, ON);
    confirmAlarm();
    Serial.println("Time to ON relay 1");
    Serial.println("relay 1 ON");
  }
  if (nowHour == offHour && nowMinute == offMinute) {
    digitalWrite(RL1pin, OFF);
    Serial.println("Time to OFF relay 1");
    Serial.println("relay 1 OFF");
  }

}

void milliAlarm() {

}

void writeStringToFlash(const char* toStore, int startAddr) {
  int i = 0;
  for (; i < LENGTH(toStore); i++) {
    EEPROM.write(startAddr + i, toStore[i]);
  }
  EEPROM.write(startAddr + i, '\0');
  EEPROM.commit();
}


String readStringFromFlash(int startAddr) {
  char in[128]; // char array of size 128 for reading the stored data 
  int i = 0;
  for (; i < 128; i++) {
    in[i] = EEPROM.read(startAddr + i);
  }
  return String(in);
}
