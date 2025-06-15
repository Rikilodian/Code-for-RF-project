#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

// OLED Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_SDA 21
#define OLED_SCL 22
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Button Pins
#define SELECT_BUTTON_PIN 12  // GPIO 12 - Select/Stop/Send
#define SCROLL_BUTTON_PIN 13  // GPIO 13 - Scroll Down

// NRF24L01 Pins
#define NRF1_CE 4
#define NRF1_CSN 5
#define NRF2_CE 16
#define NRF2_CSN 17
#define NRF3_CE 32
#define NRF3_CSN 33

// SPI Pins (ESP32 Default)
#define SCK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23

RF24 radio1(NRF1_CE, NRF1_CSN);  // Radio 1
RF24 radio2(NRF2_CE, NRF2_CSN);  // Radio 2
RF24 radio3(NRF3_CE, NRF3_CSN);  // Radio 3

byte garbageData[32];  // Random data buffer
uint64_t pipeAddress = 0xF0F0F0F0E1LL;  // Common pipe address

// Menu Variables
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
int selectedIndex = 0;
const int totalOptions = 4;
String menuOptions[totalOptions] = {"CODE1: WiFi Scan", "CODE2: Fake WiFi", "CODE3: WiFi Jammer", "CODE4: Message Sender"};
bool inMenu = true;

// WiFi Jammer Variables
bool isJamming = false;
unsigned long lastHop = 0;
unsigned long lastSpeedChange = 0;
int channelInterval = 2000;  // Channel hop every 2 sec
int speedInterval = 1500;    // Speed change every 1.5 sec

// Fake WiFi Variables
const char* ssidList[] = {"MR-Campus", "MR-Campus1", "MR-Campus2"};
const int ssidCount = 3;

// Message Sender Variables
String messages[] = {
  "Hi", "How are you?", "Good Morning", "Good Night",
  "Bye", "Take Care", "Thank You", "Welcome", 
  "All the best", "See you soon"
};
const int totalMessages = sizeof(messages) / sizeof(messages[0]);
int selectedMessage = 0;
int topMessage = 0;

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed!");
    while (1);
  }

  // Initialize Buttons
  pinMode(SELECT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(SCROLL_BUTTON_PIN, INPUT_PULLUP);

  // Initialize NRF24L01 Radios
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN);
  setupRadio(radio1);
  setupRadio(radio2);
  setupRadio(radio3);
  randomSeed(analogRead(0));  // Better randomness

  // Show Menu
  showMenu();
}

