/*
   Hello world web server
   circuits4you.com
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <math.h>
#include<time.h>
//#include "index.h" //Our HTML webpage contents
#include <ESP8266mDNS.h>        // Include the mD#include "FS.h"NS library
#include "FS.h"
#include <string>

//SSID and Password to your ESP Access Point
const char* ssid = "ESPWebServer";
const char* password = "0123456789";

// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

// defines variables
long duration;
float distance;
String html_org, html;

//float messwerte[1440];
float messwert[1440],chart[720];
float hoehe_min, hoehe_max;
float schauers[10];
float schauertotal = 0.0;
int len = (sizeof(messwert) / sizeof(messwert[0]));
int len_chart=720;

  /* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

ESP8266WebServer server(80); //Server on port 80

//==============================================================
//     This routine is executed when you open its IP in browser
//==============================================================
void handleRoot() {
  // String s = MAIN_page; //Read HTML contents
  // server.send(200, "text/html", s); //Send web page
  Website(messwert, schauers, schauertotal);
  server.send(200, "text/HTML", html); //Send web page
  }
//==============================================================
//     load requested data
//==============================================================
 void handleWebRequests(){
  if(loadFromSpiffs(server.uri())) return;
  String message = "File Not Detected\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  Serial.println(message);
}

bool loadFromSpiffs(String path){
  String dataType = "text/plain";
  if(path.endsWith("/")) path += "index.htm";
 
  if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
  else if(path.endsWith(".html")) dataType = "text/html";
  else if(path.endsWith(".htm")) dataType = "text/html";
  else if(path.endsWith(".css")) dataType = "text/css";
  else if(path.endsWith(".js")) dataType = "application/javascript";
  else if(path.endsWith(".png")) dataType = "image/png";
  else if(path.endsWith(".gif")) dataType = "image/gif";
  else if(path.endsWith(".jpg")) dataType = "image/jpeg";
  else if(path.endsWith(".ico")) dataType = "image/x-icon";
  else if(path.endsWith(".xml")) dataType = "text/xml";
  else if(path.endsWith(".pdf")) dataType = "application/pdf";
  else if(path.endsWith(".zip")) dataType = "application/zip";
  File dataFile = SPIFFS.open(path.c_str(), "r");
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
  }
 
  dataFile.close();
  return true;
}


//===============================================================
//                  replace Website Placeholder
//===============================================================
void Website(float messwert[], float schauers[], float schauertotal)
{
  //replace Placeholder in html
  html=html_org;
 html.replace("&SCHAUER_PH", String(schauers[9]));
 html.replace("&GESAMT_PH", String(schauertotal));
 html.replace("&FUELLSTAND_PH", String(messwert[len - 1]));
 html.replace("&hoehe_PH", String(String(hoehe_min)+"-"+String(hoehe_max)));
  Serial.println(html);
}

//===============================================================
//                  Chart generation
//===============================================================
String chart_string(float chart[]){
  //zeit =0
  float time =0.0;
  //inkrement der Messung
int  inkrement = 6;//h
  String chartdata="data: [{ \n";
  int j=0;
  int day=0;
  int hour = 0;
  
  for (int i=len_chart; i>=0 ; i--){
    chartdata += "x: new Date"; //(2019,9,16,22,2,0,0),
    chartdata += "(2019,9,";
    //datum bestimmen
    hour+=inkrement;
    if (hour>=24){
      hour =hour-24;
      day+=1;
    }
    chartdata +=String(day)+String(hour);
    chartdata+=",2,0,0),";
    chartdata+="\n y:"+String(chart[i]);
    chartdata +="\n }, { \n";
  }
  chartdata += "}]";
  return chartdata;
}



//===============================================================
//                     LOOP
//===============================================================
void loop(void) {
  server.handleClient();          //Handle client requests
  float schauer_V = 0;
  float V, distance = Volumen();


//hoehen MIN/MAX
  if(distance>hoehe_max){
    hoehe_max=distance;
  }
  if(distance<hoehe_min){
    hoehe_min = distance;
  }
//Schauer bestimmung
  if (V > messwert[len - 1]) { // wenn erfüllt ist wohl regen
    schauer_V += V - messwert[len - 1]; // hier noch regendauer hinzufügen
  }
  else {
    for (int i = 0; i < 10; ++i)//
    {
      schauers[i] = schauers[i + 1];
    }
    schauers[len - 1] = schauer_V;
    schauertotal += schauer_V;
    schauer_V = 0.0;
  }

  //Anpassung des Mess-Arrays
  int j=0;
  for (int i = 0; i < len - 1; ++i) // geht nur bis len -2
  {
    messwert[i] = messwert[i + 1];
        j=j+1;
  }
  messwert[0] = V; //i-ter wert wird durch i+1 ersetzt
  //Serial.print(time(time_t *tp));
  
      //Anpassung des Chart-Arrays
      if(j=2);//jetzt sind 6 h vergangen
      {
        j=0;
        
  for (int i = 0; i < len_chart - 1; ++i) // geht nur bis len -2
  {
    chart[i] = chart[i + 1];
  }
  chart[0] = messwert[0]; //i-ter wert wird durch i+1 ersetzt
  //Serial.print(time(time_t *tp));
  
      }


  String chart_stuff=chart_string( chart);
  //Serial.print(chart_stuff);
  delay((1 * 5 * 1000)); // warte für eine Minute!!!!!!!!!!!!
}
//===============================================================
//                  SETUP
//===============================================================
void setup(void) {
  //start Dateisystem
  SPIFFS.begin();


  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  Serial.begin(115200);
  Serial.println("");
  Serial.println("test");
  WiFi.mode(WIFI_AP);           //Only Access point
  WiFi.softAP(ssid, password);  //Start HOTspot removing password will disable security
  WiFi.softAPConfig(local_ip, gateway, subnet);

  IPAddress myIP = WiFi.softAPIP(); //Get IP address
  Serial.print("HotSpt IP:");
  Serial.println(myIP);

  server.on("/", handleRoot);      //Which routine to handle at root location
   server.onNotFound(handleWebRequests); //Set setver all paths are not found so we can handle as per URI
  server.begin();                  //Start server
  Serial.println("HTTP server started");
  Serial.println("test");


  if (!MDNS.begin("regentonne")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started, doesnt work on android");
    
  html_org = readHtmlFile(); //einlesen der HTML

}
//===============================================================
//                 HTML einlesen
//===============================================================

String readHtmlFile() {
  File  file = SPIFFS.open("/NiederschlagChart.html", "r");
  if (!file) {
    Serial.println("file open failed");
  }
  html_org = file.readString();
  //Serial.println(html);
  file.close();
  return html_org;
}


//===============================================================
//                 ABSTAND1
//==============================================================
float Abstand() {
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  float  distance_sum = 0.0;
  float  distance_real = 0.0;
  delayMicroseconds(2);
  for (int n = 0; n < 10; n++)
  {
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculating the distance
    distance = 0.0;
    distance = duration * 0.0001715; // nochmal Constante Berechnen!
    distance_sum = distance + distance_sum;
    delay(10);
    //controll ausgabe
    //Serial.print("Distance:");
    //Serial.println(distance);
  }
  distance_real =   distance_sum / 10.0;
  Serial.print("Distance ueber 10 gemittelt:");
  Serial.println(distance_real);
  //Serial.print("zeit:\n");
  time_t now;
  now = time(0);
  // Serial.println(ctime(&now));
  return distance_real;
}
//===============================================================
//                  Volumen
//===============================================================

float Volumen() {
  distance = Abstand();
  // hier wird das Volumen berechnet
  // ********Definitionen******
  // Offset wählen, dass wenn Tank leer (V=0) hoehe =0
  // bzw bei V=max muss h=max sein
  // in m
  float Offset = 1.0;
  //Radius Tank in m
  float r = 1.0;
  //laenge Tank in m
  float l = 2.5;

  float hoehe =Offset- distance;
  float s = 2.0 * sqrt(2 * r * hoehe - pow(hoehe, 2 ));
  float b = r * 2 * acos(1 - hoehe / r);
  //float A = l*b + s * l + 2 * S;
  float S = r * b / 2 - s * (r - hoehe) / 2;
  float V = S * l;
  Serial.print("Volumen in m3");
  Serial.println(V);
  return V, distance;
}
