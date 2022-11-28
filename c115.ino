#define SERVO 5
#define LED_GREEN 4
#define LED_RED 0
#define BUTTON 2
#define PIR 13

#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Rede Antonio_2.4GHz";
const char* WIFI_PASSWORD = "25510000";
const char* MQTT_BROKER = "test.mosquitto.org";
const char* MQTT_TOPIC_STATUS[] = {"smartgate/status/text", "smartgate/status/code"};
const char* MQTT_TOPIC_SENSOR = "smartgate/sensor";
const char* MQTT_TOPIC_CONTROL = "smartgate/control";
const int MQTT_PORT = 1883;
unsigned long g_previousMillis = 0;
unsigned long s_previousMillis = 0;
const long g_interval = 2000;
const long s_interval = 30000;


WiFiClient espClient;
PubSubClient client(espClient);

bool isOpen = false;
bool isDetecting = false;
Servo s1;

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char) payload[i];  // convert *byte to string
  }
  Serial.print(message);
  Serial.println();
  Serial.println("-----------------------");

  if (!isDetecting)
    isOpen = !(message == "0");

}


void connectToWiFi() {
  Serial.print("Conectando à rede: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando ao WiFI...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi conectado");
  }
}

void connectToMQTT() {
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);

  while (!client.connected()) {
    char* client_id = "C115-smartgate-node-mcu";

    Serial.println("Conectando ao broker do mosquitto ...");
    if (client.connect(client_id)) {
      Serial.println("Conectou ao broker.");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }

  client.subscribe(MQTT_TOPIC_CONTROL);
}

void blinkLED(int LED) {
  digitalWrite(LED, HIGH);
  delay(300);
  digitalWrite(LED, LOW);
  delay(300);
}

void setup() {
  Serial.begin(19200);

  pinMode(SERVO, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUTTON, INPUT);
  pinMode(PIR, INPUT);

  s1.attach(SERVO);
  s1.write(0);

  connectToWiFi();
  connectToMQTT();
}

void loop() {
  bool PIRValue = digitalRead(PIR);
  unsigned long currentMillis = millis();

  if (currentMillis - g_previousMillis >= g_interval) {
    g_previousMillis = currentMillis;
    client.publish(MQTT_TOPIC_STATUS[0], isOpen ? "Aberto" : "Fechado");
    client.publish(MQTT_TOPIC_STATUS[1], isOpen ? "1" : "0");
    client.publish(MQTT_TOPIC_SENSOR, isDetecting ? "Obstruído" : "Não obstruído");
  }
  if (currentMillis - s_previousMillis >= s_interval) {
    s_previousMillis = currentMillis;
    isDetecting = !isDetecting;
  }

  client.loop();

  if (digitalRead(BUTTON) == LOW && !isDetecting)
    isOpen = !isOpen;


  if (isOpen) {
    digitalWrite(LED_RED, LOW);
    blinkLED(LED_GREEN);
    s1.write(180);
  } else {
    digitalWrite(LED_GREEN, LOW);
    blinkLED(LED_RED);
    s1.write(0);
  }

}
