#include "common.h"
#include "myMqttClient.h"
#include "ssidConfig.h"

#define PIN_MQTT_LED_RED    15
#define PIN_MQTT_LED_BLUE   13
#define PIN_MQTT_LED_GREEN  12
#define PIN_MQTT_LED_ON   HIGH
#define PIN_MQTT_LED_OFF  LOW


#define DEV_PREFIX "/myiotgizmo/"

#define MQTT_SUB_PING      "ping"
#define MQTT_XUB_ALIVE     "alive"  // xub: sub and pub

#define MQTT_PUB_TICKS     "ticks"     // bumped every time alive is set
#define MQTT_PUB_OPER_MINS "oper_mins" // bumped every 10 minutes

// FWDs
bool checkWifiConnected();
bool checkMqttConnected();

MqttState mqttState;

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASSWORD);

// Create the feeds that will be used for publishing and subscribing to service changes.
// Be aware of MAXSUBSCRIPTIONS in ~/myArduinoLibraries/Adafruit_MQTT_Library/Adafruit_MQTT.h  -- currently set to 5
Adafruit_MQTT_Subscribe service_sub_ping = Adafruit_MQTT_Subscribe(&mqtt, DEV_PREFIX MQTT_SUB_PING);
Adafruit_MQTT_Subscribe service_sub_alive = Adafruit_MQTT_Subscribe(&mqtt, DEV_PREFIX MQTT_XUB_ALIVE);

Adafruit_MQTT_Publish service_pub_alive = Adafruit_MQTT_Publish(&mqtt, DEV_PREFIX MQTT_XUB_ALIVE, MQTT_QOS_1);
Adafruit_MQTT_Publish service_pub_ticks = Adafruit_MQTT_Publish(&mqtt, DEV_PREFIX MQTT_PUB_TICKS);
Adafruit_MQTT_Publish service_pub_oper_mins = Adafruit_MQTT_Publish(&mqtt, DEV_PREFIX MQTT_PUB_OPER_MINS);

static const char* const ONSTR = "on";
static const char* const OFFSTR = "off";

void initMyMqtt() {
    memset(&mqttState, 0, sizeof(mqttState));

    pinMode(PIN_MQTT_LED_RED, OUTPUT);
    pinMode(PIN_MQTT_LED_BLUE, OUTPUT);
    pinMode(PIN_MQTT_LED_GREEN, OUTPUT);

    digitalWrite(PIN_MQTT_LED_RED, PIN_MQTT_LED_ON);
    digitalWrite(PIN_MQTT_LED_BLUE, PIN_MQTT_LED_OFF);
    digitalWrite(PIN_MQTT_LED_GREEN, PIN_MQTT_LED_OFF);

#ifdef DEBUG
    // Connect to WiFi access point.
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
#endif

    // Set WiFi to station mode
    // https://www.exploreembedded.com/wiki/Arduino_Support_for_ESP8266_with_simple_test_code
    if (!WiFi.mode(WIFI_STA)) {
#ifdef DEBUG
        Serial.println("Fatal: unable to set wifi mode");
#endif
        delay(1000);
        ESP.restart();
    }
    WiFi.begin(WLAN_SSID, WLAN_PASS);
}

void myMqttLoop() {
    yield();  // make esp8266 happy

    if (!checkWifiConnected()) return;
    if (!checkMqttConnected()) return;

    // Listen for updates on any subscribed MQTT feeds and process them all.
    Adafruit_MQTT_Subscribe* subscription;
    while ((subscription = mqtt.readSubscription())) {
        const char* const message = (const char*) subscription->lastread;

        if (subscription == &service_sub_ping) {
	    ++state.mqtt_ticks;
	    sendOperState();
        } else if (subscription == &service_sub_alive) {
            // if message is not ONSTR then react byt changing it to ONSTR
	    if (strncmp(message, ONSTR, 2) == 0) {
#ifdef DEBUG
                Serial.println("service_sub_alive is stable: noop");
#endif
                continue;
            }

	    // a flip is worth 10k ticks :)
	    state.mqtt_ticks += 10000;

            if (service_pub_alive.publish(ONSTR)) {
#ifdef DEBUG
                Serial.print("published alive number: ");
                Serial.println(state.mqtt_ticks, DEC);
#endif
	    } else {
#ifdef DEBUG
                Serial.println("unable to publish mqtt: disconnecting");
#endif
                mqtt.disconnect();
            }
        } else {
#ifdef DEBUG
            Serial.print("got unexpected msg on subscription: "); 
            Serial.println(subscription->topic);
#endif
        }
    }
}

