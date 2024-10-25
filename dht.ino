#include "DHT.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "esp_system.h"
#include "NTPClient.h"

// Configurações do sensor DHT22 e do pino
#define DHTPIN 18
#define DHTTYPE DHT22

// Variável para armazenar o ID do chip ESP32
uint64_t chipid = 0;

// Instâncias de cliente WiFi e MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);  

// Instância do sensor DHT
DHT dht(DHTPIN, DHTTYPE);

// Instância para obter horário via NTP (Network Time Protocol)
WiFiUDP udp;
NTPClient ntp(udp, "a.st1.ntp.br", 0, 60000); // Define servidor e fuso horário (UTC)

void setup() 
{
  Serial.begin(115200);
  Serial.println("Iniciando DHT e conexões...");

  // Inicializa o sensor DHT
  dht.begin();

  // Conecta ao WiFi
  const char *SSID = "Redmi Note 9S"; 
  const char *PWD = "William111";
  wifi_connect(SSID, PWD);

  // Inicializa e atualiza o cliente NTP
  ntp.begin();
  ntp.forceUpdate();

  // Configuração do servidor MQTT
  char *mqttServer = "200.145.153.203";
  int mqttPort = 1883;
  mqtt_connect(mqttServer, mqttPort);

  // Obtenção do endereço MAC do ESP
  chipid = ESP.getEfuseMac();
  Serial.printf("MAC ID: %llu\n", chipid);
}

// Função para conectar ao WiFi
void wifi_connect (const char *SSID, const char *PWD)
{
  WiFi.begin(SSID, PWD);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(500);
  }
  
  Serial.println("Conectado ao WiFi.");
  Serial.println(WiFi.localIP());
}

// Callback para processar mensagens MQTT recebidas
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Mensagem recebida: ");
  for (uint i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
}

// Função para configurar e conectar ao broker MQTT
void mqtt_connect (const char *mqttServer, int mqttPort)
{
  mqttClient.setServer(mqttServer, mqttPort); 
  mqttClient.setCallback(mqtt_callback);

  if (mqttClient.connect("ESP32_DHT22"))
  {
    Serial.printf("Conectando ao MQTT...\n");
    mqttClient.subscribe("/commands"); // Inscrição no tópico de comandos
  }
}

// Função para reconectar ao MQTT se desconectado
void mqtt_reconnect ()
{
  while (!mqttClient.connected())
  {
    Serial.printf("Reconectando ao broker MQTT...\n");

    if (mqttClient.connect("ESP32_DHT22"))
    {
      Serial.printf("Conectado ao MQTT\n");
      mqttClient.subscribe("/commands");
    }
  }
}

// Loop principal do programa
void loop() 
{
  // Lê dados do sensor DHT
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // Verifica se a leitura do sensor falhou
  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("Falha ao ler o sensor DHT!");
    return;
  }
  Serial.printf("Umidade: %.2f%%\n", humidity);
  Serial.printf("Temperatura: %.2f °C\n", temperature);

  // Obtém o horário via NTP
  long time = ntp.getEpochTime(); 

  // Verifica se o MQTT está conectado
  if (!mqttClient.connected())
  {
    mqtt_reconnect();
  }
  mqttClient.loop();

  // Envia os dados do sensor a cada 5 segundos
  long now = millis();
  static long last_time = 0;

  if (now - last_time > 5000)
  {
    // Prepara dados JSON para envio via MQTT
    char data[128] = {0};
    snprintf(data, 128, "{\"Id\":%llu,\"Temperature\":%d,\"Date\":%ld,\"Humidity\":%d}", chipid, temperature, time, humidity);
    
    mqttClient.publish("dht/data", data); // Publica os dados no tópico MQTT
    last_time = now;
  }
  delay(1000); // Pausa de 1 segundo entre as leituras
}
