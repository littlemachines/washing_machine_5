#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

int motor = 9, start_button = 2, temp_button = 4, pump = 5, start_led = 12;
int program_dial = A0, buzzer = 3;
int rinse_hold_button = 6, without_spin_button = 7, pre_wash_button = 8, soak_button = 10;

int potValue = 0, screen = 0, tempCase = 0;
unsigned long lastChange = 0;
bool running = false;

// ---------- ФУНКЦИИ ----------

bool errorCheck() {
  if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
    lcd.setCursor(0, 1);
    lcd.print("ERROR: Invalid  ");
    tone(buzzer, 3729);
    return true;
  }
  noTone(buzzer);
  return false;
}

void showTime(int t) {
  bool pre = digitalRead(pre_wash_button) == LOW;
  bool soak = digitalRead(soak_button) == LOW;
  bool rh = digitalRead(rinse_hold_button) == LOW;
  bool ns = digitalRead(without_spin_button) == LOW;

  lcd.setCursor(0, 1);

  if (t == 95) {
    if (pre && rh) lcd.print("Time: 2:09     ");
    else if (pre && ns) lcd.print("Time: 2:10     ");
    else if (soak && rh) lcd.print("Time: 2:24     ");
    else if (soak && ns) lcd.print("Time: 2:25     ");
    else if (pre) lcd.print("Time: 2:23     ");
    else if (soak) lcd.print("Time: 2:36     ");
    else if (rh) lcd.print("Time: 1:45     ");
    else if (ns) lcd.print("Time: 1:46     ");
    else lcd.print("Time: 1:57     ");
  } else {
    if (pre && rh) lcd.print("Time: 2:09     ");
    else if (pre && ns) lcd.print("Time: 2:10     ");
    else if (soak && rh) lcd.print("Time: 2:24     ");
    else if (soak && ns) lcd.print("Time: 2:25     ");
    else if (pre) lcd.print("Time: 2:16     ");
    else if (soak) lcd.print("Time: 2:30     ");
    else if (rh) lcd.print("Time: 1:45     ");
    else if (ns) lcd.print("Time: 1:47     ");
    else lcd.print("Time: 1:57     ");
  }
}

void showMain(int t) {
  lcd.setCursor(0, 0);
  lcd.print("Cotton White   ");
  if (!errorCheck()) showTime(t);
}

void showSecond(int t) {
  lcd.setCursor(0, 0);
  lcd.print("Temperature: ");
  lcd.print(t);
  lcd.print((char)223);

  lcd.setCursor(0, 1);
  if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW)
    lcd.print("Spin: 0 rpm    ");
  else
    lcd.print("Spin: 1300 rpm ");
}

// ---------- SETUP ----------
void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

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

