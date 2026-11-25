#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <esp_arduino_version.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include "Painel.h"

constexpr char WIFI_SSID[] = "Garra-ESP";
constexpr char WIFI_SENHA[] = "garra1234";
constexpr char HOST_LOCAL[] = "192.168.4.1";
constexpr uint8_t NUM_MOTORES = 4;
constexpr int VELOCIDADE_MIN = 30;
constexpr int VELOCIDADE_PADRAO = 70;
constexpr uint32_t TEMPO_SEM_COMANDO_MS = 400;
constexpr uint32_t TEMPO_MAX_MS = 6000;
constexpr uint32_t VALIDADE_ESTADO_MS = 1200;
constexpr uint32_t PERIODO_MOTOR_MS = 10;
constexpr uint32_t RAMPA_MS = 150;
constexpr uint32_t PWM_FREQ = 1000;
constexpr uint8_t PWM_BITS = 8;
constexpr uint32_t PWM_MAX = (1U << PWM_BITS) - 1;

struct Ligacao {
  uint8_t pinoA;
  uint8_t pinoB;
  bool invertido;
};

// Altere o último valor para true se a direção física de um motor estiver invertida.
constexpr Ligacao MOTORES[NUM_MOTORES] = {
  {32, 33, false},  // Base: esquerda / direita
  {25, 26, false},  // Ombro: descer / subir
  {23, 22, false},  // Cotovelo: recolher / estender
  {19, 18, false}   // Garra: abrir / fechar
};

struct EstadoMotor {
  int8_t direcao;
  bool limitado;
  uint32_t inicio;
  uint32_t inicioRampa;
};

struct EstadoControle {
  EstadoMotor motores[NUM_MOTORES];
  uint32_t epoca;
  uint32_t ultimoComando;
  char dono[17];
  int velocidade;
};

WebServer server(80);
SemaphoreHandle_t mutexMotores = nullptr;
EstadoControle controle = {};
bool servidorAtivo = false;

bool iniciarPwm(uint8_t pino, uint8_t canal) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  return ledcAttachChannel(pino, PWM_FREQ, PWM_BITS, canal);
#else
  if (!ledcSetup(canal, PWM_FREQ, PWM_BITS)) return false;
  ledcAttachPin(pino, canal);
  return true;
#endif
}

void escreverPwm(uint8_t pino, uint8_t canal, uint32_t duty) {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWrite(pino, duty);
#else
  ledcWrite(canal, duty);
#endif
}

void aplicarMotor(uint8_t i, int8_t direcao, uint32_t duty) {
  const Ligacao &motor = MOTORES[i];
  const int sentido = motor.invertido ? -direcao : direcao;
  // Zera a entrada oposta antes de energizar a outra.
  if (sentido >= 0) {
    escreverPwm(motor.pinoB, i * 2 + 1, 0);
    escreverPwm(motor.pinoA, i * 2, sentido > 0 ? duty : 0);
  } else {
    escreverPwm(motor.pinoA, i * 2, 0);
    escreverPwm(motor.pinoB, i * 2 + 1, duty);
  }
}

void pararMotor(uint8_t i) {
  controle.motores[i].direcao = 0;
  aplicarMotor(i, 0, 0);
}

void liberarControle() {
  controle.dono[0] = '\0';
  ++controle.epoca;  // Invalida comandos enviados antes da parada.
}

void zerarControle() {
  for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
    pararMotor(i);
    controle.motores[i].limitado = false;
  }
  liberarControle();
}

void verificarPrazos(uint32_t agora) {
  if (controle.dono[0] && agora - controle.ultimoComando >= TEMPO_SEM_COMANDO_MS) {
    zerarControle();
    return;
  }

  for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
    EstadoMotor &motor = controle.motores[i];
    if (motor.direcao && agora - motor.inicio >= TEMPO_MAX_MS) {
      pararMotor(i);
      motor.limitado = true;
    }
  }
}

