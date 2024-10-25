const mqtt = require('mqtt');
const { Pool, Client } = require('pg'); // Biblioteca para PostgreSQL

// Configurações do Banco de Dados PostgreSQL
const client_database = new Client({
    user: 'projetoscti27',
    password: 'eq53B965',
    host: 'pgsql.projetoscti.com.br',
    database: 'projetoscti27',
});

// Conecta ao banco de dados
client_database.connect();

// Configuração do servidor MQTT
const port = "1883";
const host = `mqtt://192.168.1.112:${port}`;
var client = mqtt.connect(host);
let topic = '#'; // Tópico wildcard para receber todas as mensagens

// Conecta ao broker MQTT e subscreve ao tópico
client.on('connect', function success() {
  console.log("Conectado ao broker MQTT");
  client.subscribe(topic, function mqtt_subscribe() {
      console.log("Inscrito no tópico: " + topic);
  });
});

// Recebe e processa mensagens MQTT
client.on('message', mqtt_message);

// Função para processar mensagens recebidas
function mqtt_message(topic, message) {
    // Converte a mensagem JSON para objeto
    var J = JSON.parse(message);
    var T = J.Temperature;
    var H = J.Humidity;
    var D = J.Date;
    var I = J.Id;
    console.log('Tópico: ' + topic + ' Mensagem: ' + message);

    // Query SQL para inserir dados no banco
    const q = "INSERT INTO dht11_data(id, temperature, humidity, date) VALUES($1,$2,$3,$4) RETURNING *";
    const v = [I, T, H, D]; 
    
    // Executa a query no banco de dados PostgreSQL
    client_database.query(q, v, (err, res) => {
      if (err) 
      {
        console.log("Erro ao inserir dados: " + err.stack);
      } 
      else 
      {
        console.log("Dados inseridos: " + res.rows[0]);
      }
    });
}