// ---------- LOOP ----------
void loop() {

  potValue = analogRead(program_dial);

  if (digitalRead(temp_button) == LOW) {
    tempCase = !tempCase;
    delay(300);
    lcd.clear();
  }

  if (millis() - lastChange > 5000) {
    screen = !screen;
    lastChange = millis();
    lcd.clear();
  }

  if (potValue < 102) {
    int temp = (tempCase == 0) ? 95 : 75;

    if (screen == 0) showMain(temp);
    else showSecond(temp);

    // ---------- START ----------
    if (digitalRead(start_button) == LOW && !running) {

      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      if ((soak && pre) || (rinseHold && noSpin)) {
        lcd.clear();
        lcd.print("ERROR");
        lcd.setCursor(0, 1);
        lcd.print("Invalid combo");
        tone(buzzer, 3729);
        return;
      }

      noTone(buzzer);
      running = true;
      digitalWrite(start_led, HIGH);

      int totalMinutes = 117 + (soak ? 33 : 0) + (pre ? 19 : 0) - (noSpin ? 10 : 0) - (rinseHold ? 12 : 0);

      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      lcd.print(totalMinutes / 60);
      lcd.print(":");
      if (totalMinutes % 60 < 10) lcd.print("0");
      lcd.print(totalMinutes % 60);

      // --- СЪЩАТА ЛОГИКА НА ПРАНЕ (НЕ ПИПАМЕ) ---
      digitalWrite(pump, HIGH);
      delay(3000);
      digitalWrite(pump, LOW);

      for (int i = 0; i < 3; i++) {
        analogWrite(motor, 110);
        delay(500);
        analogWrite(motor, 82);
        delay(5000);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 0);
        delay(5000);
      }
      if (soak) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Soak");
        lcd.setCursor(0, 1);
        lcd.print("Time: 2:29");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(pump, 0);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(6 * 60000);

        analogWrite(motor, 0);
        delay(30 * 60000);
      }

      if (pre) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Pre-wash");
        lcd.setCursor(0, 1);
        lcd.print("Time: 2:15");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(motor, 255);
        delay(100);
        digitalWrite(pump, LOW);
        analogWrite(motor, 80);
        delay(18 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Main wash");
      lcd.setCursor(0, 1);
      lcd.print("Time: 1:55");

      analogWrite(pump, 127);
      delay(20000);
      analogWrite(pump, 0);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(6 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 1:50      ");
      for (int i = 0; i < 276; i++) {
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 85);
        delay(6000);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 0);
        delay(2000);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Rinses ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:42       ");
      digitalWrite(pump, HIGH);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:41       ");
      // Симулиране на мотор - пример с плавно ускорение
      int pwmValue = 70;    // стартова стойност
      int targetPWM = 166;  // крайна стойност
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
      analogWrite(motor, 166);
      delay(2 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:35       ");
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(11 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:23    ");
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
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:21    ");
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(10 * 60000);
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinsehold");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00");

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        analogWrite(motor, 0);
        delay(30000);

        lcd.clear();
        lcd.print("Phase: End");

        digitalWrite(pump, LOW);

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

        running = false;
        return;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:10    ");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Final spin");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:10    ");
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
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:09    ");
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
        lcd.clear();
        lcd.print("Phase: End");

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

        running = false;
        digitalWrite(start_led, LOW);
      }
    }
  } else if (potValue < 204) {

    int temp = (tempCase == 0) ? 60 : (tempCase == 1) ? 40
                                                      : 30;

    // --- ОБЩ ЕКРАН ---
    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Cotton Color   ");

      if (!errorCheck()) {

        bool pre = digitalRead(pre_wash_button) == LOW;
        bool soak = digitalRead(soak_button) == LOW;
        bool rh = digitalRead(rinse_hold_button) == LOW;
        bool ns = digitalRead(without_spin_button) == LOW;

        lcd.setCursor(0, 1);

        if (pre && rh) lcd.print("Time: 2:07     ");
        else if (pre && ns) lcd.print("Time: 2:09     ");
        else if (soak && rh) lcd.print("Time: 2:22     ");
        else if (soak && ns) lcd.print("Time: 2:24     ");

        else if (pre) {
          if (temp == 40) lcd.print("Time: 2:15     ");
          else lcd.print("Time: 2:22     ");
        }

        else if (soak) lcd.print("Time: 2:29     ");
        else if (rh) lcd.print("Time: 1:44     ");
        else if (ns) lcd.print("Time: 1:46     ");
        else lcd.print("Time: 1:56     ");
      }
    }

    // --- ВТОРИ ЕКРАН ---
    else {
      lcd.setCursor(0, 0);
      lcd.print("Temperature: ");
      lcd.print(temp);
      lcd.print((char)223);

      lcd.setCursor(0, 1);
      if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW)
        lcd.print("Spin: 0 rpm    ");
      else
        lcd.print("Spin: 1300 rpm ");
    }

    // ---------- START ----------
    if (digitalRead(start_button) == LOW && !running) {

      bool soak = digitalRead(soak_button) == LOW;
      bool pre = digitalRead(pre_wash_button) == LOW;
      bool noSpin = digitalRead(without_spin_button) == LOW;
      bool rinseHold = digitalRead(rinse_hold_button) == LOW;

      if ((soak && pre) || (rinseHold && noSpin)) {
        lcd.clear();
        lcd.print("ERROR");
        lcd.setCursor(0, 1);
        lcd.print("Invalid combo");
        tone(buzzer, 3729);
        return;
      }

      noTone(buzzer);

      int totalMinutes = 116 + (soak ? 33 : 0) + (pre ? 19 : 0) - (noSpin ? 10 : 0) - (rinseHold ? 12 : 0);

      running = true;
      digitalWrite(start_led, HIGH);

      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      lcd.print(totalMinutes / 60);
      lcd.print(":");
      if (totalMinutes % 60 < 10) lcd.print("0");
      lcd.print(totalMinutes % 60);

      // --- ПРАНЕ (НЕ ПИПАМЕ ЛОГИКАТА) ---
      digitalWrite(pump, HIGH);
      delay(3000);
      digitalWrite(pump, LOW);

      for (int i = 0; i < 3; i++) {
        analogWrite(motor, 110);
        delay(500);
        analogWrite(motor, 82);
        delay(5000);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 0);
        delay(5000);
      }

      if (soak) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Soak");
        lcd.setCursor(0, 1);
        lcd.print("Time: 2:28");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(pump, 0);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(6 * 60000);

        analogWrite(motor, 0);
        delay(30 * 60000);
      }

      if (pre) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Pre-wash");
        lcd.setCursor(0, 1);
        lcd.print("Time: 2:14");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(motor, 255);
        delay(100);
        digitalWrite(pump, LOW);
        analogWrite(motor, 80);
        delay(18 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Main wash");
      lcd.setCursor(0, 1);
      lcd.print("Time: 1:55");

      analogWrite(pump, 127);
      delay(20000);
      analogWrite(pump, 0);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(6 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 1:50      ");
      for (int i = 0; i < 240; i++) {
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 85);
        delay(4000);
        analogWrite(motor, 70);
        delay(4000);
        analogWrite(motor, 0);
        delay(7000);
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Rinses ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:50       ");
      digitalWrite(pump, HIGH);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:49       ");
      delay(7000);
      analogWrite(motor, 85);
      delay(3000);
      // Симулиране на мотор - пример с плавно ускорение
      int pwmValue = 85;    // стартова стойност
      int targetPWM = 110;  // крайна стойност
      unsigned long lastUpdate = millis();

      while (pwmValue < targetPWM) {
        unsigned long now = millis();
        if (now - lastUpdate >= 900) {  // 1 секунда = +1 PWM
          pwmValue++;
          analogWrite(motor, pwmValue);
          lastUpdate = now;
        }
        // Тук можеш да добавиш проверки на други бутони или да обновяваш LCD
      }
      analogWrite(motor, 110);
      delay(3 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:44       ");
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(4 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:39       ");
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
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:35       ");
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(11 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:23    ");
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
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:21    ");
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(10 * 60000);
      // --- RINSE HOLD ---
      if (rinseHold) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinsehold");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00");

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (noSpin) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        analogWrite(motor, 0);
        delay(30000);

        lcd.clear();
        lcd.print("Phase: End");

        digitalWrite(pump, LOW);

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

        running = false;
        return;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:10    ");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Final spin");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:10    ");
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
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:09    ");
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
      lcd.clear();
      lcd.print("Phase: End");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:00    ");
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
      running = false;
    }
  } else if (potValue < 307) {

    bool pre = digitalRead(pre_wash_button) == LOW;
    bool soak = digitalRead(soak_button) == LOW;
    bool rh = digitalRead(rinse_hold_button) == LOW;
    bool ns = digitalRead(without_spin_button) == LOW;

    bool error = (pre && soak) || (rh && ns);

    int temp = (tempCase == 0) ? 60 : (tempCase == 1) ? 50
                                    : (tempCase == 2) ? 40
                                                      : 30;

    // --- SCREEN 0 ---
    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Minimum iron   ");

      if (error) {
        lcd.setCursor(0, 1);
        lcd.print("ERROR: Invalid ");
        tone(buzzer, 3729);
      } else {
        noTone(buzzer);
        lcd.setCursor(0, 1);

        if (pre && rh) lcd.print("Time: 1:18     ");
        else if (pre && ns) lcd.print("Time: 1:19     ");
        else if (soak && rh) lcd.print("Time: 1:44     ");
        else if (soak && ns) lcd.print("Time: 1:45     ");
        else if (pre) lcd.print("Time: 1:23     ");
        else if (soak) lcd.print("Time: 1:49     ");
        else if (rh) lcd.print("Time: 1:11     ");
        else if (ns) lcd.print("Time: 1:12     ");
        else lcd.print("Time: 1:16     ");
      }
    }

    // --- SCREEN 1 ---
    if (screen == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Temperature: ");
      lcd.print(temp);
      lcd.print((char)223);

      lcd.setCursor(0, 1);
      if (rh || ns) lcd.print("Spin: 0 rpm    ");
      else lcd.print("Spin: 900 rpm  ");
    }

    // --- START ---
    if (digitalRead(start_button) == LOW && !running) {

      if (error) {
        lcd.clear();
        lcd.print("ERROR");
        lcd.setCursor(0, 1);
        lcd.print("Invalid combo");
        tone(buzzer, 3729);
        return;
      }

      noTone(buzzer);

      int totalMinutes = 76 + (soak ? 33 : 0) + (pre ? 7 : 0) - (ns ? 4 : 0) - (rh ? 5 : 0);

      running = true;
      digitalWrite(start_led, HIGH);

      lcd.setCursor(0, 1);
      lcd.print("Time: ");
      lcd.print(totalMinutes / 60);
      lcd.print(":");
      if (totalMinutes % 60 < 10) lcd.print("0");
      lcd.print(totalMinutes % 60);

      // --- START SEQUENCE ---
      digitalWrite(pump, HIGH);
      delay(3000);
      digitalWrite(pump, LOW);

      if (soak) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Soak");
         lcd.setCursor(0, 1);
        lcd.print("Time: 1:49");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(pump, 0);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(6 * 60000);

        analogWrite(motor, 0);
        delay(30 * 60000);
      }

      if (pre) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Pre-wash");
         lcd.setCursor(0, 1);
        lcd.print("Time: 1:23");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(motor, 255);
        delay(100);
        digitalWrite(pump, LOW);
        analogWrite(motor, 80);
        delay(6 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Main wash");
       lcd.setCursor(0, 1);
        lcd.print("Time: 1:15");

      analogWrite(pump, 127);
      delay(20000);
      analogWrite(pump, 0);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(2 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 1:13      ");
      for (int i = 0; i < 180; i++) {
        analogWrite(motor, 255);
        delay(100);
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
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Rinses ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:30      ");
      digitalWrite(pump, HIGH);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:29       ");
      analogWrite(motor, 75);
      delay(7000);
      analogWrite(motor, 90);
      delay(1500);
      analogWrite(motor, 0);
      delay(2000);
      digitalWrite(pump, LOW);
      analogWrite(motor, 255);
      delay(100);
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
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:28       ");
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(7 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:20    ");
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
      analogWrite(motor, 255);
      delay(100);
      digitalWrite(pump, HIGH);
      analogWrite(motor, 70);
      delay(7000);
      analogWrite(motor, 105);
      delay(3500);
      digitalWrite(pump, LOW);
      analogWrite(motor, 60);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:18    ");
      analogWrite(pump, 127);
      analogWrite(motor, 70);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      delay(8 * 60000);
      // --- RINSE HOLD ---
      if (rh) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinsehold");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00");

        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      // --- WITHOUT SPIN ---
      if (ns) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");

        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        analogWrite(motor, 0);
        delay(30000);

        lcd.clear();
        lcd.print("Phase: End");

        digitalWrite(pump, LOW);

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

        running = false;
        return;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:05    ");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Final spin");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:04    ");
        delay(7000);
        analogWrite(motor, 90);
        delay(1500);
        digitalWrite(pump, LOW);
        analogWrite(motor, 0);
        delay(20000);
        analogWrite(motor, 255);
        digitalWrite(pump, HIGH);
        delay(100);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:02    ");
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
      lcd.clear();
      lcd.print("Phase: End");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:00    ");
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
      running = false;
    }
  }  // --- Drain/Spin ---
  else if (potValue < 409) {
    bool pre = digitalRead(pre_wash_button) == LOW;
    bool soak = digitalRead(soak_button) == LOW;
    bool rinseHold = digitalRead(rinse_hold_button) == LOW;
    bool noSpin = digitalRead(without_spin_button) == LOW;

    // --- SCREEN DISPLAY ---
    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Drain/Spin     ");
      if (pre || soak || rinseHold) {
        lcd.setCursor(0, 1);
        lcd.print("ERROR: Invalid ");
        tone(buzzer, 3729);
      } else {
        noTone(buzzer);
        lcd.setCursor(0, 1);
        lcd.print(noSpin ? "Time: 0:01     " : "Time: 0:08     ");
      }
    } else if (screen == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Temperature: Co");
      lcd.setCursor(0, 1);
      lcd.print(noSpin ? "Spin: 0 rpm    " : "Spin: 1300 rpm ");
    }

    // --- START BUTTON ---
    if (digitalRead(start_button) == LOW && !running) {
      if (pre || soak || rinseHold) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("ERROR");
        lcd.setCursor(0, 1);
        lcd.print("Invalid combo");
        tone(buzzer, 3729);
        return;
      }

      noTone(buzzer);
      running = true;
      digitalWrite(start_led, HIGH);

      if (noSpin) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:01");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        digitalWrite(pump, LOW);
        analogWrite(motor, 0);
        lcd.setCursor(0, 0);
        lcd.print("Phase: End");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:00");
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

      // --- FINAL SPIN IF ENABLED ---
      if (!noSpin) {
        // --- DRAIN PHASE ---
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:08");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(30000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Final spin");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:07");
        analogWrite(motor, 110);
        delay(35000);
        analogWrite(motor, 150);
        delay(20000);
        analogWrite(motor, 191);
        delay(2 * 60000);
        analogWrite(motor, 212);
        delay(3000);
        analogWrite(motor, 200);
        delay(10000);
        analogWrite(motor, 225);
        delay(170000);
        analogWrite(motor, 0);
      }

      // --- END PHASE ---
      lcd.clear();
      lcd.print("Phase: End");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:00    ");
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
      digitalWrite(start_led, LOW);
      running = false;
    }
  } else if (potValue < 511) {
    // --- Променливи за бутоните ---
    bool pre = digitalRead(pre_wash_button) == LOW;
    bool soak = digitalRead(soak_button) == LOW;
    bool rinseHold = digitalRead(rinse_hold_button) == LOW;
    bool noSpin = digitalRead(without_spin_button) == LOW;
    bool error = false;

    // --- SCREEN 0 ---
    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("15' Quick Wash   ");

      if ((pre && soak) || (rinseHold && noSpin) || soak) {
        lcd.setCursor(0, 1);
        lcd.print("ERROR: Invalid  ");
        tone(buzzer, 3729);
        error = true;
      } else {
        noTone(buzzer);
        int totalMinutes = 15;
        if (pre) totalMinutes += 11;
        if (noSpin) totalMinutes -= 3;
        if (rinseHold) totalMinutes -= 4;
        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        lcd.print(totalMinutes / 60);
        lcd.print(":");
        if (totalMinutes % 60 < 10) lcd.print("0");
        lcd.print(totalMinutes % 60);
        lcd.print("   ");
      }
    }

    // --- SCREEN 1 ---
    if (screen == 1) {
      lcd.setCursor(0, 0);
      lcd.print("Temperature: Co");
      lcd.setCursor(0, 1);
      lcd.print((rinseHold || noSpin) ? "Spin: 0 rpm    " : "Spin: 1200 rpm ");
    }

    // --- START BUTTON ---
    if (digitalRead(start_button) == LOW && !running && !error) {
      running = true;
      digitalWrite(start_led, HIGH);

      // --- ФАЗИ (запазени точно както си ги написал) ---
      if (pre) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Pre-wash");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:25");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(motor, 255);
        delay(100);
        digitalWrite(pump, LOW);
        analogWrite(motor, 70);
        delay(10 * 60000);
        digitalWrite(pump, HIGH);
        analogWrite(motor, 0);
        delay(1 * 60000);
      }

      // ✅ MAIN WASH ВИНАГИ
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Main wash");
       lcd.setCursor(0, 1);
        lcd.print("Time: 0:14");

      analogWrite(pump, 127);
      analogWrite(motor, 255);
      delay(100);
      analogWrite(motor, 70);
      delay(1 * 60000);
      analogWrite(pump, 0);
      analogWrite(motor, 70);
      delay(4 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:09      ");
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Phase: Rinses ");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:09      ");
      digitalWrite(pump, HIGH);
      analogWrite(motor, 70);
      delay(1 * 60000);
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:08       ");
      analogWrite(motor, 70);
      analogWrite(pump, 127);
      delay(1 * 60000);
      digitalWrite(pump, LOW);
      analogWrite(motor, 70);
      delay(4 * 60000);

      if (rinseHold) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinsehold");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00");
        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        running = false;
        return;
      }

      if (noSpin) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Drain");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        analogWrite(motor, 0);
        delay(30000);
        lcd.clear();
        lcd.print("Phase: End");
        digitalWrite(pump, LOW);
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
        running = false;
        return;
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Final spin");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:03");
        analogWrite(motor, 0);
        digitalWrite(pump, HIGH);
        delay(1 * 60000);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(7000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:01    ");
        analogWrite(motor, 200);
        delay(2 * 60000);
        analogWrite(motor, 0);
        digitalWrite(pump, LOW);
        digitalWrite(start_led, LOW);
      }

      lcd.clear();
      lcd.print("Phase: End");
      lcd.setCursor(0, 1);
      lcd.print("Time: 0:00    ");
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
      running = false;
    }
  } else if (potValue < 614) {
    // --- Променливи за бутоните ---
    bool pre = digitalRead(pre_wash_button) == LOW;
    bool soak = digitalRead(soak_button) == LOW;
    bool rinseHold = digitalRead(rinse_hold_button) == LOW;
    bool noSpin = digitalRead(without_spin_button) == LOW;
    bool error = false;

    // --- SCREEN 0 ---
    if (screen == 0) {
      lcd.setCursor(0, 0);
      lcd.print("Starch   ");
      if (pre || soak || (rinseHold && noSpin)) {
        lcd.setCursor(0, 1);
        lcd.print("ERROR: Invalid  ");
        tone(buzzer, 3729);
        error = true;
      } else {
        noTone(buzzer);
        if (rinseHold) {
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:11    ");
        } else if (noSpin) {
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:12     ");
        } else {
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:19   ");
        }
      }

      // --- SCREEN 1 ---
      if (screen == 1) {
        lcd.setCursor(0, 0);
        lcd.print("Temperature: 30°");
        lcd.setCursor(0, 1);
        lcd.print((rinseHold || noSpin) ? "Spin: 0 rpm    " : "Spin: 1300 rpm ");
      }

      // --- START BUTTON ---
      if (digitalRead(start_button) == LOW && !running && !error) {
        running = true;
        digitalWrite(start_led, HIGH);
        int totalMinutes = 19;
        if (noSpin) totalMinutes -= 7;
        if (rinseHold) totalMinutes -= 8;

        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        lcd.print(totalMinutes / 60);
        lcd.print(":");
        if (totalMinutes % 60 < 10) lcd.print("0");
        lcd.print(totalMinutes % 60);
        lcd.print("   ");

        digitalWrite(pump, HIGH);
        delay(3000);
        digitalWrite(pump, LOW);

        // ✅ MAIN PHASES (запазени)
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Soak");
         lcd.setCursor(0, 1);
        lcd.print("Time: 0:18");
        analogWrite(pump, 127);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(1 * 60000);
        digitalWrite(pump, LOW);
        delay(9 * 60000);

        if (rinseHold) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Rinsehold");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:00");
          analogWrite(motor, 0);
          digitalWrite(pump, LOW);
          running = false;
          return;
        }

        if (noSpin) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain");
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          analogWrite(motor, 0);
          delay(30000);
          lcd.clear();
          lcd.print("Phase: End");
          digitalWrite(pump, LOW);
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
          running = false;
          return;
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:08");
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(30000);

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Final spin");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:07");
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

        lcd.clear();
        lcd.print("Phase: End");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00    ");
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
        running = false;
      }
    } else if (potValue < 717) {
      // --- Показване на програма според tempCase и бутоните ---
      bool error = false;
      switch (tempCase) {
        case 0:  // 60'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Automatic   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if (digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {

              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
              if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:09     ");
              } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:10     ");
              } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:35     ");
              } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:36    ");
              } else if (digitalRead(pre_wash_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:16     ");
              } else if (digitalRead(soak_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:42     ");
              } else if (digitalRead(rinse_hold_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:02    ");
              } else if (digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:03     ");
              } else {
                lcd.setCursor(0, 1);
                lcd.print("Time: 1:09    ");
              }
            }

            if (screen == 1) {
              lcd.setCursor(0, 0);
              lcd.print("Temperature: 40°");
              if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Spin: 0 rpm    ");
              } else {
                lcd.setCursor(0, 1);
                lcd.print("Spin: 900 rpm ");
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
            if ((soak && pre) || (rinseHold && noSpin)) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("ERROR");
              lcd.setCursor(0, 1);
              lcd.print("Invalid combo");
              tone(buzzer, 3729);
              running = false;
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
            lcd.setCursor(0, 1);
            lcd.print("Time: ");
            lcd.print(totalMinutes / 60);
            lcd.print(":");
            if (totalMinutes % 60 < 10) lcd.print("0");
            lcd.print(totalMinutes % 60);
            lcd.print("   ");
            // Пуск на помпа за 3 сек
            digitalWrite(pump, HIGH);
            delay(3000);
            digitalWrite(pump, LOW);

            // Симулиране на мотор
            for (int i = 0; i < 3; i++) {
              analogWrite(motor, 110);
              delay(500);
              analogWrite(motor, 82);
              delay(5000);
              analogWrite(motor, 255);
              delay(100);
              analogWrite(motor, 0);
              delay(5000);
            }
            if (soak) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Soak");

              analogWrite(pump, 127);
              delay(20000);
              analogWrite(pump, 0);
              analogWrite(motor, 255);
              delay(100);
              analogWrite(motor, 70);
              delay(6 * 60000);

              analogWrite(motor, 0);
              delay(30 * 60000);
            }

            if (pre) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Pre-wash");

              analogWrite(pump, 127);
              delay(20000);
              analogWrite(motor, 255);
              delay(100);
              digitalWrite(pump, LOW);
              analogWrite(motor, 80);
              delay(6 * 60000);
              digitalWrite(pump, HIGH);
              analogWrite(motor, 0);
              delay(1 * 60000);
            }

            // ✅ MAIN WASH ВИНАГИ
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Phase: Main wash");

            analogWrite(pump, 127);
            delay(20000);
            analogWrite(pump, 0);
            analogWrite(motor, 255);
            delay(100);
            analogWrite(motor, 70);
            delay(2 * 60000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 1:06     ");
            for (int i = 0; i < 168; i++) {
              analogWrite(motor, 255);
              delay(100);
              analogWrite(motor, 85);
              delay(6000);
              analogWrite(motor, 70);
              delay(7000);
              analogWrite(motor, 0);
              delay(2000);
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Phase: Rinses ");
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:25      ");
            digitalWrite(pump, HIGH);
            analogWrite(motor, 255);
            delay(100);
            analogWrite(motor, 70);
            delay(1 * 60000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:24      ");
            analogWrite(motor, 70);
            delay(7000);
            analogWrite(motor, 90);
            delay(1500);
            analogWrite(motor, 0);
            delay(2000);
            digitalWrite(pump, HIGH);
            analogWrite(motor, 255);
            delay(100);
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
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:23     ");
            analogWrite(motor, 70);
            analogWrite(pump, 127);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(7 * 60000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:15    ");
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
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:11    ");
            analogWrite(pump, 127);
            analogWrite(motor, 70);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(10 * 60000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:05    ");
            // --- RINSE HOLD ---
            if (rinseHold) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Rinsehold");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:00");

              analogWrite(motor, 0);
              digitalWrite(pump, LOW);
              running = false;
              return;
            }

            // --- WITHOUT SPIN ---
            if (noSpin) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Drain");

              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(30000);
              analogWrite(motor, 0);
              delay(30000);

              lcd.clear();
              lcd.print("Phase: End");

              digitalWrite(pump, LOW);

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

              running = false;
              return;
            } else {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Drain");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:05    ");
              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(30000);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Final spin");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:05    ");
              delay(7000);
              analogWrite(motor, 100);
              delay(2500);
              digitalWrite(pump, HIGH);
              analogWrite(motor, 60);
              delay(20000);
              analogWrite(motor, 255);
              digitalWrite(pump, HIGH);
              delay(100);
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:04    ");
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
            lcd.clear();
            lcd.print("Phase: End");
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:00    ");
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
            running = false;
          }
      }
    } else if (potValue < 819) {
      // --- Показване на програма според tempCase и бутоните ---
      bool error = false;
      switch (tempCase) {
        case 0:  // 60'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("30' Quick Wash   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(pre_wash_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:22    ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:23     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:30   ");
            }
          }

          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: 40°");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 1300 rpm ");
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
          lcd.setCursor(0, 0);
          lcd.print("ERROR");
          lcd.setCursor(0, 1);
          lcd.print("Invalid combo");
          tone(buzzer, 3729);
          running = false;
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
        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        lcd.print(totalMinutes / 60);
        lcd.print(":");
        if (totalMinutes % 60 < 10) lcd.print("0");
        lcd.print(totalMinutes % 60);
        lcd.print("   ");
        // Пуск на помпа за 3 сек
        digitalWrite(pump, HIGH);
        delay(3000);
        digitalWrite(pump, LOW);

        // ✅ MAIN WASH ВИНАГИ
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Main wash");

        analogWrite(pump, 127);
        delay(20000);
        analogWrite(pump, 0);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(6 * 60000);
        for (int i = 0; i < 40; i++) {
          analogWrite(motor, 255);
          delay(100);
          analogWrite(motor, 85);
          delay(6000);
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 0);
          delay(2000);
        }
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:14      ");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinses ");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:14      ");
        digitalWrite(pump, HIGH);
        analogWrite(motor, 70);
        delay(1 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:13      ");
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
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:12     ");
        analogWrite(motor, 255);
        analogWrite(pump, 127);
        delay(100);
        analogWrite(motor, 70);
        delay(1 * 60000);
        digitalWrite(pump, LOW);
        delay(3 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:08      ");
        digitalWrite(pump, HIGH);
        delay(1 * 60000);
        delay(7000);
        analogWrite(motor, 100);
        delay(2500);
        analogWrite(motor, 0);
        delay(25000);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(7000);
        analogWrite(motor, 100);
        delay(2500);
        digitalWrite(pump, LOW);
        analogWrite(motor, 60);
        delay(10000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:06      ");
        analogWrite(motor, 80);
        delay(4 * 60000);
        // --- RINSE HOLD ---
        if (rinseHold) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Rinsehold");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:00");

          analogWrite(motor, 0);
          digitalWrite(pump, LOW);
          running = false;
          return;
        }

        // --- WITHOUT SPIN ---
        if (noSpin) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain");

          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          analogWrite(motor, 0);
          delay(30000);

          lcd.clear();
          lcd.print("Phase: End");

          digitalWrite(pump, LOW);

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

          running = false;
          return;
        } else {
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain     ");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:07     ");
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          lcd.setCursor(0, 0);
          lcd.print("Phase: Finalspin     ");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:06      ");
          analogWrite(motor, 75);
          delay(7000);
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:06    ");
          analogWrite(motor, 200);
          analogWrite(motor, 0);
          delay(5000);
          analogWrite(motor, 255);
          delay(100);
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
        lcd.clear();
        lcd.print("Phase: End");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00    ");
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
        running = false;
      }
    } else if (potValue < 922) {
      // --- Показване на програма според tempCase и бутоните ---
      bool error = false;
      switch (tempCase) {
        case 0:  // 60'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Woollens 🧼   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            }
            if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:36     ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:37     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:40     ");
            }
          }

          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: 40°");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 1200 rpm ");
            }
          }
          break;

        case 1:  // 40°
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Woollens 🧼   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
              if (digitalRead(rinse_hold_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:36     ");
              } else if (digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:37     ");
              } else {
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:40     ");
              }
            }

            if (screen == 1) {
              lcd.setCursor(0, 0);
              lcd.print("Temperature: 30°");
              if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                lcd.setCursor(0, 1);
                lcd.print("Spin: 0 rpm    ");
              } else {
                lcd.setCursor(0, 1);
                lcd.print("Spin: 1200 rpm ");
              }
            }
            break;
            case 2:  // 30'c
              if (screen == 0) {
                lcd.setCursor(0, 0);
                lcd.print("Woollens 🧼   ");

                // --- Проверка за забранени комбинации ---


                // Проверка за забранени комбинации
                if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
                  lcd.setCursor(0, 1);
                  lcd.print("ERROR: Invalid  ");
                  tone(buzzer, 3729);  // включваме бъзъра
                  error = true;
                } else {
                  noTone(buzzer);  // няма грешка → изключваме бъзъра
                }

                // Ако няма грешка, показваме времето
                if (!error) {
                  if (digitalRead(rinse_hold_button) == LOW) {
                    lcd.setCursor(0, 1);
                    lcd.print("Time: 0:36     ");
                  } else if (digitalRead(without_spin_button) == LOW) {
                    lcd.setCursor(0, 1);
                    lcd.print("Time: 0:37     ");
                  } else {
                    lcd.setCursor(0, 1);
                    lcd.print("Time: 0:40     ");
                  }
                }

                if (screen == 1) {
                  lcd.setCursor(0, 0);
                  lcd.print("Temperature: Co");
                  if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
                    lcd.setCursor(0, 1);
                    lcd.print("Spin: 0 rpm    ");
                  } else {
                    lcd.setCursor(0, 1);
                    lcd.print("Spin: 1200 rpm ");
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
                if ((soak) || (pre) || (rinseHold && noSpin)) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("ERROR");
                  lcd.setCursor(0, 1);
                  lcd.print("Invalid combo");
                  tone(buzzer, 3729);
                  running = false;
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
                lcd.setCursor(0, 1);
                lcd.print("Time: ");
                lcd.print(totalMinutes / 60);
                lcd.print(":");
                if (totalMinutes % 60 < 10) lcd.print("0");
                lcd.print(totalMinutes % 60);
                lcd.print("   ");
                // Пуск на помпа за 3 сек
                digitalWrite(pump, HIGH);
                delay(3000);
                digitalWrite(pump, LOW);

                // ✅ MAIN WASH ВИНАГИ
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Phase: Main wash");

                analogWrite(pump, 127);
                delay(20000);
                analogWrite(pump, 0);
                analogWrite(motor, 255);
                delay(100);
                analogWrite(motor, 60);
                delay(20 * 60000);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Phase: Rinses");
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:20     ");
                analogWrite(motor, 0);
                digitalWrite(pump, HIGH);
                delay(1 * 60000);
                analogWrite(motor, 255);
                delay(100);
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 110);
                delay(1 * 60000);
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:18      ");
                analogWrite(pump, 127);
                analogWrite(motor, 60);
                delay(1 * 60000);
                digitalWrite(pump, LOW);
                delay(5 * 60000);
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:12       ");
                analogWrite(motor, 0);
                digitalWrite(pump, HIGH);
                delay(1 * 60000);
                analogWrite(motor, 255);
                delay(100);
                analogWrite(motor, 70);
                delay(7000);
                analogWrite(motor, 110);
                delay(1 * 60000);
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:10       ");
                analogWrite(pump, 127);
                analogWrite(motor, 60);
                delay(1 * 60000);
                digitalWrite(pump, LOW);
                delay(5 * 60000);
                // --- RINSE HOLD ---
                if (rinseHold) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Phase: Rinsehold");
                  lcd.setCursor(0, 1);
                  lcd.print("Time: 0:00");

                  analogWrite(motor, 0);
                  digitalWrite(pump, LOW);
                  running = false;
                  return;
                }

                // --- WITHOUT SPIN ---
                if (noSpin) {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Phase: Drain");

                  digitalWrite(pump, HIGH);
                  analogWrite(motor, 70);
                  delay(30000);
                  analogWrite(motor, 0);
                  delay(30000);

                  lcd.clear();
                  lcd.print("Phase: End");

                  digitalWrite(pump, LOW);

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

                  running = false;
                  return;
                } else {
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Phase: Drain");
                  lcd.setCursor(0, 1);
                  lcd.print("Time: 0:04   ");
                  digitalWrite(pump, HIGH);
                  analogWrite(motor, 0);
                  delay(1 * 60000);
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Phase: Final spin");
                  lcd.setCursor(0, 1);
                  lcd.print("Time: 0:03    ");
                  analogWrite(motor, 255);
                  delay(100);
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
                lcd.clear();
                lcd.print("Phase: End");
                lcd.setCursor(0, 1);
                lcd.print("Time: 0:00    ");
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
                running = false;
              }
          }
      }
    } else {
      // --- Показване на програма според tempCase и бутоните ---
      bool error = false;
      switch (tempCase) {
        case 0:  // 60'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Delicates/Silk 🧼   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW) || (digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            }
            if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:34     ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:36     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:37    ");
            }
          }

          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: 30°");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 400 rpm ");
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
              lcd.setCursor(0, 0);
              lcd.print("ERROR");
              lcd.setCursor(0, 1);
              lcd.print("Invalid combo");
              tone(buzzer, 3729);
              running = false;
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
            lcd.setCursor(0, 1);
            lcd.print("Time: ");
            lcd.print(totalMinutes / 60);
            lcd.print(":");
            if (totalMinutes % 60 < 10) lcd.print("0");
            lcd.print(totalMinutes % 60);
            lcd.print("   ");
            // Пуск на помпа за 3 сек
            digitalWrite(pump, HIGH);
            delay(3000);
            digitalWrite(pump, LOW);

            // ✅ MAIN WASH ВИНАГИ
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Phase: Main wash");

            analogWrite(pump, 127);
            delay(20000);
            analogWrite(pump, 0);
            analogWrite(motor, 255);
            delay(100);
            analogWrite(motor, 50);
            delay(21 * 60000);
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Phase: Rinses");
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:16     ");
            analogWrite(motor, 0);
            digitalWrite(pump, HIGH);
            delay(90000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:18      ");
            analogWrite(pump, 127);
            analogWrite(motor, 255);
            delay(100);
            analogWrite(pump, 127);
            analogWrite(motor, 50);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(4 * 60000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:11       ");
            analogWrite(motor, 0);
            digitalWrite(pump, HIGH);
            delay(90000);
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:09       ");
            analogWrite(pump, 127);
            analogWrite(motor, 255);
            delay(100);
            analogWrite(pump, 127);
            analogWrite(motor, 50);
            delay(1 * 60000);
            digitalWrite(pump, LOW);
            delay(3 * 60000);
            // --- RINSE HOLD ---
            if (rinseHold) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Rinsehold");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:00");

              analogWrite(motor, 0);
              digitalWrite(pump, LOW);
              running = false;
              return;
            }

            // --- WITHOUT SPIN ---
            if (noSpin) {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Drain");

              digitalWrite(pump, HIGH);
              analogWrite(motor, 70);
              delay(30000);
              analogWrite(motor, 0);
              delay(30000);

              lcd.clear();
              lcd.print("Phase: End");

              digitalWrite(pump, LOW);

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

              running = false;
              return;
            } else {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Drain");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:03   ");
              digitalWrite(pump, HIGH);
              analogWrite(motor, 0);
              delay(1 * 60000);
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Phase: Final spin");
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:02    ");
              analogWrite(motor, 255);
              delay(100);
              analogWrite(motor, 75);
              delay(7000);
              analogWrite(motor, 90);
              delay(1 * 60000);
              analogWrite(motor, 0);
              digitalWrite(pump, LOW);
              digitalWrite(start_led, LOW);
            }
            lcd.clear();
            lcd.print("Phase: End");
            lcd.setCursor(0, 1);
            lcd.print("Time: 0:00    ");
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
            running = false;
          }
          break;

        case 1:  // 60'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Delicateseasycare   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            }
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:02     ");
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:01     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:26     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:27     ");
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:06     ");
            } else if (digitalRead(soak_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:32    ");
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:53     ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:54     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:59    ");
            }
          }

          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: 40°");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 1200 rpm ");
            }
          }
          break;

        case 2:  // 40°
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Delicateseasycare   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            }
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:02     ");
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:01     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:26     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:27     ");
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:06     ");
            } else if (digitalRead(soak_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:32    ");
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:53     ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:54     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:59    ");
            }
          }

          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: 30°");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 1200 rpm ");
            }
          }
          break;
        case 3:  // 30'c
          if (screen == 0) {
            lcd.setCursor(0, 0);
            lcd.print("Delicateseasycare   ");

            // --- Проверка за забранени комбинации ---


            // Проверка за забранени комбинации
            if ((digitalRead(pre_wash_button) == LOW && digitalRead(soak_button) == LOW) || (digitalRead(rinse_hold_button) == LOW && digitalRead(without_spin_button) == LOW)) {
              lcd.setCursor(0, 1);
              lcd.print("ERROR: Invalid  ");
              tone(buzzer, 3729);  // включваме бъзъра
              error = true;
            } else {
              noTone(buzzer);  // няма грешка → изключваме бъзъра
            }

            // Ако няма грешка, показваме времето
            if (!error) {
            }
            if (digitalRead(pre_wash_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:02     ");
            } else if (digitalRead(pre_wash_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:01     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:26     ");
            } else if (digitalRead(soak_button) == LOW && digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:27     ");
            } else if (digitalRead(pre_wash_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:06     ");
            } else if (digitalRead(soak_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 1:32    ");
            } else if (digitalRead(rinse_hold_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:53     ");
            } else if (digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:54     ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Time: 0:59    ");
            }
          }
          if (screen == 1) {
            lcd.setCursor(0, 0);
            lcd.print("Temperature: Co");
            if (digitalRead(rinse_hold_button) == LOW || digitalRead(without_spin_button) == LOW) {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 0 rpm    ");
            } else {
              lcd.setCursor(0, 1);
              lcd.print("Spin: 1200 rpm ");
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
        if ((soak && pre) || (rinseHold && noSpin)) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ERROR");
          lcd.setCursor(0, 1);
          lcd.print("Invalid combo");
          tone(buzzer, 3729);
          running = false;
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
        lcd.setCursor(0, 1);
        lcd.print("Time: ");
        lcd.print(totalMinutes / 60);
        lcd.print(":");
        if (totalMinutes % 60 < 10) lcd.print("0");
        lcd.print(totalMinutes % 60);
        lcd.print("   ");
        // Пуск на помпа за 3 сек
        digitalWrite(pump, HIGH);
        delay(3000);
        digitalWrite(pump, LOW);

        if (soak) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Soak");

          analogWrite(pump, 127);
          delay(20000);
          analogWrite(pump, 0);
          analogWrite(motor, 255);
          delay(100);
          analogWrite(motor, 70);
          delay(6 * 60000);

          analogWrite(motor, 0);
          delay(30 * 60000);
        }

        if (pre) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Pre-wash");

          analogWrite(pump, 127);
          delay(20000);
          analogWrite(motor, 255);
          delay(100);
          digitalWrite(pump, LOW);
          analogWrite(motor, 80);
          delay(6 * 60000);
          digitalWrite(pump, HIGH);
          analogWrite(motor, 0);
          delay(1 * 60000);
        }

        // ✅ MAIN WASH ВИНАГИ
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Main wash");

        analogWrite(pump, 127);
        delay(30000);
        analogWrite(pump, 0);
        analogWrite(motor, 255);
        delay(100);
        analogWrite(motor, 70);
        delay(35 * 60000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Phase: Rinses");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:24     ");
        analogWrite(motor, 70);
        digitalWrite(pump, HIGH);
        delay(1 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:23      ");
        analogWrite(pump, 127);
        analogWrite(motor, 70);
        delay(2 * 60000);
        digitalWrite(pump, LOW);
        delay(3 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:18       ");
        analogWrite(motor, 70);
        digitalWrite(pump, HIGH);
        delay(1 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:17      ");
        analogWrite(pump, 127);
        analogWrite(motor, 70);
        delay(2 * 60000);
        digitalWrite(pump, LOW);
        delay(3 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:12       ");
        analogWrite(motor, 70);
        digitalWrite(pump, HIGH);
        delay(1 * 60000);
        analogWrite(pump, 127);
        analogWrite(motor, 70);
        delay(2 * 60000);
        digitalWrite(pump, LOW);
        delay(3 * 60000);
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:06       ");
        // --- RINSE HOLD ---
        if (rinseHold) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Rinsehold");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:00");

          analogWrite(motor, 0);
          digitalWrite(pump, LOW);
          running = false;
          return;
        }

        // --- WITHOUT SPIN ---
        if (noSpin) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain");

          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(30000);
          analogWrite(motor, 0);
          delay(30000);

          lcd.clear();
          lcd.print("Phase: End");

          digitalWrite(pump, LOW);

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

          running = false;
          return;
        } else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Drain");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:06   ");
          digitalWrite(pump, HIGH);
          analogWrite(motor, 70);
          delay(1 * 60000);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Phase: Final spin");
          lcd.setCursor(0, 1);
          lcd.print("Time: 0:05    ");
          analogWrite(motor, 255);
          delay(100);
          analogWrite(motor, 70);
          delay(7000);
          analogWrite(motor, 110);
          delay(3500);
          analogWrite(motor, 0);
          analogWrite(pump, LOW);
          delay(20000);
          analogWrite(motor, 255);
          delay(100);
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
        lcd.clear();
        lcd.print("Phase: End");
        lcd.setCursor(0, 1);
        lcd.print("Time: 0:00    ");
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
        running = false;
      }
    }
  }
}