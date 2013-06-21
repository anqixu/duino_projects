/*******************************************************************************
* DO NOT MODIFY - AUTO-GENERATED
*
*
* DISCLAMER:
*
* This project was created within an academic research setting, and thus should
* be considered as EXPERIMENTAL code. There may be bugs and deficiencies in the
* code, so please adjust expectations accordingly. With that said, we are
* intrinsically motivated to ensure its correctness (and often its performance).
* Please use the corresponding web repository tool (e.g. github/bitbucket/etc.)
* to file bugs, suggestions, pull requests; we will do our best to address them
* in a timely manner.
*
*
* SOFTWARE LICENSE AGREEMENT (BSD LICENSE):
*
* Copyright (c) 2013, Anqi Xu
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
*  * Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above
*    copyright notice, this list of conditions and the following
*    disclaimer in the documentation and/or other materials provided
*    with the distribution.
*  * Neither the name of the School of Computer Science, McGill University,
*    nor the names of its contributors may be used to endorse or promote
*    products derived from this software without specific prior written
*    permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/*
This code allows an Arduino-compatible device to provide synchronized trigger
signals to multiple digital cameras. The code currently supports 2 modes:

1-master-(N-1)-slaves: 1 camera is designated as master, and is set to free-run
  mode, where it captures images at a given rate. The flash output of the master
  camera is enabled, and connected to CAM#_DIN_PIN of the Arduino-compatible
  device. This trigger signal is replicated and emitted on all CAM#_DOUT_PINs,
  which should be connected to the trigger inputs of the other (N-1) slave
  cameras. This way, by configuring the slave cameras to be in external
  (hardware) trigger mode, the master camera's exposure timing can be
  synchronized to all the slaves' exposure timings.
  
  The Arduino essentially acts as a OR4 or AND4 gate in this case (see
  implementational detail below for further specifications).
  
  Due to latencies in different parts of the communication pipeline (e.g.
  trigger -> exposure delay onboard each camera; exposure end -> image buffer
  ready delay from the camera drivers; etc), the flash signal from the master
  camera may need to be delayed, so that flash signal i allows frames (i+1)
  of all cameras to be synchronized. Note that this delay parameter is not
  supported by this code; also, this parameter will depend on the exposure
  rate, frame rate, and other settings of the master camera.
  
N-slaves: this code generates trigger signals on CAM#_DOUT_PINs, which will
  allow N slave cameras set in external trigger mode to expose and grab
  frames synchronously. Both the delay and duration (a.k.a. duty cycle) can
  be set programmatically via the Arduino device's Serial interface
  (see baud rate below).

The original design is intended to be deployed on a TinyLily
(http://tiny-circuits.com/), and used to trigger 4 UEye UI-1246LE USB cameras
(http://en.ids-imaging.com/). The TinyLily's pins should be connected to
pins on the unpopulated 2x5 PCB I/O connector of all the UEye cameras,
as follows:

UEye 1 Pin 5 (3.3V)     <--->     TinyLily Pin + (VCC)
UEye 1 Pin 6 (GND)      <--->     TinyLily Pin - (GND)
UEye 1 Pin 3 (DOUT)     <--->     TinyLily Pin a0
UEye 1 Pin 4 (DIN)      <--->     TinyLily Pin 2
UEye 2 Pin 3 (DOUT)     <--->     TinyLily Pin a1
UEye 2 Pin 4 (DIN)      <--->     TinyLily Pin 3
UEye 3 Pin 3 (DOUT)     <--->     TinyLily Pin a4
UEye 3 Pin 4 (DIN)      <--->     TinyLily Pin 15 (MOSI) [pin 4 on 2x3 ICSP pad]
UEye 4 Pin 3 (DOUT)     <--->     TinyLily Pin a5
UEye 4 Pin 4 (DIN)      <--->     TinyLily Pin 16 (MISO) [pin 1 on 2x3 ICSP pad]

Note that the TinyLily's Pin 0 and Pin 1 are connected to the USB adapter's
UART TX/RX lines. If the USB adapter is disconnected, then in theory Pin 0 and
Pin 1 can be used to provide trigger synchronization for a 5th camera.
This requires minor modifications to the code, to duplicate behavior for
CAM5_DIN_PIN and CAM5_DOUT_PIN.

As an implementational detail, it is useful to have the flash output emit
active-high pulses, so that non-activated cameras will not dominate in
the OR-ed combined trigger signal. Also, because the UI-1246LE camera only
supports falling-edge external trigger, the default behavior of
ACTIVE_HIGH_NOT_MASTER_SLAVE is implemented as:

CAM<i>_DOUT_PIN := NOR(CAM1_DIN_PIN, CAM2_DIN_PIN, ...) for i = 1, 2, ...
*/