// ====================== MAIN LOOP ======================
void loop() {
  if (inMenu) {
    // Scroll Down (GPIO 13)
    if (digitalRead(SCROLL_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      selectedIndex = (selectedIndex + 1) % totalOptions;
      showMenu();
      lastDebounceTime = millis();
    }

    // Select (GPIO 12)
    if (digitalRead(SELECT_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      inMenu = false;
      lastDebounceTime = millis();

      if (selectedIndex == 0) runCode1();      // WiFi Scan
      else if (selectedIndex == 1) runCode2(); // Fake WiFi
      else if (selectedIndex == 2) runCode3(); // WiFi Jammer
      else runCode4();                         // Message Sender

      inMenu = true;  // Return to menu
      showMenu();
    }
  }

  // If CODE3 is running, check for STOP (GPIO 12)
  if (!inMenu && selectedIndex == 2) {
    if (digitalRead(SELECT_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      stopJamming();
      inMenu = true;
      showMenu();
      lastDebounceTime = millis();
    }
  }
}

// ====================== MENU FUNCTIONS ======================
void showMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Select Mode:");

  for (int i = 0; i < totalOptions; i++) {
    display.setCursor(5, (i + 1) * 15);
    display.print(i == selectedIndex ? "> " : "  ");
    display.println(menuOptions[i]);
  }
  display.display();
}

// ====================== CODE1: WiFi Scan ======================
void runCode1() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int n = WiFi.scanNetworks();

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Networks:");
  display.print("Found: ");
  display.println(n);

  for (int i = 0; i < min(n, 3); i++) {  // Show first 3 networks
    display.setCursor(0, (i + 2) * 10);
    display.print(WiFi.SSID(i));
    display.print(" (");
    display.print(WiFi.RSSI(i));
    display.println("dBm)");
  }
  display.display();

  delay(5000);  // Show for 5 sec
  WiFi.mode(WIFI_OFF);
}

// ====================== CODE2: Fake WiFi ======================
void runCode2() {
  WiFi.mode(WIFI_AP);
  unsigned long startTime = millis();
  int currentSSID = 0;

  while (millis() - startTime < 30000) {  // Run for 30 sec
    WiFi.softAP(ssidList[currentSSID]);

    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Fake WiFi:");
    display.print("Broadcasting: ");
    display.println(ssidList[currentSSID]);
    display.display();

    currentSSID = (currentSSID + 1) % ssidCount;
    delay(1000);  // Change SSID every 1 sec
  }

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

// ====================== CODE3: WiFi Jammer ======================
void runCode3() {
  isJamming = true;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("WiFi Jammer: ON");
  display.println("Press SELECT");
  display.println("to STOP");
  display.display();

  while (isJamming) {
    // Fill buffer with random data
    for (int i = 0; i < 32; i++) {
      garbageData[i] = random(0, 256);
    }

    // Transmit garbage data on all radios
    radio1.startFastWrite(garbageData, sizeof(garbageData), false, true);
    radio2.startFastWrite(garbageData, sizeof(garbageData), false, true);
    radio3.startFastWrite(garbageData, sizeof(garbageData), false, true);

    // Channel Hopping
    if (millis() - lastHop > channelInterval) {
      changeChannel(radio1);
      changeChannel(radio2);
      changeChannel(radio3);
      lastHop = millis();
    }

    // Speed Variation
    if (millis() - lastSpeedChange > speedInterval) {
      changeDataRate(radio1);
      changeDataRate(radio2);
      changeDataRate(radio3);
      lastSpeedChange = millis();
    }

    // Check for STOP (non-blocking)
    if (digitalRead(SELECT_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      stopJamming();
      break;
    }
  }
}

void stopJamming() {
  isJamming = false;
  radio1.stopListening();
  radio2.stopListening();
  radio3.stopListening();
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Jamming STOPPED");
  display.display();
  delay(1000);  // Brief pause before returning to menu
}

// ====================== CODE4: Message Sender ======================
void runCode4() {
  // Initialize radio for message sending (using radio1)
  radio1.begin();
  radio1.openWritingPipe(pipeAddress);
  radio1.setPALevel(RF24_PA_HIGH);
  radio1.setDataRate(RF24_250KBPS);
  radio1.stopListening(); // TX mode

  bool inMessageMenu = true;
  
  while(inMessageMenu) {
    showMessageMenu();
    
    // Scroll Down (GPIO 13)
    if (digitalRead(SCROLL_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      selectedMessage = (selectedMessage + 1) % totalMessages;
      if (selectedMessage < topMessage) {
        topMessage = selectedMessage;
      } else if (selectedMessage > topMessage + 4) {
        topMessage = selectedMessage - 4;
      }
      lastDebounceTime = millis();
    }
    
    // Select/Send (GPIO 12)
    if (digitalRead(SELECT_BUTTON_PIN) == LOW && millis() - lastDebounceTime > debounceDelay) {
      sendCurrentMessage();
      lastDebounceTime = millis();
      
      // After sending, return to main menu
      inMessageMenu = false;
    }
  }
  
  // Reinitialize radio for other functions
  setupRadio(radio1);
}

void showMessageMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  for (int i = 0; i < 5; i++) {
    int messageIndex = topMessage + i;
    if (messageIndex >= totalMessages) break;
    display.setCursor(0, i * 12);
    display.print(messageIndex == selectedMessage ? ">" : " ");
    display.setCursor(10, i * 12);
    display.println(messages[messageIndex]);
  }

  display.display();
}

void sendCurrentMessage() {
  String message = messages[selectedMessage];
  char buffer[32];
  message.toCharArray(buffer, sizeof(buffer));

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Sending...");
  display.display();

  bool sent = radio1.write(&buffer, sizeof(buffer));

  display.clearDisplay();
  display.setCursor(0, 0);
  if (sent) {
    display.println("Sent:");
    display.setCursor(0, 20);
    display.println(message);
  } else {
    display.println("Send Failed!");
  }
  display.display();

  delay(1000);
}

// ====================== NRF24L01 HELPER FUNCTIONS ======================
void setupRadio(RF24 &radio) {
  radio.begin();
  radio.setAutoAck(false);
  radio.setRetries(0, 0);
  radio.setPALevel(RF24_PA_MAX);
  radio.enableDynamicPayloads();
  radio.openWritingPipe(pipeAddress);
}

void changeChannel(RF24 &radio) {
  uint8_t newChannel = random(1, 83);  // 2.4GHz channels (1-82)
  radio.setChannel(newChannel);
}

void changeDataRate(RF24 &radio) {
  uint8_t newRate = random(0, 2);
  radio.setDataRate(newRate == 0 ? RF24_1MBPS : RF24_2MBPS);
}