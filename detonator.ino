#include <FiniteStateMachine.h>
#include <TM1637Display.h>
#include <Wire.h>
#include "RTClib.h"

const byte CLK_PIN = 2;
const byte DIO_PIN = 3;

const byte ARMED_PIN = 8;
const byte MOVE_PIN = 9;
const byte TRAP_PIN = 10;
const byte CONTACT_PIN = 11;

const byte BUZZER_PIN = 5;

const word IDLE_WAIT = 5000;
const unsigned long COUTING_WAIT = 150;

const byte IDLE_SYMBOL = SEG_A | SEG_D;
const byte ARMED_SYMBOL = SEG_G;

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

void idleEnter(){
  digitalWrite(LED_BUILTIN, LOW); 
  display.setBrightness(0x0f); 
}

State idle = State(idleEnter,idleUpdate,nothing);
State armed = State(armedUpdate);
State counting = State(countingEnter,countingUpdate,nothing);

State boom = State(boomEnter,boomUpdate,nothing);

FSM detonator = FSM(idle);

DateTime now;
DateTime alarm;

boolean senzors[5] = {false, false, false, false, false};

void setup() {
  pinMode(ARMED_PIN, INPUT_PULLUP);
  pinMode(TRAP_PIN, INPUT_PULLUP);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  pinMode(MOVE_PIN, INPUT_PULLUP);
  
  pinMode(LED_BUILTIN, OUTPUT);

  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  alarm = DateTime(2016,3,19,19,30,0); //19.3.2016 19:30
  
  //If remainig time is bigger then 24 hour or if it is pass the limit, set the counter to 150 seconds
  DateTime n = rtc.now();
  TimeSpan t = alarm - n;
  if ((t.days())||(n.secondstime() > alarm.secondstime())) alarm = n + TimeSpan(COUTING_WAIT);
}

void loop() {
  now = rtc.now();    
  
  if (!detonator.isInState(boom)) readSenzors();
  
  if ((detonator.isInState(idle))&&(checkSenzors())) detonator.transitionTo(armed);
  
  if (detonator.isInState(armed)){
    if (!checkSenzors()) detonator.transitionTo(idle);
    if (detonator.timeInCurrentState() >= IDLE_WAIT) detonator.transitionTo(counting);
  }
  
  if (detonator.isInState(counting)){
    if ((!checkSenzors())||(now.secondstime() >= alarm.secondstime())) detonator.transitionTo(boom);
  }
  
  detonator.update();
}

boolean checkSenzors(){
  boolean check = true;
  for(byte i=0; i<sizeof(senzors);i++)
    check = check && (senzors[i]);
    
  return check;
}

void readSenzors(){
  senzors[0] = !digitalRead(ARMED_PIN);
  senzors[1] = digitalRead(MOVE_PIN);
  senzors[2] = !digitalRead(TRAP_PIN);
  senzors[3] = !digitalRead(CONTACT_PIN);
  
  Wire.beginTransmission(DS3231_ADDRESS);
  senzors[4] = !Wire.endTransmission();
}

void showTime(){
  display.showNumberDec(now.day()*100+now.month(),true);
  delay(1000);
  display.showNumberDec(now.year());
  delay(1000);
  display.setColon(true);
  display.showNumberDec(now.hour()*100+now.minute(),true);
  delay(1000);
  display.setColon(false);
  
  display.showNumberDec(alarm.day()*100+alarm.month(),true);
  delay(1000);
  display.showNumberDec(alarm.year());
  delay(1000);
  display.setColon(true);
  display.showNumberDec(alarm.hour()*100+alarm.minute(),true);  
  delay(1000);
  display.setColon(false);
}

void beep(){
  tone(BUZZER_PIN, 2093, 250);
  delay(300);
  noTone(BUZZER_PIN);
}

void countingEnter(){  
  beep();
  showTime();
  beep();
  pinMode(BUZZER_PIN,INPUT);
}

void armedUpdate(){  
    uint8_t data[] = {0,0,0,0};
    
    unsigned long progress = detonator.timeInCurrentState();
    for (byte i = 0; i<= 3; i++)
      if ((progress / 1000) >= (i+1)) data[i] = ARMED_SYMBOL;
    
    display.setSegments(data);
}

void idleUpdate(){
    uint8_t data[] = {0,0,0,0};
    
    for(byte i = 0;i<sizeof(senzors);i++) 
      if (!senzors[i]) data[i] = IDLE_SYMBOL;
    
    display.setSegments(data);
}

void countingUpdate(){
  boolean isBlink = ((detonator.timeInCurrentState() / 500) % 2);
  
  digitalWrite(LED_BUILTIN, isBlink);  
  display.setColon(isBlink);
  
  TimeSpan t = alarm - now;

  word digits =  t.hours() ? t.hours()*100+t.minutes() : t.minutes()*100+t.seconds();
  display.showNumberDec(digits,true);
}

void boomEnter(){
  digitalWrite(LED_BUILTIN, HIGH); 
  
  display.setColon(true);
}

void boomUpdate(){
  boolean isBlink = ((detonator.timeInCurrentState() / 500) % 2);

  if (isBlink) {
    display.setBrightness(0x0f);      
    tone(BUZZER_PIN, 2637, 250);
  }else{
    display.setBrightness(0x00);
    noTone(BUZZER_PIN);
  }
  
  display.showNumberDec(0,true);  
}

void nothing(){
}
