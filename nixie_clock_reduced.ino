#include <RealTimeClockDS1307.h>
#include "Wire.h"

RealTimeClockDS1307 rtc;
const byte A = 2, B = 4, C = 7, D = 8, adjustPin = A0, setPin = A1, N[] = {3, 5, 6, 9, 10, 11}, clean[] = {3, 8, 9, 4, 0, 5, 7, 2, 6, 1};
//N[0] = 3, N[1] = 5, N[2] = 6, N[3] = 9, N[4] = 10, N[5] = 11
unsigned long timePressed, timeRefresh, rapidTime, lastCPP;
boolean twelveHour, holding = false, colonOn;
byte tempHour, tempMin, tempSec, mode, curr1, curr2, curr3, curr4, curr5, curr6;
const byte pressTime = 800;

void setup() {
  Wire.begin();
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(C, OUTPUT);
  pinMode(D, OUTPUT);
  for (int a = 0; a < 6; a++)
    pinMode(N[a], OUTPUT);
  pinMode(setPin, INPUT_PULLUP);
  pinMode(adjustPin, INPUT_PULLUP);
  rtc.readClock();
  twelveHour = rtc.readData(0x09);
  colonOn = rtc.readData(0x10);
  colons(colonOn);
  if (twelveHour && rtc.getHours() == 00 && rtc.getMinutes() == 00 && rtc.getSeconds() == 00) {
    rtc.setHours(12);
    rtc.setClock();
  }
  tempSec = rtc.getSeconds();
}

void loop() {
  multPlex();
  checkButton();
  if (millis() - lastCPP >= 600000)
    cathodePoisoningPrevention();
}

void multPlex() {
  if (mode == 0 && millis() - timeRefresh >= 1000) {
    timeRefresh = millis();
    rtc.readClock();
    if (twelveHour)
      if (rtc.getHours() / 10 == 0)
        refresh(-1, 1);
      else
        refresh(1, 1);
    else
      refresh(rtc.getHours() / 10, 1);
    refresh(rtc.getHours() % 10, 2);
    refresh(rtc.getMinutes() / 10, 3);
    refresh(rtc.getMinutes() % 10, 4);
    refresh(rtc.getSeconds() / 10, 5);
    refresh(rtc.getSeconds() % 10, 6);
  }
  else if (mode == 1) {
    if (twelveHour) {
      refresh(1, 1);
      refresh(2, 2);
    }
    else {
      refresh(2, 1);
      refresh(4, 2);
    }
    refresh(-1, 3);
    refresh(-1, 4);
    refresh(-1, 5);
    refresh(-1, 6);
  }
  else if (mode == 2) {
    if (colonOn)
      refresh(1, 1);
    else
      refresh(0, 1);
    refresh(-1, 2);
    refresh(-1, 3);
    refresh(-1, 4);
    refresh(-1, 5);
    refresh(-1, 6);
  }
  else if (mode != 0) {
    if (mode == 3) {
      if (!holding) {
        if (millis() % pressTime < 400) {
          if (twelveHour && tempHour / 10 == 0)
            refresh(-1, 1);
          else
            refresh(tempHour / 10, 1);
          refresh(tempHour % 10, 2);
        }
        else {
          refresh(-1, 1);
          refresh(-1, 2);
        }
      }
      else if (millis() - timePressed >= pressTime) {
        if (twelveHour && tempHour / 10 == 0)
          refresh(-1, 1);
        else
          refresh(tempHour / 10, 1);
        refresh(tempHour % 10, 2);
      }
    }

    else if (mode == 4) {
      if (!holding) {
        if (millis() % pressTime < 400) {
          refresh(tempMin / 10, 3);
          refresh(tempMin % 10, 4);
        }
        else {
          refresh(-1, 3);
          refresh(-1, 4);
        }
      }
      else if (millis() - timePressed >= pressTime) {
        refresh(tempMin / 10, 3);
        refresh(tempMin % 10, 4);
      }
    }

    else if (mode == 5) {
      if (!holding) {
        if (!holding && millis() % pressTime < 400) {
          refresh(tempSec / 10, 5);
          refresh(tempSec % 10, 6);
        }
        else {
          refresh(-1, 5);
          refresh(-1, 6);
        }
      }
      else if (millis() - timePressed >= pressTime) {
        refresh(tempSec / 10, 5);
        refresh(tempSec % 10, 6);
      }
    }
  }
  binOut(curr1, 0);
  binOut(curr2, 1);
  binOut(curr3, 2);
  binOut(curr4, 3);
  binOut(curr5, 4);
  binOut(curr6, 5);
}

void refresh(byte number, byte stage) {
  switch (stage) {
    case 1: curr1 = number; break;
    case 2: curr2 = number; break;
    case 3: curr3 = number; break;
    case 4: curr4 = number; break;
    case 5: curr5 = number; break;
    case 6: curr6 = number; break;
  }
}

