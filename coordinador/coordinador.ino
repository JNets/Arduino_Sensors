//Libreria I2C
#include <Wire.h>

//Libreria SPI
#include <SPI.h>

//Libreria Ethernet
#include <Ethernet.h>

//Direccion i2C del RTC
#define RTC_I2C_ADDRESS 0x68 

//Mac del Ethernet Shield
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

//Intentos de conexion usando DHCP
int dhcpIntents = 3;

//Ip asignada sino se asigna por DHCP
IPAddress ip(192, 168, 0, 10);

//Ip del servidor por defecto
IPAddress server(192, 168, 0, 100);

//Objeto EthernetClient
EthernetClient client;

//Objeto EthernetUdp
EthernetUDP Udp;

//Puerto Tcp del Servidor
int tcpPort = 80;

//Puerto Udp del Servidor
int udpPort = 10102;

//Tiempo de busqueda del servidor
unsigned long findServerTimeOut = 2000;


//******************** Inicio Clase RtcTime ************************
class RtcTime {
  public:
    RtcTime() { //Constructor

    }
    byte second;
    byte minute;
    byte hour;
    byte dayOfWeek;
    byte dayOfMonth;
    byte month;
    byte year;
    void getRtc();
    void setRtc();
    byte decToBcd(byte);
    byte bcdToDec(byte);
};

void RtcTime::getRtc() {
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.requestFrom(RTC_I2C_ADDRESS, 7);
  byte bytes[7];
  int i = 0;
  while (Wire.available()) {
    bytes[i] = Wire.read();
    i++;
  }
  if (i == 7) {
    second      = bcdToDec(bytes[0] & 0x7f);
    minute      = bcdToDec(bytes[1]);
    hour        = bcdToDec(bytes[2] & 0x3f);
    dayOfWeek   = bcdToDec(bytes[3]);
    dayOfMonth  = bcdToDec(bytes[4]);
    month       = bcdToDec(bytes[5]);
    year        = bcdToDec(bytes[6]);
  }
}

