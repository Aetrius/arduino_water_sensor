#include <Arduino.h>
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Ticker.h>

#define DEBUG true // switch to "false" for production
Ticker ticker;
WiFiClient espClient;
PubSubClient client(espClient);

#define sensorPower D6 // Power pin (GPIO 12)
#define sensorPin A0 // Analog Sensor pin (example pin A0)
#define durationSleep 30 // seconds
#define NB_TRYWIFI 20 // WiFi connection retries

int sensorData = 0;

// **************
void tick();
void loop();
void setup();
int readSensor();
void connectToHass();
void connectToWiFi();
void publishAlarmToHass(int waterLevel);
// **************

void tick()
{
    // toggle state
    int state = digitalRead(LED_BUILTIN);  // get the current state of LED_BUILTIN pin
    digitalWrite(LED_BUILTIN, !state);     // set pin to the opposite state
}

/**
 * This is a function used to get the reading
 * @return
 */
int readSensor()
{
    // Step 1: Turn the sensor ON by providing power to sensorPower pin
    digitalWrite(sensorPower, HIGH);
    // Step 2: Wait for a short moment
    delay(10);
    // Step 3: Perform the reading
    sensorData = analogRead(sensorPin);
    // Step 4: Turn the sensor OFF
    digitalWrite(sensorPower, LOW);

    return sensorData;
}

/**
 * Establishes WiFi connection
 * @return
 */
void connectToWiFi()
{
    int _try = 0;
    WiFi.mode(WIFI_STA);

    IPAddress ip(192, 168, 2, 67);  // Define the desired IP address
    IPAddress gateway(192, 168, 1, 1);  // Define the gateway address
    IPAddress subnet(255, 255, 255, 0);  // Define the subnet mask

    WiFi.config(ip, gateway, subnet);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    if (DEBUG == true) {
        Serial.println("Connecting to Wi-Fi");
    }

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        _try++;
        if (_try >= NB_TRYWIFI) {
            if (DEBUG == true) {
                Serial.println("Impossible to connect WiFi, going to deep sleep");
            }
            // ESP.deepSleep(durationSleep * 1000000);
        }
    }

    if (DEBUG == true) {
        Serial.println("Connected to Wi-Fi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    }
}

/**
 * Establishes connection to Home Assistant MQTT Broker
 * @return
 */
void connectToHass()
{
    client.setServer(MQTT_SERVER, 1883);

    // Loop until we're reconnected
    while (!client.connected()) {
        if (DEBUG == true) {
            Serial.print("Attempting MQTT connection...");
        }
        // Attempt to connect
        // If you do not want to use a username and password, change the next line to
        // if (client.connect("ESP8266Client")) {
        if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD)) {
            if (DEBUG == true) {
                Serial.println("connected");
            }
        } else {
            if (DEBUG == true) {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
            }
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

/**
 * Publishes notification to MQTT topic
 * @return
 */
void publishAlarmToHass(int waterLevel)
{
    // Create JSON string
    String json = "{\"water_level\": " + String(waterLevel) + "}";

    // Publish the JSON string to Hass through MQTT
    client.publish(MQTT_PUBLISH_TOPIC, json.c_str(), true);
    client.loop();
    if (DEBUG == true) {
        Serial.println("Alarm sent to Hass!");
    }
}

void setup()
{
    Serial.begin(115200);
    // Only print debug messages to serial if we're in debug mode
    //if (DEBUG == true) {
    delay(100);
    digitalWrite(LED_BUILTIN, LOW); // Turns the LED on
    Serial.print("D1 Mini V4.0.0 Waking up...");
    //}

    // Step 1: Set sensorPower pin as an output pin ready to receive power
    pinMode(sensorPower, OUTPUT);
    digitalWrite(sensorPower, LOW);

    // Step 2: Attach ticker to toggle LED_BUILTIN every 0.5 seconds
    ticker.attach(0.5, tick);
    delay(1000); // Wait for a second
    digitalWrite(LED_BUILTIN, HIGH);  // LED Pin set to High to turn on
    // Configure GPIO 16 with internal pull-up resistor
    pinMode(16, INPUT_PULLUP);

    // Set GPIO 16 LOW to trigger a reset when waking up
    digitalWrite(16, LOW);

    // Step 3: Wake the sensor up and get a reading
    int waterLevel = readSensor();

    if (DEBUG == true) {
        Serial.print("Water Level: ");
        Serial.println(waterLevel);
    }

    // Step 4: If water is detected then
    connectToWiFi(); // 1- connect to WiFi
    connectToHass(); // 2- connect to Home Assistant MQTT broker
    publishAlarmToHass(waterLevel); // 3- publish the water level on the MQTT topic

    // Step 5: Turn off power to sensor before going to sleep
    digitalWrite(sensorPower, LOW);

    if (DEBUG == true) {
        Serial.println("Finished run... Waiting to execute again...");
    }

    // Step 6: Go back to sleep for the next 5 seconds
    ESP.deepSleep(durationSleep * 1000000);
}

void loop()
{
  // Code inside loop function should never execute
  // as the ESP8266 will go to sleep after setup.
  // This function is just a placeholder and will never be executed.
}
