/*
    Description: Read the ADC value measured by the Watering Unit, and the water pump can be switched on and off through the middle button.
*/


#include <M5StickC.h>

#define EARTH_PIN 33
#define PUMP_PIN 32

bool pumpActiveFlag = true;
int rawADC;

void setup() { 
    M5.begin();
    M5.Lcd.setRotation(3);
    M5.Lcd.setTextColor(GREEN);
    drawInfo(0,0);
    
    pinMode(EARTH_PIN,INPUT);
    pinMode(PUMP_PIN,OUTPUT);
    
    
    //pinMode(25,OUTPUT);
    //digitalWrite(25,0);
}

void drawInfo(int earthReading, int pumpActive) {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0,0);
    
    M5.Lcd.setTextSize(2);
    M5.Lcd.println("Watering\n     Monitor");
    
    M5.Lcd.setTextSize(1);
    M5.Lcd.println("Manual toggle pump: BtnB");
    M5.Lcd.printf("\n\n\tPump Active: %d\n\tADC Reading: %d", pumpActive, earthReading);
}

void handleButtons() {
  if(M5.BtnB.wasPressed()){
      digitalWrite(PUMP_PIN, pumpActiveFlag);
      Serial.printf("Pump active? = %d\n",pumpActiveFlag); 
      pumpActiveFlag = !pumpActiveFlag;
  }
}

void loop() {
  M5.update();
  rawADC = analogRead(EARTH_PIN);
  Serial.printf("ADC: %d\n", rawADC);

  drawInfo(rawADC, pumpActiveFlag);
  
  handleButtons();
  
  delay(100);
}