void RtcTime::setRtc(){
  Wire.beginTransmission(RTC_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

byte RtcTime::decToBcd(byte val) {
  return ((val / 10 * 16) + (val % 10));
}

byte RtcTime::bcdToDec(byte val) {
  return ((val / 16 * 10) + (val % 16));
}

//******************** fin Clase RtcTime ************************


//Objeto RtcTime
RtcTime dateRtc;


boolean asignarIpDhcp(){
  int intents = dhcpIntents;
  Serial.println(F("\nInicializando Conexion Ethernet"));
  while(--intents >= 0){            
    if(Ethernet.begin(mac)){
      Serial.println(F("Conexion inicializada correctamente por DHCP"));
      ip = Ethernet.localIP();
      Serial.print(F("Se asigna la IP "));
      Serial.println(ip);     
      return true;
    }else{
      Serial.println(F("Fallo en la conexion por DHCP"));
      if(dhcpIntents > 0){
        Serial.println(F("Intentando nuevamente ..."));  
      }
    }
  }
  Serial.println(F("Imposible realizar conexion por DHCP"));
  Ethernet.begin(mac, ip);
  Serial.print(F("Usando IP Statica "));
  Serial.println(ip);
  return false;
}

boolean buscarServidor(){  
  IPAddress broadcast(ip[0], ip[1], ip[2], 255); 
  
  Udp.beginPacket(broadcast, udpPort);
  Udp.write("Hello Sensorino!");
  Udp.endPacket();

  Serial.print(F("\nIniciando busqueda de Sensorino: "));
  Serial.print(broadcast);
  Serial.print(", ");
  Serial.println(udpPort);
  
  char packetBuffer[50];
  unsigned long startTime = millis();
  
  while(millis() - startTime < findServerTimeOut){
    int packetSize = Udp.parsePacket();
    if(packetSize > 0){
      packetBuffer[packetSize] = '\0';      
      Udp.read(packetBuffer, packetSize);
      Serial.print("Udp packet: ");
      Serial.print(packetSize);
      Serial.print(" bytes, data: ");
      Serial.println(packetBuffer);
    }
    if(packetSize == 21){         
      if(strcmp(packetBuffer, "Hi! Sensorino is here") == 0){
        server = Udp.remoteIP();
        Serial.print(F("Sensorino Encontrado! IP: "));        
        Serial.println(server);
        return true;
      }
    }
  }  
  Serial.println(F("Sensorino no se ha localizado"));        
  Serial.print(F("Se asume que su direccion es: "));
  Serial.println(server);
  return false;
}

boolean verifyConnection(){
  static unsigned long lastTime = 0;
  static unsigned int failCount = 0;
  if(client.connected()){
    return true;
  }
  else if(millis() - lastTime > 2000){
    lastTime = millis();    
    client.stop();    
    Serial.println(F("Verificando conexion..."));
    Serial.print(F("Conectando al servidor: "));
    Serial.print(server);
    Serial.print(F(", Puerto: "));
    Serial.println(tcpPort);
    if (client.connect(server, tcpPort) == 1) {
      Serial.println("connectado!");   
      return true;
    }else{
      if(!buscarServidor()){
        if(++failCount > 10){
          failCount = 0;
          asignarIpDhcp();
        }
      }
    }
  }  
  return false;  
}

//Metodo setup()
void setup(){
  //Se inicializa Serial 1
  Serial1.begin(9600);
  //Se inicializa Serial
  Serial.begin(115200);
  //Se inicializa I2C
  Wire.begin();
  delay(5000);
  //Inicializar conexion
  asignarIpDhcp();
  //Inicializar Udp
  Udp.begin(udpPort);
}


String addSensorTime(String sensor){
  dateRtc.getRtc();       
  String sensorWithTime = sensor + ",\"time\":\"" + 
    String(dateRtc.dayOfMonth) + String("/") + 
    String(dateRtc.month)  + String("/") +
    String(dateRtc.year)   + String(" ") +
    String(dateRtc.hour)   + String(":") +
    String(dateRtc.minute) + String(":") +
    String(dateRtc.second) + String("\"");  
    Serial.print("Se adicionó tiempo del sensor: ");
    Serial.println(sensorWithTime);
    return sensorWithTime;
}

void sendSensor(String sensor){  
  Serial.println("Enviando sensor al servidor ....");
  String request =  String("POST /api/sensors HTTP/1.1\nContent-Length:") + 
                    String(sensor.length() + 2) + 
                    String("\nContent-Type:JSON") +
                    String("\nConnection: keep-alive\n\n{") +
                    sensor + String("}");
  client.print(request);
  Serial.println(request);
  Serial.println();
  unsigned long startTime = millis();
  boolean newLine = false;
  while(millis() - startTime < 100){
    if(client.available()){
      char c = client.read();
      Serial.write(c);   
      if(c == '\n'){
        if(newLine){
          break;
        }else{
          newLine = true;
        }
      }else{
        newLine = false;
      }
    }            
  }   
}

void readSensor(){
  static boolean onReadingSensor = false;
  static int i = 0;
  static char buffer[500];
  while(Serial1.available()){
    char c = Serial1.read();
    if(onReadingSensor){      
      if( c == '}'){
        onReadingSensor = false;
        buffer[i] = '\0';
        Serial.print("Sensor recibido: ");
        Serial.println(buffer);        
        sendSensor(addSensorTime(String(buffer)));
        break;
      }else{
        buffer[i++] = c;
      }
    }else{
      if(c == '{'){
        Serial.println("leyendo sensor........");
        onReadingSensor = true;
        i = 0;        
      }
    }
  }
}

void parseCommand(char* com){
  int i = 0;
  while(com[i] != ')'){
    char c = com[i];
    if(c == '(' || c == ','){
      com[i] = '\0';
    }   
    i++;
  }
  com[i] = '\0';
}

char* commandArg(int arg, char* com){
  int eofCount = 0;
  int i = 0;
  while(true){
    if(com[i] == '\0'){
      if(eofCount == arg){
        while(com[++i] == ' ');
        return com + i;
      }
      eofCount++;
    }
    i++;
  }
}

void executeCommand(char* com){  
  Serial.print(F("Ejecutando comando: "));
  parseCommand(com);  
  Serial.println(com);
  if(strcmp(com, "setIp") == 0){    
    byte newIp[4];
    newIp[0] = atoi(commandArg(0, com)); 
    newIp[1] = atoi(commandArg(1, com));
    newIp[2] = atoi(commandArg(2, com));
    newIp[3] = atoi(commandArg(3, com));
    setIp(newIp);
  }
}

void readCommand(){
  #define COM_SIZE 100
  static char command[COM_SIZE];
  static byte i = 0;
  static boolean reading = false;
  while(Serial.available()){
    char c = Serial.read();
    if(reading){
      if(i < COM_SIZE){
        command[i++] = c;
      }else{
        reading = false;
        return;
      }
      if(c == ')'){
        command[i] = '\0';
        reading = false;
        Serial.println(F("Comando recibido...."));
        executeCommand(command);
      }
    }else{
      if(c == '_'){
        i = 0;
        reading = true;
        Serial.println(F("Leyendo Comando...."));
      }
    }
  }  
}

void setIp(byte newIp[4]){
  IPAddress _newIp(ip[0], ip[1], ip[2], ip[3]);
  ip = _newIp;
  Serial.print(F("Estableciendo IP Estatica : "));
  Serial.println(_newIp);
  Ethernet.begin(mac,_newIp);
}

void loop(){  
  readCommand();
  if(verifyConnection()){
    readSensor();    
  }
}
