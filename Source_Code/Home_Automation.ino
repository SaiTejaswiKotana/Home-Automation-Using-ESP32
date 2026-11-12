/*
 * Home Automation Using ESP8266 NodeMCU
 * Based on the source code in the Maven Silicon internship report.
 *
 * Hardware:
 *   ESP8266 NodeMCU, DHT11, LDR module, HC-SR04,
 *   2-channel relay module, fan and light.
 *
 * IMPORTANT:
 *   1. Install the ESP8266 board package in Arduino IDE.
 *   2. Install the DHT sensor library (Adafruit) and Adafruit Unified Sensor.
 *   3. Set WIFI_SSID and WIFI_PASSWORD below before uploading.
 *   4. Verify relay polarity and LDR logic for your modules.
 *   5. The report's code uses a 20 C fan threshold; its test table mentions
 *      28 C. Verify the threshold used in your physical implementation.
 *
 * Safety:
 *   Mains-powered appliances must be wired and tested by a qualified person.
 */


#include<ESP8266WiFi.h>
#include"DHT.h"

#defineDHTPIND4//GPIO2
#defineDHTTYPEDHT11
#defineLDR_PIND3//GPIO0
#defineTRIG_PIND1//GPIO5
#defineECHO_PIND2//GPIO4
#defineRELAY_LIGHTD5//GPIO14
#defineRELAY_FAND6//GPIO12

DHTdht(DHTPIN,DHTTYPE);
WiFiServer server(80);
boolisManualMode=false;
uint8_tpreviousRelayLight=HIGH;
uint8_tpreviousRelayFan=HIGH;
constchar* ssid="tejaswi";
constchar* password="12345678";

voidserialSetup(){
Serial.begin(115200);
delay(10);
Serial.println();
}
voidDHTsetup(){
dht.begin();
}
voidLDRsetup(){
pinMode(LDR_PIN,INPUT);
}
voidultrasonicSetup(){
pinMode(TRIG_PIN,OUTPUT);
pinMode(ECHO_PIN,INPUT);
}

voidrelaySetup(){
pinMode(RELAY_LIGHT,OUTPUT);
pinMode(RELAY_FAN,OUTPUT);
digitalWrite(RELAY_LIGHT,HIGH);
digitalWrite(RELAY_FAN,HIGH);
}
voidwifiSetup(){
Serial.print("Connectingto ");
Serial.println(ssid);
WiFi.begin(ssid,password);
while(WiFi.status()!=WL_CONNECTED){
delay(500);
 Serial.print(".");
}
Serial.println("\nWiFiconnected");
Serial.print("IPaddress:");
Serial.println(WiFi.localIP());
server.begin();
}
floatgetTemperature(){
delay(1000);
floatt=dht.readTemperature();
Serial.print("Temp:");
Serial.println(t);
returnt;
}
boolisItDark(){
intstate=digitalRead(LDR_PIN);
Serial.println(state==HIGH?"It’sdark":"It’slight");
returnstate==HIGH;
 }
floatgetDistance(){
digitalWrite(TRIG_PIN,LOW);
delayMicroseconds(2);
digitalWrite(TRIG_PIN,HIGH);
delayMicroseconds(10);
digitalWrite(TRIG_PIN,LOW);
longduration=pulseIn(ECHO_PIN,HIGH);
floatdistance=duration * 0.034/2;
Serial.print("Distance:");
Serial.println(distance);
returndistance;
}
voidsetRelay(uint8_tpin,uint8_tvalue){
value=!value;
if(pin==RELAY_LIGHT&&previousRelayLight!=value){
digitalWrite(RELAY_LIGHT, value);
previousRelayLight=value;
Serial.println(value==LOW?"LightON":"LightOFF");
}
if(pin==RELAY_FAN&&previousRelayFan!=value){
digitalWrite(RELAY_FAN,value);
previousRelayFan=value;
Serial.println(value==LOW?"FanON":"FanOFF");
}
}
voidhandleAutoMode(){
floatdistance=getDistance();
floattemp=getTemperature();
booldark=isItDark();

if(distance<=10){
if(temp>=20)setRelay(RELAY_FAN,LOW);
elsesetRelay(RELAY_FAN,HIGH);

if(dark)setRelay(RELAY_LIGHT,LOW);
}else{
setRelay(RELAY_LIGHT,HIGH);
setRelay(RELAY_FAN,HIGH);
}
}
voidhandleClient(WiFiClientclient){
Stringrequest="";
while(client.connected()){
if(client.available()){
char c=client.read();
request+=c;
if(c==’\n’){
if (request.indexOf("GET/?lightOn")>=0)setRelay(RELAY_LIGHT,LOW);
if (request.indexOf("GET/?lightOff")>=0)setRelay(RELAY_LIGHT,HIGH);
if (request.indexOf("GET/?fanOn")>=0)setRelay(RELAY_FAN,LOW);
if (request.indexOf("GET/?fanOff")>=0)setRelay(RELAY_FAN,HIGH);
if (request.indexOf("GET/?manualOn")>=0)isManualMode=true;
if (request.indexOf("GET/?manualOff")>=0)isManualMode=false;

client.println("HTTP/1.1200OK");
client.println("Content-Type:text/html");
client.println();
client.println("<html><head><title>ESP8266Control</title></head><body>");
client.println("<h1>ESP8266HomeAutomation</h1>");
client.println("<p>LightControl:<ahref=’/?lightOn’><button>ON</button></a><ahref=’/?lightOff’><button>OFF</button></a></p>");
client.println("<p>Fan Control:<ahref=’/?fanOn’><button>ON</button></a><ahref=’/?fanOff’><button>OFF</button></a></p>");
client.println("<p>ManualControl:<ahref=’/?manualOn’><button>ON</button></a><ahref=’/?manualOff’><button>OFF</button></a></p>");
client.println("</body></html>");
break;
}
}
}
delay(1);
client.stop();
}
voidsetup(){
serialSetup();
DHTsetup();
LDRsetup();
ultrasonicSetup();
relaySetup();
wifiSetup();
}
voidloop(){
WiFiClientclient=server.available();
if(client)handleClient(client);
if(!isManualMode){
handleAutoMode();
}
delay(2000);
}
