//Requires PubSubClient found here: https://github.com/knolleary/pubsubclient
//
//Setup the ESP8266: http://www.instructables.com/id/Getting-Started-With-the-ESP8266-ESP-01/

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "Timer.h"
#include <led.h>


//EDIT THESE LINES TO MATCH YOUR SETUP
//-----------------------------------------------MQTT------------------------
char* houseTopic  = "house";            // Topic
#define MQTT_SERVER "192.168.0.10"      // Server
#define MQTT_PORT 1883                  // Port
//-----------------------------------------------WLAN------------------------
const char* ssid 	   = "myWlan";
const char* password = "myPassword";
//-----------------------------------------------BLINKDELAYS-----------------
unsigned int blinkIntervall 	  = 1000;		//1s 	LED		_|-|_|-|_|-|_|----...-|_____
unsigned int blinkTime	        = 15000;	//15s	TIMER	_____________|____..._______
unsigned int onTime		          = 60000;	//60s	TIMER   			      ____..._|_____

Timer wiFiTimer;
Timer blinkTimer;
Timer ledModeTimer;
int ledMode = -1;
//----------------------------------------------Lamp's------------------------
const int redLamp = 2;
const int redLed  = 0;

Led lamp(redLamp);
Led led(redLed);
//---------------------------------------------------------------------------


void callback(char* topic, byte* payload, unsigned int length);
WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, MQTT_PORT, callback, wifiClient);

void setup() {
  Serial.begin(19200);
  Serial.println("SETUP");
  led.on();
  lamp.on();
  delay(500);
  led.off();
  lamp.off();
  delay(500);
  led.on();
  lamp.on();
  delay(500);
  led.off();
  lamp.off();
    
	//start wifi subsystem
	WiFi.begin(ssid, password);
	reconnect();
}

void loop()
{
	lamp.update();
	led.update();
	//reconnect if connection is lost
	if(wiFiTimer.start(10))
	{
		if (!client.connected() && WiFi.status() == 3) 
		{
		  Serial.println("RECONNECT");reconnect();
	  }
	}
	//maintain MQTT connection
	client.loop();
  delay(10);
	//MUST delay to allow ESP8266 WIFI functions to run
  switch(ledMode)
  {
    case 0: // blink 15s
    { 
		  if(ledModeTimer.start(blinkTime))
		  {
		    Serial.println("lamp blink OFF");
			  Serial.println("lamp ON");
			  ledMode++;
			  lamp.blinkOff();
        led.blinkOff();
		    lamp.on();
        led.on();
		  } 
			else if(!lamp.isBlinking())
			{
				Serial.println("lamp blink ON");
				lamp.blinkOn(blinkIntervall);  
        led.blinkOn(blinkIntervall);
			}
			 break;
    }
    case 1: // on 60s
    {
			if(ledModeTimer.start(onTime))
			{
				Serial.println("lamp OFF");
				ledMode = -1;
				lamp.off();
        led.off();
			}
	    break;
    }
    default: 
    {
      //nothing to do
    }
  }
}

/*
Message:
{
  "Address": "house",
  "Persom": "Peter",
  "Event": "doorOpened-event",
  "Destination": "frontDoor",
  "Message": "Hello Peter",
  "Timestamp": "14.05.2018 00:31:30",
  "Type": "Family-member"
}
*/
void callback(char* topic, byte* payload, unsigned int length) //Message received
{
	String topicStr = topic; 
  String message = (char*)payload;
  message = message.substring(0,length);
	Serial.print("Message from: ");
	Serial.print(topicStr);
	Serial.print(" -> ");
	Serial.println(message);
  
	String event = message.substring(message.indexOf("domain_event_id")+15);
	event = event.substring(event.indexOf(":")+1);
	event = event.substring(event.indexOf("\"")+1);
	event = event.substring(0,event.indexOf("\""));
  
	if(event == "doorOpened-event")
	{
		Serial.println("doorOpened-event!");
		ledMode = 0;
		ledModeTimer.stop();     
	}
	
	//  echo	 	       topic	message
	//client.publish("house","door-open");
}

void reconnect() 
{
	//attempt to connect to the wifi if connection is lost
	if(WiFi.status() != WL_CONNECTED){
		//debug printing
		Serial.print("Connecting to ");
		Serial.println(ssid);
		//loop while we wait for connection
		while (WiFi.status() != WL_CONNECTED) {
			delay(100);
			Serial.print(".");
		}
		//print out some more debug once connected
		Serial.println("");
		Serial.println("WiFi connected");  
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
    led.blinkOff();
	}

	//make sure we are connected to WIFI before attemping to reconnect to MQTT
	if(WiFi.status() == WL_CONNECTED){
	// Loop until we're reconnected to the MQTT server
		while (!client.connected()) {                        
			Serial.print("Attempting MQTT connection...");
			// Generate client name based on MAC address and last 8 bits of microsecond counter
			String clientName;
			clientName += "esp8266-";
			uint8_t mac[6];
			WiFi.macAddress(mac);
			clientName += macToStr(mac);
			//if connected, subscribe to the topic(s) we want to be notified about
			if (client.connect((char*) clientName.c_str())) {
				Serial.print("\tMTQQ Connected");
                client.subscribe(houseTopic);
                client.loop();
			}
			//otherwise print failed for debugging
			else{Serial.println("\tFailed."); abort();}
		}
	}
}

String macToStr(const uint8_t* mac)//generate unique name from MAC addr
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }
  return result;
}
