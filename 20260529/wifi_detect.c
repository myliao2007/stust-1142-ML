#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

// ==========================================
// 1. 硬體（WS2812）設定區
// ==========================================
#define PIN_WS2812  48    
#define NUM_PIXELS  1     
#define BRIGHTNESS  50    
#define SCAN_INTERVAL 10000  // 10 秒掃描一次

Adafruit_NeoPixel pixels(NUM_PIXELS, PIN_WS2812, NEO_GRB + NEO_KHZ800);

uint32_t RED    = pixels.Color(255, 0, 0);
uint32_t GREEN  = pixels.Color(0, 255, 0);
uint32_t BLUE   = pixels.Color(0, 0, 255);
uint32_t YELLOW = pixels.Color(255, 150, 0); 
uint32_t OFF    = pixels.Color(0, 0, 0);

// ==========================================
// 2. 監控清單設定
// ==========================================
#define MAX_SSID_LIST 10  // 最多可監控 10 個 SSID

struct SSIDEntry {
  char ssid[32];
  bool inUse;
  bool lastDetected;  // 上一次掃描是否被檢測到
};

SSIDEntry ssidList[MAX_SSID_LIST];
int ssidCount = 1;  // 預設有 1 個：aaron

unsigned long lastScanTime = 0;

// ==========================================
// 函式宣告
// ==========================================
void setLedColor(uint32_t color);
void initSSIDList();
void scanWiFi();
void printSSIDList();
void addSSID(String newSSID);
void removeSSID(String targetSSID);

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000); 
  
  pixels.begin();
  pixels.setBrightness(BRIGHTNESS);
  pixels.show(); 

  // 設定 WiFi 為只掃描模式 (不連線)
  WiFi.mode(WIFI_STA);

  // 初始化 SSID 監控清單
  initSSIDList();

  Serial.println("\n==================================================");
  Serial.println("【WiFi SSID 監控器】終端機就緒！");
  Serial.println("命令說明：");
  Serial.println("  add SSID_NAME       - 新增監控的 SSID");
  Serial.println("  remove SSID_NAME    - 移除監控的 SSID");
  Serial.println("  list                - 顯示監控清單");
  Serial.println("  scan                - 立即掃描一次");
  Serial.println("==================================================\n");
  
  printSSIDList();
  
  // 立即進行第一次掃描
  lastScanTime = millis();
  scanWiFi();
}

void loop() {
  // 處理終端機輸入命令
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.length() > 0) {
      handleCommand(input);
    }
    
    delay(100);
  }
  
  // 每 10 秒自動掃描一次
  if (millis() - lastScanTime >= SCAN_INTERVAL) {
    lastScanTime = millis();
    scanWiFi();
  }
  
  setLedColor(GREEN);
  delay(10);
}

void setLedColor(uint32_t color) {
  for(int i=0; i<NUM_PIXELS; i++) {
    pixels.setPixelColor(i, color);
  }
  pixels.show();
}

// ==========================================
// 初始化 SSID 監控清單
// ==========================================
void initSSIDList() {
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    ssidList[i].inUse = false;
    ssidList[i].lastDetected = false;
    memset(ssidList[i].ssid, 0, sizeof(ssidList[i].ssid));
  }
  
  // 預設新增 "aaron"
  strcpy(ssidList[0].ssid, "aaron");
  ssidList[0].inUse = true;
  ssidCount = 1;
}

