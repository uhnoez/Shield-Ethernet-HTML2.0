#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Seeed_BME280.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>
byte macAdresse[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
IPAddress ipAdresse(192, 168, 1, 177);
EthernetServer webserver(80);
BME280 sensor;
bool sensorBereit = false;
float temperatur = 0;
float kelvin = 0;
float luftfeuchtigkeit = 0;
float luftdruckHPa = 0;
float luftdruckBar = 0;
float luftdruckAtm = 0;
float hoehe = 0;
float referenzdruck = 1013.25;
unsigned long letzteMessung = 0;R"WEB(<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Wetterstation</title>
<style>
*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at 15% 0%,#182f55,#080e19 65%);color:#eaf3ff;font:15px/1.5 Arial,sans-serif}main{max-width:980px;margin:auto;padding:32px 18px}h1{font-size:44px;margin:0}small{color:#a6b8d0}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:13px;margin:24px 0}.tile,.panel{border:1px solid #29405b;background:linear-gradient(135deg,#1b3048,#101b2c);border-radius:19px;padding:20px;color:inherit}.tile{--c:#79baff;text-align:left;cursor:pointer;min-height:140px;transition:.25s}.tile:nth-child(3n+2){--c:#b9a0ff}.tile:nth-child(3n){--c:#5ce8d1}.tile:hover,.tile.on{transform:translateY(-3px);border-color:var(--c);box-shadow:0 12px 30px #0007}.name{display:block;color:#b3c1d7}.num{display:block;font-size:27px;font-weight:bold;margin:14px 0}.line{display:block;background:#29405b;height:4px;border-radius:9px;overflow:hidden}.fill{display:block;width:0;height:100%;background:var(--c);transition:width .4s}.panel{margin-top:14px}#info{display:none}input,button.save{padding:10px;border:0;border-radius:9px;font:inherit}input{width:140px}button.save{background:linear-gradient(90deg,#56d6d0,#79baff);cursor:pointer}#status{float:right;color:#80decf}@media(max-width:700px){.grid{grid-template-columns:repeat(2,1fr)}}@media(max-width:420px){.grid{grid-template-columns:1fr}}
</style>
<main><span id="status">Verbinde...</span><h1>Wetterstation</h1><small>Kacheln anklicken für mehr Infos. · BME280</small><div class="grid" id="grid"></div><div class="panel" id="info"></div><div class="panel"><b>Referenzdruck (QNH)</b><form id="form"><input id="q" type="number" min="870" max="1085" step="0.01" value="1013.25" required> <button class="save">Speichern</button> <small id="msg"></small></form></div><p><small>Live-Messwerte · Aktualisierung alle 5 Sekunden</small></p></main>
<script>
const names=['Celsius','Kelvin','Luftfeuchtigkeit','Hektopascal','Bar','Atmosphären','Geschätzte Höhe'];
const jokes=['Wer Celsius benutzt, hat den absoluten Nullpunkt der Wissenschaft noch nicht verstanden.','Kelvin ist wie Celsius, nur mit 273,15 Grad mehr Selbstbewusstsein.','Schimmel gefällt das.','Physikalisch betrachtet stehen wir alle unter Druck.','Endlich eine Bar, die rund um die Uhr geöffnet hat.','Die Atmosphäre ist allgegenwärtig. Datenschutzrechtlich höchst bedenklich.','Wissenschaftlich berechnet. Persönlich nicht überprüft.'];
const units=[' °C',' K',' %',' hPa',' bar',' atm',' m'],low=[0,260,0,950,.95,.94,-100],high=[40,320,100,1050,1.05,1.04,1000];
const grid=document.getElementById('grid'),info=document.getElementById('info');let chosen=-1;
names.forEach((name,i)=>{let el=document.createElement('button');el.className='tile';el.innerHTML='<span class="name">'+name+'</span><span class="num" id="v'+i+'">--</span><span class="line"><i class="fill" id="b'+i+'"></i></span>';el.onclick=()=>{chosen=chosen===i?-1:i;document.querySelectorAll('.tile').forEach((e,j)=>e.classList.toggle('on',i===j&&chosen===j));info.style.display=chosen<0?'none':'block';info.textContent=chosen<0?'':jokes[chosen]};grid.appendChild(el)});
async function update(){try{let r=await fetch('/daten',{cache:'no-store'}),d=await r.json();if(!r.ok||!d.ok)throw 0;let v=[d.t,d.k,d.h,d.p,d.b,d.a,d.z];v.forEach((n,i)=>{document.getElementById('v'+i).textContent=n.toFixed(i===4||i===5?4:i===6?1:2)+units[i];document.getElementById('b'+i).style.width=Math.max(3,Math.min(100,(n-low[i])*100/(high[i]-low[i])))+'%'});if(document.activeElement!==document.getElementById('q'))document.getElementById('q').value=d.q.toFixed(2);document.getElementById('status').textContent='Sensor online'}catch(e){document.getElementById('status').textContent='Keine Sensordaten'}}
document.getElementById('form').onsubmit=async e=>{e.preventDefault();let r=await fetch('/setq?q='+document.getElementById('q').value);document.getElementById('msg').textContent=r.ok?'Gespeichert':'Fehler';update()};update();setInterval(update,5000);
</script></html>)WEB";void messwerteLesen() {
  if (!sensorBereit) return;
  temperatur = sensor.getTemperature();
  kelvin = temperatur + 273.15;
  luftdruckHPa = sensor.getPressure() / 100.0;
  luftdruckBar = luftdruckHPa / 1000.0;
  luftdruckAtm = luftdruckBar / 1.01325;
  luftfeuchtigkeit = sensor.getHumidity();
  hoehe = (1 - pow(luftdruckHPa / referenzdruck, 0.190284)) * kelvin / 0.0065;
}
void messwerteSeriell() {
  if (!sensorBereit) {
    Serial.println(F("BME280 Fehler"));
    return;
  }
  Serial.print(F("C: ")); Serial.print(temperatur, 2);
  Serial.print(F(" | K: ")); Serial.print(kelvin, 2);
  Serial.print(F(" | Feuchte: ")); Serial.print(luftfeuchtigkeit, 2);
  Serial.print(F("% | hPa: ")); Serial.print(luftdruckHPa, 2);
  Serial.print(F(" | bar: ")); Serial.print(luftdruckBar, 4);
  Serial.print(F(" | atm: ")); Serial.print(luftdruckAtm, 4);
  Serial.print(F(" | Hoehe: ")); Serial.println(hoehe, 1);
}
void httpKopf(EthernetClient &client, const __FlashStringHelper *inhaltstyp) {
  client.println(F("HTTP/1.1 200 OK"));
  client.print(F("Content-Type: ")); client.println(inhaltstyp);
  client.println(F("Cache-Control: no-store\r\nConnection: close\r\n"));
}
void htmlSenden(EthernetClient &client) {
  httpKopf(client, F("text/html; charset=utf-8"));
  char puffer[48];
  byte anzahl = 0;
  for (unsigned int position = 0;; position++) {
    char zeichen = pgm_read_byte(&webseite[position]);
    if (!zeichen) break;
    puffer[anzahl++] = zeichen;
    if (anzahl == sizeof(puffer)) {
      client.write((uint8_t*)puffer, anzahl);
      anzahl = 0;
    }
  }
  if (anzahl) client.write((uint8_t*)puffer, anzahl);
}
void jsonSenden(EthernetClient &client) {
  httpKopf(client, F("application/json"));
  if (!sensorBereit) {
    client.println(F("{\"ok\":false}"));
    return;
  }
  client.print(F("{\"ok\":true,\"t\":")); client.print(temperatur, 2);
  client.print(F(",\"k\":")); client.print(kelvin, 2);
  client.print(F(",\"h\":")); client.print(luftfeuchtigkeit, 2);
  client.print(F(",\"p\":")); client.print(luftdruckHPa, 2);
  client.print(F(",\"b\":")); client.print(luftdruckBar, 4);
  client.print(F(",\"a\":")); client.print(luftdruckAtm, 4);
  client.print(F(",\"z\":")); client.print(hoehe, 1);
  client.print(F(",\"q\":")); client.print(referenzdruck, 2);
  client.println('}');
}
void httpAnfrageBearbeiten(EthernetClient &client) {
  char anfrageText[64];
  byte laenge = 0;
  bool ersteZeile = true;
  bool leereZeile = true;
  bool headerFertig = false;
  unsigned long beginn = millis();
  while (client.connected() && millis() - beginn < 1000UL) {
    if (!client.available()) continue;
    char zeichen = client.read();
    if (ersteZeile) {
      if (zeichen == '\n') { anfrageText[laenge] = 0; ersteZeile = false; }
      else if (zeichen != '\r' && laenge < sizeof(anfrageText) - 1) anfrageText[laenge++] = zeichen;
    }
    if (zeichen == '\n') {
      if (leereZeile) { headerFertig = true; break; }
      leereZeile = true;
    } else if (zeichen != '\r') leereZeile = false;
  }
  if (!headerFertig) return;

  if (strncmp(anfrageText, "GET /daten ", 11) == 0) {
    jsonSenden(client);
  } else if (strncmp(anfrageText, "GET /setq?q=", 12) == 0) {
    char *ende;
    double neuerDruck = strtod(anfrageText + 12, &ende);
    if (ende == anfrageText + 12 || (*ende != ' ' && *ende != '&') || neuerDruck < 870 || neuerDruck > 1085) {
      client.println(F("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n"));
      return;
    }
    referenzdruck = neuerDruck;
    messwerteLesen();
    httpKopf(client, F("text/plain"));
    client.println(F("OK"));
  } else if (strncmp(anfrageText, "GET / ", 6) == 0) {
    htmlSenden(client);
  } else {
    client.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n"));
  }
}
void setup() {
  Serial.begin(9600);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Wire.begin();
  sensorBereit = sensor.init();
  Ethernet.begin(macAdresse, ipAdresse);
  webserver.begin();
  Serial.print(F("Webserver: http://"));
  Serial.println(Ethernet.localIP());
  messwerteLesen();
  messwerteSeriell();
  letzteMessung = millis();
}
void loop() {
  if (millis() - letzteMessung >= 2000UL) {
    letzteMessung = millis();
    messwerteLesen();
    messwerteSeriell();
  }
  EthernetClient client = webserver.available();
  if (client) {
    httpAnfrageBearbeiten(client);
    delay(1);
    client.stop();
  }
}
