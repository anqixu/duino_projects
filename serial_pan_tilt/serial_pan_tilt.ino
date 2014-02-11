#include <Servo.h>

#define SILENT

char buffer[1000];
int buffer_i = 0;

Servo panServo;
Servo tiltServo;

int panZeroDegreeMSEC = 1500;
int panFourtyFiveDegreeMSEC = 2000;

int tiltZeroDegreeMSEC = 1373;
int tiltFourtyFiveDegreeMSEC = 1823;

int minMSEC = 650;
int maxMSEC = 2350;

void setup() {
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  
  panServo.attach(9);
  tiltServo.attach(10);
  
  Serial.begin(115200);
  #ifndef SILENT
  Serial.println("Ready for instructions: <pan degrees> <tilt degrees>;");
  #endif
}

void loop() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == ';') {
      buffer[buffer_i] = 0;
      char* p = buffer;
      char* str;
      
      double newPanAngle = 0, newTiltAngle = 0;
      int hasPanAngle = false;
      int hasTiltAngle = false;
      
      while ((str = strtok_r(p, " ", &p)) != NULL) {
        if (!hasPanAngle) {
          newPanAngle = atof(str);
          hasPanAngle = true;
        } else if (!hasTiltAngle) {
          newTiltAngle = atof(str);
          hasTiltAngle = true;
        }
      }
      
      if (hasPanAngle && hasTiltAngle) {
        int newPanMSEC = newPanAngle/45.0*(panFourtyFiveDegreeMSEC - panZeroDegreeMSEC) + panZeroDegreeMSEC;
        int newTiltMSEC = newTiltAngle/45.0*(tiltFourtyFiveDegreeMSEC - tiltZeroDegreeMSEC) + tiltZeroDegreeMSEC;
        
        if (newPanMSEC  < minMSEC) newPanMSEC  = minMSEC;
        if (newPanMSEC  > maxMSEC) newPanMSEC  = maxMSEC;
        if (newTiltMSEC < minMSEC) newTiltMSEC = minMSEC;
        if (newTiltMSEC > maxMSEC) newTiltMSEC = maxMSEC;
        
        panServo.writeMicroseconds(newPanMSEC);
        tiltServo.writeMicroseconds(newTiltMSEC);
        
        #ifndef SILENT
        Serial.print("New pan/tilt: ");
        Serial.print(newPanMSEC, DEC);
        Serial.print(", ");
        Serial.println(newTiltMSEC, DEC);
        #endif
      } else {
        #ifndef SILENT
        Serial.print("Parser failed: ");
        Serial.println(buffer);
        #endif
      }
      
      Serial.println(' ');
      buffer_i = 0;
    } else {
      buffer[buffer_i] = ch;
      buffer_i += 1;
    }
  }
}


