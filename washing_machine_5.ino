#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const uint8_t motor = 9;
const uint8_t start_button = 2;
const uint8_t temp_button = 4;
const uint8_t pump = 5;
const uint8_t start_led = 12;
const uint8_t program_dial = A0;
const uint8_t buzzer = 3;
const uint8_t rinse_hold_button = 6;
const uint8_t without_spin_button = 7;
const uint8_t pre_wash_button = 8;
const uint8_t soak_button = 10;

int potValue = 0;
unsigned long lastChange = 0;
int screen = 0;
int tempCase = 0;
bool running = false;

void lcdPrintAt(uint8_t col, uint8_t row, const __FlashStringHelper* text);
void lcdShowLine0Cleared(const __FlashStringHelper* line0);
void lcdShowLinesCleared(const __FlashStringHelper* line0, const __FlashStringHelper* line1);
void primePump3s();
void runStartupMotorPattern3x();
void beepShort6();
void beepSingleLongGap6();
void beepTripleLongGap6();
void pump127For(unsigned long durationMs);
void motorPulseMax100();
void lcdShowPhaseEndCleared();
void lcdShowPhaseDrainCleared();
void lcdShowTimeZeroRow1();
void lcdShowPhaseDrainWithRow1(const __FlashStringHelper* line1);
void lcdShowPhaseFinalSpinWithRow1(const __FlashStringHelper* line1);
void showPreviewInvalidError();
int maxTempCaseForPotValue(int dialValue);

bool isInvalidCombo(bool soak, bool pre, bool rinseHold, bool noSpin) {
  return (soak && pre) || (rinseHold && noSpin);
}

bool showStandardInvalidPreviewError() {
  if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
    showPreviewInvalidError();
    return true;
  }
  noTone(buzzer);
  return false;
}

__attribute__((noinline)) void showInvalidComboError() {
  lcdShowLinesCleared(F("ERROR"), F("Invalid combo"));
  tone(buzzer, 3729);
  running = false;
}

__attribute__((noinline)) void printTotalTimeAtRow1(int totalMinutes) {
  lcd.setCursor(0, 1);
  lcd.print(F("Time: "));
  lcd.print(totalMinutes / 60);
  lcd.print(F(":"));
  if (totalMinutes % 60 < 10) lcd.print(F("0"));
  lcd.print(totalMinutes % 60);
  lcd.print(F("   "));
}

__attribute__((noinline)) void lcdPrintAt(uint8_t col, uint8_t row, const __FlashStringHelper* text) {
  lcd.setCursor(col, row);
  lcd.print(text);
}

__attribute__((noinline)) void lcdShowLine0Cleared(const __FlashStringHelper* line0) {
  lcd.clear();
  lcdPrintAt(0, 0, line0);
}

__attribute__((noinline)) void lcdShowLinesCleared(const __FlashStringHelper* line0, const __FlashStringHelper* line1) {
  lcd.clear();
  lcdPrintAt(0, 0, line0);
  lcdPrintAt(0, 1, line1);
}

__attribute__((noinline)) void primePump3s() {
  digitalWrite(pump, HIGH);
  delay(3000);
  digitalWrite(pump, LOW);
}

__attribute__((noinline)) void runStartupMotorPattern3x() {
  for (int i = 0; i < 3; i++) {
    analogWrite(motor, 110);
    delay(500);
    analogWrite(motor, 82);
    delay(5000);
    motorPulseMax100();
    analogWrite(motor, 0);
    delay(5000);
  }
}

__attribute__((noinline)) void beepShort6() {
  for (int i = 0; i < 6; i++) {
    tone(buzzer, 3729);
    delay(1000);
    noTone(buzzer);
    delay(1000);
  }
}

__attribute__((noinline)) void beepSingleLongGap6() {
  for (int i = 0; i < 6; i++) {
    tone(buzzer, 3729);
    delay(1000);
    noTone(buzzer);
    delay(1000);
    delay(30000);
  }
}

__attribute__((noinline)) void beepTripleLongGap6() {
  for (int i = 0; i < 6; i++) {
    tone(buzzer, 3729);
    delay(1000);
    noTone(buzzer);
    delay(1000);
    tone(buzzer, 3729);
    delay(1000);
    noTone(buzzer);
    delay(1000);
    tone(buzzer, 3729);
    delay(1000);
    noTone(buzzer);
    delay(1000);
    delay(30000);
  }
}

__attribute__((noinline)) void pump127For(unsigned long durationMs) {
  analogWrite(pump, 127);
  delay(durationMs);
  analogWrite(pump, 0);
}

__attribute__((noinline)) void motorPulseMax100() {
  analogWrite(motor, 255);
  delay(100);
}

__attribute__((noinline)) void lcdShowPhaseEndCleared() {
  lcdShowLine0Cleared(F("Phase: End"));
}

__attribute__((noinline)) void lcdShowPhaseDrainCleared() {
  lcdShowLine0Cleared(F("Phase: Drain"));
}

__attribute__((noinline)) void lcdShowTimeZeroRow1() {
  lcdPrintAt(0, 1, F("Time: 0:00    "));
}

__attribute__((noinline)) void lcdShowPhaseDrainWithRow1(const __FlashStringHelper* line1) {
  lcdShowLinesCleared(F("Phase: Drain"), line1);
}

__attribute__((noinline)) void lcdShowPhaseFinalSpinWithRow1(const __FlashStringHelper* line1) {
  lcdShowLinesCleared(F("Phase: Final spin"), line1);
}

__attribute__((noinline)) void showPreviewInvalidError() {
  lcdPrintAt(0, 1, F("ERROR: Invalid  "));
  tone(buzzer, 3729);
}

__attribute__((noinline)) int maxTempCaseForPotValue(int dialValue) {
  if (dialValue < 102) return 1;  // Cotton White
  if (dialValue < 204) return 2;  // Cotton Color
  if (dialValue < 307) return 3;  // Minimum Iron
  if (dialValue < 409) return 0;  // Drain / Spin
  if (dialValue < 511) return 0;  // 15' Quick Wash
  if (dialValue < 614) return 0;  // Starch
  if (dialValue < 717) return 0;  // Automatic
  if (dialValue < 819) return 0;  // 30' Quick Wash
  if (dialValue < 922) return 2;  // Woollens
  return 3;                       // Delicates / Silk
}

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(motor, OUTPUT);
  pinMode(start_button, INPUT_PULLUP);
  pinMode(start_led, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(temp_button, INPUT_PULLUP);
  pinMode(pump, OUTPUT);
  pinMode(rinse_hold_button, INPUT_PULLUP);
  pinMode(without_spin_button, INPUT_PULLUP);
  pinMode(pre_wash_button, INPUT_PULLUP);
  pinMode(soak_button, INPUT_PULLUP);
}

