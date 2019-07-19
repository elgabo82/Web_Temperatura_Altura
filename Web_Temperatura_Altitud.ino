/******************************************************************
                                        
 Web_Temperatura_Altitud
 
 Permite monitorear la temperatura y la altura, conectandose por
 defecto a una red que se debe agregar, o bien presionando el boton
 de Switch/IO25 para cambiar a una red propia con DHCP.
 
 Fecha: 17-07-2019                       
 gabrielmorejon@gmail.com                                      
 gabriel.morejon@grupofmo.com                                  
 Script de Libre Uso                                           
 GNU/GPL v3                                                    
                                                               
 https://www.grupofmo.com
 Gabriel Eduardo Morejon Lopez                                 
 Portoviejo/Ecuador                                            
                                                               
                                                               
 Yubox                                             
 Edgar Landivar                                                
 https://www.yubox.com                                         
                                                               
 
 Rui Santos
 Original en: https://randomnerdtutorials.com 
 
 Se debe mejorar la funcionalidad
                                                                      
******************************************************************/



#include <SPI.h>
#include <DNSServer.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BMP085_U.h>
#include <WiFi.h>
#include <Wire.h>

const char* nombre_red = "RED"; //Nombre de la red a la cual se va a conectar de forma predeterminada
const char* clave_red = "CLAVE"; //Clave de la red

#define SEALEVELPRESSURE_HPA (1012.00)

Adafruit_BMP280 bmp; // I2C
String estado26 = "off";
String estado27 = "off";

// Assign output variables to GPIO pins
const int salida26 = 26; //PIN 26
const int salida27 = 27; //PIN 27
unsigned long retardoTiempo;
float temp, pres, hume;
String temperaturaweb;
String alturaweb;
const float altura = 0;
int val = 0;     // variable para la lectura de los valores

const byte DNS_PORT = 53;
IPAddress apIP(210, 110, 10, 254);
DNSServer servidorDNS;
String direccion_ip_modoap;

String responseHTML_ModoAP = ""
  "<!DOCTYPE html><html><head><title>CaptivePortal</title></head><body>"
  "<h1>Hello World!</h1><p>This is a captive portal example. All requests will "
  "be redirected here.</p></body></html>";


WiFiServer servidorWeb(80); // Set web server port number to 80
String header; // Variable para almacenar la peticion HTTP

