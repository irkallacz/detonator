#include <FiniteStateMachine.h>
#include <TM1637Display.h>
#include <Wire.h>
#include <RTClib.h>
#include <Bounce2.h>

const byte CLK_PIN = 7;
const byte DIO_PIN = 6;

const byte ARMED_PIN = 3;
const byte MOVE_PIN = 9;
const byte TRAP_PIN = 4;
const byte CONTACT_PIN = 5;

const byte BUZZER_PIN = 8;

const word IDLE_WAIT = 5000; //ms
const word CONTACT_WAIT = 1000; //ms
const word ARMED_WAIT = 3000; //ms

const word TIME_WAIT = 1000; //ms

const unsigned long COUTING_WAIT = 150; //sec

const byte IDLE_SYMBOL = SEG_A | SEG_D;
const byte ARMED_SYMBOL = SEG_G;

const DateTime date = DateTime(2016,3,19,20,00,0); //19.03.2016 20:00

TM1637Display display(CLK_PIN, DIO_PIN);
RTC_DS3231 rtc;

Bounce contact = Bounce(); 
Bounce button = Bounce(); 

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

  contact.attach(CONTACT_PIN);
  contact.interval(CONTACT_WAIT);

  button.attach(ARMED_PIN);
  button.interval(ARMED_WAIT);
  
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  //if remainig time is bigger then 24 hours, or if it is pass the date, set the counter to 150 seconds
  DateTime n = rtc.now();
  TimeSpan t = n - date;
  alarm = ((t.days())||(n.secondstime() > date.secondstime())) ? n + TimeSpan(COUTING_WAIT) : date;
}

void loop() {
  //read data from senzors, unless already in explosion
  if (!detonator.isInState(boom)) readSenzors();
  
  //if in idle and nothig in senzors go to armed state
  if ((detonator.isInState(idle))&&(checkSenzors())) detonator.transitionTo(armed);
  
  //if in armed state for 5 seconds go to couting state
  //if in armed state and got something in senzors go back to idle
  if (detonator.isInState(armed)){
    if (detonator.timeInCurrentState() >= IDLE_WAIT) detonator.transitionTo(counting);
    if (!checkSenzors()) detonator.transitionTo(idle);
  }
  
  //if in couting state and time is up, or something in senzors, go to explosion
  if (detonator.isInState(counting)){
    now = rtc.now();    
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
  button.update();
  //senzors[0] = !digitalRead(ARMED_PIN);
  senzors[0] = !button.read();
  senzors[1] = digitalRead(MOVE_PIN);
  senzors[2] = !digitalRead(TRAP_PIN);
  //senzors[3] = !digitalRead(CONTACT_PIN);
  
  contact.update();
  senzors[3] = contact.read();
  
  Wire.beginTransmission(DS3231_ADDRESS);
  senzors[4] = !Wire.endTransmission();
}

void showTime(const DateTime t){
  display.showNumberDec(t.day()*100+t.month(),true);
  delay(TIME_WAIT);
  display.showNumberDec(t.year());
  delay(TIME_WAIT);
  display.setColon(true);
  display.showNumberDec(t.hour()*100+t.minute(),true);
  delay(TIME_WAIT);
  display.setColon(false);
}

void beep(){
  tone(BUZZER_PIN, 2093, 250);
  delay(300);
  noTone(BUZZER_PIN);
}

void countingEnter(){  
  beep();
  showTime(now);
  showTime(date);
  beep();
  pinMode(BUZZER_PIN,INPUT);
}

void armedUpdate(){  
    uint8_t data[] = {0,0,0,0};
    
    unsigned long progress = detonator.timeInCurrentState();
    for (byte i = 0; i<= 3; i++)
      if ((progress / 1000) >= (i+1)) data[i] = ARMED_SYMBOL;
    
    display.setColon(false);
    display.setSegments(data);
}

void idleUpdate(){
    uint8_t data[] = {0,0,0,0};
    
    display.setColon(false);
    
    for(byte i = 0;i < 4; i++) 
      if (!senzors[i]) data[i] = IDLE_SYMBOL;
    
    display.setColon(senzor[4]);  
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