void tarefaMotores(void *) {
  TickType_t anterior = xTaskGetTickCount();

  for (;;) {
    xSemaphoreTake(mutexMotores, portMAX_DELAY);
    const uint32_t agora = millis();
    verificarPrazos(agora);

    const uint32_t alvo = PWM_MAX * controle.velocidade / 100;
    for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
      const EstadoMotor &motor = controle.motores[i];
      if (motor.direcao) {
        const uint32_t decorrido = agora - motor.inicioRampa;
        const uint32_t duty = decorrido >= RAMPA_MS ? alvo : alvo * decorrido / RAMPA_MS;
        aplicarMotor(i, motor.direcao, duty);
      }
    }
    xSemaphoreGive(mutexMotores);

    vTaskDelayUntil(&anterior, pdMS_TO_TICKS(PERIODO_MOTOR_MS));
  }
}

bool lerNumero(const String &texto, uint32_t &numero) {
  if (texto.length() == 0 || texto.length() > 10) return false;
  uint64_t valor = 0;
  for (size_t i = 0; i < texto.length(); ++i) {
    if (texto[i] < '0' || texto[i] > '9') return false;
    valor = valor * 10 + (texto[i] - '0');
  }
  if (valor > UINT32_MAX) return false;
  numero = static_cast<uint32_t>(valor);
  return true;
}

bool clienteValido(const String &cliente) {
  if (cliente.length() != 16) return false;
  for (size_t i = 0; i < cliente.length(); ++i) {
    const char c = cliente[i];
    if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
  }
  return true;
}

void enviarEstado(int codigo, const EstadoControle &estado, uint32_t agora) {
  char resposta[384];
  snprintf(
    resposta, sizeof(resposta),
    "{\"epoch\":%lu,\"stamp\":%lu,\"owner\":\"%s\",\"speed\":%d,\"motors\":["
    "{\"direction\":%d,\"limited\":%s},{\"direction\":%d,\"limited\":%s},"
    "{\"direction\":%d,\"limited\":%s},{\"direction\":%d,\"limited\":%s}]}",
    static_cast<unsigned long>(estado.epoca), static_cast<unsigned long>(agora),
    estado.dono, estado.velocidade,
    estado.motores[0].direcao, estado.motores[0].limitado ? "true" : "false",
    estado.motores[1].direcao, estado.motores[1].limitado ? "true" : "false",
    estado.motores[2].direcao, estado.motores[2].limitado ? "true" : "false",
    estado.motores[3].direcao, estado.motores[3].limitado ? "true" : "false");
  server.sendHeader("Cache-Control", "no-store");
  server.send(codigo, "application/json", resposta);
}

void lerEstado() {
  xSemaphoreTake(mutexMotores, portMAX_DELAY);
  const uint32_t agora = millis();
  verificarPrazos(agora);
  const EstadoControle estado = controle;
  xSemaphoreGive(mutexMotores);
  enviarEstado(200, estado, agora);
}

bool comandoLocal() {
  if (server.hostHeader() == HOST_LOCAL && server.header("X-Garra") == "1") return true;
  server.send(403, "text/plain", "Comando recusado");
  return false;
}

