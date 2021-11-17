#include <WiFi.h>
#include <PubSubClient.h>


/* Definições do LED */
#define tofflight     25


/* Defines do MQTT */
#define TOPIC_RX  "ESP/Embarcados_RX"
#define TOPIC_TX   "ESP/Embarcados_TX"


#define CLIENT_ID  "Embarcados"//ID do dispositivo (Client Id, também chamado de client name)


const char* SSID = "Gustavo1"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "Gustavo1"; // Senha da rede WI-FI que deseja se conectar

const char* SERVER = "baja.maua.br"; //Servidor (broker)
int PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

/* Prototypes */
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);

//variables declaration:

int convtensao;
int convcorrente;


/* Função: inicializa e conecta-se na rede WI-FI desejada
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initWiFi(void)
{
  delay(10);
  Serial.println("------Conexao WI-FI------");
  Serial.print("Conectando-se na rede: ");
  Serial.println(SSID);
  Serial.println("Aguarde");

  reconnectWiFi();
}

/* Função: inicializa parâmetros de conexão MQTT(endereço do
           broker, porta e seta função de callback)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void initMQTT(void)
{
  MQTT.setServer(SERVER, PORT);   //informa qual broker e porta deve ser conectado
  MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

/* Função: função de callback
           esta função é chamada toda vez que uma informação de
           um dos tópicos subescritos chega)
   Parâmetros: nenhum
   Retorno: nenhum
*/
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{
  String msg;

  /* obtem a string do payload recebido */
  for (int i = 0; i < length; i++)
  {
    char c = (char)payload[i];
    msg += c;
  }

  Serial.print("Chegou a seguinte string via MQTT: ");
  Serial.println(msg);

  /* toma ação dependendo da string recebida */
  if (msg.equals("1"))
  {
    digitalWrite(tofflight, HIGH);
    Serial.print("LED aceso mediante comando MQTT");
  }

  if (msg.equals("0"))
  {
    digitalWrite(tofflight, LOW);
    Serial.print("LED apagado mediante comando MQTT");
  }
}

/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
           em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectMQTT(void)
{
  while (!MQTT.connected())
  {
    Serial.print("* Tentando se conectar ao Broker MQTT: ");
    Serial.println(SERVER);
    if (MQTT.connect(CLIENT_ID))
    {
      Serial.println("Conectado com sucesso ao broker MQTT!");
      MQTT.subscribe(TOPIC_RX);
    }
    else
    {
      Serial.println("Falha ao reconectar no broker.");
      Serial.println("Havera nova tentatica de conexao em 2s");
      delay(2000);
    }
  }
}
/* Função: verifica o estado das conexões WiFI e ao broker MQTT.
           Em caso de desconexão (qualquer uma das duas), a conexão
           é refeita.
   Parâmetros: nenhum
   Retorno: nenhum
*/
void VerificaConexoesWiFIEMQTT(void)
{
  if (!MQTT.connected())
    reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

  reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

/* Função: reconecta-se ao WiFi
   Parâmetros: nenhum
   Retorno: nenhum
*/
void reconnectWiFi(void)
{
  //se já está conectado a rede WI-FI, nada é feito.
  //Caso contrário, são efetuadas tentativas de conexão
  if (WiFi.status() == WL_CONNECTED)
    return;

  WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado com sucesso na rede ");
  Serial.print(SSID);
  Serial.println("IP obtido: ");
  Serial.println(WiFi.localIP());
}

/* Função de setup */
void setup()
{
  Serial.begin(115200);

  /* Configuração do pino ligado ao LED como output
     e inicialização do mesmo em LOW */
  pinMode(tofflight, OUTPUT);
  digitalWrite(tofflight, LOW);

  /* Configuração do pino dos sensores como INPUT */
  pinMode(34, INPUT);
  pinMode(35, INPUT);

  /* Inicializa a conexao wi-fi */
  initWiFi();

  /* Inicializa a conexao ao broker MQTT */
  initMQTT();
}

/* Loop principal */
void loop()
{

  /* garante funcionamento das conexões WiFi e ao broker MQTT */
  VerificaConexoesWiFIEMQTT();

  /*  Envia as strings ao dashboard MQTT */
  getData();
  sendData();

  /* keep-alive da comunicação com broker MQTT */
  MQTT.loop();

  /* Refaz o ciclo após 500 milisegundos */
  delay(500);
}

void getData() {

  convtensao = analogRead(34);
  convtensao = map(convtensao, 0, 4095, 0, 36);
  Serial.print("Tensão no Conversor = ");
  Serial.println(convtensao);

 convcorrente = map(analogRead(35), 0, 4095, 0, 200);
  Serial.print("Corrente no Conversor = ");
  Serial.println(convcorrente);

  printf("==============================================================================\n");

  delay (500);
}

void sendData() {
  if (!MQTT.connected())
    reconnectWiFi();

  if (sendValues(convtensao, convcorrente))
  {
    Serial.println("Successfully send data");
  }
  else
  {
    Serial.println("Failed to send data");
  }

}



bool sendValues (int convtensao, int convcorrente)
{
  char json[250];
  sprintf(json,  "{\"convtensao\":%d, \"convcorrente\":%d}", convtensao, convcorrente);

  if (!MQTT.publish(TOPIC_TX, json))
    return false;
  else
    return true;
}
