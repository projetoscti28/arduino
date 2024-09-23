#include "DHT.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "esp_system.h"
#include "NTPClient.h"


#define DHTPIN 18
#define DHTTYPE DHT22

//Variaveis  esp_random
uint64_t chipid = 0;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);  

DHT dht(DHTPIN, DHTTYPE);

WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", 0, 60000);

void setup() 
{
  Serial.begin(115200);
  Serial.println("DHTxx test!");
  
  //inicializa o sensor dht
  dht.begin();
  
  // Conectar WiFi
  const char *SSID = "Redmi Note 9S"; 
  const char *PWD = "William111";
  wifi_connect(SSID, PWD);

  //servidor ntp
  ntp.begin();
  ntp.forceUpdate(); 

  //MQTT
  char *mqttServer = "200.145.153.203";
  int mqttPort = 1883;

  mqtt_connect(mqttServer, mqttPort);

  //Mac Address ESP
  chipid= ESP.getEfuseMac();
  Serial.printf("%llu\n",chipid);
}

void wifi_connect (const char *SSID, const char *PWD)
{
  WiFi.begin(SSID,PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf("...");
    delay(500);
  }
  
  Serial.print("Connected.\n");
  Serial.println(WiFi.localIP());
}

void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Callback - Message:");
  for (uint i = 0; i < length; i++) ]
  {
    Serial.print((char)payload[i]);
  }
}

void mqtt_connect (const char *mqttServer, int mqttPort)
{
  mqttClient.setServer(server, port); 
  mqttClient.setCallback(callback);

  if (mqttClient.connect("ESP32_DHT22"))
  {
    Serial.printf("Connecting ...\n");
    mqttClient.subscribe("/commands");
  }
}

void mqtt_reconnect ()
{
  while (!mqttClient.connected())
  {
    Serial.printf("Reconnecting to Mqtt Broker ..\n");

    if (mqttClient.connect("ESP32_DHT22"))
    {
      Serial.printf("Connecting \n");
      mqttClient.subscribe("/commands");
    }
  }
  
  Serial.printf("Conected to Mqtt Broker ...\n");
}

void loop() 
{
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if ( isnan(humidity) || isnan(temperature) ) 
  {
    Serial.println("Failed to read from DHT sensor!\n");
    return;
  }
  Serial.printf("Umidade: %.2f  \n", humidity, "%");
  Serial.printf("Temperatura: %.2f °C \n", temperature);

  // pega a hora via ntp
  long time = ntp.getEpochTime(); 

// Verificando conexao
  if ( !mqttClient.connected() )
  {
    mqtt_reconnect();
  }
  mqttClient.loop();

   long now = millis();

  //variavel estatica usada para receber o valor de now 
  static long last_time = 0;

  //envio de 5 em 5 segundos os valores do sensor para o broker mqtt
  if(now - last_time >5000)
  {
    //DHT11 Read
    char data [128] = {0};
    snprintf(data,128,"{\"Id\":%llu,\"Temperature\":%d,\"Date\":%ld,\"Humidity\":%d}",chipid, temperature, time, humidity);
    
    mqttClient.publish("dht/data", data);
    
    last_time=now;
  }
  delay(1000);
}

