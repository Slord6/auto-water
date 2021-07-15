#include <M5StickC.h>

#define EARTH_PIN 33
#define PUMP_PIN 32

int screenSelection = 0;
int secondarySelection = 0;
typedef void (*BtnHandler) (int, int);

int minTimeBetweenWatering = 20000; // Water every 20 seconds max
int autoPumpActiveTime = 3000; // 3 seconds
int pumpLastActivated = -9999; // at what millis() was the pump last activated (ie went from off -> on)
int requiresWateringReading = 2000; //What ADC reading do we need for the pump to activate
int readingWaitTime = 60000; // Check if we can water once every 60 secs
int lastCheckedToWater = -9999; // when the system last checked if we needed to water
int lastWatered = -9999; // when the system last watered

int lowPowerSleepSeconds = 1200;
int lowPowerActiveSeconds = 60;
int lowPowerLastWoke = 0;

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
  Serial.printf("Pump active: %d\n", pumpActiveFlag);

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

int secondsSinceWatered() {
  return (millis() - lastCheckedToWater) / 1000;
}

bool pumpShouldTurnOff(int pumpActive) {
  return pumpActive == 1 && (millis() - pumpLastActivated) > autoPumpActiveTime; 
}

bool canGoLowPower() {
  return (millis() - lowPowerLastWoke) > (lowPowerActiveSeconds * 1000);
}

int timeToNextSleepSeconds() {
  int timeSinceWoke = millis() - lowPowerLastWoke;
  int remainingAwake = (lowPowerActiveSeconds * 1000) - timeSinceWoke;
  return remainingAwake / 1000;
}

void lowPowerController(int earthReading, int pumpActive) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0,10);
  M5.Lcd.println("Ready to go low power...");
  M5.Lcd.printf("Time since last water: %ds\n", secondsSinceWatered());
  M5.Lcd.printf("Battery Voltage: %.3f\n", M5.Axp.GetBatVoltage());
  M5.Lcd.println("\nBtnA to cancel");

  int timeToNextSleep = timeToNextSleepSeconds();
  // if we're more than 1s behind schedule, we probably just re-entered the low-power screen
  // so we give the user 10s to skip
  if(timeToNextSleep < -1) {
    // New fake last woke time is now, less how long we should be awake for, plus 10s to give the buffer
    lowPowerLastWoke = millis() - (lowPowerActiveSeconds * 1000) + (10 * 1000);
  }
  M5.Lcd.printf("Sleeping in: %ds\n", timeToNextSleep);

  if(!canGoLowPower()) return;

  Serial.printf("Entering light sleep (%ds)\n", lowPowerSleepSeconds);
  setPump(false); // double make sure that the pump isn't going to be running whilst the system sleeps
  delay(100); //allow serial to complete

  
  M5.Axp.SetLDO2(false); //lcd off
  M5.Axp.LightSleep(SLEEP_SEC(lowPowerSleepSeconds));
  lowPowerLastWoke = millis(); // update tracker for last wake time
  Serial.printf("Woke up (millis is %d)\n", millis());
  M5.Axp.SetLDO2(true); //lcd on

  // Setup system so that a check will happen when it's meant to
  //waterCheck(0,0);
  //lastCheckedToWater -= (sleepTime * 1000);
  Serial.printf("Time since last water: %ds\n", secondsSinceWatered());
}

void infoController(int earthReading, int pumpActive) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 5);

  M5.Lcd.setTextSize(1);
  M5.Lcd.printf("Last watering: %ds ago\n", secondsSinceWatered());
  int timeToReading = (readingWaitTime - (millis() - lastCheckedToWater))/1000;
  M5.Lcd.printf("Next water check: %ds\n", timeToReading);
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
  M5.Lcd.setCursor(0, 60);
  M5.Lcd.println("Next screen: BtnA");
  M5.Lcd.println("3s pump: BtnB");
}

void pumpOffController(int earthReading, int pumpActive) {
  setPump(false);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(15, 30);
  M5.Lcd.println("Pump forced OFF");
}

void nextScreen(int earthReading, int pumpActive) {
  screenSelection++;
  
  int screenCount = 5;
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
    case 3:
      pumpOffController(earthReading, pumpActive);
      break;
    case 4:
      lowPowerController(earthReading, pumpActive);
      break;
  }
}

bool handleButtons(int earthReading, int pumpActive) {
  if (M5.BtnB.wasPressed()) {
    getBtnBHandler()(earthReading, pumpActive);
    return true;
  }
  if (M5.BtnA.wasPressed()) {
    getBtnAHandler()(earthReading, pumpActive);
    return true;
  }
  return false;
}

bool canCheckToWater() {
  return (millis() - lastCheckedToWater) > readingWaitTime;
}

bool canWater() {
  return (millis() - lastWatered) > minTimeBetweenWatering;
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
