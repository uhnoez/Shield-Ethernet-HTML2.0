#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include <Seeed_BME280.h>
#include <avr/pgmspace.h>

// Zweiter Arduino: MAC-Ende 0x02, IP-Ende 178
byte mac[]={0x02,0,0,0,0,0x01};
IPAddress ip(192,168,1,177);
EthernetServer server(80);
BME280 bme;
bool ok=false;
float t,k,h,p,b,a,z,q=1013.25;
unsigned long zuletzt=0;

// Sieben Kacheln stehen direkt im HTML; JavaScript aktualisiert nur Werte und Klicks.
// Webseite liegt im Flash, nicht im 2-KB-Arbeitsspeicher.
const char html[] PROGMEM=R"WEB(<!doctype html><html lang="de"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Wetterstation</title>
<style>
*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at 15% 0%,#182f55,#080e19 65%);color:#eaf3ff;font:15px/1.5 Arial,sans-serif}main{max-width:980px;margin:auto;padding:32px 18px}h1{font-size:44px;margin:0}small{color:#a6b8d0}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:13px;margin:24px 0}.tile,.panel{border:1px solid #29405b;background:linear-gradient(135deg,#1b3048,#101b2c);border-radius:19px;padding:20px;color:inherit}.tile{--c:#79baff;text-align:left;cursor:pointer;min-height:140px;transition:.25s}.tile:nth-child(3n+2){--c:#b9a0ff}.tile:nth-child(3n){--c:#5ce8d1}.tile:hover,.tile.on{transform:translateY(-3px);border-color:var(--c);box-shadow:0 12px 30px #0007}.name{display:block;color:#b3c1d7}.num{display:block;font-size:27px;font-weight:bold;margin:14px 0}.line{display:block;background:#29405b;height:4px;border-radius:9px;overflow:hidden}.fill{display:block;width:0;height:100%;background:var(--c);transition:width .4s}.panel{margin-top:14px}#info{display:none}input,button.save{padding:10px;border:0;border-radius:9px;font:inherit}input{width:140px}button.save{background:linear-gradient(90deg,#56d6d0,#79baff);cursor:pointer}#status{float:right;color:#80decf}@media(max-width:700px){.grid{grid-template-columns:repeat(2,1fr)}}@media(max-width:420px){.grid{grid-template-columns:1fr}}
</style>
<main><span id="status">Verbinde...</span><h1>Wetterstation</h1><small>Kacheln anklicken für mehr Infos. · BME280</small>
<div class="grid" id="grid">
<button class="tile" type="button" data-i="0"><span class="name">Celsius</span><span class="num" id="v0">--</span><span class="line"><i class="fill" id="b0"></i></span></button>
<button class="tile" type="button" data-i="1"><span class="name">Kelvin</span><span class="num" id="v1">--</span><span class="line"><i class="fill" id="b1"></i></span></button>
<button class="tile" type="button" data-i="2"><span class="name">Luftfeuchtigkeit</span><span class="num" id="v2">--</span><span class="line"><i class="fill" id="b2"></i></span></button>
<button class="tile" type="button" data-i="3"><span class="name">Hektopascal</span><span class="num" id="v3">--</span><span class="line"><i class="fill" id="b3"></i></span></button>
<button class="tile" type="button" data-i="4"><span class="name">Bar</span><span class="num" id="v4">--</span><span class="line"><i class="fill" id="b4"></i></span></button>
<button class="tile" type="button" data-i="5"><span class="name">Atmosphären</span><span class="num" id="v5">--</span><span class="line"><i class="fill" id="b5"></i></span></button>
<button class="tile" type="button" data-i="6"><span class="name">Geschätzte Höhe</span><span class="num" id="v6">--</span><span class="line"><i class="fill" id="b6"></i></span></button>
</div><div class="panel" id="info"></div><div class="panel"><b>Referenzdruck (QNH)</b><form id="form"><input id="q" type="number" min="870" max="1085" step="0.01" value="1013.25" required> <button class="save">Speichern</button> <small id="msg"></small></form></div><p><small>Live-Messwerte · Aktualisierung alle 5 Sekunden</small></p></main>
<script>
const jokes=['Wer Celsius benutzt, hat den absoluten Nullpunkt der Wissenschaft noch nicht verstanden.','Kelvin ist wie Celsius, nur mit 273,15 Grad mehr Selbstbewusstsein.','Schimmel gefällt das.','Physikalisch betrachtet stehen wir alle unter Druck.','Endlich eine Bar, die rund um die Uhr geöffnet hat.','Die Atmosphäre ist allgegenwärtig. Datenschutzrechtlich höchst bedenklich.','Wissenschaftlich berechnet. Persönlich nicht überprüft.'];
const units=[' °C',' K',' %',' hPa',' bar',' atm',' m'],low=[0,260,0,950,.95,.94,-100],high=[40,320,100,1050,1.05,1.04,1000];
const info=document.getElementById('info');let chosen=-1;
document.querySelectorAll('.tile').forEach((el,i)=>{el.onclick=()=>{chosen=chosen===i?-1:i;document.querySelectorAll('.tile').forEach((e,j)=>e.classList.toggle('on',chosen===j));info.style.display=chosen<0?'none':'block';info.textContent=chosen<0?'':jokes[chosen]}});
async function update(){try{let r=await fetch('/daten',{cache:'no-store'}),d=await r.json();if(!r.ok||!d.ok)throw 0;let v=[d.t,d.k,d.h,d.p,d.b,d.a,d.z];v.forEach((n,i)=>{document.getElementById('v'+i).textContent=n.toFixed(i===4||i===5?4:i===6?1:2)+units[i];document.getElementById('b'+i).style.width=Math.max(3,Math.min(100,(n-low[i])*100/(high[i]-low[i])))+'%'});if(document.activeElement!==document.getElementById('q'))document.getElementById('q').value=d.q.toFixed(2);document.getElementById('status').textContent='Sensor online'}catch(e){document.getElementById('status').textContent='Keine Sensordaten'}}
document.getElementById('form').onsubmit=async e=>{e.preventDefault();let r=await fetch('/setq?q='+document.getElementById('q').value);document.getElementById('msg').textContent=r.ok?'Gespeichert':'Fehler';update()};update();setInterval(update,5000);
</script></body></html>)WEB";