void binOut(byte number, byte stage) {
  digitalWrite(stage == 0 ? N[5] : N[stage - 1], LOW);
  digitalWrite(A, HIGH); 
  digitalWrite(B, HIGH);
  digitalWrite(C, HIGH);
  digitalWrite(D, HIGH);
  digitalWrite(N[stage], HIGH);
  if (number == 255) {
    digitalWrite(A, HIGH);
    digitalWrite(B, HIGH);
    digitalWrite(C, HIGH);
    digitalWrite(D, HIGH);
  }
  else {
    digitalWrite(A, bitRead(number, 0)); 
    digitalWrite(B, bitRead(number, 1));
    digitalWrite(C, bitRead(number, 2));
    digitalWrite(D, bitRead(number, 3));
  }
  delay(2);
}

void checkButton() {
  if (!digitalRead(setPin) && !holding) {
    timePressed = millis();
    holding = true;
    setBut();
  }
  else if (!digitalRead(adjustPin) && !holding) {
    timePressed = millis();
    holding = true;
    adjustBut();
  }
  else if (digitalRead(adjustPin) && digitalRead(setPin))
    holding = false;
  else if (!digitalRead(adjustPin) && holding && millis() - timePressed >= pressTime  && millis() - rapidTime >= 75 && mode != 1) {
    adjustBut();
    rapidTime = millis();
  }
}

void setBut() {
  if (mode < 5) {
    mode++;
    if (mode == 5)
      tempSec = 0;
    if (mode == 3) {
      refresh(twelveHour && tempHour / 10 == 0 ? -1 : tempHour / 10, 1);
      refresh(tempHour % 10, 2);
      refresh(tempMin / 10, 3);
      refresh(tempMin % 10, 4);
      refresh(tempSec / 10, 5);
      refresh(tempSec % 10, 6);
    }
    else if (mode == 4) {
      refresh(twelveHour && tempHour / 10 == 0 ? -1 : tempHour / 10, 1);
      refresh(tempHour % 10, 2);
    }
    else if (mode == 5) {
      refresh(tempMin / 10, 3);
      refresh(tempMin % 10, 4);
    }
  }
  else {
    mode = 0;
    if (twelveHour)
      rtc.switchTo12h();
    else
      rtc.switchTo24h();
    colons(colonOn);
    rtc.setHours(tempHour);
    rtc.setMinutes(tempMin);
    rtc.setSeconds(tempSec);
    rtc.setClock();
    rtc.start();
  }
  if (mode == 1) {
    rtc.stop();
    colons(0);
    tempHour = rtc.getHours();
    tempMin = rtc.getMinutes();
    tempSec = rtc.getSeconds();
  }
}

void adjustBut() {
  switch (mode) {
    case 1: twelveHour = !twelveHour;
      rtc.writeData(0x09, twelveHour);
      if (twelveHour) {
        if (tempHour == 0)
          tempHour = 12;
        else if (tempHour >= 13)
          tempHour -= 12;
      }
      break;
    case 2: colonOn = !colonOn;
    rtc.writeData(0x10, colonOn);
      break;
    case 3: if (twelveHour) {
        if (tempHour < 12)
          tempHour++;
        else tempHour = 1;
      }
      else {
        if (tempHour < 23)
          tempHour++;
        else
          tempHour = 0;
      }
      break;
    case 4: if (tempMin < 59) tempMin++; else tempMin = 0; break;
    case 5: if (tempSec < 59) tempSec++; else tempSec = 0; break;
  }
}

void cathodePoisoningPrevention() {
  lastCPP = millis();
  for (byte a = 0; a < 6; a++)
    digitalWrite(N[a], LOW);
  for (byte c = 0; c < 3; c++) {
    digitalWrite(N[2 * c], HIGH);
    digitalWrite(N[2 * c + 1], HIGH);
    for (byte d = 0; d < 3; d++)
      for (byte n = 0; n <= 9; n++)
      {
        digitalWrite(A, bitRead(clean[n], 0));
        digitalWrite(B, bitRead(clean[n], 1));
        digitalWrite(C, bitRead(clean[n], 2));
        digitalWrite(D, bitRead(clean[n], 3));
        delay(150);
      }
    digitalWrite(N[2 * c], LOW);
    digitalWrite(N[2 * c + 1], LOW);
  }
  digitalWrite(A, HIGH);
  digitalWrite(B, HIGH);
  digitalWrite(C, HIGH);
  digitalWrite(D, HIGH);
  for (byte a = 0; a < 6; a++)
    digitalWrite(N[a], LOW);
}

void colons(boolean mode) {
  Wire.beginTransmission(0x68);
  Wire.write(0x0E);
  if (mode == 0)
    Wire.write(B00000100);
  else
    Wire.write(B00000000);

  Wire.endTransmission();
}
