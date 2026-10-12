#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Seeed_BME280.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <string.h>

// Fuer den zweiten UNO: MAC-Ende 0x02 und IP-Ende 178 verwenden.
byte macAdresse[] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};
IPAddress ipAdresse(192, 168, 1, 177);
EthernetServer webserver(80);

BME280 sensor;
bool sensorBereit = false;
float temperatur = 0, kelvin = 0, luftfeuchtigkeit = 0;
float luftdruckHPa = 0, luftdruckBar = 0, luftdruckAtm = 0;
float hoehe = 0, referenzdruck = 1013.25;
unsigned long letzteMessung = 0;

// PROGMEM: Die Webseite bleibt im Flash-Speicher des UNO.
// Die sieben Kacheln sind direktes HTML und werden NICHT per JS erzeugt.
const char webseite[] PROGMEM = R"WEB(<!doctype html>
<html lang="de">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Wetterstation</title>
<style>
*{box-sizing:border-box}
body{margin:0;background:radial-gradient(ellipse at 15% 0%,#182f55,#080e19 65%);color:#eaf3ff;font:15px/1.5 Arial,sans-serif}
main{max-width:980px;margin:auto;padding:32px 18px}
h1{font-size:44px;margin:0}small{color:#a6b8d0}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:13px;margin:24px 0}
.tile,.panel{border:1px solid #29405b;background:linear-gradient(135deg,#1b3048,#101b2c);border-radius:19px;padding:20px;color:inherit}
.tile{--c:#79baff;text-align:left;cursor:pointer;min-height:140px;transition:transform .25s,box-shadow .25s,border-color .25s}
.tile:nth-child(3n+2){--c:#b9a0ff}.tile:nth-child(3n){--c:#5ce8d1}
.tile:hover,.tile.on{transform:translateY(-3px);border-color:var(--c);box-shadow:0 12px 30px #0007}
.name{display:block;color:#b3c1d7}.num{display:block;font-size:27px;font-weight:bold;margin:14px 0}
.line{display:block;background:#29405b;height:4px;border-radius:9px;overflow:hidden}
.fill{display:block;width:0;height:100%;background:var(--c);transition:width .4s}
.panel{margin-top:14px}#info{display:none}
input,button.save{padding:10px;border:0;border-radius:9px;font:inherit}
input{width:140px}button.save{background:linear-gradient(90deg,#56d6d0,#79baff);cursor:pointer}
#status{float:right;color:#80decf}
@media(max-width:700px){.grid{grid-template-columns:repeat(2,minmax(0,1fr))}}
@media(max-width:420px){.grid{grid-template-columns:1fr}}
</style>
</head>
<body>
<main>
<span id="status">Verbinde...</span>
<h1>Wetterstation</h1>
<small>Kacheln anklicken für mehr Infos. &middot; BME280</small>
<div class="grid">
<button class="tile" type="button"><span class="name">Celsius</span><span class="num" id="v0">--</span><span class="line"><i class="fill" id="b0"></i></span></button>
<button class="tile" type="button"><span class="name">Kelvin</span><span class="num" id="v1">--</span><span class="line"><i class="fill" id="b1"></i></span></button>
<button class="tile" type="button"><span class="name">Luftfeuchtigkeit</span><span class="num" id="v2">--</span><span class="line"><i class="fill" id="b2"></i></span></button>
<button class="tile" type="button"><span class="name">Hektopascal</span><span class="num" id="v3">--</span><span class="line"><i class="fill" id="b3"></i></span></button>
<button class="tile" type="button"><span class="name">Bar</span><span class="num" id="v4">--</span><span class="line"><i class="fill" id="b4"></i></span></button>
<button class="tile" type="button"><span class="name">Atmosphären</span><span class="num" id="v5">--</span><span class="line"><i class="fill" id="b5"></i></span></button>
<button class="tile" type="button"><span class="name">Geschätzte Höhe</span><span class="num" id="v6">--</span><span class="line"><i class="fill" id="b6"></i></span></button>
</div>
<div class="panel" id="info"></div>
<div class="panel"><b>Referenzdruck (QNH)</b>
<form id="form"><input id="q" type="number" min="870" max="1085" step="0.01" value="1013.25" required>
<button class="save" type="submit">Speichern</button> <small id="msg"></small></form></div>
<p><small>Live-Messwerte &middot; Aktualisierung alle 5 Sekunden</small></p>
</main>
<script>
const sprueche=[
'Wer Celsius benutzt, hat den absoluten Nullpunkt der Wissenschaft noch nicht verstanden.',
'Kelvin ist wie Celsius, nur mit 273,15 Grad mehr Selbstbewusstsein.',
'Schimmel gefällt das.',
'Physikalisch betrachtet stehen wir alle unter Druck.',
'Endlich eine Bar, die rund um die Uhr geöffnet hat.',
'Die Atmosphäre ist allgegenwärtig. Datenschutzrechtlich höchst bedenklich.',
'Wissenschaftlich berechnet. Persönlich nicht überprüft.'
];
const einheiten=[' °C',' K',' %',' hPa',' bar',' atm',' m'];
const minimum=[0,260,0,950,.95,.94,-100];
const maximum=[40,320,100,1050,1.05,1.04,1000];
const kacheln=document.querySelectorAll('.tile');
const info=document.getElementById('info');
const status=document.getElementById('status');
const qnh=document.getElementById('q');
let ausgewaehlt=-1;

kacheln.forEach((kachel,i)=>{
 kachel.onclick=()=>{
  ausgewaehlt=ausgewaehlt===i?-1:i;
  kacheln.forEach((element,j)=>element.classList.toggle('on',j===ausgewaehlt));
  info.style.display=ausgewaehlt<0?'none':'block';
  info.textContent=ausgewaehlt<0?'':sprueche[ausgewaehlt];
 };
});

async function aktualisieren(){
 try{
  const antwort=await fetch('/daten',{cache:'no-store'});
  if(!antwort.ok)throw Error('HTTP '+antwort.status);
  const daten=await antwort.json();
  if(!daten.ok)throw Error('BME280 nicht bereit');
  const werte=[daten.t,daten.k,daten.h,daten.p,daten.b,daten.a,daten.z];
  for(let i=0;i<7;i++){
   const zahl=Number(werte[i]);
   if(!Number.isFinite(zahl))throw Error('Ungueltiger Messwert');
   const nachkommastellen=(i===4||i===5)?4:(i===6?1:2);
   document.getElementById('v'+i).textContent=zahl.toFixed(nachkommastellen)+einheiten[i];
   const breite=Math.max(3,Math.min(100,(zahl-minimum[i])*100/(maximum[i]-minimum[i])));
   document.getElementById('b'+i).style.width=breite+'%';
  }
  if(document.activeElement!==qnh)qnh.value=Number(daten.q).toFixed(2);
  status.textContent='Sensor online';
 }catch(fehler){
  status.textContent='Keine Sensordaten';
  console.error('Wetterstation:',fehler);
 }
}

document.getElementById('form').onsubmit=async event=>{
 event.preventDefault();
 const meldung=document.getElementById('msg');
 try{
  const antwort=await fetch('/setq?q='+encodeURIComponent(qnh.value),{cache:'no-store'});
  if(!antwort.ok)throw Error('QNH konnte nicht gespeichert werden');
  meldung.textContent='Gespeichert';
  await aktualisieren();
 }catch(fehler){
  meldung.textContent='Fehler';
  console.error('QNH:',fehler);
 }
};
aktualisieren();
setInterval(aktualisieren,5000);
</script>
</body>
</html>)WEB";

void messwerteLesen() {
  if (!sensorBereit) return;
  temperatur = sensor.getTemperature();
  kelvin = temperatur + 273.15;
  luftdruckHPa = sensor.getPressure() / 100.0;
  luftdruckBar = luftdruckHPa / 1000.0;
  luftdruckAtm = luftdruckBar / 1.01325;
  luftfeuchtigkeit = sensor.getHumidity();
  // Bisherige, temperaturabhaengige barometrische Hoehenformel.
  hoehe = (1.0 - pow(luftdruckHPa / referenzdruck, 0.190284)) * kelvin / 0.0065;
}

void messwerteSeriell() {
  if (!sensorBereit) {
    Serial.println(F("BME280 nicht gefunden"));
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

// HTTP-Header: Browser erfaehrt Dateityp und Ende der Uebertragung.
void httpKopf(EthernetClient &client, const __FlashStringHelper *typ, int laenge) {
  client.println(F("HTTP/1.1 200 OK"));
  client.print(F("Content-Type: ")); client.println(typ);
  client.println(F("Cache-Control: no-store"));
  if (laenge >= 0) {
    client.print(F("Content-Length: "));
    client.println(laenge);
  }
  client.println(F("Connection: close"));
  client.println();
}

// Ein Block kann auch nur teilweise angenommen werden: Rest erneut senden.
bool blockSenden(EthernetClient &client, const uint8_t *daten, byte laenge) {
  byte verschickt = 0;
  unsigned long start = millis();
  while (verschickt < laenge && millis() - start < 1500UL) {
    size_t n = client.write(daten + verschickt, laenge - verschickt);
    if (n > 0) {
      verschickt += n;
      start = millis();
    } else {
      delay(2);
    }
  }
  return verschickt == laenge;
}

void htmlSenden(EthernetClient &client) {
  // Die exakte Byteanzahl hilft dem Browser, abgeschnittenes HTML zu erkennen.
  httpKopf(client, F("text/html; charset=utf-8"), sizeof(webseite) - 1);
  uint8_t puffer[96];
  byte anzahl = 0;
  for (unsigned int position = 0; position < sizeof(webseite) - 1; position++) {
    puffer[anzahl++] = pgm_read_byte(&webseite[position]);
    if (anzahl == sizeof(puffer)) {
      if (!blockSenden(client, puffer, anzahl)) return;
      anzahl = 0;
    }
  }
  if (anzahl > 0) blockSenden(client, puffer, anzahl);
}

void jsonSenden(EthernetClient &client) {
  httpKopf(client, F("application/json; charset=utf-8"), -1);
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
  char anfrage[80] = {0};
  byte position = 0;
  bool ersteZeile = true, zeileLeer = true, headerFertig = false;
  unsigned long beginn = millis();
  while (millis() - beginn < 1500UL) {
    if (!client.available()) {
      if (!client.connected()) break;
      delay(1);
      continue;
    }
    char zeichen = client.read();
    if (ersteZeile) {
      if (zeichen == '\n') {
        anfrage[position] = '\0';
        ersteZeile = false;
      } else if (zeichen != '\r' && position < sizeof(anfrage) - 1) {
        anfrage[position++] = zeichen;
      }
    }
    if (zeichen == '\n') {
      if (zeileLeer) { headerFertig = true; break; }
      zeileLeer = true;
    } else if (zeichen != '\r') {
      zeileLeer = false;
    }
  }
  if (!headerFertig) return;

  if (strncmp(anfrage, "GET /daten ", 11) == 0) {
    jsonSenden(client);
  } else if (strncmp(anfrage, "GET /setq?q=", 12) == 0) {
    char *ende;
    double neuerDruck = strtod(anfrage + 12, &ende);
    if (ende == anfrage + 12 || *ende != ' ' || !(neuerDruck >= 870.0 && neuerDruck <= 1085.0)) {
      client.println(F("HTTP/1.1 400 Bad Request"));
      client.println(F("Connection: close"));
      client.println();
      return;
    }
    referenzdruck = neuerDruck;
    messwerteLesen();
    Serial.print(F("Neuer QNH: ")); Serial.println(referenzdruck, 2);
    httpKopf(client, F("text/plain"), -1);
    client.println(F("OK"));
  } else if (strncmp(anfrage, "GET / ", 6) == 0) {
    htmlSenden(client);
  } else {
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Connection: close"));
    client.println();
  }
}

void setup() {
  Serial.begin(9600);
  // SD-Karten-Chip-Select der ueblichen Ethernet Shields deaktivieren.
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  Wire.begin();
  sensorBereit = sensor.init();
  Ethernet.begin(macAdresse, ipAdresse);
  webserver.begin();
  Serial.print(F("Webserver: http://")); Serial.println(Ethernet.localIP());
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
    delay(5);
    client.stop();
  }
}