bool checkWifiConnected() {
    static bool lastConnected = false;

    const bool currConnected = WiFi.status() == WL_CONNECTED;

    if (lastConnected != currConnected) {

        if (currConnected) {
#ifdef DEBUG
            Serial.println("WiFi connected");
            Serial.print("IP address: "); Serial.println(WiFi.localIP());
#endif

            // idem potent. If it fails, this is a game stopper...
            if (!mqtt.subscribe(&service_sub_alive) ||
		!mqtt.subscribe(&service_sub_ping)) {
#ifdef DEBUG
                Serial.println("Fatal: unable to subscribe to mqtt");
                delay(9000);
#endif
                delay(1000);
                ESP.restart();
            }

        } else {
#ifdef DEBUG
            Serial.println("WiFi disconnected");
#endif
            digitalWrite(PIN_MQTT_LED_RED, PIN_MQTT_LED_ON);
            digitalWrite(PIN_MQTT_LED_GREEN, PIN_MQTT_LED_OFF);
        }

        lastConnected = currConnected;
    }

    return currConnected;
}

bool setWill() {
  // Set MQTT last will topic, payload, QOS, and retain. This needs
  // to be called before connect() because it is sent as part of the
  // connect control packet.
  // bool will(const char *topic, const char *payload, uint8_t qos = 0, uint8_t retain = 0);
  return mqtt.will(DEV_PREFIX MQTT_XUB_ALIVE, OFFSTR, MQTT_QOS_1, /*retain*/ 1);
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
bool checkMqttConnected() {
    static bool lastMqttConnected = false;

    // not attempt to reconnect if there are reconnect ticks outstanding
    if (mqttState.reconnectTicks > 0) return false;

    const bool currMqttConnected = mqtt.connected();

    // noop?
    if (lastMqttConnected && currMqttConnected) return true;

    digitalWrite(PIN_MQTT_LED_RED, currMqttConnected ? PIN_MQTT_LED_OFF : PIN_MQTT_LED_ON);
    digitalWrite(PIN_MQTT_LED_GREEN, currMqttConnected ? PIN_MQTT_LED_ON : PIN_MQTT_LED_OFF);

    if (currMqttConnected) {
#ifdef DEBUG
        Serial.println("MQTT Connected!");
#endif
	service_pub_alive.publish(ONSTR);
        sendOperState();
    } else {
#ifdef DEBUG
        Serial.print("MQTT is connecting... ");
#endif

	if (!setWill()) {
#ifdef DEBUG
	  Serial.println("Fatal: unable to set mqtt will");
#endif
	  delay(1000);
	  ESP.restart();
	}

        // Note: the connect call can block for up to 6 seconds.
        //       when mqtt is out... be aware.
        const int8_t ret = mqtt.connect();
        if (ret != 0) {
#ifdef DEBUG
            Serial.println(mqtt.connectErrorString(ret));
#endif
            mqtt.disconnect();

            // do not attempt to connect before a few ticks
            mqttState.reconnectTicks = defaultMqttReconnect;
        } else {
#ifdef DEBUG
        Serial.println("done.");
#endif
        }
    }

    lastMqttConnected = currMqttConnected;
    return currMqttConnected;
}

void mqtt1SecTick() {
    static uint8_t lastFlags = 0;
    static uint32_t lastLightModeChanges = 0;

    if (mqttState.reconnectTicks > 0) --mqttState.reconnectTicks;

#ifdef DEBUG
    if (! mqtt.connected()) {
        Serial.print("mqtt1SecTick"); 
        Serial.print(" reconnectTics: "); Serial.print(mqttState.reconnectTicks, DEC);
        Serial.print(" mqtt_led: "); Serial.print(digitalRead(PIN_MQTT_LED_GREEN) == PIN_MQTT_LED_ON ? "on" : "off");
        Serial.println("");
    }
#endif
}

void mqtt10MinTick() {
#ifdef DEBUG
    Serial.println("mqtt10MinTick -- sending gratuitous state");
#endif

    state.mqtt_oper_mins += 10;

    // gratuitous
    sendOperState();
}

void sendOperState() {
    if (! mqtt.connected()) return;

    if (!service_pub_ticks.publish(state.mqtt_ticks) ||
	!service_pub_oper_mins.publish(state.mqtt_oper_mins)) {
#ifdef DEBUG
        Serial.println("Unable to publish operational state");
#endif
        // commented out because that is not the end of the world
        // mqtt.disconnect();
    }
}
