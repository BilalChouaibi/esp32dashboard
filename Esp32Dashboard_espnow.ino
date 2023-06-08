#include <WiFi.h>
#include <esp_now.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ESPmDNS.h>

const unsigned long MEASURE_TIMEOUT = 25000UL; // 25ms = ~8m à 340m/s
const float SOUND_SPEED = 340.0 / 1000;

IPAddress local_IP(192, 168, 1, 79);
IPAddress gateway(192, 168, 1, 254);
IPAddress subnet(255, 255, 255, 0);

const char* ssid = "UFI-SPRINT";
const char* password = "SvCn5%pE92@GvL";

AsyncWebServer server(80);

typedef struct struct_message {
  int id;
  int value;
  int battery;
} struct_message;
struct_message board1;
struct_message board2;
struct_message board3;
struct_message board4;
struct_message boardsStruct[4]= {board1, board2,board3,board4};

void OnDataRecv(const uint8_t* mac_addr, const uint8_t* incomingData, int len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Packet received from: ");
  Serial.println(macStr);

  struct_message receivedData;
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  Serial.printf("Board ID %u: %u bytes\n", receivedData.id, len);

  if (receivedData.id >= 1 && receivedData.id <= 4) {
    boardsStruct[receivedData.id - 1].value = receivedData.value;
    boardsStruct[receivedData.id - 1].battery = receivedData.battery;
    Serial.printf("dist value: %d \n", boardsStruct[receivedData.id - 1].value);
    Serial.printf("bat value: %d \n", boardsStruct[receivedData.id - 1].battery);
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  
  WiFi.mode(WIFI_STA);
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  
  if (!MDNS.begin("dashboard")) {
    Serial.println("Error starting mDNS");
    return;
  }
  
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur d'initialisation ESP-NOW");
    return;
  }
  
  Serial.println(WiFi.localIP());
  esp_now_register_recv_cb(OnDataRecv);
  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/asupppr.html");
  });

  server.on("/admin", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/admin.html");
  });

  server.on("/invite", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/invite.html");
  });

  for (int i = 1; i <= 4; i++) {
    String distPath = "/dist" + String(i);
    String batPath = "/bat" + String(i);
    
    server.on(distPath.c_str(), HTTP_GET, [i](AsyncWebServerRequest* request) {
      request->send_P(200, "text/plain", String(boardsStruct[i - 1].value).c_str());
    });

    server.on(batPath.c_str(), HTTP_GET, [i](AsyncWebServerRequest* request) {
      request->send_P(200, "text/plain", String(boardsStruct[i - 1].battery).c_str());
    });
  }

  server.begin();
}

void loop() {
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());
  Serial.println("");
  delay(2000);
}
