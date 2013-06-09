/*
  TinyDuino 16 LED Random Patterns Demo
  Language: Wiring/Arduino
 
  This program builds on top of Ken Burns' 16 LED Demo, and
  displays various LED patterns on the 16 Edge LED TinyShield
  Board. Multi-LED patterns are attained by quickly cycling
  through individual LEDs, which are controlled via Charlieplexing.
  
 Created 8 Jun 2013
 by Anqi Xu

 This example code is in the public domain.

 http://www.tiny-circuits.com
 */
 

#define LEDs_OFF 0x0
#define LED01 0x0001
#define LED02 0x0002
#define LED03 0x0004
#define LED04 0x0008
#define LED05 0x0010
#define LED06 0x0020
#define LED07 0x0040
#define LED08 0x0080
#define LED09 0x0100
#define LED10 0x0200
#define LED11 0x0400
#define LED12 0x0800
#define LED13 0x1000
#define LED14 0x2000
#define LED15 0x4000
#define LED16 0x8000


int LEDs_pattern = LEDs_OFF;
int LEDs_currPosID = 0;
int prevTime;
int currTime;


void setup() {
  randomSeed(analogRead(0));

  pinMode(5, INPUT);  
  pinMode(6, INPUT);  
  pinMode(7, INPUT);  
  pinMode(8, INPUT);  
  pinMode(9, INPUT);
  
  LEDs_pattern = LEDs_OFF;
  LEDs_currPosID = 0;
  
  Serial.begin(57600);
  
  prevTime = millis();
};


void loop() {
  int prevChangeTime = millis();
  int currChangeTime;
  const int NUM_ANIMATION_TYPES = 6;
  int animationType = random(0, NUM_ANIMATION_TYPES);
  
  while (1) {
    switch (animationType) {
      case 1:
        animate_LEDs_dual_doors(50);
        break;
      case 2:
        animate_LEDs_dual_bars(50);
        break;
      case 3:
        animate_LEDs_dual_claws(50);
        break;
      case 4:
        animate_LEDs_bar(25);
        break;
      case 5:
        animate_LEDs_random_fade_in(25);
        delayWhileSpinningLEDs(25*16);
        animate_LEDs_random_fade_out(25);
        updateLEDsPattern(LEDs_OFF);
        delayWhileSpinningLEDs(25*16);
        break;
      default:
        animate_LEDs_night_rider(25);
        break;
    }
    
    currChangeTime = millis();
    if (currChangeTime - prevChangeTime > 5000 || currChangeTime < prevChangeTime) {
      prevChangeTime = currChangeTime;
      
      //animationType += 1;
      //if (animationType >= NUM_ANIMATION_TYPES) animationType = 0;
      animationType = random(0, NUM_ANIMATION_TYPES);
    }
  }
};


void animate_LEDs_random_fade_in(int delayMSEC) {
  int LEDstate = 0;
  unsigned int pattern = LEDs_OFF;
  unsigned int LEDpermutation[16];
  updateLEDsPattern(LEDs_OFF);
  spinLEDs();
  
  // Randomly permute LED order via Knuth shuffle and XOR swaps
  for (int i = 0; i < 16; i++) { LEDpermutation[i] = (LED01 << i); }
  for (int i = 0; i < 15; i++) {
    int j = random(i, 16);
    if (i != j) {
      int temp = LEDpermutation[i];
      LEDpermutation[i] = LEDpermutation[j];
      LEDpermutation[j] = temp;
    }
  }
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 16) { return; }
      pattern = pattern | LEDpermutation[LEDstate - 1];
      updateLEDsPattern(pattern);
    }
    
    spinLEDs();
  }
};