// Parameters - Begin
#define ENABLE_SERIAL 1
#define SERIAL_BAUD_RATE 57600
#define SERIAL_MAX_CHARS_PER_LINE 1000

#define CAM1_DIN_PIN A0
#define CAM2_DIN_PIN A1
#define CAM3_DIN_PIN A4
#define CAM4_DIN_PIN A5
#define CAM1_DOUT_PIN 2
#define CAM2_DOUT_PIN 3
#define CAM3_DOUT_PIN 15
#define CAM4_DOUT_PIN 16
// Parameters - End


enum {
	ACTIVE_HIGH_NOT_MASTER_SLAVE,
  ACTIVE_HIGH_MASTER_SLAVE,
  ACTIVE_LOW_MANUAL_TRIGGER_SLAVES,
  ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES,
};


int mode = ACTIVE_HIGH_NOT_MASTER_SLAVE;
unsigned long int manualTriggerRateMS = 66; // default 15Hz
unsigned long int manualTriggerDurationMS = 1;
unsigned long int lastManualTriggerActiveMS = millis();
byte serialInput[SERIAL_MAX_CHARS_PER_LINE+1];
int serialInputLength = 0;


void setup() {
  pinMode(CAM1_DIN_PIN, INPUT);
  pinMode(CAM2_DIN_PIN, INPUT);
  pinMode(CAM3_DIN_PIN, INPUT);
  pinMode(CAM4_DIN_PIN, INPUT);
  pinMode(CAM1_DOUT_PIN, OUTPUT);
  pinMode(CAM2_DOUT_PIN, OUTPUT);
  pinMode(CAM3_DOUT_PIN, OUTPUT);
  pinMode(CAM4_DOUT_PIN, OUTPUT);
  
#if ENABLE_SERIAL
  for (unsigned int i = 0; i < SERIAL_MAX_CHARS_PER_LINE+1; i++) { serialInput[i] = 0; }
  Serial.begin(SERIAL_BAUD_RATE);
  printPrompt();
#endif
};


void loop() {
  // Process incoming bytes from serial line
#if ENABLE_SERIAL
  if (Serial.available() > 0) {
    byte incomingByte = Serial.read();
    if (incomingByte == '\n' || incomingByte == '\r' || serialInputLength >= SERIAL_MAX_CHARS_PER_LINE) {
      parseSerialInput();
      if (serialInputLength > 0) {
        printPrompt();
      }
      serialInputLength = 0;
    } else {
      serialInput[serialInputLength++] = incomingByte;
    }
  }
#endif

  // Update digital output pins
  int dout = HIGH;
  unsigned long int nowMS;
  switch (mode) {
  case ACTIVE_LOW_MANUAL_TRIGGER_SLAVES:
  case ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES:
    nowMS = millis();
    
    // Add an integer number of rateMS to lastActiveMS until difference to nowMS is within rateMS
    if (nowMS - lastManualTriggerActiveMS >= manualTriggerRateMS) {
      lastManualTriggerActiveMS += ((unsigned long int) ((nowMS - lastManualTriggerActiveMS)/manualTriggerRateMS))*manualTriggerRateMS;
    }
    
    if (nowMS < lastManualTriggerActiveMS) { // millis() wrap-around; default to active period
      lastManualTriggerActiveMS = nowMS;
      dout = (mode == ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES) ? HIGH : LOW;
    } else if (nowMS - lastManualTriggerActiveMS <= manualTriggerDurationMS) { // active period
      dout = (mode == ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES) ? HIGH : LOW;
    } else { // inactive period
      dout = (mode == ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES) ? LOW : HIGH;
    }
    break;
  case ACTIVE_HIGH_MASTER_SLAVE:
    dout = activeHighCombineDIN();
    break;
  case ACTIVE_HIGH_NOT_MASTER_SLAVE:
  default:
    mode = ACTIVE_HIGH_NOT_MASTER_SLAVE;
    dout = (activeHighCombineDIN() == HIGH) ? LOW : HIGH;
    break;
  }
  
  digitalWrite(CAM1_DOUT_PIN, dout);
  digitalWrite(CAM2_DOUT_PIN, dout);
  digitalWrite(CAM3_DOUT_PIN, dout);
  digitalWrite(CAM4_DOUT_PIN, dout);
};


int activeLowCombineDIN() { // a.k.a. AND(DIN_1, DIN_2, ...)
  if (digitalRead(CAM1_DIN_PIN) == LOW) return LOW;
  if (digitalRead(CAM2_DIN_PIN) == LOW) return LOW;
  if (digitalRead(CAM3_DIN_PIN) == LOW) return LOW;
  if (digitalRead(CAM4_DIN_PIN) == LOW) return LOW;
  return HIGH;
};


