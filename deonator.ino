#include <FiniteStateMachine.h>
#include <TM1637Display.h>
#include <TimeLib.h>

const byte CLK_PIN = 2;
const byte DIO_PIN = 3;

const byte ARMED_NUM = 0;
const byte ARMED_PIN = 8;

const byte TIME_NUM = 1;
const byte TIME_PIN = 9;

const byte TRAP_NUM = 2;
const byte TRAP_PIN = 10;

const byte CONTACT_NUM = 3;  
const byte CONTACT_PIN = 11;

const byte BUZZER_PIN = 5;

const word IDLE_WAIT = 5000;
const unsigned long COUTING_WAIT = 210000;

const byte IDLE_SYMBOL = SEG_A | SEG_D;
const byte ARMED_SYMBOL = SEG_G;

TM1637Display display(CLK_PIN, DIO_PIN);

void idleEnter(){
  digitalWrite(LED_BUILTIN, LOW); 
  display.setBrightness(0x0f); 
}

State idle = State(idleEnter,idleUpdate,nothing);
State armed = State(armedUpdate);
State counting = State(countingEnter,countingUpdate,nothing);

State boom = State(boomEnter,boomUpdate,nothing);

FSM detonator = FSM(idle);

boolean senzors[4] = {false, false, false, false};

unsigned long timeInCurrentState = 0;

void setup() {
  pinMode(ARMED_PIN, INPUT_PULLUP);
  pinMode(TRAP_PIN, INPUT_PULLUP);
  pinMode(CONTACT_PIN, INPUT_PULLUP);
  pinMode(TIME_PIN, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  //timeInCurrentState = detonator.timeInCurrentState();
  
  if (!detonator.isInState(boom)) readSenzors();
  
  if ((detonator.isInState(idle))&&(checkSenzors())) detonator.transitionTo(armed);
  
  if (detonator.isInState(armed)){
    if (!checkSenzors()) detonator.transitionTo(idle);
    if (detonator.timeInCurrentState() >= IDLE_WAIT) detonator.transitionTo(counting);
  }
  
  if (detonator.isInState(counting)){
    if ((!checkSenzors())||(detonator.timeInCurrentState() >= COUTING_WAIT)) detonator.transitionTo(boom);
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
  senzors[TRAP_NUM] = !digitalRead(TRAP_PIN);
  senzors[CONTACT_NUM] = digitalRead(CONTACT_PIN);
  senzors[ARMED_NUM] = !digitalRead(ARMED_PIN);
  senzors[TIME_NUM] = !digitalRead(TIME_PIN);
}

void countingEnter(){  
  tone(BUZZER_PIN, 2093, 250);
  delay(300);
  noTone(BUZZER_PIN);
  pinMode(BUZZER_PIN,INPUT);
}

void armedUpdate(){  
    uint8_t data[] = {0,0,0,0};
    
    unsigned long progress = detonator.timeInCurrentState();
    for (byte i = 0; i<= 3; i++)
      if ((progress / 1000) >= (i+1)) data[i] = SEG_G;
    
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
  
  time_t t = (COUTING_WAIT - detonator.timeInCurrentState()) / 1000;

  word digits =  hour(t) ? hour(t)*100+minute(t) : minute(t)*100+second(t);
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
