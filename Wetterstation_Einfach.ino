#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Seeed_BME280.h>

byte mac[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
IPAddress ip(192, 168, 1, 177);
EthernetServer server(80);

BME280 bme280;
bool sensorOK = false;
float referenzdruck = 1013.25;

void setup()
{
  Serial.begin(9600);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Wire.begin();
  sensorOK = bme280.init();

  if (!sensorOK)
  {
    Serial.println(F("FEHLER: BME280 nicht gefunden!"));
  }

  Ethernet.begin(mac, ip);
  server.begin();

  Serial.print(F("Webserver erreichbar unter: http://"));
  Serial.println(Ethernet.localIP());
}

void loop()
{
  EthernetClient client = server.available();

  if (client)
  {
    Serial.println(F("Client verbunden"));
    bool currentLineIsBlank = true;
    unsigned long start = millis();

    while (client.connected() && millis() - start < 1500UL)
    {
      if (client.available())
      {
        char c = client.read();

        if (c == '\n' && currentLineIsBlank)
        {
          client.println(F("HTTP/1.1 200 OK"));
          client.println(F("Content-Type: text/html; charset=utf-8"));
          client.println(F("Connection: close"));
          client.println();

          client.println(F("<!DOCTYPE html>"));
          client.println(F("<html lang='de'>"));
          client.println(F("<head>"));
          client.println(F("<meta charset='utf-8'>"));
          client.println(F("<meta http-equiv='refresh' content='5'>"));
          client.println(F("<title>Wetterstation</title>"));
          client.println(F("</head>"));
          client.println(F("<body>"));
          client.println(F("<h1>Arduino Wetterstation</h1>"));

          if (!sensorOK)
          {
            client.println(F("<p>FEHLER: BME280 nicht gefunden!</p>"));
          }
          else
          {
            float temperatur = bme280.getTemperature();
            float luftfeuchtigkeit = bme280.getHumidity();
            float druckHPa = bme280.getPressure() / 100.0;
            float kelvin = temperatur + 273.15;
            float barWert = druckHPa / 1000.0;
            float atmosphaere = barWert / 1.01325;
            float hoehe = (1.0 - pow(druckHPa / referenzdruck, 0.190284))
                          * kelvin / 0.0065;

            client.print(F("<p>Temperatur: "));
            client.print(temperatur, 2);
            client.println(F(" &deg;C</p>"));

            client.print(F("<p>Temperatur in Kelvin: "));
            client.print(kelvin, 2);
            client.println(F(" K</p>"));

            client.print(F("<p>Luftfeuchtigkeit: "));
            client.print(luftfeuchtigkeit, 2);
            client.println(F(" %</p>"));

            client.print(F("<p>Luftdruck: "));
            client.print(druckHPa, 2);
            client.println(F(" hPa</p>"));

            client.print(F("<p>Luftdruck in Bar: "));
            client.print(barWert, 4);
            client.println(F(" bar</p>"));

            client.print(F("<p>Atmosphaerendruck: "));
            client.print(atmosphaere, 4);
            client.println(F(" atm</p>"));

            client.print(F("<p>Geschaetzte Hoehe: "));
            client.print(hoehe, 1);
            client.println(F(" Meter</p>"));

            client.print(F("<p>Referenzdruck (QNH): "));
            client.print(referenzdruck, 2);
            client.println(F(" hPa</p>"));
          }

          client.println(F("</body>"));
          client.println(F("</html>"));
          break;
        }

        if (c == '\n')
        {
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          currentLineIsBlank = false;
        }
      }
    }

    delay(1);
    client.stop();
    Serial.println(F("Client getrennt"));
  }
}
