#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <NeoPixelBus.h>
#include <WiFiManager.h>
#include <EEPROM.h>


#define light_name "lucs strip" //default light name
#define lightsCount 1
#define pixelCount 90

uint8_t gHue = 0;
// if you want to setup static ip uncomment these 3 lines and line 72
//IPAddress strip_ip ( 192,  168,   10,  95);
//IPAddress gateway_ip ( 192,  168,   10,   1);
//IPAddress subnet_mask(255, 255, 255,   0);
int lmin;
int lmax;

uint8_t rgb[lightsCount][3], bri[lightsCount], sat[lightsCount], color_mode[lightsCount], scene;
bool light_state[lightsCount], in_transition;
int ct[lightsCount], hue[lightsCount];
float step_level[lightsCount][3], current_rgb[lightsCount][3], x[lightsCount], y[lightsCount];
byte mac[6];
//String lstate = 'nothing';
String lstate = ""; 
ESP8266WebServer server(80);
int currentnr = 1;
RgbColor red = RgbColor(255, 0, 0);
//RgbColor green = RgbColor(0, 255, 0);
RgbColor green = RgbColor(255, 18, 0);
RgbColor white = RgbColor(255);
RgbColor black = RgbColor(0);
RgbColor variable = RgbColor(255, 18, 0);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(pixelCount);


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void infoLight(RgbColor color) {
  // Flash the strip in the selected color. White = booted, green = WLAN connected, red = WLAN could not connect
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, color);
    strip.Show();
    delay(10);
    strip.SetPixelColor(i, black);
    strip.Show();
  }
}

void setup() {
randomSeed(312312319);
  strip.Begin();
  strip.Show();
  EEPROM.begin(512);
//IPAddress ip;
//IPAddress gateway;
//IPAddress netmask;



 // WiFi.config(ip.fromString("192.168.178.137"), gateway.fromString("192.168.178.1"), netmask.fromString("255.255.255.0"));

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(120);
  wifiManager.autoConnect(light_name);

  if (! light_state[0]) {
    infoLight(white);
    while (WiFi.status() != WL_CONNECTED) {
      infoLight(red);
      delay(500);
    }
    // Show that we are connected
    infoLight(green);

  }

  WiFi.macAddress(mac);
  ArduinoOTA.begin();
ArduinoOTA.setHostname("esp8266");

  server.on("/detect", []() {
    server.send(200, "text/plain", "{\"hue\": \"strip\",\"lights\": " + (String)lightsCount + ",\"name\": \"" light_name "\",\"modelid\": \"Wqrld\",\"mac\": \"" + String(mac[5], HEX) + ":"  + String(mac[4], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[0], HEX) + "\"}");
  });
  server.on("/lstate", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/json", "{\"state\": \"" + lstate + "\"}");
  });
  server.on("/setstate", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    lstate = server.arg("state");
    server.send(200, "text/plain", "Worked");
  });

    server.on("/select", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    lmin = server.arg("min").toInt();
    lmax = server.arg("max").toInt();
    server.send(200, "text/plain", "Worked");
  });
  
  server.on("/setcolor", []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    //server.args("r")
    //server.args("g")
    //server.args("b")
    variable = RgbColor(server.arg("r").toInt(), server.arg("g").toInt(), server.arg("b").toInt());
    server.send(200, "text/plain", "Worked");
  });
  //http://192.168.178.137/setcolor?r=225&=20&b=255
  server.on("/", []() {
server.sendHeader("Access-Control-Allow-Origin", "*");
 /*   if (server.hasArg("reset")) {
      ESP.reset();
    }
    if (server.hasArg("green")) {
      lstate = "green";
    }

    if (server.hasArg("variable")) {
      lstate = "variable";
    }
    
        if (server.hasArg("none")) {
      lstate = "";
    }*/
    String http_content = "<!doctype html>";
    http_content += "<html>";
    http_content += "<head>";
    http_content += "<meta charset=\"utf-8\">";
    http_content += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
    http_content += "<title>Light Setup</title>";
    http_content += "<link rel=\"stylesheet\" href=\"https://unpkg.com/purecss@0.6.2/build/pure-min.css\">";
    http_content += "</head>";
    http_content += "<body>";
    http_content += "<a class=\"pure-button  pure-button-primary\" href=\"/?green=true\">green</a>";
    http_content += "<a class=\"pure-button  pure-button-primary\" href=\"/?variable=true\">variable</a>";
    http_content += "<a class=\"pure-button  pure-button-primary\" href=\"/?none=true\">none</a>";
    http_content += "<a class=\"pure-button  pure-button-primary\" href=\"/?reset=true\">reset</a>";
    http_content += "<h1>Coming to you</h1>";
    http_content += "light state:" + lstate;
    http_content += "<input type=\"color\" id=\"html5colorpicker\" onchange=\"clickColor()\" value=\"#ff0000\" style=\"width:85%;\">";
    http_content += "</body>";
    http_content += "<script>function clickColor(){fetch(\"/?color=\" + document.getElementById(\"html5colorpicker\"\")}";
    http_content += "</html>";


    server.send(200, "text/html", http_content);

  });

  server.onNotFound(handleNotFound);

  server.begin();
}
void flow(){
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, variable);
    strip.Show();
    delay(10);
    strip.SetPixelColor(i, black);
    strip.Show();
  }
  delay(1000);
}
void solid(){
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, variable);
    strip.Show();
  }
  delay(1000);
}
void twinkle(){
  int i = random(1, pixelCount);
      strip.SetPixelColor(i, variable);
    strip.Show();
    delay(random(100, 200));
        strip.SetPixelColor(i, black);
    strip.Show();
    delay(random(50, 1200));
}
void clean(){
  for (int i = 0; i < pixelCount; i++)
  {
    strip.SetPixelColor(i, RgbColor(0, 0, 0));
    strip.Show();
  }
  delay(2000);
}

