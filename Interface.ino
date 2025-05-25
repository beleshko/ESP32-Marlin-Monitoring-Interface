#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESP_8_BIT_GFX.h>
#include <time.h>

// Material Design Dark Theme Palette (RGB332)
#define COLOR_BACKGROUND    0x00  // Черный фон
#define COLOR_SURFACE       0x24  // Темно-серая поверхность
#define COLOR_SURFACE_VAR   0x49  // Вариант поверхности
#define COLOR_ON_SURFACE    0xFF  // Белый текст на поверхности
#define COLOR_PRIMARY       0x9F  // Material Blue
#define COLOR_ON_PRIMARY    0x00  // Черный на первичном
#define COLOR_SECONDARY     0x7C  // Material Teal
#define COLOR_ERROR         0xE0  // Material Red
#define COLOR_SUCCESS       0x1C  // Material Green
#define COLOR_WARNING       0xFC  // Material Orange
#define COLOR_OUTLINE       0x92  // Серая граница
#define COLOR_OUTLINE_VAR   0x6D  // Вариант границы

const char* ssid     = "Wifi name";
const char* password = "Wifi pass";
const uint16_t tcpPort = 4097;
const uint16_t uartBridgePort = 23;  // Telnet port

// Экран и отступы
const uint16_t SCREEN_W = 240;
const uint16_t SCREEN_H = 240;
const uint16_t SCREEN_OFFSET_X = 0;
const uint8_t CARD_RADIUS = 8;
const uint8_t SPACING_S = 4;
const uint8_t SPACING_M = 8;
const uint8_t SPACING_L = 16;

ESP_8_BIT_GFX videoOut(true, 8);
AsyncServer server(tcpPort);
AsyncServer uartBridgeServer(uartBridgePort);  // Сервер для UART моста
AsyncClient* client = nullptr;
AsyncClient* uartClient = nullptr;  // Клиент для UART моста

float tempHistory[120];
uint16_t historyIndex = 0;

// Текущее состояние принтера и связи
struct PrinterState {
  float extruder;
  float targetExtruder;
  float bed;
  float targetBed;
  float progress;
  float smoothExt;
  float smoothBed;
  bool connected;  // связь с клиентом
  bool uartBridgeConnected;  // связь с UART мостом
} printer;

// Статус Mega-связи
bool megaConnected = false;
unsigned long lastMegaMillis = 0;

// Иконки
const uint8_t ICON_EXTRUDER[8]        = {0x18,0x3C,0x7E,0xFF,0xFF,0x7E,0x3C,0x18};
const uint8_t ICON_BED[8]             = {0x00,0x7E,0x42,0x42,0x7E,0x3C,0x18,0x00};
const uint8_t ICON_WIFI_ON[8]         = {0x08,0x1C,0x2A,0x49,0x08,0x14,0x22,0x00};
const uint8_t ICON_WIFI_OFF[8]        = {0x08,0x1C,0x2A,0x49,0x08,0x94,0xA2,0x40};
const uint8_t ICON_PROGRESS[8]        = {0x3C,0x42,0x99,0xA5,0xA5,0x99,0x42,0x3C};
const uint8_t ICON_BRIDGE[8]          = {0x81,0x42,0x24,0x18,0x18,0x24,0x42,0x81};

String getTimeStr() {
  struct tm t;
  if (!getLocalTime(&t)) return "--:--";
  char buf[6];
  strftime(buf, sizeof(buf), "%H:%M", &t);
  return String(buf);
}

void drawIcon8x8(uint16_t x, uint16_t y, const uint8_t* icon, uint8_t color) {
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      if (icon[i] & (1 << (7 - j))) {
        videoOut.drawPixel(x + j, y + i, color);
      }
    }
  }
}

void drawRoundedRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r, uint8_t color) {
  videoOut.fillRect(x + r, y, w - 2 * r, h, color);
  videoOut.fillRect(x, y + r, w, h - 2 * r, color);
  for (int i = 0; i < r; i++) {
    for (int j = 0; j < r; j++) {
      if (i * i + j * j <= r * r) {
        videoOut.drawPixel(x + r - i - 1, y + r - j - 1, color);
        videoOut.drawPixel(x + w - r + i, y + r - j - 1, color);
        videoOut.drawPixel(x + r - i - 1, y + h - r + j, color);
        videoOut.drawPixel(x + w - r + i, y + h - r + j, color);
      }
    }
  }
}