void loop() {
  potValue = analogRead(program_dial);

  // Смяна на екран на 5 сек
  if (millis() - lastChange > 5000) {
    screen++;
    if (screen > 1) screen = 0;
    lastChange = millis();
    lcd.clear();
  }

  int maxTempCase = maxTempCaseForPotValue(potValue);
  if (tempCase > maxTempCase) {
    tempCase = 0;
  }

  // Смяна на температура
  if (digitalRead(temp_button) == LOW) {
    tempCase++;
    if (tempCase > maxTempCase) tempCase = 0;
    delay(300);  // кратък debounce
    lcd.clear();
  }

  if (potValue < 102) {
    bool error = false;
    switch (tempCase) {
      case 0:
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Cotton White   "));

          error = showStandardInvalidPreviewError();

          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:09     "));
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:10     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:24     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:25     "));
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:23     "));
            } else if (digitalRead(soak_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:36     "));
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:45     "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:46     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 1:57     "));
            }
          }
        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 95°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
          }
        }
        break;

      case 1:
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Cotton White   "));

          error = showStandardInvalidPreviewError();

          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:09     "));
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:10     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:24     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:25     "));
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:16     "));
            } else if (digitalRead(soak_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 2:30     "));
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:45     "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:47    "));
            } else {
              lcdPrintAt(0, 1, F("Time: 1:57     "));
            }
          }
        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 75°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
          }
        }
        break;

    }

    if (digitalRead(start_button) == LOW && !running) {
              bool soak = digitalRead(soak_button) == LOW;
              bool pre = digitalRead(pre_wash_button) == LOW;
              bool noSpin = digitalRead(without_spin_button) == LOW;
              bool rinseHold = digitalRead(rinse_hold_button) == LOW;

              if (isInvalidCombo(soak, pre, rinseHold, noSpin)) {
                showInvalidComboError();
                return;
              }
              noTone(buzzer);

              int totalMinutes = 117;

              if (soak) totalMinutes += 33;
              if (pre) totalMinutes += 19;
              if (noSpin) totalMinutes -= 10;
              if (rinseHold) totalMinutes -= 12;

              running = true;
              digitalWrite(start_led, HIGH);

              printTotalTimeAtRow1(totalMinutes);
              // Пуск на помпа за 3 сек
              primePump3s();

              // Симулиране на мотор
              runStartupMotorPattern3x();

              if (soak) {
                lcdShowLine0Cleared(F("Phase: Soak"));

                pump127For(20000);
                motorPulseMax100();
                analogWrite(motor, 70);
                delay(6 * 60000);

                analogWrite(motor, 0);
                delay(30 * 60000);
              }

              if (pre) {
                lcdShowLine0Cleared(F("Phase: Pre-wash"));

                analogWrite(pump, 127);
                delay(20000);
                motorPulseMax100();
                digitalWrite(pump, LOW);
                analogWrite(motor, 80);
                delay(18 * 60000);
                digitalWrite(pump, HIGH);
                analogWrite(motor, 0);
                delay(1 * 60000);
              }

              // ✅ MAIN WASH
              lcdShowLine0Cleared(F("Phase: Main wash"));
              printTotalTimeAtRow1(totalMinutes);

              pump127For(20000);
              motorPulseMax100();
              analogWrite(motor, 70);
              delay(6 * 60000);
              lcdPrintAt(0, 1, F("Time: 1:50      "));

              for (int i = 0; i < 276; i++) {
                motorPulseMax100();
                analogWrite(motor, 85);
                delay(6000);
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 0);
                delay(2000);
              }

              lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:42       "));
              digitalWrite(pump, HIGH);
              motorPulseMax100();
              analogWrite(motor, 70);
              delay(1 * 60000);
              lcdPrintAt(0, 1, F("Time: 0:41       "));

              // Симулиране на мотор - плавно ускорение
              int pwmValue = 70;
              int targetPWM = 166;
              unsigned long lastUpdate = millis();

              while (pwmValue < targetPWM) {
                unsigned long now = millis();
                if (now - lastUpdate >= 1000) {
                  pwmValue++;
                  analogWrite(motor, pwmValue);
                  lastUpdate = now;
                }
              }
              analogWrite(motor, 166);
              delay(2 * 60000);

              lcdPrintAt(0, 1, F("Time: 0:35       "));
              analogWrite(motor, 70);
              analogWrite(pump, 127);
              delay(1 * 60000);
              digitalWrite(pump, LOW);
              delay(11 * 60000);

              lcdPrintAt(0, 1, F("Time: 0:23    "));
              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(1 * 60000);
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 85);
              delay(3000);

              pwmValue = 85;
              targetPWM = 166;
              lastUpdate = millis();

              while (pwmValue < targetPWM) {
                unsigned long now = millis();
                if (now - lastUpdate >= 180) {
                  pwmValue++;
                  analogWrite(motor, pwmValue);
                  lastUpdate = now;
                }
              }
              analogWrite(motor, 166);
              delay(20000);

              lcdPrintAt(0, 1, F("Time: 0:21    "));
              analogWrite(pump, 127);
              analogWrite(motor, 70);
              delay(1 * 60000);
              digitalWrite(pump, LOW);
              delay(10 * 60000);

              if (rinseHold) {
                lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

                analogWrite(motor, 0);
                digitalWrite(pump, LOW);
                running = false;
                return;
              }

              if (noSpin) {
                lcdShowPhaseDrainCleared();

                digitalWrite(pump, HIGH);
                analogWrite(motor, 70);
                delay(30000);
                analogWrite(motor, 0);
                delay(30000);

                lcdShowPhaseEndCleared();

                digitalWrite(pump, LOW);

                beepShort6();

                running = false;
                return;
              } else {
                lcdShowPhaseDrainWithRow1(F("Time: 0:10    "));
                digitalWrite(pump, HIGH);
                analogWrite(motor, 70);
                delay(30000);
                lcdShowPhaseFinalSpinWithRow1(F("Time: 0:10    "));
                delay(7000);
                analogWrite(motor, 90);
                delay(1500);
                analogWrite(motor, 0);
                delay(10000);
                digitalWrite(pump, LOW);
                delay(10000);

                analogWrite(motor, 255);
                digitalWrite(pump, HIGH);
                delay(100);
                lcdPrintAt(0, 1, F("Time: 0:09    "));
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 100);
                delay(2500);
                digitalWrite(pump, LOW);
                analogWrite(motor, 60);
                delay(10000);
                digitalWrite(pump, HIGH);
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 110);
                delay(3500);
                digitalWrite(pump, LOW);
                analogWrite(motor, 60);
                delay(10000);
                digitalWrite(pump, HIGH);
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 110);
                delay(6000);
                analogWrite(motor, 111);
                delay(500);
                analogWrite(motor, 150);
                delay(20000);
                analogWrite(motor, 191);
                delay(150000);
                analogWrite(motor, 212);
                delay(5000);
                analogWrite(motor, 200);
                delay(40000);
                analogWrite(motor, 237);
                delay(4000);
                analogWrite(motor, 225);
                delay(1 * 60000);
                analogWrite(motor, 0);
                delay(5000);
                digitalWrite(pump, LOW);
                digitalWrite(start_led, LOW);
              }

              lcdShowPhaseEndCleared();
              lcdShowTimeZeroRow1();
              beepTripleLongGap6();

              running = false;
            }
    } else if (potValue < 204) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60°C
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Cotton Color   "));

          // --- Проверка за забранени комбинации ---
          error = showStandardInvalidPreviewError();

          // --- Показване на времето ако няма грешка ---
          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:07     "));
            else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:09     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:22     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:24     "));
            else if (digitalRead(pre_wash_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:22     "));
            else if (digitalRead(soak_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:29     "));
            else if (digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:44     "));
            else if (digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:46     "));
            else
              lcdPrintAt(0, 1, F("Time: 1:56     "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 60°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW)
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          else
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
        }
        break;

      case 1:  // 40°C
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Cotton Color   "));

          error = showStandardInvalidPreviewError();

          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:07     "));
            else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:09     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:22     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:24     "));
            else if (digitalRead(pre_wash_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:15     "));
            else if (digitalRead(soak_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:29     "));
            else if (digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:44     "));
            else if (digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:46     "));
            else
              lcdPrintAt(0, 1, F("Time: 1:56     "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 40°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW)
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          else
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
        }
        break;

      case 2:  // 30°C
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Cotton Color   "));

          error = showStandardInvalidPreviewError();

          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:07     "));
            else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:09     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:22     "));
            else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:24     "));
            else if (digitalRead(pre_wash_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:22     "));
            else if (digitalRead(soak_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 2:29     "));
            else if (digitalRead(rinse_hold_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:44     "));
            else if (digitalRead(without_spin_button) == LOW)
              lcdPrintAt(0, 1, F("Time: 1:46     "));
            else
              lcdPrintAt(0, 1, F("Time: 1:56     "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 30°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW)
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          else
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
        }
        break;
    }  // switch tempCase

    // --- Старт бутон ---
    if (digitalRead(start_button) == LOW && !running) {
      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      // Забранени комбинации
      if (isInvalidCombo(soak, pre, rinseHold, noSpin)) {
        showInvalidComboError();
        return;
      }
      noTone(buzzer);

      // --- БАЗОВО ВРЕМЕ ---
      int totalMinutes = 116;  // 1:56

      // --- ДОБАВКИ ---
      if (soak) totalMinutes += 33;
      if (pre) totalMinutes += 19;

      // --- НАМАЛЕНИЯ ---
      if (noSpin) totalMinutes -= 10;
      if (rinseHold) totalMinutes -= 12;
      running = true;
      digitalWrite(start_led, HIGH);
      printTotalTimeAtRow1(totalMinutes);
      // Пуск на помпа за 3 сек
      primePump3s();

      // Симулиране на мотор
      runStartupMotorPattern3x();

      if (soak) {
        lcdShowLine0Cleared(F("Phase: Soak"));

        pump127For(20000);
        motorPulseMax100();
        analogWrite(motor, 70);
        delay(6 * 60000);

        analogWrite(motor, 0);
        delay(30 * 60000);
      }

      if (pre) {
        lcdShowLine0Cleared(F("Phase: Pre-wash"));

        analogWrite(pump, 127);
        delay(20000);
        motorPulseMax100();
        digitalWrite(pump, LOW);
        analogWrite(motor, 80);
        delay(18 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcdShowLine0Cleared(F("Phase: Main wash"));
      printTotalTimeAtRow1(totalMinutes);

      pump127For(20000);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(6 * 60000);
      lcdPrintAt(0, 1, F("Time: 1:50      "));
      for (int i = 0; i < 240; i++) {
        motorPulseMax100();
        analogWrite(motor, 85);
        delay(4000);
        analogWrite(motor, 70);
        delay(4000);
        analogWrite(motor, 0);
        delay(7000);
      }
      lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:50       "));
      digitalWrite(pump, HIGH);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:49       "));
      delay(7000);
      analogWrite(motor, 85);
      delay(3000);

      // Симулиране на мотор - плавно ускорение
      int pwmValue = 85;
      int targetPWM = 110;
      unsigned long lastUpdate = millis();

      while (pwmValue < targetPWM) {
        unsigned long now = millis();
        if (now - lastUpdate >= 900) {
          pwmValue++;
          analogWrite(motor, pwmValue);
          lastUpdate = now;
        }
      }
      analogWrite(motor, 110);
      delay(3 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:44       "));
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(4 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:39       "));
      analogWrite(motor, 70);
      delay(1 * 60000);
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 85);
      delay(3000);
      // Симулиране на мотор - пример с плавно ускорение
      pwmValue = 85;
      targetPWM = 166;
      lastUpdate = millis();

      while (pwmValue < targetPWM) {
        unsigned long now = millis();
        if (now - lastUpdate >= 500) {  // 1 секунда = +1 PWM
          pwmValue++;
          analogWrite(motor, pwmValue);
          lastUpdate = now;
        }
        // Тук можеш да добавиш проверки на други бутони или да обновяваш LCD
      }
      analogWrite(motor, 166);
      delay(2 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:35       "));
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(11 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:23    "));
      digitalWrite(pump, HIGH);
      analogWrite(motor, 70);
      delay(1 * 60000);
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 85);
      delay(3000);
      // Симулиране на мотор - пример с плавно ускорение
      pwmValue = 85;
      targetPWM = 166;
      lastUpdate = millis();

      while (pwmValue < targetPWM) {
        unsigned long now = millis();
        if (now - lastUpdate >= 180) {  // 1 секунда = +1 PWM
          pwmValue++;
          analogWrite(motor, pwmValue);
          lastUpdate = now;
        }
        // Тук можеш да добавиш проверки на други бутони или да обновяваш LCD
      }
      analogWrite(motor, 166);
      delay(20000);
      lcdPrintAt(0, 1, F("Time: 0:21    "));
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(10 * 60000);
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcdShowPhaseDrainCleared();

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        analogWrite(motor, 0);
        delay(30000);

        lcdShowPhaseEndCleared();

        digitalWrite(pump, LOW);

        beepShort6();

        running = false;
        return;
      } else {
        lcdShowPhaseDrainWithRow1(F("Time: 0:10    "));
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcdShowPhaseFinalSpinWithRow1(F("Time: 0:10    "));
        delay(7000);
        analogWrite(motor, 90);
        delay(1500);
        analogWrite(motor, 0);
        delay(10000);
        digitalWrite(pump, LOW);
        delay(10000);
        analogWrite(motor, 255);
        digitalWrite(pump, HIGH);
        delay(100);
        lcdPrintAt(0, 1, F("Time: 0:09    "));
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 100);
        delay(2500);
        digitalWrite(pump, LOW);
        analogWrite(motor, 60);
        delay(10000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 110);
        delay(3500);
        digitalWrite(pump, LOW);
        analogWrite(motor, 60);
        delay(10000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 110);
        delay(6000);
        analogWrite(motor, 111);
        delay(500);
        analogWrite(motor, 150);
        delay(20000);
        analogWrite(motor, 191);
        delay(1000);
        analogWrite(motor, 212);
        delay(5000);
        analogWrite(motor, 200);
        delay(15000);
        analogWrite(motor, 237);
        delay(4000);
        analogWrite(motor, 225);
        delay(210000);
        analogWrite(motor, 0);
        delay(5000);
        digitalWrite(pump, LOW);
        digitalWrite(start_led, LOW);
      }
      lcdShowPhaseEndCleared();
      lcdShowTimeZeroRow1();
      beepTripleLongGap6();
      running = false;
    }
  } else if (potValue < 307) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Minimum iron   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:18     "));
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:19     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:44     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:45    "));
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:23     "));
            } else if (digitalRead(soak_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:49     "));
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:11    "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:12     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 1:16    "));
            }
          }

        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 60°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 900 rpm "));
          }
        }
        break;

          case 1:  // 40°
            if (screen == 0) {
              lcdPrintAt(0, 0, F("Minimum iron   "));

              // --- Проверка за забранени комбинации ---

              // Проверка за забранени комбинации
              if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
                showPreviewInvalidError();  // включваме бъзъра
                error = true;
              } else {
                noTone(buzzer);  // няма грешка → изключваме бъзъра
              }

              // Ако няма грешка, показваме времето
              if (!error) {
                if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:18     "));
                } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:19     "));
                } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:44     "));
                } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:45    "));
                } else if (digitalRead(pre_wash_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:23     "));
                } else if (digitalRead(soak_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:49     "));
                } else if (digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:11    "));
                } else if (digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:12     "));
                } else {
                  lcdPrintAt(0, 1, F("Time: 1:16    "));
                }
              }
            }

                if (screen == 1) {
                  lcdPrintAt(0, 0, F("Temperature: 50°"));
              if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
              } else {
                lcdPrintAt(0, 1, F("Spin: 900 rpm "));
              }
            }
            break;
          case 2:  // 30'c
            if (screen == 0) {
              lcdPrintAt(0, 0, F("Minimum iron   "));

              // --- Проверка за забранени комбинации ---
              error = false;

              // Проверка за забранени комбинации
              if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
                showPreviewInvalidError();  // включваме бъзъра
                error = true;
              } else {
                noTone(buzzer);  // няма грешка → изключваме бъзъра
              }

              // Ако няма грешка, показваме времето
              if (!error) {
                if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:18     "));
                } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:19     "));
                } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:44     "));
                } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:45    "));
                } else if (digitalRead(pre_wash_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:23     "));
                } else if (digitalRead(soak_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:49     "));
                } else if (digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:11    "));
                } else if (digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 1:12     "));
                } else {
                  lcdPrintAt(0, 1, F("Time: 1:16    "));
                }
              }

            } else if (screen == 1) {
              lcdPrintAt(0, 0, F("Temperature: 40°"));
              if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
              } else {
                lcdPrintAt(0, 1, F("Spin: 900 rpm "));
              }
            }
            break;

              case 3:  // 30°C
                if (screen == 0) {
                  lcdPrintAt(0, 0, F("Minimum iron   "));

                  // --- Проверка за забранени комбинации ---
                  error = false;
                  if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
                    showPreviewInvalidError();  // включваме бъзъра
                    error = true;
                  } else {
                    noTone(buzzer);
                  }

                  if (!error) {
                    if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:18     "));
                    } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:19     "));
                    } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:44     "));
                    } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:45    "));
                    } else if (digitalRead(pre_wash_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:23     "));
                    } else if (digitalRead(soak_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:49     "));
                    } else if (digitalRead(rinse_hold_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:11    "));
                    } else if (digitalRead(without_spin_button) == LOW) {
                      lcdPrintAt(0, 1, F("Time: 1:12     "));
                    } else {
                      lcdPrintAt(0, 1, F("Time: 1:16    "));
                    }
                  }
                }

                if (screen == 1) {
                  lcdPrintAt(0, 0, F("Temperature: 30°"));
                  if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                    lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
                  } else {
                    lcdPrintAt(0, 1, F("Spin: 900 rpm "));
                  }
                }
                break;
            }  // switch tempCase

        // --- Старт бутон ---
        if (digitalRead(start_button) == LOW && !running) {
            bool soak = digitalRead(soak_button) == LOW;
            bool pre = digitalRead(pre_wash_button) == LOW;
            bool noSpin = digitalRead(without_spin_button) == LOW;
            bool rinseHold = digitalRead(rinse_hold_button) == LOW;

            // Забранени комбинации
            if (isInvalidCombo(soak, pre, rinseHold, noSpin)) {
              showInvalidComboError();
              return;
            }
            noTone(buzzer);

            // --- БАЗОВО ВРЕМЕ ---
            int totalMinutes = 76;  // 1:16 за 30°C, базово време

            // --- ДОБАВКИ ---
            if (soak) totalMinutes += 33;
            if (pre) totalMinutes += 7;

            // --- НАМАЛЕНИЯ ---
            if (noSpin) totalMinutes -= 4;
            if (rinseHold) totalMinutes -= 5;
            running = true;
            digitalWrite(start_led, HIGH);
            printTotalTimeAtRow1(totalMinutes);

            // Пуск на помпа за 3 сек
            primePump3s();

            // Симулиране на мотор
            runStartupMotorPattern3x();

            if (soak) {
              lcdShowLine0Cleared(F("Phase: Soak"));

              pump127For(20000);
              motorPulseMax100();
              analogWrite(motor, 70);
              delay(6 * 60000);

              analogWrite(motor, 0);
              delay(30 * 60000);
            }

            if (pre) {
              lcdShowLine0Cleared(F("Phase: Pre-wash"));

              analogWrite(pump, 127);
              delay(20000);
              motorPulseMax100();
              digitalWrite(pump, LOW);
              analogWrite(motor, 80);
              delay(6 * 60000);
              digitalWrite(pump, HIGH);
              analogWrite(motor, 0);
              delay(1 * 60000);
            }

            // ✅ MAIN WASH ВИНАГИ
            lcdShowLine0Cleared(F("Phase: Main wash"));
            printTotalTimeAtRow1(totalMinutes);

            pump127For(20000);
            motorPulseMax100();
            analogWrite(motor, 70);
            delay(2 * 60000);
            lcdPrintAt(0, 1, F("Time: 1:13      "));

            for (int i = 0; i < 180; i++) {
              motorPulseMax100();
              analogWrite(motor, 85);
              delay(6000);
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 0);
              delay(2000);
            }

            analogWrite(pump, 127);
            analogWrite(motor, 85);
            delay(1 * 60000);

            lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:30      "));
            digitalWrite(pump, HIGH);
            motorPulseMax100();
            analogWrite(motor, 70);
            delay(1 * 60000);
            lcdPrintAt(0, 1, F("Time: 0:29       "));
            analogWrite(motor, 75);
            delay(7000);
            analogWrite(motor, 90);
            delay(1500);
            analogWrite(motor, 0);
            delay(2000);
            digitalWrite(pump, LOW);
            motorPulseMax100();
            analogWrite(motor, 60);
            delay(10000);
            digitalWrite(pump, HIGH);
            delay(2000);
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 105);
            delay(3500);
            digitalWrite(pump, LOW);
            analogWrite(motor, 60);
            lcdPrintAt(0, 1, F("Time: 0:28       "));
            analogWrite(motor, 70);
            analogWrite(pump, 127);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(7 * 60000);
            lcdPrintAt(0, 1, F("Time: 0:20    "));
            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(1 * 60000);
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 90);
            delay(500);
            digitalWrite(pump, LOW);
            analogWrite(motor, 0);
            delay(10000);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 105);
            delay(3500);
            digitalWrite(pump, LOW);
            analogWrite(motor, 60);
            lcdPrintAt(0, 1, F("Time: 0:18    "));
            analogWrite(pump, 127);
            analogWrite(motor, 70);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(8 * 60000);

            // --- RINSE HOLD ---
            if (rinseHold) {
              lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

              analogWrite(motor, 0);
              digitalWrite(pump, LOW);
              running = false;
              return;
            }

            // --- WITHOUT SPIN ---
            if (noSpin) {
              lcdShowPhaseDrainCleared();

              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(30000);
              analogWrite(motor, 0);
              delay(30000);

              lcdShowPhaseEndCleared();

              digitalWrite(pump, LOW);

              beepShort6();

              running = false;
              return;
            } else {
              lcdShowPhaseDrainWithRow1(F("Time: 0:05    "));
              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(30000);
              lcdShowPhaseFinalSpinWithRow1(F("Time: 0:04    "));
              delay(7000);
              analogWrite(motor, 90);
              delay(1500);
              digitalWrite(pump, LOW);
              analogWrite(motor, 0);
              delay(20000);
              analogWrite(motor, 255);
              digitalWrite(pump, HIGH);
              delay(100);
              lcdPrintAt(0, 1, F("Time: 0:02    "));
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 122);
              delay(3500);
              digitalWrite(pump, HIGH);
              analogWrite(motor, 75);
              delay(7000);
              analogWrite(motor, 110);
              delay(6000);
              analogWrite(motor, 111);
              delay(1000);
              analogWrite(motor, 150);
              delay(40000);
              analogWrite(motor, 0);
              delay(5000);
              digitalWrite(pump, LOW);
              digitalWrite(start_led, LOW);
            }
            lcdShowPhaseEndCleared();
            lcdShowTimeZeroRow1();
            beepTripleLongGap6();
            running = false;
          }
        } else if (potValue < 409) {

    switch (tempCase) {

      case 0:  // Drain / Spin

        // --- Четене на бутони ---
        bool soak = digitalRead(soak_button) == LOW;
        bool pre = digitalRead(pre_wash_button) == LOW;
        bool rinseHold = digitalRead(rinse_hold_button) == LOW;
        bool noSpin = digitalRead(without_spin_button) == LOW;

        bool error = false;

        // --- SCREEN 0 ---
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Drain/Spin     "));

          // Забранени комбинации
          if (soak || pre || rinseHold) {
            lcdPrintAt(0, 1, F("ERROR: Invalid "));
            tone(buzzer, 3729);
            error = true;
          } else {
            noTone(buzzer);
          }

          if (!error) {
            if (noSpin) {
              lcdPrintAt(0, 1, F("Time: 0:01     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 0:08     "));
            }
          }
        }

        // --- SCREEN 1 ---
        else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: Co"));
          if (noSpin) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
          }
        }

        // --- START БУТОН ---
        if (digitalRead(start_button) == LOW && !running) {

          soak = digitalRead(soak_button) == LOW;
          pre = digitalRead(pre_wash_button) == LOW;
          rinseHold = digitalRead(rinse_hold_button) == LOW;
          noSpin = digitalRead(without_spin_button) == LOW;

          if (soak || pre || rinseHold) {
            lcdShowLinesCleared(F("ERROR"), F("Invalid combo"));
            tone(buzzer, 3729);
            return;
          }

          noTone(buzzer);
          running = true;
          digitalWrite(start_led, HIGH);

          if (noSpin) {
            // --- WITHOUT SPIN ---
            lcdShowPhaseDrainCleared();
            motorPulseMax100();
            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(30000);
            digitalWrite(pump, LOW);
            lcdPrintAt(0, 0, F("Phase: End"));
            lcdPrintAt(0, 1, F("Time: 0:00"));

            beepTripleLongGap6();

            digitalWrite(start_led, LOW);
            running = false;

          } else {
            // --- NORMAL SPIN ---
            lcdShowPhaseDrainWithRow1(F("Time: 0:08    "));
            digitalWrite(pump, HIGH);
            motorPulseMax100();
            analogWrite(motor, 70);
            delay(30000);

            lcdShowPhaseFinalSpinWithRow1(F("Time: 0:07    "));
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 110);
            delay(35000);
            lcd.print(F("Time: 0:06    "));
            analogWrite(motor, 150);
            delay(20000);
            analogWrite(motor, 191);
            lcd.print(F("Time: 0:05    "));
            delay(2 * 60000);
            lcd.print(F("Time: 0:04    "));
            analogWrite(motor, 212);
            delay(3000);
            analogWrite(motor, 200);
            lcd.print(F("Time: 0:03   "));
            delay(10000);
            analogWrite(motor, 225);
            delay(170000);
            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
          }

          // --- END PHASE ---
          lcdShowPhaseEndCleared();
          lcdShowTimeZeroRow1();

          beepTripleLongGap6();

          digitalWrite(start_led, LOW);
          running = false;
        }

        break;
    }
  } else if (potValue < 511) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("15' Quick Wash   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW) || (digitalRead(soak_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:22     "));
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:23     "));
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:26    "));
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:11    "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:12     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 0:15   "));
            }
          }

        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: Co"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1200 rpm "));
          }
        }
        break;

        // --- Старт бутон ---
        if (digitalRead(start_button) == LOW && !running) {
          bool soak = digitalRead(soak_button) == LOW;
          bool pre = digitalRead(pre_wash_button) == LOW;
          bool noSpin = digitalRead(without_spin_button) == LOW;
          bool rinseHold = digitalRead(rinse_hold_button) == LOW;

          // Забранени комбинации
          if ((soak && pre) || (rinseHold && noSpin) || (soak)) {
            lcd.clear();
            showInvalidComboError();
            return;
          }
          noTone(buzzer);

          // --- БАЗОВО ВРЕМЕ ---
          int totalMinutes = 15;  // 1:56

          // --- ДОБАВКИ ---
          if (pre) totalMinutes += 11;

          // --- НАМАЛЕНИЯ ---
          if (noSpin) totalMinutes -= 3;
          if (rinseHold) totalMinutes -= 4;
          running = true;
          digitalWrite(start_led, HIGH);
          printTotalTimeAtRow1(totalMinutes);
          // Пуск на помпа за 3 сек
          primePump3s();

          if (pre) {
            lcdShowLine0Cleared(F("Phase: Pre-wash"));

            analogWrite(pump, 127);
            delay(20000);
            motorPulseMax100();
            digitalWrite(pump, LOW);
            analogWrite(motor, 70);
            delay(10 * 60000);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 0);
            delay(1 * 60000);
          }

          // ✅ MAIN WASH ВИНАГИ
          lcdShowLine0Cleared(F("Phase: Main wash"));
          printTotalTimeAtRow1(totalMinutes);

          analogWrite(pump, 127);
          motorPulseMax100();
          analogWrite(motor, 70);
          delay(1 * 60000);
          analogWrite(pump, 0);
          analogWrite(motor, 70);
          delay(4 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:09      "));
          lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:09      "));
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:08       "));
          analogWrite(motor, 70);
          analogWrite(pump, 127);
          delay(1 * 60000);
          digitalWrite(pump, LOW);
          analogWrite(motor, 70);
          delay(4 * 60000);
          // --- RINSE HOLD ---
          if (rinseHold) {
            lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
            running = false;
            return;
          }

          // --- WITHOUT SPIN ---
          if (noSpin) {
            lcdShowPhaseDrainCleared();

            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(1 * 60000);
            analogWrite(motor, 0);
            delay(30000);

            lcdShowPhaseEndCleared();

            digitalWrite(pump, LOW);

            beepShort6();

            running = false;
            return;
          } else {
            lcdShowPhaseFinalSpinWithRow1(F("                "));  // 16 интервала
            lcdPrintAt(0, 1, F("Time: 0:03"));
            analogWrite(motor, 0);
            digitalWrite(pump, HIGH);
            delay(1 * 60000);
            motorPulseMax100();
            analogWrite(motor, 70);
            delay(7000);
            lcdPrintAt(0, 1, F("Time: 0:01    "));
            analogWrite(motor, 200);
            delay(2 * 60000);
            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
            digitalWrite(start_led, LOW);
          }
          lcdShowPhaseEndCleared();
          lcdShowTimeZeroRow1();
          beepTripleLongGap6();
          running = false;
        }
    }
  } else if (potValue < 614) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Starch   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if (digitalRead(pre_wash_button) == LOW || digitalRead(soak_button) == LOW || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {

            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:11    "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:12     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:19   "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 30°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
          }
        }
        break;

        // --- Проверка за забранени комбинации ---
        bool error = false;
    }

    // --- Старт бутон ---
    if (digitalRead(start_button) == LOW && !running) {
      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      // Забранени комбинации
      if ((soak) || (pre) || (rinseHold && noSpin)) {
        lcd.clear();
        showInvalidComboError();
        return;
      }
      noTone(buzzer);

      // --- БАЗОВО ВРЕМЕ ---
      int totalMinutes = 19;  // 1:56

      // --- ДОБАВКИ ---

      // --- НАМАЛЕНИЯ ---
      if (noSpin) totalMinutes -= 7;
      if (rinseHold) totalMinutes -= 8;
      running = true;
      digitalWrite(start_led, HIGH);
      printTotalTimeAtRow1(totalMinutes);
      // Пуск на помпа за 3 сек
      primePump3s();


      // ✅ MAIN WASH ВИНАГИ
      lcdShowLine0Cleared(F("Phase: Soak"));

      analogWrite(pump, 127);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(10 * 60000);
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcdShowPhaseDrainCleared();

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        analogWrite(motor, 0);
        delay(30000);

        lcdShowPhaseEndCleared();

        digitalWrite(pump, LOW);

        beepShort6();

        running = false;
        return;
      } else {
        lcdShowPhaseDrainWithRow1(F("Time: 0:08"));
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcdShowPhaseFinalSpinWithRow1(F("                "));  // 16 интервала
        lcdPrintAt(0, 1, F("Time: 0:07"));
        analogWrite(motor, 75);
        delay(7000);
        analogWrite(motor, 110);
        delay(33000);
        analogWrite(motor, 111);
        delay(500);
        analogWrite(motor, 150);
        delay(19000);
        analogWrite(motor, 191);
        delay(140000);
        analogWrite(motor, 212);
        delay(5000);
        analogWrite(motor, 200);
        delay(30000);
        analogWrite(motor, 237);
        delay(5000);
        analogWrite(motor, 225);
        delay(70000);
        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        digitalWrite(start_led, LOW);
      }
      lcdShowPhaseEndCleared();
      lcdShowTimeZeroRow1();
      beepTripleLongGap6();
      running = false;
    }
  } else if (potValue < 717) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Automatic   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if (digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {

            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:09     "));
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:10     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:35     "));
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:36    "));
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:16     "));
            } else if (digitalRead(soak_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:42     "));
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:02    "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 1:03     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 1:09    "));
            }
          }

        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 40°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 900 rpm "));
          }
        }
        break;

        // --- Старт бутон ---
        if (digitalRead(start_button) == LOW && !running) {
          bool soak = digitalRead(soak_button) == LOW;
          bool pre = digitalRead(pre_wash_button) == LOW;
          bool noSpin = digitalRead(without_spin_button) == LOW;
          bool rinseHold = digitalRead(rinse_hold_button) == LOW;

          // Забранени комбинации
          if (isInvalidCombo(soak, pre, rinseHold, noSpin)) {
            showInvalidComboError();
            return;
          }
          noTone(buzzer);
          noTone(buzzer);

          // --- БАЗОВО ВРЕМЕ ---
          int totalMinutes = 69;  // 1:56

          // --- ДОБАВКИ ---
          if (soak) totalMinutes += 33;
          if (pre) totalMinutes += 7;

          // --- НАМАЛЕНИЯ ---
          if (noSpin) totalMinutes -= 6;
          if (rinseHold) totalMinutes -= 7;


          running = true;
          digitalWrite(start_led, HIGH);
          printTotalTimeAtRow1(totalMinutes);
          // Пуск на помпа за 3 сек
          primePump3s();

          // Симулиране на мотор
          runStartupMotorPattern3x();
          if (soak) {
            lcdShowLine0Cleared(F("Phase: Soak"));

            pump127For(20000);
            motorPulseMax100();
            analogWrite(motor, 70);
            delay(6 * 60000);

            analogWrite(motor, 0);
            delay(30 * 60000);
          }

          if (pre) {
            lcdShowLine0Cleared(F("Phase: Pre-wash"));

            analogWrite(pump, 127);
            delay(20000);
            motorPulseMax100();
            digitalWrite(pump, LOW);
            analogWrite(motor, 80);
            delay(6 * 60000);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 0);
            delay(1 * 60000);
          }

          // ✅ MAIN WASH ВИНАГИ
          lcdShowLine0Cleared(F("Phase: Main wash"));
          printTotalTimeAtRow1(totalMinutes);

          pump127For(20000);
          motorPulseMax100();
          analogWrite(motor, 70);
          delay(2 * 60000);
          lcdPrintAt(0, 1, F("Time: 1:06     "));
          for (int i = 0; i < 168; i++) {
            motorPulseMax100();
            analogWrite(motor, 85);
            delay(6000);
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 0);
            delay(2000);
          }
          lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:25      "));
          digitalWrite(pump, HIGH);
          motorPulseMax100();
          analogWrite(motor, 70);
          delay(1 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:24      "));
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 90);
          delay(1500);
          analogWrite(motor, 0);
          delay(2000);
          digitalWrite(pump, HIGH);
          motorPulseMax100();
          analogWrite(motor, 60);
          delay(10000);
          digitalWrite(pump, HIGH);
          delay(2000);
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 105);
          delay(3500);
          digitalWrite(pump, LOW);
          analogWrite(motor, 60);
          delay(6000);
          lcdPrintAt(0, 1, F("Time: 0:23     "));
          analogWrite(motor, 70);
          analogWrite(pump, 127);
          delay(1 * 60000);
          digitalWrite(pump, LOW);
          delay(7 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:15    "));
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          analogWrite(motor, 75);
          delay(7000);
          analogWrite(motor, 95);
          delay(2500);
          digitalWrite(pump, LOW);
          analogWrite(motor, 60);
          delay(10000);
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 105);
          delay(3500);
          digitalWrite(pump, LOW);
          analogWrite(motor, 60);
          delay(10000);
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 85);
          delay(3000);
          // Симулиране на мотор - пример с плавно ускорение
          int pwmValue = 85;
          int targetPWM = 150;
          unsigned long lastUpdate = millis();

          while (pwmValue < targetPWM) {
            unsigned long now = millis();
            if (now - lastUpdate >= 1000) {  // 1 секунда = +1 PWM
              pwmValue++;
              analogWrite(motor, pwmValue);
              lastUpdate = now;
            }
            // Тук можеш да добавиш проверки на други бутони или да обновяваш LCD
          }
          analogWrite(motor, 150);
          delay(2 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:11    "));
          analogWrite(pump, 127);
          analogWrite(motor, 70);
          delay(1 * 60000);
          digitalWrite(pump, LOW);
          delay(10 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:05    "));
          // --- RINSE HOLD ---
          if (rinseHold) {
            lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
            running = false;
            return;
          }

          // --- WITHOUT SPIN ---
          if (noSpin) {
            lcdShowPhaseDrainCleared();

            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(30000);
            analogWrite(motor, 0);
            delay(30000);

            lcdShowPhaseEndCleared();

            digitalWrite(pump, LOW);

            beepShort6();

            running = false;
            return;
          } else {
            lcdShowPhaseDrainWithRow1(F("Time: 0:05    "));
            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(30000);
            lcdShowPhaseFinalSpinWithRow1(F("Time: 0:05    "));
            delay(7000);
            analogWrite(motor, 100);
            delay(2500);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 60);
            delay(20000);
            analogWrite(motor, 255);
            digitalWrite(pump, HIGH);
            delay(100);
            lcdPrintAt(0, 1, F("Time: 0:04    "));
            analogWrite(motor, 75);
            delay(7000);
            analogWrite(motor, 122);
            delay(3500);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 75);
            delay(7000);
            analogWrite(motor, 110);
            delay(6000);
            analogWrite(motor, 111);
            delay(1000);
            analogWrite(motor, 150);
            delay(2 * 60000);
            analogWrite(motor, 0);
            delay(5000);
            digitalWrite(pump, LOW);
            digitalWrite(start_led, LOW);
          }
          lcdShowPhaseEndCleared();
          lcdShowTimeZeroRow1();
          beepTripleLongGap6();
          running = false;
        }
    }
  } else if (potValue < 819) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("30' Quick Wash   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(pre_wash_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
            if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:22    "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:23     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 0:30   "));
            }
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 40°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1300 rpm "));
          }
        }
        break;

        // --- Проверка за забранени комбинации ---
        bool error = false;
    }

    // --- Старт бутон ---
    if (digitalRead(start_button) == LOW && !running) {
      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      // Забранени комбинации
      if ((rinseHold && noSpin) || (soak) || (pre)) {
        lcd.clear();
        showInvalidComboError();
        return;
      }
      noTone(buzzer);

      // --- БАЗОВО ВРЕМЕ ---
      int totalMinutes = 30;  // 1:56

      // --- НАМАЛЕНИЯ ---
      if (noSpin) totalMinutes -= 7;
      if (rinseHold) totalMinutes -= 8;
      running = true;
      digitalWrite(start_led, HIGH);
      printTotalTimeAtRow1(totalMinutes);
      // Пуск на помпа за 3 сек
      primePump3s();

      // ✅ MAIN WASH ВИНАГИ
      lcdShowLine0Cleared(F("Phase: Main wash"));
      printTotalTimeAtRow1(totalMinutes);

      pump127For(20000);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(6 * 60000);
      for (int i = 0; i < 40; i++) {
        motorPulseMax100();
        analogWrite(motor, 85);
        delay(6000);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 0);
        delay(2000);
      }
      lcdPrintAt(0, 1, F("Time: 0:14      "));
      lcdShowLinesCleared(F("Phase: Rinses "), F("Time: 0:14      "));
      digitalWrite(pump, HIGH);
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:13      "));
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 75);
      delay(2000);
      digitalWrite(pump, LOW);
      analogWrite(motor, 0);
      delay(30000);
      analogWrite(motor, 255);
      digitalWrite(pump, HIGH);
      delay(100);
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 85);
      delay(500);
      analogWrite(motor, 0);
      delay(30000);
      lcdPrintAt(0, 1, F("Time: 0:12     "));
      analogWrite(motor, 255);
      analogWrite(pump, 127);
      delay(100);
      analogWrite(motor, 70);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(3 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:08      "));
      digitalWrite(pump, HIGH);
      delay(1 * 60000);
      delay(7000);
      analogWrite(motor, 100);
      delay(2500);
      analogWrite(motor, 0);
      delay(25000);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 100);
      delay(2500);
      digitalWrite(pump, LOW);
      analogWrite(motor, 60);
      delay(10000);
      lcdPrintAt(0, 1, F("Time: 0:06      "));
      analogWrite(motor, 80);
      delay(4 * 60000);
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcdShowPhaseDrainCleared();

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        analogWrite(motor, 0);
        delay(30000);

        lcdShowPhaseEndCleared();

        digitalWrite(pump, LOW);

        beepShort6();

        running = false;
        return;
      } else {
        lcdPrintAt(0, 0, F("Phase: Drain     "));
        lcdPrintAt(0, 1, F("Time: 0:07     "));
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        lcdPrintAt(0, 0, F("Phase: Finalspin     "));
        lcdPrintAt(0, 1, F("Time: 0:06      "));
        analogWrite(motor, 75);
        delay(7000);
        lcdPrintAt(0, 1, F("Time: 0:06    "));
        analogWrite(motor, 200);
        analogWrite(motor, 0);
        delay(5000);
        analogWrite(motor, 75);
        delay(7000);
        analogWrite(motor, 110);
        delay(6000);
        analogWrite(motor, 150);
        delay(7000);
        analogWrite(motor, 200);
        delay(3 * 60000);
        analogWrite(motor, 225);
        delay(2 * 60000);
        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        digitalWrite(start_led, LOW);
      }
      lcdShowPhaseEndCleared();
      lcdShowTimeZeroRow1();
      beepTripleLongGap6();
      running = false;
    }
  } else if (potValue < 922) {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Woollens 🧼   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:36     "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:37     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:40     "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 40°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1200 rpm "));
          }
        }
        break;

      case 1:  // 40°
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Woollens 🧼   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
            if (digitalRead(rinse_hold_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:36     "));
            } else if (digitalRead(without_spin_button) == LOW) {
              lcdPrintAt(0, 1, F("Time: 0:37     "));
            } else {
              lcdPrintAt(0, 1, F("Time: 0:40     "));
            }
          }

        } else if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 30°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 1200 rpm "));
          }
        }
        break;
      case 2:  // 30'c
            if (screen == 0) {
              lcdPrintAt(0, 0, F("Woollens 🧼   "));

              // --- Проверка за забранени комбинации ---


              // Проверка за забранени комбинации
              if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
                showPreviewInvalidError();  // включваме бъзъра
                error = true;
              } else {
                noTone(buzzer);  // няма грешка → изключваме бъзъра
              }

              // Ако няма грешка, показваме времето
              if (!error) {
                if (digitalRead(rinse_hold_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 0:36     "));
                } else if (digitalRead(without_spin_button) == LOW) {
                  lcdPrintAt(0, 1, F("Time: 0:37     "));
                } else {
                  lcdPrintAt(0, 1, F("Time: 0:40     "));
                }
              }

            } else if (screen == 1) {
              lcdPrintAt(0, 0, F("Temperature: Co"));
              if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
              } else {
                lcdPrintAt(0, 1, F("Spin: 1200 rpm "));
              }
            }
            break;

            // --- Старт бутон ---
            if (digitalRead(start_button) == LOW && !running) {
              bool soak = digitalRead(soak_button) == LOW;
              bool pre = digitalRead(pre_wash_button) == LOW;
              bool noSpin = digitalRead(without_spin_button) == LOW;
              bool rinseHold = digitalRead(rinse_hold_button) == LOW;

              // Забранени комбинации
              if ((soak) || (pre) || (rinseHold && noSpin)) {
                lcd.clear();
                showInvalidComboError();
                return;
              }
              noTone(buzzer);

              // --- БАЗОВО ВРЕМЕ ---
              int totalMinutes = 40;  // 1:56
              // --- НАМАЛЕНИЯ ---
              if (noSpin) totalMinutes -= 3;
              if (rinseHold) totalMinutes -= 4;
              running = true;
              digitalWrite(start_led, HIGH);
              printTotalTimeAtRow1(totalMinutes);
              // Пуск на помпа за 3 сек
              primePump3s();

              // ✅ MAIN WASH ВИНАГИ
              lcdShowLine0Cleared(F("Phase: Main wash"));
              printTotalTimeAtRow1(totalMinutes);

              pump127For(20000);
              motorPulseMax100();
              analogWrite(motor, 60);
              delay(20 * 60000);
              lcdShowLinesCleared(F("Phase: Rinses"), F("Time: 0:20     "));
              analogWrite(motor, 0);
              digitalWrite(pump, HIGH);
              delay(1 * 60000);
              motorPulseMax100();
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 110);
              delay(1 * 60000);
              lcdPrintAt(0, 1, F("Time: 0:18      "));
              analogWrite(pump, 127);
              analogWrite(motor, 60);
              delay(1 * 60000);
              digitalWrite(pump, LOW);
              delay(5 * 60000);
              lcdPrintAt(0, 1, F("Time: 0:12       "));
              analogWrite(motor, 0);
              digitalWrite(pump, HIGH);
              delay(1 * 60000);
              motorPulseMax100();
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 110);
              delay(1 * 60000);
              lcdPrintAt(0, 1, F("Time: 0:10       "));
              analogWrite(pump, 127);
              analogWrite(motor, 60);
              delay(1 * 60000);
              digitalWrite(pump, LOW);
              delay(5 * 60000);
              // --- RINSE HOLD ---
              if (rinseHold) {
                lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

                analogWrite(motor, 0);
                digitalWrite(pump, LOW);
                running = false;
                return;
              }

              // --- WITHOUT SPIN ---
              if (noSpin) {
                lcdShowPhaseDrainCleared();

                digitalWrite(pump, HIGH);
                analogWrite(motor, 70);
                delay(30000);
                analogWrite(motor, 0);
                delay(30000);

                lcdShowPhaseEndCleared();

                digitalWrite(pump, LOW);

                beepShort6();

                running = false;
                return;
              } else {
                lcdShowPhaseDrainWithRow1(F("Time: 0:04   "));
                digitalWrite(pump, HIGH);
                analogWrite(motor, 0);
                delay(1 * 60000);
                lcdShowPhaseFinalSpinWithRow1(F("Time: 0:03    "));
                motorPulseMax100();
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 110);
                delay(30000);
                analogWrite(motor, 200);
                delay(2 * 60000);
                analogWrite(motor, 0);
                digitalWrite(pump, LOW);
                digitalWrite(start_led, LOW);
              }
              lcdShowPhaseEndCleared();
              lcdShowTimeZeroRow1();
              beepTripleLongGap6();
              running = false;
            }
        }
    } else {
    // --- Показване на програма според tempCase и бутоните ---
    bool error = false;
    switch (tempCase) {
      case 0:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Delicates/Silk 🧼   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:34     "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:36     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:37    "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 30°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 400 rpm "));
          }
        }

        if (digitalRead(start_button) == LOW && !running) {
          bool soak = digitalRead(soak_button) == LOW;
          bool pre = digitalRead(pre_wash_button) == LOW;
          bool noSpin = digitalRead(without_spin_button) == LOW;
          bool rinseHold = digitalRead(rinse_hold_button) == LOW;

          // Забранени комбинации
          if ((soak) || (pre) || (rinseHold && noSpin)) {
            lcd.clear();
            showInvalidComboError();
            return;
          }
          noTone(buzzer);

          // --- БАЗОВО ВРЕМЕ ---
          int totalMinutes = 37;  // 1:56
          // --- НАМАЛЕНИЯ ---
          if (noSpin) totalMinutes -= 1;
          if (rinseHold) totalMinutes -= 3;
          running = true;
          digitalWrite(start_led, HIGH);
          printTotalTimeAtRow1(totalMinutes);
          // Пуск на помпа за 3 сек
          primePump3s();

          // ✅ MAIN WASH ВИНАГИ
          lcdShowLine0Cleared(F("Phase: Main wash"));
          printTotalTimeAtRow1(totalMinutes);

          pump127For(20000);
          motorPulseMax100();
          analogWrite(motor, 50);
          delay(21 * 60000);
          lcdShowLinesCleared(F("Phase: Rinses"), F("Time: 0:16     "));
          analogWrite(motor, 0);
          digitalWrite(pump, HIGH);
          delay(90000);
          lcdPrintAt(0, 1, F("Time: 0:18      "));
          analogWrite(pump, 127);
          motorPulseMax100();
          analogWrite(pump, 127);
          analogWrite(motor, 50);
          delay(1 * 60000);
          digitalWrite(pump, LOW);
          delay(4 * 60000);
          lcdPrintAt(0, 1, F("Time: 0:11       "));
          analogWrite(motor, 0);
          digitalWrite(pump, HIGH);
          delay(90000);
          lcdPrintAt(0, 1, F("Time: 0:09       "));
          analogWrite(pump, 127);
          motorPulseMax100();
          analogWrite(pump, 127);
          analogWrite(motor, 50);
          delay(1 * 60000);
          digitalWrite(pump, LOW);
          delay(3 * 60000);
          // --- RINSE HOLD ---
          if (rinseHold) {
            lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
            running = false;
            return;
          }

          // --- WITHOUT SPIN ---
          if (noSpin) {
            lcdShowPhaseDrainCleared();

            digitalWrite(pump, HIGH);
            analogWrite(motor, 70);
            delay(30000);
            analogWrite(motor, 0);
            delay(30000);

            lcdShowPhaseEndCleared();

            digitalWrite(pump, LOW);

            beepShort6();

            running = false;
            return;
          } else {
            lcdShowPhaseDrainWithRow1(F("Time: 0:03   "));
            digitalWrite(pump, HIGH);
            analogWrite(motor, 0);
            delay(1 * 60000);
            lcdShowPhaseFinalSpinWithRow1(F("Time: 0:02    "));
            motorPulseMax100();
            analogWrite(motor, 75);
            delay(7000);
            analogWrite(motor, 90);
            delay(1 * 60000);
            analogWrite(motor, 0);
            digitalWrite(pump, LOW);
            digitalWrite(start_led, LOW);
          }
          lcdShowPhaseEndCleared();
          lcdShowTimeZeroRow1();
          beepTripleLongGap6();
          running = false;
        }
        break;

      case 1:  // 60'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Delicateseasycare   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:02     "));
          } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:01     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:26     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:27     "));
          } else if (digitalRead(pre_wash_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:06     "));
          } else if (digitalRead(soak_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:32    "));
          } else if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:53     "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:54     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:59    "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 40°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 600 rpm "));
          }
        }
        break;

      case 2:  // 40°
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Delicateseasycare   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:02     "));
          } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:01     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:26     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:27     "));
          } else if (digitalRead(pre_wash_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:06     "));
          } else if (digitalRead(soak_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:32    "));
          } else if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:53     "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:54     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:59    "));
          }
        }

        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: 30°"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 600 rpm "));
          }
        }
        break;
      case 3:  // 30'c
        if (screen == 0) {
          lcdPrintAt(0, 0, F("Delicateseasycare   "));

          // --- Проверка за забранени комбинации ---


          // Проверка за забранени комбинации
          if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
            showPreviewInvalidError();  // включваме бъзъра
            error = true;
          } else {
            noTone(buzzer);  // няма грешка → изключваме бъзъра
          }

          // Ако няма грешка, показваме времето
          if (!error) {
          }
          if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:02     "));
          } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:01     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:26     "));
          } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:27     "));
          } else if (digitalRead(pre_wash_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:06     "));
          } else if (digitalRead(soak_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 1:32    "));
          } else if (digitalRead(rinse_hold_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:53     "));
          } else if (digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Time: 0:54     "));
          } else {
            lcdPrintAt(0, 1, F("Time: 0:59    "));
          }
        }
        if (screen == 1) {
          lcdPrintAt(0, 0, F("Temperature: Co"));
          if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
            lcdPrintAt(0, 1, F("Spin: 0 rpm    "));
          } else {
            lcdPrintAt(0, 1, F("Spin: 600 rpm "));
          }
        }
        break;
    }

    // --- Старт бутон ---
    if (digitalRead(start_button) == LOW && !running) {
      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      // Забранени комбинации
      if (isInvalidCombo(soak, pre, rinseHold, noSpin)) {
        showInvalidComboError();
        return;
      }
      noTone(buzzer);

      // --- БАЗОВО ВРЕМЕ ---
      int totalMinutes = 59;  // 1:56
      // --- НАМАЛЕНИЯ ---
      if (soak) totalMinutes += 33;
      if (pre) totalMinutes += 7;
      if (noSpin) totalMinutes -= 5;
      if (rinseHold) totalMinutes -= 6;
      running = true;
      digitalWrite(start_led, HIGH);
      printTotalTimeAtRow1(totalMinutes);
      // Пуск на помпа за 3 сек
      primePump3s();

      if (soak) {
        lcdShowLine0Cleared(F("Phase: Soak"));

        pump127For(20000);
        motorPulseMax100();
        analogWrite(motor, 70);
        delay(6 * 60000);

        analogWrite(motor, 0);
        delay(30 * 60000);
      }

      if (pre) {
        lcdShowLine0Cleared(F("Phase: Pre-wash"));

        analogWrite(pump, 127);
        delay(20000);
        motorPulseMax100();
        digitalWrite(pump, LOW);
        analogWrite(motor, 80);
        delay(6 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcdShowLine0Cleared(F("Phase: Main wash"));
      printTotalTimeAtRow1(totalMinutes);

      pump127For(30000);
      motorPulseMax100();
      analogWrite(motor, 70);
      delay(35 * 60000);
      lcdShowLinesCleared(F("Phase: Rinses"), F("Time: 0:24     "));
      analogWrite(motor, 70);
      digitalWrite(pump, HIGH);
      delay(1 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:23      "));
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(2 * 60000);
      digitalWrite(pump, LOW);
      delay(3 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:18       "));
      analogWrite(motor, 70);
      digitalWrite(pump, HIGH);
      delay(1 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:17      "));
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(2 * 60000);
      digitalWrite(pump, LOW);
      delay(3 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:12       "));
      analogWrite(motor, 70);
      digitalWrite(pump, HIGH);
      delay(1 * 60000);
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(2 * 60000);
      digitalWrite(pump, LOW);
      delay(3 * 60000);
      lcdPrintAt(0, 1, F("Time: 0:06       "));
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcdShowLinesCleared(F("Phase: Rinsehold"), F("Time: 0:00"));

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcdShowPhaseDrainCleared();

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        analogWrite(motor, 0);
        delay(30000);

        lcdShowPhaseEndCleared();

        digitalWrite(pump, LOW);

        beepShort6();

        running = false;
        return;
      } else {
        lcdShowPhaseDrainWithRow1(F("Time: 0:06   "));
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        lcdShowPhaseFinalSpinWithRow1(F("Time: 0:05    "));
        motorPulseMax100();
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 110);
        delay(3500);
        analogWrite(motor, 0);
        analogWrite(pump, LOW);
        delay(20000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 110);
        delay(3500);
        digitalWrite(pump, LOW);
        analogWrite(motor, 60);
        delay(20000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 75);
        delay(7000);
        analogWrite(motor, 110);
        delay(30000);
        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        digitalWrite(start_led, LOW);
      }
      lcdShowPhaseEndCleared();
      lcdShowTimeZeroRow1();
      beepTripleLongGap6();
      running = false;
    }
  }
}
