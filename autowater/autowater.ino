#include <M5StickC.h>

#define EARTH_PIN 33
#define PUMP_PIN 32

int requiresWaterReading = 2000;

int lowPowerSleepSeconds = 1200; //20 mins
int readingWaitTime = 30000; // 30 secs
int checksPerWake = 3;

int maxPumpActiveTimeSeconds = 4;

bool pumpActiveFlag = true;

void setup() {
  M5.begin();
  Serial.println("Device active");
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextColor(GREEN);

  pinMode(EARTH_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
}

void setPump(int active) {
  pumpActiveFlag = active;
  digitalWrite(PUMP_PIN, pumpActiveFlag);
  Serial.printf("Pump active: %d\n", pumpActiveFlag);
}

void togglePump() {
  setPump(!pumpActiveFlag);
}

void deepSleep(int timeSeconds) {
  Serial.printf("Deep sleep %ds (millis: %d)\n", timeSeconds, millis());
  M5.Axp.DeepSleep(SLEEP_SEC(timeSeconds));
  Serial.printf("Shouln't see this - but woke up from deep sleep (millis: %d)", millis());
}

void lightSleep() {
  Serial.printf("Light sleep (millis is %d)\n", millis());
  M5.Axp.SetLDO2(false); //lcd off
  M5.Axp.LightSleep(SLEEP_SEC(lowPowerSleepSeconds));
  Serial.printf("Woke up (millis is %d)\n", millis());
  M5.Axp.SetLDO2(true); //lcd on
}

void printStatus() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(5,25);
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Check water");
  M5.Lcd.setCursor(15,45);
  M5.Lcd.println("cycle...");
}

void waterTick(int earthReading, int pumpActive) {
  if(earthReading >= requiresWaterReading) {
    Serial.printf("Busy water (%d)\n", earthReading);
    setPump(true);
    delay(maxPumpActiveTimeSeconds * 1000);
    setPump(false);
    Serial.println("Water complete");
  }
}

void loop() {
  Serial.println("Loop start");
  printStatus();

  for(int i = 1; i <= checksPerWake; i++) {
    Serial.printf("Checks loop %d\n", i);
    
    int rawADC = analogRead(EARTH_PIN);
    waterTick(rawADC, pumpActiveFlag);
    
    if(i != checksPerWake) delay(readingWaitTime);
  }

  deepSleep(lowPowerSleepSeconds);
}
