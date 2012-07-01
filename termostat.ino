#include <LiquidCrystal.h>
#include <Wire.h>

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);

const int   buttonPlusPin   = 13;
const int   buttonMinusPin  = 12;
const int   buttonMenuPin   = 10;
const int   relayPin        = 11;
const int   tmp102Address   = 0x48;

int   appState         = 0;
long  previousMillis   = 0;
float requiredTemp     = 17.5;
int   buttonPlusState  = LOW;
int   buttonMinusState = LOW;
int   buttonMenuState  = LOW;
long  currentMillis    = 0;
float currentTemp      = 0;
float tempTolerance    = 0.5;
int   relayDelay       = 10;
int   relayOverride    = 0;
long  relayLastStop    = 0;
boolean relayMayCount  = false;

//statistics
float minTemp = 0;
float maxTemp = 0;

void setup(){
  pinMode(buttonPlusPin, INPUT);
  pinMode(buttonMinusPin, INPUT);
  pinMode(buttonMenuPin, INPUT);
  pinMode(relayPin, OUTPUT);
  lcd.begin(16, 2);
  Wire.begin();
  Serial.begin(9600);
}

void loop(){
  currentMillis = millis();
  currentTemp = getCurrentTemp();
  buttonMenuState  = digitalRead(buttonMenuPin);
  
  //set statistics
  if(currentTemp < minTemp || minTemp == 0) minTemp = currentTemp;
  if(currentTemp > maxTemp || minTemp == 0) maxTemp = currentTemp;
    
  //select view using menu button
  if (buttonMenuState == HIGH && currentMillis - previousMillis > 200) {
    previousMillis = currentMillis;
    if (appState == 5) {
      appState = 0;
    } else {
      appState++;      
    }
  }
  
  //show selected view
  switch(appState){
    case 0:
      homeView();
      break;    
    case 1:
      toleranceView();
      break;
    case 2:
      delayView();
      break;
    case 3:
      minMaxView();
      break;
    case 4:
      lastView();
      break;
    case 5:
      overrideView();
      break;
  }
  
  //temperature control
  if (tooHot() && relayOverride == 0){
    if(enoughRelayRest() || relayLastStop == 0){
      digitalWrite(relayPin, HIGH);
      relayLastStop = 0;
      relayMayCount = true;
    }
  } else {
    digitalWrite(relayPin, LOW);
    if(relayMayCount){
      relayLastStop = millis();
      relayMayCount = false;
    }
  }
  //overriding thermostat
  if(relayOverride == 1) digitalWrite(relayPin, HIGH);
  if(relayOverride == 2) digitalWrite(relayPin, LOW);
}

void homeView(){
  requiredTemp = readFromKeyboard(requiredTemp, 0.1, 200, 100);
  
  //display lcd data
  lcd.setCursor(0, 0);
  lcd.print("Actual  : ");
  lcd.print(currentTemp);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Necesar : ");
  lcd.print(requiredTemp);
  lcd.print("C");
}

void toleranceView(){
  tempTolerance = readFromKeyboard(tempTolerance, 0.05, 200, 5);
  
  //display lcd data
  lcd.setCursor(0, 0);
  lcd.print("1. Toleranta    ");
  lcd.setCursor(0, 1);
  lcd.print("   ");
  lcd.print(tempTolerance);
  lcd.print("C        ");
}

void delayView(){
  relayDelay = readFromKeyboard(relayDelay, 1, 200, 60);
  //display lcd data
  lcd.setCursor(0, 0);
  lcd.print("2. Intarziere   ");
  lcd.setCursor(0, 1);
  lcd.print("   ");
  lcd.print(relayDelay);
  lcd.print(" minute        ");
}

void minMaxView(){
  //display lcd data
  lcd.setCursor(0, 0);
  lcd.print("3. Minim  Maxim ");
  lcd.setCursor(0, 1);
  lcd.print("   ");
  lcd.print(minTemp);
  lcd.print("C ");
  lcd.print(maxTemp);
  lcd.print("C");
}

void lastView(){
  lcd.setCursor(0, 0);
  lcd.print("4. Repaos            ");
  lcd.setCursor(0, 1);
  lcd.print("   ");
  if(relayLastStop == 0){
    lcd.print("Pana acum nu       ");
  } else {
    print_time(currentMillis - relayLastStop);
    lcd.print("                 ");
  }
}

void overrideView(){
  relayOverride = readFromKeyboard(relayOverride, 1, 200, 2);
  
  //display lcd data
  lcd.setCursor(0, 0);
  lcd.print("5. Fortat       ");
  lcd.setCursor(0, 1);
  if(relayOverride == 0){
    lcd.print("Fara fortare    ");
  }
  if(relayOverride == 1){
    lcd.print("Mereu pornit    ");
  }
  if(relayOverride == 2){
    lcd.print("Mereu oprit     ");
  }
}

float readFromKeyboard(float val, float increment, int pressDelay, float maxim){
  buttonPlusState  = digitalRead(buttonPlusPin);
  buttonMinusState = digitalRead(buttonMinusPin);
  if (buttonPlusState == HIGH && currentMillis - previousMillis > pressDelay) {
    previousMillis = currentMillis;
    val += increment;
    if (val > maxim) val = 0;
  }
  if (buttonMinusState == HIGH && currentMillis - previousMillis > pressDelay) {
    previousMillis = currentMillis;
    val -= increment;
    if (val < 0) val = 0;
  }
  return val;
}

float getCurrentTemp(){
  Wire.requestFrom(tmp102Address, 2); 
  byte MSB = Wire.read();
  byte LSB = Wire.read();
  int TemperatureSum = ((MSB << 8) | LSB) >> 4;
  return TemperatureSum * 0.0625;
}

boolean tooHot(){
  return (requiredTemp < currentTemp + tempTolerance) || (requiredTemp < currentTemp - tempTolerance);
}

boolean enoughRelayRest(){
  return millis() - relayLastStop > relayDelay * 60000;
}

// argument is time in milliseconds
void print_time(unsigned long t_milli)
{
    char buffer[20];
    int days, hours, mins, secs;
    int fractime;
    unsigned long inttime;

    inttime  = t_milli / 1000;
    fractime = t_milli % 1000;
    // inttime is the total number of number of seconds
    // fractimeis the number of thousandths of a second

    // number of days is total number of seconds divided by 24 divided by 3600
    days     = inttime / (24*3600);
    inttime  = inttime % (24*3600);

    // Now, inttime is the remainder after subtracting the number of seconds
    // in the number of days
    hours    = inttime / 3600;
    inttime  = inttime % 3600;

    // Now, inttime is the remainder after subtracting the number of seconds
    // in the number of days and hours
    mins     = inttime / 60;
    inttime  = inttime % 60;

    // Now inttime is the number of seconds left after subtracting the number
    // in the number of days, hours and minutes. In other words, it is the
    // number of seconds.
    secs = inttime;

    // Don't bother to print days
    sprintf(buffer, "%02d:%02d:%02d.%03d", hours, mins, secs, fractime);
    lcd.print(buffer);
}
