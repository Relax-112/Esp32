#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include <WiFi.h>
#include <PubSubClient.h>

// 🔹 Configurações de Wi-Fi
const char* ssid = "NET";
const char* password = "PASSE NET";

// 🔹 Configurações do MQTT
const char* mqtt_server = "IP"; // Use o IP correto do broker
const int mqtt_port = 1883;
const char* mqtt_topic = "casa/quarto/temperatura";

WiFiClient espClient;
PubSubClient client(espClient);

// 🔹 Configuração do MLX90640
Adafruit_MLX90640 mlx90640;
float mlx90640To[768]; // Buffer para armazenar as temperaturas de 32x24 pixels

// 🔹 Definição dos pinos do LED e buzzer
const int ledPin = 2;  
const int buzzerPin = 4;

void setup() {
  Serial.begin(115200);

  // Inicializa o Wi-Fi e MQTT
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);

  // Configura pinos de saída
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  // Configura o I2C
  Wire.begin(19, 5); // SDA no GPIO19, SCL no GPIO5
  Serial.println("Inicializando MLX90640...");

  // Inicializa o sensor de temperatura
  if (!mlx90640.begin(0x33, &Wire)) {
    Serial.println("Erro ao inicializar o MLX90640!");
    while (1);
  }
  Serial.println("Sensor MLX90640 pronto!");

  mlx90640.setMode(MLX90640_CHESS);
  mlx90640.setRefreshRate(MLX90640_2_HZ); // Aumentei para 2Hz para melhor resposta
}

void loop() {
  // Conectar ao MQTT se necessário
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  Serial.println("Lendo temperatura...");

  if (mlx90640.getFrame(mlx90640To) != 0) {
    Serial.println("Erro ao ler o frame!");
    return;
  }

  // 🔹 Calcula temperatura média
  float somaTemperaturas = 0;
  for (int i = 0; i < 768; i++) {
    somaTemperaturas += mlx90640To[i];
  }
  float temperaturaMedia = somaTemperaturas / 768;

  // 🔹 Publica a temperatura no MQTT
  char msg[10];
  snprintf(msg, 10, "%.2f", temperaturaMedia);
  client.publish(mqtt_topic, msg);
  Serial.print("Temperatura publicada: ");
  Serial.println(msg);

  // 🔹 Controle do LED e do buzzer
  if (temperaturaMedia > 30.0) {
    digitalWrite(ledPin, HIGH);
    tone(buzzerPin, 1000);
  } else {
    digitalWrite(ledPin, LOW);
    noTone(buzzerPin);
  }

  delay(5000); // Espera 5 segundos antes da próxima leitura
}

// 🔹 Configuração do Wi-Fi
void setup_wifi() {
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
}

// 🔹 Reconectar ao MQTT caso perca a conexão
void reconnect() {
  while (!client.connected()) {
    Serial.println("Tentando conectar ao MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Conectado ao MQTT!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}
