/*
    Description: Read the ADC value measured by the Watering Unit, and the water pump can be switched on and off through the middle button.
*/


#include <M5StickC.h>

#define EARTH_PIN 33
#define PUMP_PIN 32

int screenSelection = 0;
int secondarySelection = 0;
typedef void (*BtnHandler) (int, int);

int minTimeBetweenWatering = 30000; // Water every 30 seconds max
int autoPumpActiveTime = 3000; // 3 seconds
int pumpLastActivated = -9999;
int requiresWateringReading = 2000;
int readingWaitTime = 30000; // Check if we can water once every 30 secs
int lastCheckedToWater = -9999;
int lastWatered = -9999;

bool pumpActiveFlag = true;
int rawADC;

void setup() {
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextColor(GREEN);
  handleScreen(0, 0);

  pinMode(EARTH_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
}

void setPump(int active) {
  pumpActiveFlag = active;
  digitalWrite(PUMP_PIN, pumpActiveFlag);
  Serial.printf("Pump active? = %d\n", pumpActiveFlag);

  // we store the lastWatered when the pump is turned off
  if(!active) lastWatered = millis();
  // we store the last activated when the pump is turned on
  if(active) pumpLastActivated = millis();
}

void togglePump() {
  pumpActiveFlag = !pumpActiveFlag;
  setPump(pumpActiveFlag);
}

void btnTogglePump(int earthReading, int pumpActive) {
  togglePump();
}

void screenOffController(int earthReading, int pumpActive) {
  M5.Axp.ScreenBreath(7);
}

void infoController(int earthReading, int pumpActive) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 5);

  M5.Lcd.setTextSize(1);
  int secondsSinceWatered = (millis() - lastCheckedToWater) / 1000;
  M5.Lcd.printf("Last water check: %ds ago\n", secondsSinceWatered);
  M5.Lcd.printf("Reading required to water: %d\n", requiresWateringReading);
  M5.Lcd.printf("\nPump Active: %d\nADC Reading: %d", pumpActive, earthReading);
}

void historyController(int earthReading, int pumpActive) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 5);

  M5.Lcd.setTextSize(1);
  int secondsSinceWatered = (millis() - lastCheckedToWater) / 1000;
  M5.Lcd.printf("History not yet available\n", secondsSinceWatered);
}

void titleController(int earthReading, int pumpActive) {
  M5.Axp.ScreenBreath(10);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(15, 30);

  M5.Lcd.setTextSize(2);
  M5.Lcd.println("AutoWater");
  
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(5, 60);
  M5.Lcd.println("Next screen: BtnA");
  M5.Lcd.println("Toggle pump: BtnB");
}

void nextScreen(int earthReading, int pumpActive) {
  screenSelection++;
  
  int screenCount = 4;
  if (screenSelection > (screenCount - 1)) screenSelection = 0;
  
  secondarySelection = 0;
}

BtnHandler getBtnAHandler() {
  switch (screenSelection) {
    case 0:
      return &nextScreen;
  }
}

BtnHandler getBtnBHandler() {
  int selectionCount = 1;
  if (secondarySelection > (selectionCount - 1)) secondarySelection = 0;

  switch (secondarySelection) {
    case 0:
      return &btnTogglePump;
  }
}

void handleScreen(int earthReading, int pumpActive) {

  // ADDING A SCREEN?
  // Update screenCount in nextScreen()
  switch (screenSelection) {
    case 0:
      titleController(earthReading, pumpActive);
      break;
    case 1:
      infoController(earthReading, pumpActive);
      break;
    case 2:
      historyController(earthReading, pumpActive);
      break;
    case 3: // SCREEN OFF MUST BE LAST SCREEN
      screenOffController(earthReading, pumpActive);
      break;
  }
}

void handleButtons(int earthReading, int pumpActive) {
  if (M5.BtnB.wasPressed()) {
    getBtnBHandler()(earthReading, pumpActive);
  }
  if (M5.BtnA.wasPressed()) {
    getBtnAHandler()(earthReading, pumpActive);
  }
}

bool canCheckToWater() {
  return (millis() - lastCheckedToWater) > readingWaitTime;
}

bool canWater() {
  return (millis() - lastWatered) > minTimeBetweenWatering;
}

bool pumpShouldTurnOff(int pumpActive) {
  return pumpActive == 1 && (millis() - pumpLastActivated) > autoPumpActiveTime; 
}

void waterCheck(int earthReading, int pumpActive) {
  if(pumpShouldTurnOff(pumpActive)) {
    setPump(false);
  }

  if(!canCheckToWater()) return;
  lastCheckedToWater = millis();
  if(!canWater()) return;
  if(earthReading >= requiresWateringReading) setPump(true);
}

void loop() {
  M5.update();
  rawADC = analogRead(EARTH_PIN);
  Serial.printf("ADC: %d\n", rawADC);

  handleScreen(rawADC, pumpActiveFlag);
  handleButtons(rawADC, pumpActiveFlag);

  waterCheck(rawADC, pumpActiveFlag);

  delay(100);
}
