#define TINY_GSM_MODEM_A6

#include <TinyGsmClient.h>
#include <TinyGPS++.h>

#include <SoftwareSerial.h>
#include <AltSoftSerial.h>
#include <LowPower.h>

SoftwareSerial SerialAT(5, 6); // RX, TX
static const uint32_t GPSBaud = 9600;

// The TinyGPS++ object
TinyGPSPlus gps;

// The serial connection to the GPS device
AltSoftSerial altSerial;

#define TINY_GSM_DEBUG Serial
//#define DUMP_AT_COMMANDS

#if !defined(TINY_GSM_RX_BUFFER)
#define TINY_GSM_RX_BUFFER 650
#endif

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[]  = "telkomsel";
const char gprsUser[] = "wap";
const char gprsPass[] = "wap123";

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// Server details
const char serverName[] = "yourServerName";

/* create an instance of PubSubClient client */
TinyGsmClient client(modem);

int timer = 300; //Default timer 3 Menit
// SETTING MESSAGE
long lastMsg = 0;
char msg[100];
int sleepMode = 60000; //set 1 minute for sleepMode

//KONEKSI MODEM
void modemConnect() {
#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  Serial.print("Waiting for network...");
  while (!modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(3000);
    return;
  }
  Serial.println(" success");

  if (modem.isNetworkConnected()) {
    Serial.println("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(3000);
//    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);  
    return;
  }
  Serial.println(" success");

  if (modem.isGprsConnected()) {
    Serial.println("GPRS connected");
  }
#endif
}


void setup() {
  Serial.begin(9600);
  delay(10);
  altSerial.begin(GPSBaud);
  Serial.println("Wait...");
  SerialAT.begin(9600);
  delay(3000);
  Serial.println("Initializing modem...");
  //  modem.restart();
  modem.init();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem Info: ");
  Serial.println(modemInfo);

#if TINY_GSM_USE_GPRS
  // Unlock your SIM card with a PIN if needed
  if ( GSM_PIN && modem.getSimStatus() != 3 ) {
    modem.simUnlock(GSM_PIN);
  }
#endif
  modemConnect();
}

void loop() {

  while (altSerial.available() > 0)
    if (gps.encode(altSerial.read()))
      if (millis() > timer && gps.charsProcessed() < 10)
      {
        Serial.println(F("No GPS detected: check wiring."));
        while (true);
      }

  if (!modem.isNetworkConnected()) {
    modemConnect();
  } else {
    if (gps.location.isValid())
    {
      if (client.connect(serverName, 80)) {
        Serial.print(gps.location.lat(), 6);
        Serial.print(F(","));
        Serial.print(gps.location.lng(), 6);
        Serial.println("");
        double gpsLng = gps.location.lng();
        double gpsLat = gps.location.lat();
        String contentType = "application/x-www-form-urlencoded";
        String postData = "key_marker=upload_key_marker_here&lng_tracking=" + String(gpsLng, 6) + "&lat_tracking=" + String(gpsLat, 6);
        client.println("POST yourEndpoint HTTP/1.1");
        client.println("Host: yourhost");
        client.println("Content-Type: application/x-www-form-urlencoded");
        client.println("Connection: close");
        client.println("User-Agent: Arduino/1.0");
        client.print("Content-Length: ");
        client.println(postData.length());
        client.println();
        client.print(postData);
        client.println();
      } else {
        Serial.println("connection failed");
      }
    } else {
      Serial.println("GPS NOT AVAILABLE");
    }
    delay(timer);
  }
}
