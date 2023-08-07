#include "secrets.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"


#include "DHT.h"
#define DHTPIN 4     // Pin digital conectado al sensor DHT
#define DHTTYPE DHT11   // Tipo de sensor DHT (DHT 11)


#define AWS_IOT_PUBLISH_TOPIC   "mobile/mensajes"   // Tema MQTT para publicar datos
#define AWS_IOT_SUBSCRIBE_TOPIC "sensor/command"   // Tema MQTT para suscribirse a mensajes entrantes


float h;   // Variable para almacenar la humedad
float t;   // Variable para almacenar la temperatura


DHT dht(DHTPIN, DHTTYPE);   // Crear una instancia del sensor DHT


WiFiClientSecure net = WiFiClientSecure();   // Cliente seguro de WiFi
PubSubClient client(net);   // Cliente MQTT usando el cliente seguro de WiFi


void connectAWS()
{
  // Configurar el modo de Wi-Fi como estación y conectar a la red Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
 
  Serial.println("Connecting to Wi-Fi");
 
  // Esperar hasta que se establezca la conexión Wi-Fi
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }


  // Configurar WiFiClientSecure para usar las credenciales del dispositivo AWS IoT
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
 
  // Conectar al broker MQTT en el punto de conexión de AWS IoT definido anteriormente
  client.setServer(AWS_IOT_ENDPOINT, 8883);

  // Configurar el tiempo de keep-alive a 60 segundos
  client.setKeepAlive(60);

  // Crear un manejador de mensajes
  client.setCallback(messageHandler);
 
  Serial.println("Connecting to AWS IOT");
 
  // Intentar conectarse a AWS IoT
  while (!client.connect(THINGNAME))
  {
    Serial.print(".");
    delay(100);
  }


  // Si no se pudo conectar a AWS IoT, mostrar un mensaje de error
  if (!client.connected())
  {
    Serial.println("AWS IoT Timeout!");
    return;
  }


  // Suscribirse a un tema para recibir mensajes
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
 
  Serial.println("AWS IoT Connected!");
}


// Función para publicar datos de temperatura y humedad en formato JSON a AWS IoT
void publishMessage()
{
  unsigned long currentTimestamp = millis();


  // Crear un objeto JSON para almacenar los datos
  StaticJsonDocument<200> doc;
  // Timestamp
  doc["timestamp"] = currentTimestamp;
  // Values
  doc["humidity"] = h;
  doc["temperature"] = t;
  // Unit
  doc["humidityUnit"] = "%";
  doc["temperatureUnit"] = "°C";
  // Notes
  doc["notes"] = "TEST";  
  char jsonBuffer[512];
 
  // Serializar el objeto JSON en una cadena JSON
  serializeJson(doc, jsonBuffer);
 
  // Publicar la cadena JSON en el tema MQTT
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}


// Función que se ejecuta cuando llega un mensaje en el tema suscrito
void messageHandler(char* topic, byte* payload, unsigned int length)
{
  Serial.print("incoming: ");
  Serial.println(topic);
 
  // Analizar el mensaje JSON recibido
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);
  const char* message = doc["message"];
  Serial.println(message);
  int value = doc["value"];
  Serial.println(value);

}


void setup()
{
  Serial.begin(115200);
  // Conectar a AWS IoT
  connectAWS();
  // Inicializar el sensor DHT
  dht.begin();
}


void loop()
{
  // Leer la humedad y temperatura del sensor DHT
  h = dht.readHumidity();
  t = dht.readTemperature();
 
  // Verificar si las lecturas del sensor son válidas (no sean NaN)
  if (isnan(h) || isnan(t))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
 
  // Mostrar la humedad y temperatura en el monitor serial
  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.println(F("°C "));
 
  // Publicar los datos en AWS IoT
  publishMessage();
  // Realizar el ciclo de manejo de MQTT
  client.loop();
  // Esperar 30 segundos antes de leer y publicar nuevamente los datos
  delay(10000);
}