void animate_LEDs_random_fade_out(int delayMSEC) {
  int LEDstate = 0;
  unsigned int pattern = 0xFFFF;
  unsigned int LEDpermutation[16];
  updateLEDsPattern(0xFFFF);
  spinLEDs();
  
  // Randomly permute LED order via Knuth shuffle and XOR swaps
  for (int i = 0; i < 16; i++) { LEDpermutation[i] = (LED01 << i); }
  for (int i = 0; i < 15; i++) {
    int j = random(i, 16);
    if (i != j) {
      int temp = LEDpermutation[i];
      LEDpermutation[i] = LEDpermutation[j];
      LEDpermutation[j] = temp;
    }
  }
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 16) { return; }
      pattern = pattern & ~LEDpermutation[LEDstate - 1];
      updateLEDsPattern(pattern);
    }
    
    spinLEDs();
  }
};


void animate_LEDs_dual_claws(int delayMSEC) {
  int LEDstate = 0;
  int LEDdualState;
  int pattern;
  int dualPattern;
  updateLEDsPattern(LEDs_OFF);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 16) { return; }

      LEDdualState = LEDstate;
      if (LEDdualState >= 8) LEDdualState = 16 - LEDdualState;
      
      pattern = LED08;
      dualPattern = LED09;
      for (int i = 0; i < LEDdualState; i++) {
        pattern = (pattern >> 1) | LED08;
        dualPattern = (dualPattern << 1) | LED09;
      }
      updateLEDsPattern(pattern | dualPattern);
    }
    
    spinLEDs();
  }
};


void animate_LEDs_dual_doors(int delayMSEC) {
  int LEDstate = 0;
  int LEDdualState;
  int pattern;
  int dualPattern;
  updateLEDsPattern(LEDs_OFF);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 16) { return; }

      LEDdualState = LEDstate;
      if (LEDdualState >= 8) LEDdualState = 16 - LEDdualState;
      
      pattern = LED01;
      dualPattern = LED16;
      for (int i = 0; i < LEDdualState; i++) {
        pattern = (pattern << 1) | LED01;
        dualPattern = (dualPattern >> 1) | LED16;
      }
      updateLEDsPattern(pattern | dualPattern);
    }
    
    spinLEDs();
  }
};


void animate_LEDs_bar(int delayMSEC) {
  int LEDstate = 0;
  int pattern;
  int dualPattern;
  updateLEDsPattern(LEDs_OFF);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate >= 64) { return; }

      if (LEDstate < 16) {
        pattern = LED01;
        for (int i = 0; i < LEDstate; i++) {
          pattern = (pattern << 1) | LED01;
        }
        updateLEDsPattern(pattern);
      } else if (LEDstate < 32) {
        pattern = 0xFFFF;
        for (int i = 0; i < (LEDstate - 16); i++) {
          pattern = (pattern << 1);
        }
        updateLEDsPattern(pattern);
      } else if (LEDstate < 48) {
        pattern = LED16;
        for (int i = 0; i < (LEDstate - 32); i++) {
          pattern = (pattern >> 1) | LED16;
        }
        updateLEDsPattern(pattern);
      } else {
        pattern = 0xFFFF;
        for (int i = 0; i < (LEDstate - 48); i++) {
          pattern = (pattern >> 1) & ~LED16;
        }
        updateLEDsPattern(pattern);
      }
    }
    
    spinLEDs();
  }
};


void animate_LEDs_dual_bars(int delayMSEC) {
  int LEDstate = 0;
  int pattern;
  int dualPattern;
  updateLEDsPattern(LEDs_OFF);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 16) { return; }

      if (LEDstate < 8) {
        pattern = LED01;
        dualPattern = LED16;
        for (int i = 0; i < LEDstate; i++) {
          pattern = (pattern << 1) | LED01;
          dualPattern = (dualPattern >> 1) | LED16;
        }
        updateLEDsPattern(pattern | dualPattern);
      } else {
        pattern = 0x0FF;
        dualPattern = 0xFF00;
        for (int i = 0; i < (LEDstate - 8); i++) {
          pattern = (pattern << 1);
          dualPattern = (dualPattern >> 1) & ~LED16;
        }
        pattern = pattern & 0x00FF;
        dualPattern = dualPattern & 0xFF00;
        updateLEDsPattern(pattern | dualPattern);
      }
    }
    
    spinLEDs();
  }
};


