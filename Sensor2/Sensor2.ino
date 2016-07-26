#include <DHT.h>
int sensor = 7;
int temp, humedad;
DHT dht (sensor, DHT11);

void enviarSensor(char* id, int valor, char* unidad){
  Serial.print("{ \"id\":\"");
  Serial.print(id);
  Serial.print("\","); 
  Serial.print("\"val\":");
  Serial.print(valor,DEC);
  Serial.print(",\"unit\":\"");
  Serial.print(unidad);
  Serial.print("\"}");
}

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void loop() {
  temp = dht.readTemperature();
  enviarSensor("t2", temp, "C");
  delay(1000);
  
  humedad = dht.readHumidity();    
  enviarSensor("h2", humedad, "%");   
  delay(1000);
}