// ==========================================
// 掃描 WiFi 網路
// ==========================================
void scanWiFi() {
  Serial.println("\n🔍 掃描周遭 WiFi 網路...");
  setLedColor(YELLOW);

  int n = WiFi.scanNetworks();
  Serial.printf("找到 %d 個網路。\n", n);

  // 重置上一次的檢測狀態
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    if (ssidList[i].inUse) {
      ssidList[i].lastDetected = false;
    }
  }

  if (n > 0) {
    // 巡迴所有掃描到的 SSID
    for (int i = 0; i < n; i++) {
      String foundSSID = WiFi.SSID(i);
      int32_t rssi = WiFi.RSSI(i);

      // 與監控清單進行比對
      for (int j = 0; j < MAX_SSID_LIST; j++) {
        if (ssidList[j].inUse && foundSSID == String(ssidList[j].ssid)) {
          ssidList[j].lastDetected = true;
          
          // 🎯 發現被監控的 SSID
          Serial.println();
          Serial.println("╔═══════════════════════════════════════╗");
          Serial.printf("║  📡 我發現 '%s' 出現在附近\n", ssidList[j].ssid);
          Serial.printf("║  訊號強度: %d dBm\n", rssi);
          Serial.println("╚═══════════════════════════════════════╝");
          
          // 閃爍黃燈提示
          for (int k = 0; k < 3; k++) {
            setLedColor(YELLOW);
            delay(200);
            setLedColor(GREEN);
            delay(200);
          }
          
          break;
        }
      }
    }
  }

  WiFi.scanDelete();
  Serial.println("掃描完成。");
  Serial.println("--------------------------------------------------");
  
  setLedColor(GREEN);
}

// ==========================================
// 顯示監控清單
// ==========================================
void printSSIDList() {
  Serial.println("\n📋 目前監控的 SSID 清單：");
  Serial.println("--------------------------------------------------");
  
  int count = 0;
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    if (ssidList[i].inUse) {
      count++;
      String status = ssidList[i].lastDetected ? "✅ 最後檢測到" : "❌ 未檢測到";
      Serial.printf("%d. %s [%s]\n", count, ssidList[i].ssid, status.c_str());
    }
  }
  
  if (count == 0) {
    Serial.println("(空)");
  }
  
  Serial.println("--------------------------------------------------\n");
}

// ==========================================
// 新增 SSID 到監控清單
// ==========================================
void addSSID(String newSSID) {
  // 檢查是否已存在
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    if (ssidList[i].inUse && String(ssidList[i].ssid) == newSSID) {
      Serial.printf("⚠️  '%s' 已在監控清單中。\n", newSSID.c_str());
      return;
    }
  }

  // 尋找空位
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    if (!ssidList[i].inUse) {
      strncpy(ssidList[i].ssid, newSSID.c_str(), sizeof(ssidList[i].ssid) - 1);
      ssidList[i].ssid[sizeof(ssidList[i].ssid) - 1] = '\0';
      ssidList[i].inUse = true;
      ssidList[i].lastDetected = false;
      ssidCount++;
      
      Serial.printf("✅ 已新增 '%s' 到監控清單。\n", newSSID.c_str());
      printSSIDList();
      return;
    }
  }

  Serial.printf("❌ 監控清單已滿（最多 %d 個）。\n", MAX_SSID_LIST);
}

// ==========================================
// 移除 SSID 從監控清單
// ==========================================
void removeSSID(String targetSSID) {
  for (int i = 0; i < MAX_SSID_LIST; i++) {
    if (ssidList[i].inUse && String(ssidList[i].ssid) == targetSSID) {
      memset(ssidList[i].ssid, 0, sizeof(ssidList[i].ssid));
      ssidList[i].inUse = false;
      ssidList[i].lastDetected = false;
      ssidCount--;
      
      Serial.printf("✅ 已移除 '%s' 從監控清單。\n", targetSSID.c_str());
      printSSIDList();
      return;
    }
  }

  Serial.printf("❌ 未找到 '%s' 在監控清單中。\n", targetSSID.c_str());
}

// ==========================================
// 處理終端機命令
// ==========================================
void handleCommand(String command) {
  if (command.startsWith("add ")) {
    String newSSID = command.substring(4);
    newSSID.trim();
    if (newSSID.length() > 0) {
      addSSID(newSSID);
    } else {
      Serial.println("❌ 用法: add SSID_NAME");
    }
  }
  else if (command.startsWith("remove ")) {
    String targetSSID = command.substring(7);
    targetSSID.trim();
    if (targetSSID.length() > 0) {
      removeSSID(targetSSID);
    } else {
      Serial.println("❌ 用法: remove SSID_NAME");
    }
  }
  else if (command == "list") {
    printSSIDList();
  }
  else if (command == "scan") {
    lastScanTime = millis();
    scanWiFi();
  }
  else {
    Serial.println("❌ 未知命令。輸入 'add', 'remove', 'list', 或 'scan'。");
  }
}
