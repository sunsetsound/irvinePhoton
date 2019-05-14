#include <SSD1306_128x32.h>

#include "Particle.h"

//Use I2C with OLED RESET pin on D4
#define OLED_RESET D4
SSD1306_128x32 oled(OLED_RESET);

STARTUP(WiFi.selectAntenna(ANT_EXTERNAL));
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

void mainThread(void * param);
Thread thread("mainThread", mainThread);
void backgroundThread(void * param);
Thread thread2("backgroundThread", backgroundThread);
system_tick_t lastThreadTime = 0;

bool spkrOn;
int light;
int button;
int percentage;
double voltage;
double filterVoltage = (analogRead(A2) / 4095.0) * 3.3 * 4.0; //we initallized instead of declared to 
unsigned long old_time = 0;

void setup() {
  Particle.connect();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.display();
  RGB.control(false);
  Particle.variable("per", old_time);
  Particle.variable("voltage", filterVoltage);
  Particle.function("relayHandler", relayHandler);
  pinMode(A1, INPUT); //photoresistor on A1
  pinMode(A2, INPUT);
  pinMode(D5, INPUT); //D5 is button
  pinMode(D3, OUTPUT); //Relay Pin
  Time.zone(-8);
  Time.beginDST();
  relayHandler("off"); //to initialize off screen
}

void loop() {

}

void mainThread(void * param) { //this thread gets voltage readings and calculates RMS voltage, runs slow bc of calculations
  while (true) {
    if (spkrOn) {
      oled.clearDisplay();
      oled.setCursor(0, 0);
      oled.setTextSize(1);
      if (Particle.connected()) {
        oled.print("WiFi:");
        String SSID = String(WiFi.SSID());
        if (SSID.length() > 5) { //Cuts the SSID if the string is too long
          oled.print(SSID.substring(0, 5));
        } 
        else { //add spaces and print extra spaces to keep format
          oled.print(SSID);
          for (int i = SSID.length(); i < 5; i++) {
            oled.print(" ");
          }
        }
        oled.print("   ");
      } 
      else {
        oled.print("WiFi: None");
        oled.print("   ");
      }

      if (Time.hourFormat12() < 10) { //adds an extra space in the time if it is single digit
        oled.print(" ");
      }

      oled.print(Time.hourFormat12());
      oled.print(":");

      if (Time.minute() < 10) { //adds the leading 0 in the minute <10
        oled.print("0");
      }

      oled.print(Time.minute());

      if (Time.isPM()) //adds am+pm after the time
        oled.println(" PM");
      else
        oled.println(" AM");

      /*
      if(millis() - old_time >= 1000)
      {
            voltage = (analogRead(A2)/4095.0)*3.3*4.0;//analogRead will map 0-3.3v as 0 to 4095; multiply by 4 resistors to get vbatt
            filterVoltage = (0.2*voltage)+((1-0.2)*filterVoltage);
            percentage = map(filterVoltage,9.0,12.6,0.0,100.0);
            old_time = millis(); //update old_time to current millis()
       }
       */
      voltage = (analogRead(A2) / 4095.0) * 3.3 * 4.0; //analogRead will map 0-3.3v as 0 to 4095; multiply by 4 resistors to get vbatt

      if (voltage < 9 || voltage > 13) { //helps filter extreme values that will throw off calculations
        delay(50);
        voltage = (analogRead(A2) / 4095.0) * 3.3 * 4.0;
      }
      filterVoltage = (0.1 * voltage) + ((1 - 0.1) * filterVoltage);
      percentage = map(filterVoltage, 9.0, 12.6, 0.0, 100.0);
      if (percentage > 100) {
        percentage = 100;
      }

      oled.setTextSize(2);
      if (percentage < 0) {
        oled.print("--%");
      } else if (percentage < 10) {
        oled.print("0");
        oled.print(percentage);
        oled.print("%");
      } else {
        oled.print(percentage);
        oled.print("%");
      }
      oled.setTextSize(1);
      oled.setCursor(90, 10);
      oled.print(voltage);
      oled.print("V");
      oled.setCursor(85, 17);
      oled.print(int((percentage / 100.0) * 10200));
      oled.print("mAh");
      oled.display();
      delay(25);
    }
    delay(25);
    os_thread_delay_until( & lastThreadTime, 500);

  }
}

void backgroundThread(void * param) { //dims LED and handles button read
  while (true) {
    button = digitalRead(D5);
    if (button == 1) {
      relayHandler(" ");
    }
    //status LED dimming
    light = analogRead(A1); //max value of 4095, 12bit resolution at 3.3v
    light = map(light, 0, 4095, 10, 255);
    RGB.brightness(light);

    os_thread_delay_until( & lastThreadTime, 25);
  }
}
    
//Handles Requests to turn on the speaker from Google Assistant, Alexa, and IFTTT
bool relayHandler(String args) {
  if (args == "on" || args == "connect") {
    digitalWrite(D3, HIGH);
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextColor(WHITE);
    oled.setTextSize(2.5);
    oled.println("Sunset");
    oled.println("   Sounds");
    oled.display();
    delay(1500);
    spkrOn = TRUE;
  } else if (args == "off" || args == "disconnect") {
    digitalWrite(D3, LOW);
    oled.setCursor(0, 0);
    spkrOn = FALSE;
    oled.clearDisplay();
    oled.display();
    delay(500);
  } else if (spkrOn) {
    digitalWrite(D3, LOW);
    oled.setCursor(0, 0);
    spkrOn = FALSE;
    oled.clearDisplay();
    oled.display();
    delay(500);
  } else {
    digitalWrite(D3, HIGH);
    oled.clearDisplay();
    oled.setCursor(0, 0);
    oled.setTextColor(WHITE);
    oled.setTextSize(2);
    oled.println("Sunset");
    oled.println("   Sounds");
    oled.display();
    delay(1500);
    spkrOn = TRUE;
  }
  return spkrOn;
}
