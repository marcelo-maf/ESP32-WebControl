#include <Arduino.h>            
#include <WiFi.h>               
#include <ESPAsyncWebServer.h>  
#include <Preferences.h>        

const char* ssid = "ESP32-SmartView-Pro";
const char* password = "12345678";

const int ledPlaca = 2;         
const int ledVerde = 12;        
const int ledAmarelo = 14;      
const int ledVermelho = 27;     
const int potPin = 34;          

int limiteMin, limiteMax;
AsyncWebServer server(80);
Preferences preferencias;       

String montarPagina() {
  String html = R"rawliteral(
<!DOCTYPE html><html><head><title>SmartView Pro</title>
<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>
<style>
  body{text-align:center;font-family:sans-serif;background-color:#1a1a1a;color:white;margin:0;padding:10px;}
  .trilho{position:relative;width:90%;max-width:500px;height:45px;background:#333;margin:20px auto;border-radius:12px;border:2px solid #555;overflow:hidden;}
  .barra{position:absolute;height:100%;width:0%;transition:0.3s;left:50%;}
  .marcador-centro{position:absolute;left:50%;width:3px;height:100%;background:white;z-index:10;margin-left:-1px;}
  .monitor{background:#2a2a2a;padding:15px;border-radius:20px;width:90%;max-width:400px;margin:10px auto;border:1px solid #444;}
  .controles{margin:20px 0;}
  button{width:130px;height:55px;font-size:18px;margin:10px;border-radius:12px;border:none;cursor:pointer;font-weight:bold;transition:0.2s;}
  button:active{transform: scale(0.95);}
  .btn-on{background-color:#4dff88;color:black;} 
  .btn-off{background-color:#ff4d4d;color:white;}
  input{width:90px;height:40px;text-align:center;font-size:18px;border-radius:8px;border:none;margin:5px;background:#444;color:white;}
  .bg-ok{background-color:#4dff88;} .bg-alerta{background-color:#ff4d4d;}
</style></head><body>
  <h2>SISTEMA SMARTVIEW CENTER</h2>
  
  <div class='controles'>
    <button class='btn-on' onclick="fetch('/on')">LIGAR</button>
    <button class='btn-off' onclick="fetch('/off')">DESLIGAR</button>
  </div>

  <div class='trilho'><div class='marcador-centro'></div><div id='minhaBarra' class='barra bg-ok'></div></div>
  
  <div class='monitor'>
    <div id='valorPot' style='font-size:65px;font-weight:bold;'>0000</div>
    <p id='statusText' style='font-size:20px;'>ESTABILIZANDO...</p>
  </div>

  <div style='background:#333;padding:15px;border-radius:15px;display:inline-block;margin-top:10px;'>
    Mín: <input type='number' id='minInput' value='VAL_MIN' onchange='enviarLimites()'>
    Máx: <input type='number' id='maxInput' value='VAL_MAX' onchange='enviarLimites()'>
  </div>

<script>
  function enviarLimites(){ 
    let min=document.getElementById('minInput').value; 
    let max=document.getElementById('maxInput').value; 
    fetch('/set?min='+min+'&max='+max); 
  }
  setInterval(function(){ 
    fetch('/valor').then(r=>r.json()).then(data=>{ 
      document.getElementById('valorPot').innerHTML = data.val;
      let b = document.getElementById('minhaBarra');
      let st = document.getElementById('statusText');
      let min = parseInt(document.getElementById('minInput').value);
      let max = parseInt(document.getElementById('maxInput').value);
      let meio = (min + max) / 2; 
      let rangeMetade = (max - min) / 2;
      
      let porcentagem = ((data.val - meio) / rangeMetade) * 50;
      
      if(porcentagem >= 0){ 
        b.style.left = '50%'; 
        b.style.width = Math.min(porcentagem, 50) + '%'; 
      } else { 
        b.style.left = (50 + Math.max(porcentagem, -50)) + '%'; 
        b.style.width = Math.abs(Math.max(porcentagem, -50)) + '%'; 
      }

      if(data.val > max || data.val < min){ 
        b.className='barra bg-alerta'; st.innerHTML='FORA DA FAIXA!'; st.style.color='#ff4d4d';
      } else { 
        b.className='barra bg-ok'; st.innerHTML='SINAL ESTÁVEL'; st.style.color='#4dff88';
      }
    }); 
  }, 1000);
</script></body></html>
)rawliteral";

  html.replace("VAL_MIN", String(limiteMin));
  html.replace("VAL_MAX", String(limiteMax));
  return html;
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPlaca, OUTPUT); pinMode(ledVerde, OUTPUT); 
  pinMode(ledAmarelo, OUTPUT); pinMode(ledVermelho, OUTPUT);
  
  preferencias.begin("ajustes", false);
  limiteMin = preferencias.getInt("min", 1000);
  limiteMax = preferencias.getInt("max", 3000);
  
  WiFi.softAP(ssid, password);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(200, "text/html", montarPagina()); 
  });

  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){ 
    digitalWrite(ledPlaca, HIGH); 
    request->send(200); 
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){ 
    digitalWrite(ledPlaca, LOW); 
    request->send(200); 
  });
  
  server.on("/set", HTTP_GET, [](AsyncWebServerRequest *request){ 
    if(request->hasParam("min")) { limiteMin = request->getParam("min")->value().toInt(); preferencias.putInt("min", limiteMin); } 
    if(request->hasParam("max")) { limiteMax = request->getParam("max")->value().toInt(); preferencias.putInt("max", limiteMax); } 
    request->send(200); 
  });

  server.on("/valor", HTTP_GET, [](AsyncWebServerRequest *request){ 
    int v = analogRead(potPin);
    digitalWrite(ledVerde, v >= limiteMin && v <= limiteMax);
    digitalWrite(ledVermelho, v < limiteMin || v > limiteMax);
    String json = "{\"val\":" + String(v) + "}";
    request->send(200, "application/json", json); 
  });
  
  server.begin();
}
void loop() {}
