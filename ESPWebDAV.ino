// Using the WebDAV server with Rigidbot 3D printer.
// Printer controller is a variation of Rambo running Marlin firmware

#include "serial.h"
#include "parser.h"
#include "config.h"
#include "network.h"
#include "gcode.h"
#include "sdControl.h"
#include <ArduinoOTA.h>

#define Hostname      "ESPWebDAV"
#define Version       "1.0.6"

// LED is connected to GPIO2 on this board
#define INIT_LED			{pinMode(2, OUTPUT);}
#define LED_ON				{digitalWrite(2, LOW);}
#define LED_OFF				{digitalWrite(2, HIGH);}

// ------------------------
void setup() {
	SERIAL_INIT(115200);
	INIT_LED;
  SERIAL_ECHOLN("");SERIAL_ECHO("Version: ");SERIAL_ECHOLN(Version);
  config.VerSion(Version);
	
	blink();
	
	sdcontrol.setup();

	// ----- WIFI -------
  if(config.load() == 1) { // Connected before
    if(!network.start()) {
      SERIAL_ECHOLN("Connect fail, please check your INI file or set the wifi config and connect again");
      SERIAL_ECHOLN("- M50: Set the wifi ssid , 'M50 ssid-name'");
      SERIAL_ECHOLN("- M51: Set the wifi password , 'M51 password'");
      SERIAL_ECHOLN("- M52: Start to connect the wifi");
      SERIAL_ECHOLN("- M53: Check the connection status");
    }
  }
  else {
    SERIAL_ECHOLN("Welcome to FYSETC: www.fysetc.com");
    SERIAL_ECHOLN("Please set the wifi config first");
    SERIAL_ECHOLN("- M50: Set the wifi ssid , 'M50 ssid-name'");
    SERIAL_ECHOLN("- M51: Set the wifi password , 'M51 password'");
    SERIAL_ECHOLN("- M52: Start to connect the wifi");
    SERIAL_ECHOLN("- M53: Check the connection status");
  }
  ArduinoOTA.setHostname(Hostname);
     ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH) {
          type = "sketch";
      } else { // U_FS
          type = "filesystem";
      }
      // NOTE: if updating FS this would be the place to unmount FS using FS.end()
      SERIAL_ECHOLN("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
      SERIAL_ECHOLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) {
          SERIAL_ECHOLN("Auth Failed");
      } else if (error == OTA_BEGIN_ERROR) {
          SERIAL_ECHOLN("Begin Failed");
      } else if (error == OTA_CONNECT_ERROR) {
          SERIAL_ECHOLN("Connect Failed");
      } else if (error == OTA_RECEIVE_ERROR) {
          SERIAL_ECHOLN("Receive Failed");
      } else if (error == OTA_END_ERROR) {
          SERIAL_ECHOLN("End Failed");
      }
  });
  ArduinoOTA.begin();
  SERIAL_ECHOLN("Ready");
//  SERIAL_ECHO("IP address: ");
//  SERIAL_ECHOLN(WiFi.localIP());
  
}

// ------------------------
void loop() {
  // handle the request
	network.handle();

  // Handle gcode
  gcode.Handle();

  // blink
  statusBlink();
  
  // OTA
  ArduinoOTA.handle();
}

// ------------------------
void blink()	{
// ------------------------
	LED_ON; 
	delay(100); 
	LED_OFF; 
	delay(400);
}

// ------------------------
void errorBlink()	{
// ------------------------
	for(int i = 0; i < 100; i++)	{
		LED_ON; 
		delay(50); 
		LED_OFF; 
		delay(50);
	}
}

void statusBlink() {
  static unsigned long time = 0;
  if(millis() > time + 1000 ) {
    if(network.isConnecting()) {
      LED_OFF;
    }
    else if(network.isConnected()) {
      LED_ON; 
  		delay(50); 
  		LED_OFF; 
    }
    else {
      LED_ON;
    }
    time = millis();
  }

  // SPI bus not ready
	//if(millis() < spiBlockoutTime)
	//	blink();
}