void messen(){
  if(!ok)return;
  t=bme.getTemperature();k=t+273.15;
  p=bme.getPressure()/100.0;b=p/1000.0;a=b/1.01325;
  h=bme.getHumidity();z=(1-pow(p/q,0.190284))*k/0.0065;
}

void seriell(){
  if(!ok){Serial.println(F("BME280 Fehler"));return;}
  Serial.print(F("C: "));Serial.print(t,2);
  Serial.print(F(" | K: "));Serial.print(k,2);
  Serial.print(F(" | Feuchte: "));Serial.print(h,2);
  Serial.print(F("% | hPa: "));Serial.print(p,2);
  Serial.print(F(" | bar: "));Serial.print(b,4);
  Serial.print(F(" | atm: "));Serial.print(a,4);
  Serial.print(F(" | Hoehe: "));Serial.println(z,1);
}

void kopf(EthernetClient &c,const __FlashStringHelper *typ){
  c.println(F("HTTP/1.1 200 OK"));
  c.print(F("Content-Type: "));c.println(typ);
  c.println(F("Cache-Control: no-store\r\nConnection: close\r\n"));
}

void seite(EthernetClient &c){
  kopf(c,F("text/html; charset=utf-8"));
  char buf[48];byte n=0;
  for(unsigned int i=0;;i++){
    char x=pgm_read_byte(&html[i]);if(!x)break;
    buf[n++]=x;
    if(n==sizeof(buf)){if(c.write((uint8_t*)buf,n)!=n)return;n=0;}
  }
  if(n)c.write((uint8_t*)buf,n);
}

void daten(EthernetClient &c){
  kopf(c,F("application/json"));
  if(!ok){c.println(F("{\"ok\":false}"));return;}
  c.print(F("{\"ok\":true,\"t\":"));c.print(t,2);
  c.print(F(",\"k\":"));c.print(k,2);
  c.print(F(",\"h\":"));c.print(h,2);
  c.print(F(",\"p\":"));c.print(p,2);
  c.print(F(",\"b\":"));c.print(b,4);
  c.print(F(",\"a\":"));c.print(a,4);
  c.print(F(",\"z\":"));c.print(z,1);
  c.print(F(",\"q\":"));c.print(q,2);
  c.println('}');
}

void anfrage(EthernetClient &c){
  char req[64];byte n=0;bool first=true,empty=true,done=false;
  unsigned long start=millis();
  while(c.connected() && millis()-start<1000UL){
    if(!c.available())continue;
    char x=c.read();
    if(first){if(x=='\n'){req[n]=0;first=false;}
      else if(x!='\r'&&n<sizeof(req)-1)req[n++]=x;}
    if(x=='\n'){if(empty){done=true;break;}empty=true;}
    else if(x!='\r')empty=false;
  }
  if(!done)return;
  if(strncmp(req,"GET /daten ",11)==0)daten(c);
  else if(strncmp(req,"GET /setq?q=",12)==0){
    char *end;double value=strtod(req+12,&end);
    if(end==req+12||(*end!=' '&&*end!='&')||value<870||value>1085){
      c.println(F("HTTP/1.1 400 Bad Request\r\nConnection: close\r\n"));return;
    }
    q=value;messen();kopf(c,F("text/plain"));c.println(F("OK"));
  }
  else if(strncmp(req,"GET / ",6)==0)seite(c);
  else c.println(F("HTTP/1.1 404 Not Found\r\nConnection: close\r\n"));
}

void setup(){
  Serial.begin(9600);
  pinMode(4,OUTPUT);digitalWrite(4,HIGH);
  Wire.begin();ok=bme.init();
  Ethernet.begin(mac,ip);server.begin();
  Serial.print(F("Webserver: http://"));Serial.println(Ethernet.localIP());
  messen();seriell();zuletzt=millis();
}

void loop(){
  if(millis()-zuletzt>=2000UL){zuletzt=millis();messen();seriell();}
  EthernetClient c=server.available();
  if(c){anfrage(c);c.flush();delay(5);c.stop();}
}
