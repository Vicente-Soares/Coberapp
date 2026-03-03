#define WM_NOASYNC

#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <WebServer.h>
#include <ESPmDNS.h>   // mDNS para acessar via nome

#define CONFIG_BUTTON 35
#define RELAY_OPEN 13
#define RELAY_CLOSE 26
#define LIMIT_OPEN 14
#define LIMIT_CLOSE 33

WebServer server(80);
Preferences preferences;

// ===== IP FIXO (AJUSTE SE NECESSÁRIO PARA SUA REDE) =====
IPAddress local_IP(192, 168, 1, 50);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
// ==========================================================

void setup() {
  Serial.begin(115200);

  pinMode(CONFIG_BUTTON, INPUT_PULLUP);
  pinMode(RELAY_OPEN, OUTPUT);
  pinMode(RELAY_CLOSE, OUTPUT);
  pinMode(LIMIT_OPEN, INPUT_PULLUP);
  pinMode(LIMIT_CLOSE, INPUT_PULLUP);

  digitalWrite(RELAY_OPEN, HIGH);
  digitalWrite(RELAY_CLOSE, HIGH);

  preferences.begin("wifi", false);

  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS);

  WiFiManager wm;
  bool res;

  if (digitalRead(CONFIG_BUTTON) == LOW) {
    Serial.println("🟡 Modo configuração forçado.");
    res = wm.startConfigPortal("ESP32_Config", "12345678");
  } else {
    res = wm.autoConnect("ESP32_AutoConfig", "12345678");
  }

  if (!res) {
    Serial.println("❌ Falha conexão. Reiniciando...");
    delay(2000);
    ESP.restart();
  }

  Serial.println("✅ Conectado ao Wi-Fi!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());

  preferences.putString("ssid", WiFi.SSID());
  preferences.putString("pass", WiFi.psk());

  // ===== Inicia mDNS (opcional) =====
  if (MDNS.begin("piscina")) {
    Serial.println("🌐 mDNS ativo: http://piscina.local");
  } else {
    Serial.println("⚠️ Falha ao iniciar mDNS");
  }
  // ==================================

  setupControlServer();
  Serial.println("🌐 Servidor web iniciado!");
}

void setupControlServer() {

  server.on("/", []() {
    String html = "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Controle da Piscina</title>";
    html += "<style>body{font-family:Arial;text-align:center;background:#f4f4f4;margin:0;padding:20px;}";
    html += "h1{color:#333;}a{display:block;text-decoration:none;font-size:20px;color:white;background:#007bff;padding:10px;margin:10px auto;border-radius:5px;width:80%;}a:hover{background:#0056b3;}";
    html += "</style></head><body><h1>Controle da Piscina</h1>";
    html += "<a href='/open'>Abrir Piscina</a>";
    html += "<a href='/close'>Fechar Piscina</a>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  });

  server.on("/open", []() {
    if (digitalRead(LIMIT_OPEN) == LOW) {
      Serial.println("➡️ Abrindo piscina...");
      digitalWrite(RELAY_OPEN, LOW);
      digitalWrite(RELAY_CLOSE, HIGH);
      server.send(200, "text/plain", "Abrindo piscina...");
    } else {
      server.send(200, "text/plain", "Piscina já está aberta.");
    }
  });

  server.on("/close", []() {
    if (digitalRead(LIMIT_CLOSE) == LOW) {
      Serial.println("⬅️ Fechando piscina...");
      digitalWrite(RELAY_CLOSE, LOW);
      digitalWrite(RELAY_OPEN, HIGH);
      server.send(200, "text/plain", "Fechando piscina...");
    } else {
      server.send(200, "text/plain", "Piscina já está fechada.");
    }
  });

  server.begin();
}

void loop() {
  server.handleClient();

  // Segurança: desliga relé ao atingir fim de curso
  if (digitalRead(LIMIT_OPEN) == HIGH) {
    digitalWrite(RELAY_OPEN, HIGH);
  }

  if (digitalRead(LIMIT_CLOSE) == HIGH) {
    digitalWrite(RELAY_CLOSE, HIGH);
  }
}
