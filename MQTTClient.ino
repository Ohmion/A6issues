// Select your modem:
#define TINY_GSM_MODEM_A6

// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

// Define the serial console for debug prints, if needed
#define TINY_GSM_DEBUG SerialMon

// Add a reception delay, if needed
#define TINY_GSM_YIELD() { delay(2); }

#define SerialMon Serial
#define SerialAT Serial3

// Your GPRS credentials
const char apn[]  = "orangeworld";
const char gprsUser[] = "orange";
const char gprsPass[] = "orange";

// MQTT details
const char* broker = "m21.cloudmqtt.com";

const char* topicLed = "GsmClientTest/led";
const char* topicInit = "GsmClientTest/init";
const char* topicLedStatus = "GsmClientTest/ledStatus";

#include <TinyGsmClient.h>
#include <PubSubClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else

TinyGsm modem(SerialAT);
#endif
TinyGsmClient client(modem);
PubSubClient mqtt(client);

#define LED_PIN 13
int ledStatus = LOW;

long lastReconnectAttempt = 0;

void setup() {

  // Set console baud rate
  SerialMon.begin(115200);
  delay(10);

  // Set your reset, enable, power pins here
  pinMode(LED_PIN, OUTPUT);

  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);

  pinMode(23, OUTPUT);
  digitalWrite(23, LOW);

  SerialMon.println("Wait...");

  // Set GSM module baud rate
  SerialAT.begin(115200);
  delay(3000);

  // Restart takes quite some time
  // To skip it, call init() instead of restart()
  SerialMon.println("Initializing modem...");
  modem.restart();
  // modem.init();

  String modemInfo = modem.getModemInfo();
  SerialMon.print("Modem: ");
  SerialMon.println(modemInfo);

  // Unlock your SIM card with a PIN
  modem.simUnlock("xxxx");

#if TINY_GSM_USE_WIFI
  SerialMon.print(F("Setting SSID/password..."));
  if (!modem.networkConnect(wifiSSID, wifiPass)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");
#endif

#if TINY_GSM_USE_GPRS && defined TINY_GSM_MODEM_XBEE
  // The XBee must run the gprsConnect function BEFORE waiting for network!
  modem.gprsConnect(apn, gprsUser, gprsPass);
#endif

  SerialMon.print("Waiting for network...");
  if (!modem.waitForNetwork(240000L)) {
    SerialMon.println(" fail");
    delay(10000);
    return;
  }
  SerialMon.println(" OK");

  if (modem.isNetworkConnected()) {
    SerialMon.println("Network connected");
  }


    SerialMon.print(F("Connecting to "));
  SerialMon.print(apn);
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
    SerialMon.println(" fail");
      delay(10000);
      return;
  }
  SerialMon.println(" OK");


  // MQTT Broker setup
  mqtt.setServer(broker, 12492);
  mqtt.setCallback(mqttCallback);
}

boolean mqttConnect() {
  SerialMon.print("Connecting to ");
  SerialMon.print(broker);

  // Connect to MQTT Broker
  //boolean status = mqtt.connect("GsmClientTest");

  // Or, if you want to authenticate MQTT:
  boolean status = mqtt.connect("GsmClientName", "xxxx", "xxxx");

  if (status == false) {
    SerialMon.println(" fail");
    return false;
  }
  SerialMon.println(" OK");
  mqtt.publish(topicInit, "GsmClientTest started");
  mqtt.subscribe(topicLed);
  return mqtt.connected();
}

void loop() {

  if (!mqtt.connected()) {
    SerialMon.println("=== MQTT NOT CONNECTED ===");
    // Reconnect every 10 seconds
    unsigned long t = millis();
    if (t - lastReconnectAttempt > 10000L) {
      lastReconnectAttempt = t;
      if (mqttConnect()) {
        lastReconnectAttempt = 0;
      }
    }
    delay(100);
    return;
  }

  mqtt.loop();
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  SerialMon.print("Message arrived [");
  SerialMon.print(topic);
  SerialMon.print("]: ");
  SerialMon.write(payload, len);
  SerialMon.println();

  // Only proceed if incoming message's topic matches
  if (String(topic) == topicLed) {
    ledStatus = !ledStatus;
    digitalWrite(LED_PIN, ledStatus);
    mqtt.publish(topicLedStatus, ledStatus ? "1" : "0");
  }
}
