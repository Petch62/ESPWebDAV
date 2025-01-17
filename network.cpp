#include "network.h"
#include "serial.h"
#include "config.h"
#include "pins.h"
#include "ESP8266WiFi.h"
#include "ESPWebDAV.h"
#include "sdControl.h"

IPAddress staticIP(0,0,0,0);
IPAddress gateway(0,0,0,0);
IPAddress subnet(255,255,255,0);
IPAddress nullIp(0,0,0,0);

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +\
  String(ipAddress[1]) + String(".") +\
  String(ipAddress[2]) + String(".") +\
  String(ipAddress[3])  ;
}

bool Network::start() {
  staticIP.fromString(config.ip());
  gateway.fromString(config.gate());
  
  SERIAL_ECHO("SSID:");SERIAL_ECHOLN(config.ssid());
  SERIAL_ECHO("PASS:");SERIAL_ECHOLN(config.password());
  SERIAL_ECHO(" IP :");SERIAL_ECHOLN(config.ip());
  SERIAL_ECHO("GATE:");SERIAL_ECHOLN(config.gate());
  SERIAL_ECHOLN(" ");
//  SERIAL_ECHO("S IP:");SERIAL_ECHO(staticIP[0]);SERIAL_ECHO(".");SERIAL_ECHO(staticIP[1]);SERIAL_ECHO(".");SERIAL_ECHO(staticIP[2]);SERIAL_ECHO(".");SERIAL_ECHOLN(staticIP[3]);
//  SERIAL_ECHO("GATE:");SERIAL_ECHO(gateway[0]);SERIAL_ECHO(".");SERIAL_ECHO(gateway[1]);SERIAL_ECHO(".");SERIAL_ECHO(gateway[2]);SERIAL_ECHO(".");SERIAL_ECHOLN(gateway[3]);

  wifiConnected = false;
  wifiConnecting = true;
  
  // Set hostname first
  WiFi.hostname(HOSTNAME);
  // Reduce startup surge current
  WiFi.setAutoConnect(false);
  WiFi.mode(WIFI_STA);
  WiFi.setPhyMode(WIFI_PHY_MODE_11N);
  SERIAL_ECHO("DHCP:");
  if (staticIP != nullIp) {WiFi.config(staticIP, gateway, subnet, gateway);SERIAL_ECHOLN("OFF");} else {SERIAL_ECHOLN("ON");}
  WiFi.begin(config.ssid(), config.password());

  // Wait for connection
  unsigned int timeout = 0;
  while(WiFi.status() != WL_CONNECTED) {
    //blink();
    SERIAL_ECHO(".");
    timeout++;
    if(timeout++ > WIFI_CONNECT_TIMEOUT/100) {
      SERIAL_ECHOLN("");
      wifiConnecting = false;
      return false;
    }
    else
      delay(100);
  }

  SERIAL_ECHOLN("");
  SERIAL_ECHO("Connected to "); SERIAL_ECHOLN(config.ssid());
  SERIAL_ECHO("IP address: "); SERIAL_ECHOLN(WiFi.localIP());
  SERIAL_ECHO("RSSI: "); SERIAL_ECHOLN(WiFi.RSSI());
  SERIAL_ECHO("Mode: "); SERIAL_ECHOLN(WiFi.getPhyMode());
  SERIAL_ECHO("Asscess to SD at the Run prompt : \\\\"); SERIAL_ECHO(WiFi.localIP());SERIAL_ECHOLN("\\DavWWWRoot");

  wifiConnected = true;

  config.save();
  String sIp = IpAddress2String(WiFi.localIP());
  config.save_ip(sIp.c_str());

  SERIAL_ECHOLN("Going to start DAV server");
  if(startDAVServer() < 0) return false;
  wifiConnecting = false;

  return true;
}

int Network::startDAVServer() {
  if(!sdcontrol.canWeTakeBus()) {
    return -1;
  }
  sdcontrol.takeBusControl();
  
  // start the SD DAV server
  if(!dav.init(SD_CS, SPI_FULL_SPEED, SERVER_PORT))   {
    DBG_PRINT("ERROR: "); DBG_PRINTLN("Failed to initialize SD Card");
    // indicate error on LED
    //errorBlink();
    initFailed = true;
  }
  else {
    //blink();
  }
  
  sdcontrol.relinquishBusControl();
  DBG_PRINTLN("FYSETC WebDAV server started");
  dav.ntpforceupdate();
  return 0;
}

bool Network::isConnected() {
  return wifiConnected;
}

bool Network::isConnecting() {
  return wifiConnecting;
}

// a client is waiting and FS is ready and other SPI master is not using the bus
bool Network::ready() {
  if(!isConnected()) return false;
  
  // do it only if there is a need to read FS
	if(!dav.isClientWaiting())	return false;
	
	if(initFailed) {
	  dav.rejectClient("Failed to initialize SD Card");
	  return false;
	}
	
	// has other master been using the bus in last few seconds
	if(!sdcontrol.canWeTakeBus()) {
		dav.rejectClient("Marlin is reading from SD card");
		return false;
	}

	return true;
}

void Network::handle() {
  if(network.ready()) {
	  sdcontrol.takeBusControl();
	  dav.handleClient();
	  sdcontrol.relinquishBusControl();
	}
 dav.ntpupdate();
}

Network network;