void setup() { 


  Serial.begin(115200);
  pinMode(salida26, OUTPUT);
  pinMode(salida27, OUTPUT);
  // Ajusta las salidas a LOW
  digitalWrite(salida26, LOW);
  digitalWrite(salida27, LOW);
  conectarRED(); //Por defecto inicia conectando a una red predeterminada
  
   
  Serial.println(F("BMP280 test"));

  // Initialising the UI will init the display too.
  //display.init();
  //display.flipScreenVertically();
  //display.setFont(ArialMT_Plain_10);

  bool status;
    
  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bmp.begin(0x76);  //I2C address can be 0x77 or 0x76 (by default 0x77 set in library)
  if (!status) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  servidorWeb.begin();
    
  Serial.println("-- Default Test --");
  retardoTiempo = 1000;

  Serial.println();

  // put your setup code here, to run once:
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(27, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(34, OUTPUT);
  
  pinMode(25, INPUT); // declare pushbutton as input
}


void loop() { 
  //display.clear(); // Clear al display
  
  val = digitalRead(25);  // Lee la entrada
  if (val == LOW) {         // Revisa que la entrada sea LOW (boton liberado)
    //Serial.println("Boton presionado - usando el Board nuevo"); 
    //18-07-2019 20:58 Se añade la logica para cambiar de modo cliente a modo AP.
    cambiarModoRed();
    procesoDNS();
    //IPAddress arr_ip_addr = WiFi.localIP();
    //Serial.println(arr_ip_addr); // Muestra en el serial la IP 
    delay(1000);
  } else {
    IPAddress arr_ip_addr = WiFi.localIP();
    Serial.println("IPv4: ");
    
    if (direccion_ip_modoap == "") {
      Serial.println(arr_ip_addr);
    }
    else if (direccion_ip_modoap != "") {
      Serial.println(direccion_ip_modoap);
    }
    delay(1000);
    
    //Serial.println("No presionado"); 
    mostrarValores();    
  }
  
  WiFiClient client = servidorWeb.available();   // Escucha el servidor por peticiones de clientes
  
  
  if (client) {                             // Si un cliente nuevo se conecta
    Serial.println("Cliente nuevo.");          // Se muestra el mensaje en el serial
    String currentLine = "";                // Para mantener los datos de entrada del cliente que se conecta
    while (client.connected()) {            // Mientras el cliente este conectado
      if (client.available()) {             
        char c = client.read();             
        Serial.write(c);
        header += c;
        if (c == '\n') {                    
          
          if (currentLine.length() == 0) {
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Enciende los GPIOs
            if (header.indexOf("GET /26/on") >= 0) {
              Serial.println("GPIO 26 ENCENDIDO");
              estado26 = "on";
              digitalWrite(salida26, HIGH);
            } else if (header.indexOf("GET /26/off") >= 0) {
              Serial.println("GPIO 26 off");
              estado26 = "off";
              digitalWrite(salida26, LOW);
            } else if (header.indexOf("GET /27/on") >= 0) {
              Serial.println("GPIO 27 on");
              estado27 = "on";
              digitalWrite(salida27, HIGH);
            } else if (header.indexOf("GET /27/off") >= 0) {
              Serial.println("GPIO 27 ENCENDIDO");
              estado27 = "off";
              digitalWrite(salida27, LOW);
            }
            
            // HTML
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS para el estilo de los botones            
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>Sensores de Temperatura y Altura v&iacute;a Web.</h1>");
            client.println("<p>GrupoFMO - <a href=\"https://www.grupofmo.com\">https://www.grupofmo.com</a></p>");
            client.println("<p><a href=\"mailto:gabriel.morejon@grupofmo.com\">gabriel.morejon@grupofmo.com</a></p>");
            client.println("<p><a href=\"mailto:gabrielmorejon@gmail.com\">gabrielmorejon@gmail.com</a></p>");
            
            client.println("<p><b>Temperatura:</b> " + temperaturaweb + " C</p>");
            client.println("<p><b>Altura aprox.:</b> " + alturaweb + " m</p>");
            
            
            // Muestra el estado actual del GPIO26 ENCENDIDO/APAGADO
            client.println("<p>GPIO 26 - Estado " + estado26 + "</p>");
            // Si su estado es off, se muestra el boton de ENCENDIDO
            if (estado26=="off") {
              client.println("<p><a href=\"/26/on\"><button class=\"button\">ENCENDER</button></a></p>");
            } else {
              client.println("<p><a href=\"/26/off\"><button class=\"button button2\">APAGAR</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 27  
            client.println("<p>GPIO 27 - Estado: " + estado27 + "</p>");
            // Si el estado es off, se puestra el boton de encendido
            if (estado27=="off") {
              client.println("<p><a href=\"/27/on\"><button class=\"button\">ENCENDER</button></a></p>");
            } else {
              client.println("<p><a href=\"/27/off\"><button class=\"button button2\">APAGAR</button></a></p>");
            }
            client.println("</body></html>");            
           
            client.println(); //Linea en blanco
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;    
        }
      }
    }
    // Limpia la variable de cabecera
    header = "";
    // Cierra la conexion
    client.stop();
    Serial.println("Cliente desconectado.");
    Serial.println("");
      
    }  

  
  delay(retardoTiempo);
}

void cambiarModoRed(){
  WiFi.mode(WIFI_AP); //Inicia en modo AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP("RED_SENSORES");
  IPAddress direccion_ip = WiFi.localIP(); //Obtiene la IP local
  Serial.println(direccion_ip);
  
  direccion_ip_modoap = String(direccion_ip);
  servidorDNS.start(DNS_PORT, "*", apIP);

  servidorWeb.begin();
}

void procesoDNS() {
  servidorDNS.processNextRequest();
  WiFiClient client = servidorWeb.available();   // Escuchar conexiones

  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.print(responseHTML_ModoAP);
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}


void mostrarValores() {

  temp = (float)bmp.readTemperature();
  //hume = (float)bmp.readHumidity();
  pres = (float)(bmp.readPressure() / 100.0F);

  temperaturaweb = String(temp);
  Serial.print("Temperatura = ");
  Serial.print(temp);
  Serial.println(" *C");

  Serial.print("Presion = ");

  Serial.print(pres);
  Serial.println(" hPa");

  Serial.print("Altitud Aprox.  = ");
  Serial.print(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  alturaweb = String(bmp.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");


  
  Serial.println();
}

void conectarRED() {
  WiFi.begin(nombre_red, clave_red);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Estableciendo conexion a la red WiFi..");
  }
  Serial.println("Conectado a la red: ");
  Serial.println(nombre_red);
  delay(1000);
}

