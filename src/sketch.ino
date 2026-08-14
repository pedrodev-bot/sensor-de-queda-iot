// --- Libraries ---
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include <math.h>
#include <iostream>
#include <limits>

Adafruit_MPU6050 mpu;

// --- Configuração do Servidor Web ---
WebServer server(80);
String estadoDispositivo = "Monitorando (Estável)";
String intensidadeUltimaQueda = "Nenhuma";
float aceleracaoAtual = 0.0;

// --- Inteligência do Dashboard ---
unsigned long tempoUltimoAlarme = 0;
int contadorSOS = 0;
int contadorQuedasCanceladas = 0;
int contadorQuedasConfirmadas = 0;
const unsigned long TEMPO_ALERTA_VISUAL = 120000; // Mantém a caixa vermelha por 2 minutos (120000 ms)

// --- Configuração Alertas ---
// Chave de Bypass para testes locais
const bool MODO_OFFLINE = false; // Mude para 'false' quando for apresentar com internet

// Credenciais do WiFi
const char* ssid = "";
const char* password = "";

// URL do Sistema de Alerta
String telegramToken = "";
String telegramChatID = "";

// Controle Bidirecional do Telegram
int ultimoUpdateIdTelegram = 0; 
unsigned long tempoUltimaChecagemTelegram = 0;
const unsigned long INTERVALO_CHECAGEM_TELEGRAM = 3000; // Checa mensagens a cada 3 segundos

// Configuração dos pinos (LED)
const int PINO_LED_STATUS = 2;
const int PINO_BUZZER = 5;

// Pinos do Sensor Touch
const int PINO_TOUCH_SINAL = 13;  // Fio Azul (Sinal do toque)
const int PINO_TOUCH_GND = 4;     // Fio Marrom (GND Virtual)
const int PINO_TOUCH_VCC = 14;    // Fio Roxo (3V3 Virtual)

// --- Parâmetros do Algoritmo ---
// Limiares em m/s^2 (A gravidade normal é ~9.8)
const float LIMIAR_QUEDA_LIVRE = 7.0;  // Perto de 0G
const float LIMIAR_IMPACTO = 15.0;     // Pico de impacto (aprox. 2.5G)
const int JANELA_TEMPO_MS = 2000;      // Tempo máximo entre a queda e o impacto

// Parâmetros de Inatividade e Bateria
unsigned long tempoUltimoMovimento = 0;
int nivelAlertaInatividade = 0; // Máquina de estados: 0=Normal, 1=Leve, 2=Moderado, 3=Grave

// Valores para TESTE DE BANCADA (1 min, 2 min, 3 min)
// Para o produto final, usar as multiplicações comentadas ao lado!
const unsigned long TEMPO_ALERTA_LEVE = 60000;      // Final: 45 * 60000 (45 minutos)
const unsigned long TEMPO_ALERTA_MODERADO = 120000; // Final: 180 * 60000 (3 horas)
const unsigned long TEMPO_ALERTA_GRAVE = 180000;    // Final: 540 * 60000 (9 horas)

const float LIMIAR_MOVIMENTO = 1.5; 

// --- Filtro de Movimento Sustentado ---
unsigned long ultimoPicoMovimento = 0;
unsigned long inicioMovimentoTemp = 0;
const unsigned long TEMPO_MOVIMENTO_CONTINUO = 4000; 

int nivelBateriaVirtual = 100;

// --- (DEBUG) Inicializando varíaveis para debug ---
/* DEBUG
float min_value = std::numeric_limits<float>::max();
float max_value = std::numeric_limits<float>::min(); */

// --- Constantes ---
// Variáveis de estado
bool emQuedaLivre = false;
unsigned long tempoInicioQueda = 0;