// App Bar с Mega- и Wi-Fi статусом
void drawAppBar() {
  const uint16_t barH = 44;
  videoOut.fillRect(SCREEN_OFFSET_X, 0, SCREEN_W, barH, COLOR_SURFACE);
  videoOut.drawLine(SCREEN_OFFSET_X, barH, SCREEN_OFFSET_X + SCREEN_W - 1, barH, COLOR_OUTLINE_VAR);

  videoOut.setCursor(SCREEN_OFFSET_X + SPACING_L, SPACING_S + 10);
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_ON_SURFACE);
  videoOut.print("MONITOR");

  videoOut.setCursor(SCREEN_OFFSET_X + SPACING_L, SPACING_S + 20);
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_OUTLINE);
  videoOut.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "No IP");

  const int rightPad = 4;
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_SECONDARY);
  int16_t tx = SCREEN_OFFSET_X + SCREEN_W - rightPad - 5 * 6;
  videoOut.setCursor(tx, SPACING_S + 3);
  videoOut.print(getTimeStr());

  // UART Bridge статус
  drawIcon8x8(SCREEN_OFFSET_X + SCREEN_W - 90, SPACING_S + 16, ICON_BRIDGE, 
               printer.uartBridgeConnected ? COLOR_SUCCESS : COLOR_OUTLINE);

  // Mega статус
  const char* lab = megaConnected ? "MEGA ON" : "MEGA OFF";
  videoOut.setTextSize(1);
  videoOut.setTextColor(megaConnected ? COLOR_SUCCESS : COLOR_ERROR);
  int16_t lw = strlen(lab) * 6;
  videoOut.setCursor(SCREEN_OFFSET_X + SCREEN_W - rightPad - lw, SPACING_S + 20);
  videoOut.print(lab);
}

void drawTemperatureCard(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         const char* title, float cur, float tgt,
                         const uint8_t* icon, uint8_t accent) {
    x += SCREEN_OFFSET_X;
    drawRoundedRect(x, y, w, h, CARD_RADIUS, COLOR_SURFACE);
    videoOut.drawRect(x, y, w, h, COLOR_OUTLINE);
    drawIcon8x8(x + SPACING_M, y + SPACING_M, icon, accent);

    // Заголовок
    videoOut.setCursor(x + SPACING_M + 12, y + SPACING_M + 2);
    videoOut.setTextSize(1);
    videoOut.setTextColor(COLOR_SECONDARY);
    videoOut.print(title);

    // Текущая температура — только число
    videoOut.setCursor(x + SPACING_M, y + SPACING_M + 16);
    videoOut.setTextSize(2);
    videoOut.setTextColor(COLOR_ON_SURFACE);
    videoOut.printf("%.0f", cur);

    // Метка Target
    videoOut.setCursor(x + SPACING_M, y + h - 20);
    videoOut.setTextSize(1);
    videoOut.setTextColor(COLOR_OUTLINE);
    videoOut.print("Target");

    // Целевое значение — только число
    videoOut.setCursor(x + SPACING_M, y + h - 10);
    videoOut.setTextSize(1);
    videoOut.setTextColor(COLOR_ON_SURFACE);
    videoOut.printf("%.0f", tgt);

    // Полоса прогресса по температуре
    if (tgt > 0) {
        uint16_t barW = w - 2 * SPACING_M;
        uint16_t f = map(constrain(cur, 0, tgt), 0, tgt, 0, barW);
        videoOut.fillRect(x + SPACING_M, y + h - 4, barW, 2, COLOR_OUTLINE_VAR);
        if (f) videoOut.fillRect(x + SPACING_M, y + h - 4, f, 2, accent);
    }
}

void drawTemperatureChart() {
  uint16_t x = SPACING_S, y = 150, w = SCREEN_W - 2 * SPACING_S, h = 50;
  drawRoundedRect(x, y, w, h, CARD_RADIUS, COLOR_SURFACE);
  videoOut.drawRect(x, y, w, h, COLOR_OUTLINE);
  videoOut.setCursor(x + SPACING_M, y - 12);
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_ON_SURFACE);
  videoOut.print("TEMPERATURE HISTORY");
  uint16_t cx = x + SPACING_M, cy = y + SPACING_S;
  uint16_t cw = w - 2 * SPACING_M, ch = h - 2 * SPACING_S;
  for (int i = 1; i < 4; i++) {
    uint16_t ly = cy + i * ch / 4;
    videoOut.drawLine(cx, ly, cx + cw, ly, COLOR_OUTLINE_VAR);
  }
  for (int i = 1; i < 120; i++) {
    float t1 = tempHistory[(historyIndex + i - 1) % 120];
    float t2 = tempHistory[(historyIndex + i) % 120];
    if (t1 > 0 && t2 > 0) {
      uint16_t x1 = cx + (i - 1) * cw / 119;
      uint16_t x2 = cx + i * cw / 119;
      uint16_t y1 = cy + ch - map(constrain(t1, 0, 250), 0, 250, 0, ch);
      uint16_t y2 = cy + ch - map(constrain(t2, 0, 250), 0, 250, 0, ch);
      videoOut.drawLine(x1, y1, x2, y2, COLOR_WARNING);
    }
  }
}