void animate_LEDs_night_rider(int delayMSEC) {
  int LEDstate = 0;
  updateLEDsPattern(LED01 << LEDstate);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      
      LEDstate += 1;
      if (LEDstate > 29) { return; }
        
      if (LEDstate < 16) {
        updateLEDsPattern(LED01 << LEDstate);
      } else {
        updateLEDsPattern(LED01 << (30 - LEDstate));
      }
    }
    
    spinLEDs();
  }
};


void loop_LED_binary_count(int delayMSEC) {
  int LEDstate = 0;
  updateLEDsPattern(LEDstate);
  
  while (1) {
    currTime = millis();
    if (currTime - prevTime >= delayMSEC || currTime < prevTime) {
      prevTime = currTime;
      LEDstate += 1;
      if (LEDstate == 0) return;
      updateLEDsPattern(LEDstate);
    }
    
    spinLEDs();
  }
};


void updateLEDsPattern(int newPattern) {
  LEDs_pattern = newPattern;
  LEDs_currPosID = 0;
};


// Iteratively toggle on one of 16 LEDs depending on requested state
void spinLEDs() {
  if (LEDs_pattern == LEDs_OFF) {
    ledOn(0);
    LEDs_currPosID = 0;
  } else {
    int candPos;
    for (int i = 0; i < 16; i++) {
      candPos = ((LEDs_currPosID + i) % 16);
      if (LEDs_pattern & (LED01 << candPos)) {
        ledOn(candPos + 1);
        LEDs_currPosID = (candPos + 1) % 16;
        break;
      }
    }
  }
  usleep(400);
};


// Busy-loop implementation of micro-second sleep
// NOTE: may not be microsecond-accurate, depending on CPU clock rate
void usleep(unsigned long dt) {
  unsigned long startTime = micros();
  unsigned long nowTime = micros();
  while ((nowTime >= startTime) && (nowTime - startTime < dt)) { nowTime = micros(); }
};


void delayWhileSpinningLEDs(unsigned long dt) {
  unsigned long startTime = millis();
  unsigned long nowTime = millis();
  while ((nowTime >= startTime) && (nowTime - startTime < dt)) {
    spinLEDs();
    nowTime = millis();
  }
};

void ledOn(int iLedNum) {
  int tempNum;
  
  if ((iLedNum >= 1 && iLedNum <= 4) || (iLedNum >= 9 && iLedNum <= 10)) {
    digitalWrite(5, ((iLedNum % 2) == 1) ? HIGH : LOW);
    pinMode(5, OUTPUT);
  } else {
    pinMode(5, INPUT);
  }
  
  if ((iLedNum >= 1 && iLedNum <= 2) || (iLedNum >= 5 && iLedNum <= 8)) {
    tempNum = iLedNum;
    if (tempNum <= 2) tempNum++; // to flip last bit
    digitalWrite(6, ((tempNum % 2) == 1) ? HIGH : LOW);
    pinMode(6, OUTPUT);
  } else {
    pinMode(6, INPUT);
  }
  
  if ((iLedNum >= 3 && iLedNum <= 6) || (iLedNum >= 11 && iLedNum <= 14)) {
    digitalWrite(7, ((iLedNum % 2) == 1) ? LOW : HIGH);
    pinMode(7, OUTPUT);
  } else {
    pinMode(7, INPUT);
  }

  if ((iLedNum >= 7 && iLedNum <= 12) || (iLedNum >= 15 && iLedNum <= 16)) {
    tempNum = iLedNum;
    if (tempNum >= 11 && tempNum <= 12) tempNum++; // to flip last bit
    digitalWrite(8, ((tempNum % 2) == 1) ? LOW : HIGH);
    pinMode(8, OUTPUT);
  } else {
    pinMode(8, INPUT);
  }

  if ((iLedNum >= 13 && iLedNum <= 16)) {
    digitalWrite(9, ((iLedNum % 2) == 1) ? HIGH : LOW);
    pinMode(9, OUTPUT);
  } else {
    pinMode(9, INPUT);
  }
};

