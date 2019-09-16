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
#include <sstream> //für std::to_string workaround

//SSID and Password to your ESP Access Point
const char* ssid = "ESPWebServer";
const char* password = "12345678";

// defines pins numbers
const int trigPin = 2;  //D4
const int echoPin = 0;  //D3

// defines variables
long duration;
float distance;
std::string html;

// starte das Dateisystem


namespace patch  // für std::to_string workaround
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}

#include <iostream>



/* Put IP Address details */
IPAddress local_ip(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);



ESP8266WebServer server(80); //Server on port 80
//float messwerte[1440];
float messwert[5];
float schauers[10];
float schauertotal = 0.0;
int len = (sizeof(messwert) / sizeof(messwert[0]));




//==============================================================
//     This rutine is exicuted when you open its IP in browser
//==============================================================
void handleRoot() {
  // String s = MAIN_page; //Read HTML contents
  // server.send(200, "text/html", s); //Send web page
  String s = Website( messwert, schauers, schauertotal);
  server.send(200, "text/HTML", s ); //Send web page
  delay(5000);
}


//===============================================================
//                  WEBSITE
//===============================================================
String Website(float messwert[], float schauers[], float schauertotal)
{
  Serial.println("test:we bsite");
  Serial.println(messwert[1]);
  String  s = "Aktueller Fuellstand: \t";
  s += String(messwert[len - 1]);
  s += "\n Letzter Regenschauer: \t";
  s += String(schauers[9]);
  Serial.println(s);
  //server.send(200, "text/plain", s); //Send web page
  //return s;


  String S = "<html><head><title> Regetonnen NodeMCU </title></head> <body>";
  S += "<h2>Aktueller Fuellstand: \t </h2> <h1>";
  S += String(messwert[len - 1]) + "\t m^3" ;
  S += "</h1> \n <h2> letzter Schauer: \t</h2> <h1>";
  S += String(schauers[9]) + "\t m^3";
  S += "</h1> \n <h2>\n Gesamter input (Regen und Manuell): \t</h2> <h1>" + String(schauertotal) + "\t m^3" +
       S += " </body> </html>" ;

  html = ersetzten(html,"&FUELLSTAND_PH", patch::to_string(messwert[len - 1]));
 // html = ersetzten(html, "&SCHAUER_PH", schauers[9]]);
 // html = ersetzten(html, "&GESAMT_PH", schauertotal);

}

//===============================================================
//                 Stringersetzen
//==============================================================


void ersetzten(std::string input, std::string unterstring, std::string ersetzer)
{
  input.replace(input.find(unterstring), unterstring.length(), (ersetzer));
}




//===============================================================
//                     LOOP
//===============================================================
void loop(void) {
  Serial.print("Laenge Messwerte:");
  Serial.println(len);
  //  for (int i = 0; i < len; ++i){
  // messwert[i]=0.0;
  // }
  server.handleClient();          //Handle client requests
  float schauer_V = 0;
  float V = Volumen();

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

  for (int i = 0; i < len - 1; ++i) // geht nur bis len -2
  {
    messwert[i] = messwert[i + 1];
  }
  messwert[len - 1] = V; //i-ter wert wird durch i+1 ersetzt

  delay((1 * 5 * 1000)); // warte für eine Minute!!!!!!!!!!!!
}
//===============================================================
//                  SETUP
//===============================================================
void setup(void) {
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

  server.begin();                  //Start server
  Serial.println("HTTP server started");
  Serial.println("test");


  if (!MDNS.begin("regentonne")) {             // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started, doesnt work on android");


  html = readHtmlFile();

}

//===============================================================
//                 HTML einlesen
//===============================================================

std::string readHtmlFile() {
  File  file = SPIFFS.open("/zisterne.html", "r");
  if (!file) {
    Serial.println("file open failed");
  }
  html = file.readString();
  Serial.println(html);
  file.close();
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

  float hoehe = distance - Offset;
  float s = 2.0 * sqrt(2 * r * hoehe - pow(hoehe, 2 ));
  float b = r * 2 * acos(1 - hoehe / r);
  //float A = l*b + s * l + 2 * S;
  float S = r * b / 2 - s * (r - hoehe) / 2;
  float V = S * l;
  return V;
}
