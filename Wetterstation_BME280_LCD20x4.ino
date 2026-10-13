#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Seeed_BME280.h"
#include <math.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);
BME280 bme280;

const float referenzDruck = 1020.0;

float temperatur = 0;
float feuchtigkeit = 0;
float druck = 0;
float hoehe = 0;

float minTemp = 1000;
float maxTemp = -1000;

float vergleichsDruck = 0;
float druckAenderung = 0;

unsigned long letzteMessung = 0;
unsigned long letzterWechsel = 0;
unsigned long letzterDruckvergleich = 0;

byte seite = 0;
bool trendBereit = false;

void zeileLoeschen(byte zeile) {
  lcd.setCursor(0, zeile);
  lcd.print(F("                    "));
  lcd.setCursor(0, zeile);
}

void balken(float wert, float minimum, float maximum) {
  int anzahl = constrain(
    (int)((wert - minimum) * 10.0 / (maximum - minimum)),
    0, 10
  );

  lcd.print('[');
  for (int i = 0; i < 10; i++) {
    if (i < anzahl) {
      lcd.write((uint8_t)255);
    } else {
      lcd.print(' ');
    }
  }
  lcd.print(']');
}

void messwerteLesen() {
  temperatur = bme280.getTemperature();
  feuchtigkeit = bme280.getHumidity();
  druck = bme280.getPressure() / 100.0;

  hoehe = 44330.0 *
          (1.0 - pow(druck / referenzDruck, 0.1903));

  if (temperatur < minTemp) minTemp = temperatur;
  if (temperatur > maxTemp) maxTemp = temperatur;
}

void hauptanzeige() {
  zeileLoeschen(0);
  lcd.print(F("Temp:"));
  lcd.print(temperatur, 1);
  lcd.print(F(" C RH:"));
  lcd.print(feuchtigkeit, 0);
  lcd.print('%');

  zeileLoeschen(1);
  lcd.print(F("Druck: "));
  lcd.print(druck, 1);
  lcd.print(F(" hPa"));
}

void zusatzanzeige() {
  zeileLoeschen(2);
  zeileLoeschen(3);

  switch (seite) {
    case 0:
      lcd.setCursor(0, 2);
      lcd.print(F("Hoehe: "));
      lcd.print(hoehe, 1);
      lcd.print(F(" m"));

      lcd.setCursor(0, 3);
      lcd.print(F("=== WETTERSTATION =="));
      break;

    case 1:
      lcd.setCursor(0, 2);
      lcd.print(F("Raumklima: "));

      if (temperatur >= 20 && temperatur <= 24 &&
          feuchtigkeit >= 40 && feuchtigkeit <= 60) {
        lcd.print(F("ANGENEHM"));
      } else if (feuchtigkeit < 40) {
        lcd.print(F("ZU TROCKEN"));
      } else if (feuchtigkeit > 60) {
        lcd.print(F("ZU FEUCHT"));
      } else if (temperatur < 20) {
        lcd.print(F("ZU KALT"));
      } else {
        lcd.print(F("ZU WARM"));
      }

      lcd.setCursor(0, 3);
      lcd.print(F("Feuchte: "));
      balken(feuchtigkeit, 0, 100);
      break;

    case 2:
      lcd.setCursor(0, 2);
      lcd.print(F("Min:"));
      lcd.print(minTemp, 1);
      lcd.print(F(" Max:"));
      lcd.print(maxTemp, 1);

      lcd.setCursor(0, 3);
      lcd.print(F("Temp: "));
      balken(temperatur, 0, 40);
      break;

    case 3:
      lcd.setCursor(0, 2);
      lcd.print(F("Trend: "));

      if (!trendBereit) {
        lcd.print(F("WARTEN"));
      } else if (druckAenderung > 0.5) {
        lcd.print(F("STEIGEND"));
      } else if (druckAenderung < -0.5) {
        lcd.print(F("FALLEND"));
      } else {
        lcd.print(F("STABIL"));
      }

      lcd.setCursor(0, 3);
      lcd.print(F("Aenderung: "));
      if (!trendBereit) {
        lcd.print(F("-- hPa"));
      } else {
        if (druckAenderung >= 0) lcd.print('+');
        lcd.print(druckAenderung, 1);
        lcd.print(F(" hPa"));
      }
      break;
  }
}

void setup() {
  Wire.begin();
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print(F("=== WETTERSTATION =="));
  lcd.setCursor(0, 1);
  lcd.print(F("BME280 + LCD 20x4"));
  lcd.setCursor(0, 2);
  lcd.print(F("Initialisierung..."));

  if (!bme280.init()) {
    lcd.clear();
    lcd.print(F("BME280 FEHLER!"));
    while (1);
  }

  delay(1500);
  lcd.clear();
  messwerteLesen();
  vergleichsDruck = druck;
  hauptanzeige();
  zusatzanzeige();

  letzteMessung = millis();
  letzterWechsel = millis();
  letzterDruckvergleich = millis();
}

void loop() {
  unsigned long jetzt = millis();

  if (jetzt - letzteMessung >= 2000UL) {
    letzteMessung = jetzt;
    messwerteLesen();
    hauptanzeige();
    zusatzanzeige();
  }

  if (jetzt - letzterWechsel >= 8000UL) {
    letzterWechsel = jetzt;
    seite++;
    if (seite > 3) seite = 0;
    zusatzanzeige();
  }

  if (jetzt - letzterDruckvergleich >= 300000UL) {
    druckAenderung = druck - vergleichsDruck;
    vergleichsDruck = druck;
    letzterDruckvergleich = jetzt;
    trendBereit = true;
  }
}