int activeHighCombineDIN() { // a.k.a. OR(DIN_1, DIN_2, ...)
  if (digitalRead(CAM1_DIN_PIN) == HIGH) return HIGH;
  if (digitalRead(CAM2_DIN_PIN) == HIGH) return HIGH;
  if (digitalRead(CAM3_DIN_PIN) == HIGH) return HIGH;
  if (digitalRead(CAM4_DIN_PIN) == HIGH) return HIGH;
  return LOW;
};


#if ENABLE_SERIAL
void printPrompt() {
  Serial.println("");
  Serial.println("Status:");
  Serial.print("- Mode: ");
  switch (mode) {
  case ACTIVE_LOW_MANUAL_TRIGGER_SLAVES:
    Serial.println("ACTIVE_LOW_MANUAL_TRIGGER_SLAVES");
    break;
  case ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES:
    Serial.println("ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES");
    break;
  case ACTIVE_HIGH_MASTER_SLAVE:
    Serial.println("ACTIVE_HIGH_MASTER_SLAVE");
    break;
  case ACTIVE_HIGH_NOT_MASTER_SLAVE:
  default:
    mode = ACTIVE_HIGH_NOT_MASTER_SLAVE;
    Serial.println("ACTIVE_HIGH_NOT_MASTER_SLAVE");
    break;
  }
  Serial.print("- Manual Trigger Rate: ");
  Serial.print(manualTriggerRateMS, DEC);
  Serial.println(" ms");
  Serial.print("- Manual Trigger Active Duration: ");
  Serial.print(manualTriggerDurationMS, DEC);
  Serial.println(" ms");
  Serial.println("");
  Serial.println("Commands:");
  Serial.println("- m <0|1|2|3>: update mode");
  Serial.println("- r <RATE>: set time between consecutive manual triggers to <RATE> ms");
  Serial.println("- d <DURATION>: set manual trigger active duration to <DURATION> ms");
  Serial.println("");
  // NOTE: for some unknown reason, adding more printout text here causes the
  //       TinyLily / TinyDuino to misbehave and constantly reset.
};


void parseSerialInput() {
  if (serialInputLength == 0) {
    return;
  } else if (serialInputLength < 3) {
    Serial.println("ERROR: unexpected command syntax");
    return;
  }
  
  if (serialInput[0] == 'm') { // Update mode
    int modeID = parseNum();
    if (modeID < 0 || modeID >= 4) {
      Serial.println("ERROR: invalid ID for mode (m) command\n");
      return;
    } else {
      if (mode != modeID) {
        mode = modeID;
        
        // Reset manual trigger active time, so that manual trigger occurs immediately after
        if (mode == ACTIVE_LOW_MANUAL_TRIGGER_SLAVES || mode == ACTIVE_HIGH_MANUAL_TRIGGER_SLAVES) {
          lastManualTriggerActiveMS = millis() - manualTriggerDurationMS;
        }
      }
    }
  } else if (serialInput[0] == 'r') { // Update manual trigger rate
    int rate = parseNum();
    if (rate <= 0 || rate > 10000) {
      Serial.println("ERROR: invalid value for rate (r) command; value should be within [1, 10000] (unit: ms)\n");
      return;
    } else {
      manualTriggerRateMS = rate;
      if (manualTriggerDurationMS > manualTriggerRateMS) {
        manualTriggerDurationMS = manualTriggerRateMS/2;
      }
    }
  } else if (serialInput[0] == 'd') { // Update manual trigger duration
    int duration = parseNum();
    if (duration < 0 || duration > manualTriggerRateMS) {
      Serial.println("ERROR: invalid value for duration (d) command; value should be within [1, rate] (unit: ms)\n");
      return;
    } else {
      manualTriggerDurationMS = duration;
    }
  } else {
    Serial.println("ERROR: unrecognized command type\n");
  }
};


long int parseNum() {
  long int result = -1;
  int currDigit = 0;
  for (unsigned int i = 2; i < serialInputLength; i++) {
    if (serialInput[i] == ' ' || serialInput[i] == '\n' || serialInput[i] == '\r' || serialInput[i] == '\t' || serialInput[i] == '.') {
      break;
    } else {
      currDigit = serialInput[i] - '0';
      if (currDigit > 9) {
        Serial.println("ERROR: number parser failed");
        return -1;
      } else {
        if (result < 0) { // first digit
          result = currDigit;
        } else { // non-first digit
          result = result * 10 + currDigit;
        }
      }
    }
  }
  return result;
};
#endif