void specific(){
    for (int i = 0; i < pixelCount; i++)
  {
    if(i > lmin && i < lmax){
    strip.SetPixelColor(i, variable);
    }
    else{
      strip.SetPixelColor(i, RgbColor(0, 0, 0));
    }
    strip.Show();
  }
}


void lines(){
strip.SetPixelColor(currentnr, variable);
strip.Show();
for (int i = 0; i < 88; i++){
currentnr = currentnr + 1;
strip.SetPixelColor(currentnr, variable);
strip.SetPixelColor(currentnr - 2, black);
  delay(300);
}
 delay(1000); 
}
// 90 leds




void hu(){
  for(int i = 0; i < pixelCount; i++){
    RgbColor newvar = variable;
  uint8_t ran = random(-10, 10);
  if(ran >= 0){
newvar.Lighten(ran);
  }else{
    ran = ran + 10;
    newvar.Darken(ran);
  }
    strip.SetPixelColor(i, newvar);
    
  }
strip.Show();
  delay(1000);
}
void meteor(){
  int len = random(3, 20);
  int i = random(0, pixelCount - len);
  
  for(int v = 0; v < len; v++){
  strip.SetPixelColor(i + v, variable);
  
  delay(100);
  strip.Show();
  }
  for(int v = 0; v < len; v++){
  strip.SetPixelColor(i + v, black);
  delay(100);
  strip.Show();
  }
  delay(random(500, 5000));
}

void noise(){
    
  for (int i = 0; i < pixelCount; i++)
  {
   int incr = random(-30, 0) - 30;
    strip.SetPixelColor(i, RgbColor(variable.R + incr, variable.G + incr, variable.B + incr));
 //   strip.SetPixelColor(i, black);
    strip.Show();
  }
  delay(50);
}


void lightengine(){
if(lstate == "flow"){
  flow();
}
  else if(lstate == "solid"){
  solid();
}
  else if(lstate == "none"){
  clean();
}
  else if(lstate == "specific"){
  specific();
}
  else if(lstate == "noise"){
  noise();
}


  else if(lstate == "meteor"){
    clean();
  meteor();
}
  else if(lstate == "hue"){
    hu();

}
  else if(lstate == "lines"){
    lines();

}

  else if(lstate == "twinkle"){
    clean();
  twinkle();
}

}





void loop() {
  
//  patterns[currentPatternIndex].pattern();

  //FastLED.show();


  
  lightengine();
  ArduinoOTA.handle();
  server.handleClient();
  
}