void receberControle() {
  if (!comandoLocal()) return;

  const String cliente = server.arg("client");
  const String movimentos = server.arg("motion");
  int8_t direcoes[NUM_MOTORES] = {};
  uint32_t epoca = 0;
  uint32_t enviadoEm = 0;
  uint32_t novaVelocidade = 0;

  if (!clienteValido(cliente) || movimentos.length() != NUM_MOTORES
      || !lerNumero(server.arg("epoch"), epoca) || !lerNumero(server.arg("stamp"), enviadoEm)
      || (server.hasArg("speed") && (!lerNumero(server.arg("speed"), novaVelocidade)
          || novaVelocidade < VELOCIDADE_MIN || novaVelocidade > 100))) {
    server.send(400, "text/plain", "Comando invalido");
    return;
  }

  bool temMovimento = false;
  for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
    switch (movimentos[i]) {
      case '-': direcoes[i] = -1; break;
      case '+': direcoes[i] = 1; break;
      case '0': break;
      default:
        server.send(400, "text/plain", "Movimento invalido");
        return;
    }
    if (direcoes[i]) temMovimento = true;
  }

  xSemaphoreTake(mutexMotores, portMAX_DELAY);
  const uint32_t agora = millis();
  verificarPrazos(agora);
  const bool aceito = epoca == controle.epoca
    && agora - enviadoEm <= VALIDADE_ESTADO_MS
    && (!controle.dono[0] || cliente == controle.dono);

  if (aceito) {
    if (novaVelocidade) controle.velocidade = novaVelocidade;
    if (temMovimento) {
      if (!controle.dono[0]) cliente.toCharArray(controle.dono, sizeof(controle.dono));
      controle.ultimoComando = agora;
    }

    for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
      EstadoMotor &motor = controle.motores[i];
      if (!direcoes[i]) {
        pararMotor(i);
        motor.limitado = false;
      } else if (!motor.limitado && motor.direcao != direcoes[i]) {
        if (motor.direcao) pararMotor(i);
        else motor.inicio = agora;
        motor.direcao = direcoes[i];
        motor.inicioRampa = agora;
      }
    }
    if (!temMovimento && controle.dono[0]) liberarControle();
  }

  const EstadoControle estado = controle;
  xSemaphoreGive(mutexMotores);
  enviarEstado(aceito ? 200 : 409, estado, agora);
}

void receberParada() {
  if (!comandoLocal()) return;

  const String cliente = server.arg("client");
  const bool global = server.arg("stop") == "all";
  if (!global && !clienteValido(cliente)) {
    server.send(400, "text/plain", "Comando invalido");
    return;
  }

  xSemaphoreTake(mutexMotores, portMAX_DELAY);
  const uint32_t agora = millis();
  verificarPrazos(agora);
  if (global || !controle.dono[0] || cliente == controle.dono) zerarControle();
  const EstadoControle estado = controle;
  xSemaphoreGive(mutexMotores);
  enviarEstado(200, estado, agora);
}

void setup() {
  Serial.begin(115200);
  for (const Ligacao &motor : MOTORES) {
    pinMode(motor.pinoA, OUTPUT);
    pinMode(motor.pinoB, OUTPUT);
    digitalWrite(motor.pinoA, LOW);
    digitalWrite(motor.pinoB, LOW);
  }

  for (uint8_t i = 0; i < NUM_MOTORES; ++i) {
    if (!iniciarPwm(MOTORES[i].pinoA, i * 2) || !iniciarPwm(MOTORES[i].pinoB, i * 2 + 1)) {
      Serial.println("Falha no PWM. Controle desativado.");
      return;
    }
    aplicarMotor(i, 0, 0);
  }

  controle.epoca = esp_random();
  controle.velocidade = VELOCIDADE_PADRAO;
  mutexMotores = xSemaphoreCreateMutex();
  if (!mutexMotores || xTaskCreate(tarefaMotores, "motores", 4096, nullptr, 3, nullptr) != pdPASS) {
    Serial.println("Falha ao iniciar a tarefa de seguranca. Controle desativado.");
    return;
  }

  IPAddress ip(192, 168, 4, 1);
  if (!WiFi.mode(WIFI_AP) || !WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0))
      || !WiFi.softAP(WIFI_SSID, WIFI_SENHA)) {
    Serial.println("Falha ao iniciar o ponto de acesso.");
    return;
  }

  const char *headers[] = {"X-Garra"};
  server.collectHeaders(headers, 1);
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Cache-Control", "no-store");
    server.send_P(200, "text/html; charset=utf-8", PAGINA_HTML);
  });
  server.on("/api/status", HTTP_GET, lerEstado);
  server.on("/api/control", HTTP_POST, receberControle);
  server.on("/api/stop", HTTP_POST, receberParada);
  server.onNotFound([]() { server.send(404, "text/plain", "Pagina nao encontrada"); });
  server.begin();
  servidorAtivo = true;

  Serial.printf("Wi-Fi: %s | Acesse http://%s\n", WIFI_SSID, HOST_LOCAL);
}

void loop() {
  if (servidorAtivo) server.handleClient();
  delay(1);
}