// --- TEMPLATE HTML DO DASHBOARD ---
const char paginaHTML[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Dashboard do Gadget</title>
  <style>
    body { font-family: 'Segoe UI', Arial, sans-serif; text-align: center; background-color: #f0f2f5; margin: 0; padding: 20px; }
    .card { background: white; padding: 30px; border-radius: 12px; box-shadow: 0 4px 15px rgba(0,0,0,0.1); max-width: 450px; margin: 0 auto; }
    h2 { color: #333; margin-top: 0;}
    .status-box { font-size: 22px; font-weight: bold; padding: 15px; border-radius: 8px; margin: 20px 0; background-color: #e7f3ff; color: #0056b3; border: 2px solid #b8daff; transition: all 0.3s; }
    .status-alerta { background-color: #ffcccc; color: #cc0000; border-color: #ff9999; }
    .data-text { font-size: 18px; color: #555; margin-bottom: 25px; }
    .historico-container { display: flex; flex-direction: column; gap: 12px; text-align: left; background: #f9f9f9; padding: 20px; border-radius: 8px; border: 1px solid #eee; }
    .hist-item { display: flex; justify-content: space-between; align-items: center; font-size: 15px; color: #444; font-weight: 500;}
    .badge { font-weight: bold; font-size: 16px; padding: 4px 12px; border-radius: 20px; }
    .badge-sos { background: #ffe6e6; color: #cc0000; border: 1px solid #ffcccc; }
    .badge-canc { background: #e6ffe6; color: #008000; border: 1px solid #ccffcc; }
    .badge-conf { background: #fff0b3; color: #b38600; border: 1px solid #ffe680; }
    .badge-tempo { background: #e6f2ff; color: #0066cc; border: 1px solid #cce5ff; }
  </style>
  <script>
    setInterval(function() {
      fetch('/dados')
        .then(response => response.json())
        .then(data => {
          let statusDiv = document.getElementById("status");
          statusDiv.innerText = data.estado;
          
          let blocoGravidade = document.getElementById("blocoGravidade");
          
          if(data.estado.includes("PERIGO") && data.intensidade !== "Nenhuma") {
            blocoGravidade.style.display = "block";
            document.getElementById("intensidade").innerText = data.intensidade;
          } else {
            blocoGravidade.style.display = "none";
          }
          
          document.getElementById("contSOS").innerText = data.contadorSOS;
          document.getElementById("contCanc").innerText = data.contadorQuedasCanceladas;
          document.getElementById("contConf").innerText = data.contadorQuedasConfirmadas;
          
          // [NOVO] Lógica do Cronômetro de Inatividade
          let sec = parseInt(data.tempoInativo);
          let h = Math.floor(sec / 3600);
          let m = Math.floor((sec % 3600) / 60);
          let s = sec % 60;
          let tempoFormatado = (h > 0 ? h + "h " : "") + (m > 0 ? m + "m " : "") + s + "s";
          document.getElementById("tempoInat").innerText = tempoFormatado;
          
          if(data.estado.includes("ALERTA") || data.estado.includes("PERIGO") || data.estado.includes("AVISO")) {
            statusDiv.className = "status-box status-alerta";
          } else {
            statusDiv.className = "status-box";
          }
        });
    }, 1000);
  </script>
</head>
<body>
  <div class="card">
    <h2>Assistente de Cuidado IoT</h2>
    <div id="status" class="status-box">Carregando status...</div>
    
    <p class="data-text" id="blocoGravidade" style="display: none;">Gravidade do Impacto: <strong id="intensidade">Nenhuma</strong></p>
    
    <div class="historico-container">
        <!-- [NOVO] Relógio de inatividade adicionado na interface -->
        <div class="hist-item">⏳ Tempo em Repouso: <span id="tempoInat" class="badge badge-tempo">0s</span></div>
        <div class="hist-item">🆘 Botão SOS (Pânico): <span id="contSOS" class="badge badge-sos">0</span></div>
        <div class="hist-item">⚠️ Quedas Canceladas: <span id="contCanc" class="badge badge-canc">0</span></div>
        <div class="hist-item">🚨 Quedas Confirmadas: <span id="contConf" class="badge badge-conf">0</span></div>
    </div>
  </div>
</body>
</html>
)rawliteral";

// --- ROTAS DO SERVIDOR ---
void lidarRaiz() { server.send(200, "text/html", paginaHTML); }

void lidarDados() {
  // Calcula há quantos segundos o idoso está parado
  unsigned long tempoInativoSegundos = (millis() - tempoUltimoMovimento) / 1000;
  
  String json = "{";
  json += "\"estado\":\"" + estadoDispositivo + "\", ";
  json += "\"intensidade\":\"" + intensidadeUltimaQueda + "\", ";
  json += "\"contadorSOS\":\"" + String(contadorSOS) + "\", ";
  json += "\"contadorQuedasCanceladas\":\"" + String(contadorQuedasCanceladas) + "\", ";
  json += "\"contadorQuedasConfirmadas\":\"" + String(contadorQuedasConfirmadas) + "\", ";
  json += "\"tempoInativo\":\"" + String(tempoInativoSegundos) + "\""; // [NOVO] Envia para o JS
  json += "}";
  
  server.send(200, "application/json", json);
}

// ==========================================
// MOTOR DE LÓGICA FUZZY (SUGENO)
// ==========================================

// Função matemática que calcula o grau de pertinência (0.0 a 1.0)
float calcularPertinencia(float x, float a, float b, float c, float d) {
  if (x <= a || x >= d) return 0.0;
  if (x >= b && x <= c) return 1.0;
  if (x > a && x < b) return (x - a) / (b - a);
  if (x > c && x < d) return (d - x) / (d - c);
  return 0.0;
}

// Inferência de Regras e Defuzzificação
float calcularProbabilidadeQueda(float impactoG, unsigned long tempoMs) {
  // 1. Fuzzificação do Impacto (m/s²)
  // O limite 'd' de imp_grave é 9999. Qualquer batida acima de 45 será 100% grave, sem teto.
  float imp_leve   = calcularPertinencia(impactoG, 0, 0, 10, 12);
  float imp_mod    = calcularPertinencia(impactoG, 10, 12, 15, 18);
  float imp_grave  = calcularPertinencia(impactoG, 15, 18, 9999, 9999);

  // 2. Fuzzificação do Tempo de Queda Livre (ms)
  // Se a queda livre durar mais de 800ms, ela continua sendo 100% longa.
  float tempo_curto = calcularPertinencia(tempoMs, 0, 0, 300, 800);
  float tempo_longo = calcularPertinencia(tempoMs, 300, 800, 9999, 9999);

  // 3. Aplicação das Regras (Usa 'min' para a lógica AND)
  // Regra 1: SE Impacto é Grave E Tempo é Longo ENTÃO Probabilidade = 99%
  float w1 = min(imp_grave, tempo_longo); float out1 = 99.0;
  
  // Regra 2: SE Impacto é Grave E Tempo é Curto ENTÃO Probabilidade = 85%
  float w2 = min(imp_grave, tempo_curto); float out2 = 85.0;
  
  // Regra 3: SE Impacto é Moderado E Tempo é Longo ENTÃO Probabilidade = 75%
  float w3 = min(imp_mod, tempo_longo);   float out3 = 75.0;
  
  // Regra 4: SE Impacto é Moderado E Tempo é Curto ENTÃO Probabilidade = 45% (Pode ser só uma esbarrada)
  float w4 = min(imp_mod, tempo_curto);   float out4 = 45.0;
  
  // Regra 5: SE Impacto é Leve ENTÃO Probabilidade = 10% (Ignora o tempo)
  float w5 = imp_leve;                    float out5 = 10.0; 

  // 4. Defuzzificação (Média Ponderada Sugeno)
  float soma_pesos = w1 + w2 + w3 + w4 + w5;
  if (soma_pesos == 0) return 0.0;

  return (w1*out1 + w2*out2 + w3*out3 + w4*out4 + w5*out5) / soma_pesos;
}

// --- Inicialização do acessório ---
void setup() {
  Serial.begin(115200);
  
  if (MODO_OFFLINE == false) {
    conectarWiFi();
  } else {
    Serial.println("\n[!] AVISO: MODO OFFLINE ATIVADO.");
    Serial.println("[!] O WiFi e o Telegram foram desativados para testes locais.\n");
  }

  // --- Preparando a Energia Virtual para o Sensor Touch ---
  // 1. Fio Marrom: Transforma o pino 4 em Terra (0V)
  pinMode(PINO_TOUCH_GND, OUTPUT);
  digitalWrite(PINO_TOUCH_GND, LOW);  
  
  // 2. Fio Roxo: Transforma o pino 14 em Energia (3.3V)
  pinMode(PINO_TOUCH_VCC, OUTPUT);
  digitalWrite(PINO_TOUCH_VCC, HIGH); 
  
  // 3. Fio Azul: Configura o pino 15 para "ouvir" o toque do dedo
  pinMode(PINO_TOUCH_SINAL, INPUT);
  
  Wire.begin(21, 22);

  // 4. Configura o pino do LED para enviar energia (OUTPUT)
  pinMode(PINO_LED_STATUS, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  // Garante que o LED comece desligado
  digitalWrite(PINO_LED_STATUS, LOW);
  digitalWrite(PINO_BUZZER, LOW);

  // 5. Conferir conexão com o acelerômetro (MPU6050)
  if (!mpu.begin()) {
    Serial.println("Falha ao encontrar o chip MPU6050.");
    while (1) { 
      digitalWrite(PINO_LED_STATUS, !digitalRead(PINO_LED_STATUS));
      delay(10);
      }
  }
  
  // 6. Se o código chegou até aqui, o sensor ligou com sucesso!
  // Acende o LED para mostrar que está tudo pronto.
  digitalWrite(PINO_LED_STATUS, HIGH);

  // 7. Inicializando o intervalo do acelerômetro para 16G.
  // Importante para garantir leitura de aceleração resultantes altas, possíveis em quedas.
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);

  // 8. Estabilização do sensor.
  Serial.println("Estabilizando filtros internos do sensor (Aquecimento)...");
  for (int i = 0; i < 20; i++){
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    delay(10);
  }
  Serial.println("Sensor estabilizado e pronto para iniciar detecções!");
  
  Serial.println("Sistema de Deteccao de Quedas Ativo!");
  
  // 9. Configura as rotas e inicia o servidor web
  server.on("/", lidarRaiz);
  server.on("/dados", lidarDados);
  server.begin();
  Serial.println("Servidor Web HTTP iniciado!");
}

void loop() {
  // Mantém o servidor web escutando requisições
  server.handleClient();
  
  unsigned long tempoAtual = millis();

  // ==========================================
  // 1. VERIFICAÇÃO DO BOTÃO TOUCH (SOS)
  // ==========================================
  if (digitalRead(PINO_TOUCH_SINAL) == HIGH) {
    Serial.println("\n--- SENSOR TOUCH ACIONADO: SOS! ---");
    
    // Adiciona +1 no histórico e trava a tela no vermelho
    contadorSOS++;
    estadoDispositivo = "ALERTA: SOS Acionado!";
    tempoUltimoAlarme = millis();
    
    for (int i = 0; i < 3; i++) {
      tone(PINO_BUZZER, 3000); delay(150); noTone(PINO_BUZZER); delay(100);
    }
    enviarMensagemTelegram("%F0%9F%86%98+EMERGENCIA:+O+idoso+acionou+o+botao+de+panico!+Ajuda+necessaria.");
    
    // Espera inteligente (não trava o site enquanto conta os 3 segundos)
    unsigned long inicioTrava = millis();
    while(millis() - inicioTrava < 3000) {
      server.handleClient(); // Mantém o site vivo!
      delay(10);
    }
    while (digitalRead(PINO_TOUCH_SINAL) == HIGH) {
      server.handleClient(); 
      delay(10);
    }
  }

  // ==========================================
  // 2. VERIFICAÇÃO DO ACELERÔMETRO (QUEDA)
  // ==========================================
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float a_res = sqrt(pow(a.acceleration.x, 2) + pow(a.acceleration.y, 2) + pow(a.acceleration.z, 2));

  // Atualiza a leitura de aceleração global
  aceleracaoAtual = a_res;

  // Reset Inteligente após 2 minutos (Apenas para Quedas e SOS)
  if (estadoDispositivo.indexOf("SOS") >= 0 || estadoDispositivo.indexOf("Queda Confirmada") >= 0) {
    
    // Usa millis() diretamente no momento da checagem para evitar o "Underflow"
    if (millis() - tempoUltimoAlarme > TEMPO_ALERTA_VISUAL) {
      estadoDispositivo = "Monitorando (Estável)"; 
      intensidadeUltimaQueda = "Nenhuma"; 
    }
  }
  
  // ==========================================
  // 3. VERIFICAÇÃO DE ATIVIDADE (SEDENTARISMO)
  // ==========================================
  // 1. Registra qualquer solavanco acima do limiar
  if (abs(a_res - 9.8) > LIMIAR_MOVIMENTO) {
    ultimoPicoMovimento = tempoAtual;
  }

  // 2. Validador de Movimento Sustentado
  // Tolera um espaço de até 1 segundo entre as passadas para considerar que "está em movimento"
  if (tempoAtual - ultimoPicoMovimento < 1000) {
    if (inicioMovimentoTemp == 0) {
      inicioMovimentoTemp = tempoAtual; // Marca o início da caminhada/movimentação
    }
    
    // Se ele continuou se mexendo pelo tempo mínimo exigido (Ex: 4 segundos)
    if (tempoAtual - inicioMovimentoTemp > TEMPO_MOVIMENTO_CONTINUO) {
      tempoUltimoMovimento = tempoAtual; // Movimento validado! Zera o cronômetro.
      
      // Reseta a máquina de estados e limpa a tela se havia um alerta
      if (nivelAlertaInatividade > 0) {
        nivelAlertaInatividade = 0;
        if (estadoDispositivo.indexOf("Inatividade") >= 0 || estadoDispositivo.indexOf("Repouso") >= 0) {
          estadoDispositivo = "Monitorando (Estável)";
        }
      }
    }
  } else {
    // Se passou mais de 1 segundo sem registrar picos, ele parou. Descarta a tentativa.
    inicioMovimentoTemp = 0;
  }

  // 3. Verifica a progressão do sedentarismo (Máquina de Estados)
  unsigned long tempoParado = tempoAtual - tempoUltimoMovimento;

  if (tempoParado > TEMPO_ALERTA_GRAVE && nivelAlertaInatividade < 3) {
    nivelAlertaInatividade = 3;
    estadoDispositivo = "PERIGO: Repouso Crítico de pelo menos 9h!"; 
    Serial.println("\n-> ALERTA 3: Repouso Critico.");
    enviarMensagemTelegram("%E2%9D%8C+ALERTA+CRÍTICO:+Longo+período+de+repouso+passivo.+Verifique+o+idoso+imediatamente!+%0A%F0%9F%94%8B+Bateria:+" + String(nivelBateriaVirtual) + "%25");
  } 
  else if (tempoParado > TEMPO_ALERTA_MODERADO && nivelAlertaInatividade < 2) {
    nivelAlertaInatividade = 2;
    estadoDispositivo = "AVISO: Inatividade Alta de pelo menos 3h!"; 
    Serial.println("\n-> ALERTA 2: Inatividade Alta.");
    enviarMensagemTelegram("%E2%9A%A0%EF%B8%8F+AVISO:+Mais+de+3+horas+de+inatividade.+É+importante+levantar+para+estimular+a+circulação.+%0A%F0%9F%94%8B+Bateria:+" + String(nivelBateriaVirtual) + "%25");
  } 
  else if (tempoParado > TEMPO_ALERTA_LEVE && nivelAlertaInatividade < 1) {
    nivelAlertaInatividade = 1;
    estadoDispositivo = "LEMBRETE: Hora de se movimentar"; 
    Serial.println("\n-> ALERTA 1: Sugestão de Movimento.");
    enviarMensagemTelegram("%E2%84%B9%EF%B8%8F+LEMBRETE:+O+idoso+está+há+algum+tempo+parado.+Que+tal+uma+pequena+caminhada?+%0A%F0%9F%94%8B+Bateria:+" + String(nivelBateriaVirtual) + "%25");
  }

  // ==========================================
  // 4. ESCUTA ATIVA DO TELEGRAM (Long Polling)
  // ==========================================
  if (tempoAtual - tempoUltimaChecagemTelegram > INTERVALO_CHECAGEM_TELEGRAM) {
    checarComandosTelegram();
    tempoUltimaChecagemTelegram = tempoAtual;
  }

  // --- (DEBUG) Armazena os valores de aceleração resultantes máximos e mínimos ---
  /* DEBUG
  max_value = max(max_value, a_res);
  min_value = min(min_value, a_res);

  / --- (DEBUG) Imprime os valores crus de cada eixo e o resultante ---
  /* Serial.print("X: "); Serial.print(a.acceleration.x);
  Serial.print(" | Y: "); Serial.print(a.acceleration.y);
  Serial.print(" | Z: "); Serial.print(a.acceleration.z);
  Serial.print(" ===> Resultante (A_res): "); 
  Serial.println(a_res);
  Serial.print(" ===> max_value: "); 
  Serial.println(max_value);
  Serial.print(" ===> min_value: "); 
  Serial.println(min_value); */

  // 1. Detecta a Fase 1: Queda Livre
  if (a_res < LIMIAR_QUEDA_LIVRE) {
    if (!emQuedaLivre) {
      emQuedaLivre = true;
      tempoInicioQueda = tempoAtual;
      Serial.println("ATENCAO: Queda livre detectada...");
    }
  }

  // 2. Detecta a Fase 2: O Impacto (se estivermos em queda)
  if (emQuedaLivre) {
    // Verifica se já passou muito tempo (não foi uma queda, apenas um solavanco)
    if (tempoAtual - tempoInicioQueda > JANELA_TEMPO_MS) {
      emQuedaLivre = false; // Reseta o estado
    } 
    // Se ocorreu um impacto forte logo após a queda livre
    else if (a_res > LIMIAR_IMPACTO) {
      Serial.println("\n===========================");
      Serial.println("!!! ALARME: QUEDA CONFIRMADA !!!");
      Serial.println("===========================\n");
      
      emQuedaLivre = false; 

      // Muda o site na mesma hora para avisar que está checando
      estadoDispositivo = "PERIGO: Avaliando possivel queda...";
      
      bool cancelado = aguardarCancelamento(10);
      
      if (cancelado == true) {
        Serial.println("-> Sistema resetado. Voltando a monitorar...\n");
        estadoDispositivo = "Monitorando (Estável)"; 
        intensidadeUltimaQueda = "Nenhuma"; 
        contadorQuedasCanceladas++;
        
        // Zera o sedentarismo pois o idoso se mexeu e tocou no botão
        tempoUltimoMovimento = tempoAtual; 
        nivelAlertaInatividade = 0; 
      } 
      else {
        Serial.println("-> TEMPO ESGOTADO. ENVIANDO ALERTA PARA A NUVEM!");

        // Atualiza a variável para o Dashboard exibir o bloco na tela
        intensidadeUltimaQueda = String(a_res, 1) + " m/s²";
        
        // IA: Chama o motor Fuzzy para prever a certeza da queda
        unsigned long duracaoQueda = tempoAtual - tempoInicioQueda;
        float probabilidade = calcularProbabilidadeQueda(a_res, duracaoQueda);
        
        Serial.print("-> Probabilidade calculada pela Lógica Fuzzy: ");
        Serial.print(probabilidade);
        Serial.println("%");

        // Formata a mensagem com os dados matemáticos
        String msgAlerta = "%F0%9F%9A%A8+ALERTA:+Queda+detectada!+Idoso+precisa+de+ajuda.%0A";
        msgAlerta += "%E2%9A%99%EF%B8%8F+Análise+da+IA:+" + String(probabilidade, 1) + "%25+de+Certeza%0A";
        msgAlerta += "Impacto+Registrado:+" + String(a_res, 1) + "+m/s%C2%B2";
        
        enviarMensagemTelegram(msgAlerta);
        
        estadoDispositivo = "PERIGO: Queda Confirmada! (IA: " + String(probabilidade, 0) + "%)";
        tempoUltimoAlarme = millis();

        contadorQuedasConfirmadas++;
        
        tempoUltimoMovimento = tempoAtual; 
        nivelAlertaInatividade = 0; 
      }
      
      digitalWrite(PINO_LED_STATUS, HIGH);
    }
  }

  delay(10);
  /* DEBUG delay(500); // Imprime 2 vezes por segundo para dar tempo de ler */
}

void conectarWiFi(){
  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Conectado com Sucesso!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());  
}

void enviarMensagemTelegram(String textoMensagem){
  if (MODO_OFFLINE == true) {
    Serial.print("-> [Bypass]: O MODO OFFLINE está ativado.");
    return;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Enviando alerta para a nuvem...");

    HTTPClient http;
    // Monta a URL completa juntando as peças
    String urlCompleta = "https://api.telegram.org/bot" + telegramToken + "/sendMessage?chat_id=" + telegramChatID + "&text=" + textoMensagem;

    http.begin(urlCompleta);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      if (httpResponseCode >= 200 and httpResponseCode < 300) {
        Serial.print("Alerta enviado! Código de resposta HTTP: ");
      } else {
        Serial.print("Tentativa de alerta com aviso. Código HTTP: ");
      }
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Falha ao enviar alerta. Erro HTTP: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("ERRO: WiFi desconectado. Impossivel enviar alerta.");
  }
}

bool aguardarCancelamento(int tempoSegundos) {
  unsigned long inicio = millis();
  unsigned long duracao = tempoSegundos * 1000;
  bool estadoBip = false;
  unsigned long ultimoBip = 0;

  Serial.println("-> INICIANDO JANELA DE CANCELAMENTO (10s)...");

  // Fica preso aqui dentro enquanto o tempo não acabar
  while (millis() - inicio < duracao) {
    
    server.handleClient(); // Mantém o painel web vivo durante os 10s de apito
    
    // Faz o buzzer apitar intermitente para alertar o idoso
    if (millis() - ultimoBip > 500) {
      estadoBip = !estadoBip;
      if (estadoBip) {
        tone(PINO_BUZZER, 4000); // Som grave de aviso
      } else {
        noTone(PINO_BUZZER);
      }
      ultimoBip = millis();
    }

    // Fica escutando o sensor touch sem parar
    if (digitalRead(PINO_TOUCH_SINAL) == HIGH) {
      Serial.println("\n[!] ALARME CANCELADO PELO USUARIO [!]");
      noTone(PINO_BUZZER);
      
      // Dá 2 bips agudos e rápidos para confirmar que cancelou com sucesso
      tone(PINO_BUZZER, 3000); delay(150); noTone(PINO_BUZZER); delay(100);
      tone(PINO_BUZZER, 3000); delay(150); noTone(PINO_BUZZER);

      // Trava de Segurança: Espera o usuário tirar o dedo antes de fechar a função
      while (digitalRead(PINO_TOUCH_SINAL) == HIGH) {
        delay(10);
      }
      delay(200); // Dá um fôlego extra de uma fração de segundo para estabilizar

      // Reseta o Dashboard Web porque o alarme foi falso
      estadoDispositivo = "Monitorando (Estável)";
      
      return true; // Informa que o alarme foi cancelado
    }
    delay(10); // Pequeno atraso para não sobrecarregar o processador
  }

  // Se saiu do while, é porque o tempo esgotou e ninguém apertou o botão
  noTone(PINO_BUZZER);
  return false; 
}

void checarComandosTelegram() {
  // Não faz requisição se estiver offline ou sem WiFi
  if (MODO_OFFLINE == true || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  
  // O offset garante que o bot só leia mensagens NOVAS (ignorando as que já respondeu)
  String url = "https://api.telegram.org/bot" + telegramToken + "/getUpdates?limit=1&offset=" + String(ultimoUpdateIdTelegram + 1);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();

    // Deserialização do JSON (Compatível com ArduinoJson v7)
    JsonDocument doc; 
    DeserializationError error = deserializeJson(doc, payload);

    // Se o JSON for válido e houver alguma mensagem na fila
    if (!error && doc["result"].size() > 0) {
      JsonObject update = doc["result"][0];
      
      // Atualiza o ID para não ler essa mensagem de novo
      ultimoUpdateIdTelegram = update["update_id"].as<int>();

      String chat_id = update["message"]["chat"]["id"].as<String>();
      String textoRecebido = update["message"]["text"].as<String>();

      Serial.println("\n[!] Comando recebido via Telegram: " + textoRecebido);

      // --- ROTEAMENTO DOS COMANDOS ---
      if (textoRecebido == "/status") {
        unsigned long tempoInat = (millis() - tempoUltimoMovimento) / 1000;
        
        String estadoURL = estadoDispositivo;
        estadoURL.replace(" ", "+");
        
        String relatorio = "%F0%9F%93%8A+**RELATÓRIO+AO+VIVO**%0A";
        relatorio += "Estado:+" + estadoURL + "%0A";
        relatorio += "Quedas+Confirmadas:+" + String(contadorQuedasConfirmadas) + "%0A";
        relatorio += "Tempo+Repouso:+" + String(tempoInat) + "s%0A";
        relatorio += "%0A%F0%9F%94%97+Acesse+o+Dashboard:+http://" + WiFi.localIP().toString();

        enviarMensagemTelegram(relatorio);
      } 
      else if (textoRecebido == "/bateria") {
        enviarMensagemTelegram("%F0%9F%94%8B+**STATUS+DA+BATERIA**%0ANível+atual:+" + String(nivelBateriaVirtual) + "%25");
      }
    }
  }
  http.end();
}