void drawProgressCard() {
  uint16_t x = SPACING_S, y = SCREEN_H - 35;
  uint16_t w = SCREEN_W - 2 * SPACING_S, h = 25;
  drawRoundedRect(x, y, w, h, CARD_RADIUS, COLOR_SURFACE);
  videoOut.drawRect(x, y, w, h, COLOR_OUTLINE);
  drawIcon8x8(x + SPACING_M, y + SPACING_S, ICON_PROGRESS, COLOR_PRIMARY);
  videoOut.setCursor(x + SPACING_M + 12, y + SPACING_S + 2);
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_SECONDARY);
  videoOut.print("PRINT PROGRESS");
  uint16_t bx = x + SPACING_M;
  uint16_t by = y + h - 8;
  uint16_t bw = w - 2 * SPACING_M - 20;
  drawRoundedRect(bx, by, bw, 4, 2, COLOR_OUTLINE_VAR);
  uint16_t fw = map(printer.progress, 0, 100, 0, bw);
  if (fw > 2) drawRoundedRect(bx, by, fw, 4, 2, COLOR_PRIMARY);
  videoOut.setCursor(x + w - 18, y + h - 10);
  videoOut.setTextSize(1);
  videoOut.setTextColor(COLOR_ON_SURFACE);
  videoOut.printf("%.0f%%", printer.progress);
}

void drawMaterialInterface() {
  videoOut.startWrite();
  videoOut.fillScreen(COLOR_BACKGROUND);
  drawAppBar();
  drawTemperatureCard(SPACING_S, 52, 110, 75, "EXTRUDER", printer.smoothExt, printer.targetExtruder, ICON_EXTRUDER, COLOR_WARNING);
  drawTemperatureCard(SCREEN_W/2 + SPACING_S/2, 52, 110, 75, "BED", printer.smoothBed, printer.targetBed, ICON_BED, COLOR_PRIMARY);
  drawTemperatureChart();
  drawProgressCard();
  videoOut.endWrite();
}

void setupTime() {
  configTime(3*3600, 0, "pool.ntp.org");
  struct tm t;
  while (!getLocalTime(&t)) delay(200);
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  btStop();
  setCpuFrequencyMhz(240);
  videoOut.begin();
  videoOut.fillScreen(COLOR_BACKGROUND);
  videoOut.setTextColor(COLOR_ON_SURFACE);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  setupTime();
  
  // Сервер для мониторинга (оригинальный)
  server.onClient([](void*, AsyncClient* nc) {
    if (!client || !client->connected()) {
      client = nc;
      client->onData([](void*, AsyncClient*, void* d, size_t l) { 
        Serial2.write((uint8_t*)d, l); 
      });
      client->onDisconnect([](void*, AsyncClient*) { 
        client = nullptr; 
        printer.connected = false; 
      });
      printer.connected = true;
    }
  }, nullptr);
  server.begin();
  
  // Сервер для UART моста
  uartBridgeServer.onClient([](void*, AsyncClient* nc) {
    if (uartClient && uartClient->connected()) {
      nc->close();  // Разрешаем только одно подключение
      return;
    }
    
    uartClient = nc;
    printer.uartBridgeConnected = true;
    
    // Обработка данных от клиента -> отправка в UART
    uartClient->onData([](void*, AsyncClient*, void* data, size_t len) {
      Serial2.write((uint8_t*)data, len);
    });
    
    // Обработка отключения клиента
    uartClient->onDisconnect([](void*, AsyncClient*) {
      uartClient = nullptr;
      printer.uartBridgeConnected = false;
    });
    
    // Отправляем приветственное сообщение
    uartClient->write("# ESP32 UART Bridge Connected\n");
    
  }, nullptr);
  uartBridgeServer.begin();
  
  Serial.println("WiFi UART Bridge started on port " + String(uartBridgePort));
  Serial.println("Monitor server started on port " + String(tcpPort));
  Serial.println("IP: " + WiFi.localIP().toString());
}

void loop() {
  static uint32_t lastUpdate = 0;
  
  // Чтение данных из UART
  while (Serial2.available()) {
    String data = Serial2.readStringUntil('\n');
    lastMegaMillis = millis();
    megaConnected = true;
    
    // Отправляем данные клиенту UART моста
    if (uartClient && uartClient->connected()) {
      uartClient->write((data + "\n").c_str());
    }
    
    // Парсим данные для мониторинга
    if (data.startsWith("T:")) {
      sscanf(data.c_str(), "T:%f /%f B:%f /%f", &printer.extruder, &printer.targetExtruder, &printer.bed, &printer.targetBed);
      tempHistory[historyIndex] = printer.extruder;
      historyIndex = (historyIndex + 1) % 120;
    } else if (data.startsWith("M73")) {
      sscanf(data.c_str(), "M73 P%f", &printer.progress);
    }
  }
  
  // Проверяем соединение с Mega
  if (millis() - lastMegaMillis > 1000) {
    megaConnected = false;
  }
  
  // Сглаживание температур
  printer.smoothExt += (printer.extruder - printer.smoothExt) * 0.15;
  printer.smoothBed += (printer.bed - printer.smoothBed) * 0.15;
  
  // Обновление экрана
  if (millis() - lastUpdate > 66) {
    drawMaterialInterface();
    lastUpdate = millis();
  }
  
  printer.connected = (WiFi.status() == WL_CONNECTED) && client;
  videoOut.waitForFrame();
}
