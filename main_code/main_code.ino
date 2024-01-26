#include <WiFi.h>
#include "MAX30105.h"
#include <Adafruit_MLX90614.h>
#include <Wire.h>

uint32_t red[110];
uint32_t ir[110];
const char *ssid =  "naruto";                                    
const char *pass =  "narutouzumaki";
char jsonBuffer[6000] = "{\"write_api_key\":\"VB7UCU77QOZZPU1S\",\"updates\":[";
WiFiClient client;
char server[] = "api.thingspeak.com";
unsigned long deltaT = 1;

const int MPU = 0x68; // MPU6050 I2C address
float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float elapsedTime, currentTime, previousTime;
int c = 0;
int button = 0;
float prevTime = 0, pTime = 0;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
MAX30105 particleSensor;

void setup() {
    Serial.begin(9600);
    Serial.println("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");
    pinMode(23,INPUT);
//////////////////////////////////////////MAX sensor code/////////////////////////////////////////////////////////////////////////////////////
  if (particleSensor.begin() == false)
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  particleSensor.setup();

////////////////////////////////////////temperature sensor////////////////////////////////////////////////////////////////////////////////////
  if (!mlx.begin()) {
    Serial.println("Error connecting to MLX sensor. Check wiring.");
    while (1);
  };

///////////////////////////////////////MPU6050////////////////////////////////////////////////////////////////////////////////////////////////
  Wire.begin();                 
  Wire.beginTransmission(MPU);       
  Wire.write(0x6B);                 
  Wire.write(0x00);                  
  Wire.endTransmission(true);       

}

void loop() {
  button = digitalRead(23);
  if(button == 1) {
    Serial.println("Place finger on the sensor");
    delay(3000);
    button = digitalRead(23);
  }

  if(button == 1) {
    sendData((float)mlx.readObjectTempF());
  }
  else Serial.println("Reading aborted");

// Accelerometer and gyroscope output

 Wire.beginTransmission(MPU);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); 

  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
 
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)
 
  previousTime = currentTime;      
  currentTime = millis();            
  elapsedTime = (currentTime - previousTime) / 1000; 
  Wire.beginTransmission(MPU);
  Wire.write(0x43); 
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true); 
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0; 
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroX = GyroX + 0.56; 
  GyroY = GyroY - 2; 
  GyroZ = GyroZ + 0.79; 
  gyroAngleX = gyroAngleX + GyroX * elapsedTime; 
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
  yaw =  yaw + GyroZ * elapsedTime;
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  
  Serial.print(AccX);
  Serial.print("/");
  Serial.print(AccY);
  Serial.print("/");
  Serial.println(AccZ);

}
//sends data in bulk and then sends over temperature reading.
void updatesJson(char* jsonBuffer,uint32_t x, uint32_t y){

  strcat(jsonBuffer,"{\"delta_t\":");
 
  size_t lengthT = String(deltaT).length();
  char temp[9];
  String(deltaT).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");

  strcat(jsonBuffer, "\"field1\":");
  lengthT = String(x).length();
  String(x).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,",");

  strcat(jsonBuffer, "\"field2\":");
  lengthT = String(y).length();
  String(y).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,"},");

}

void httpRequest(char* data) {

  client.stop();
  String data_length = String(strlen(data)+1); 
  Serial.println(data);
 
  if (client.connect(server, 80)) {
    client.println("POST /channels/2394086/bulk_update.json HTTP/1.1"); // 
    client.println("Host: api.thingspeak.com");
    client.println("User-Agent: mw.doc.bulk-update (Arduino ESP8266)");
    client.println("Connection: close");
    client.println("Content-Type: application/json");
    client.println("Content-Length: "+data_length);
    client.println();
    client.println(data);
  }
  else {
    Serial.println("Failure: Failed to connect to ThingSpeak");
  }
  delay(250); 
  client.parseFloat();
  String resp = String(client.parseInt());
  Serial.println("Response code:"+resp); 
  delay(1000);
}

void sendData(float temperature) {
  Serial.print("Object temperature is : " + String(temperature));

  for(int i = 0; i < 101; i++) {
    red[i] = particleSensor.getRed();
    ir[i] = particleSensor.getIR();
    Serial.print(red[i]);
    Serial.print(",");
    Serial.println(ir[i]);
  }

  for(int i = 0; i < 100; i++) updatesJson(jsonBuffer,ir[i],red[i]);
  
  //last entry for temperature
  strcat(jsonBuffer,"{\"delta_t\":");
  size_t lengthT = String(deltaT).length();
  char temp[7];
  strcat(jsonBuffer,"1");
  strcat(jsonBuffer,",");
  strcat(jsonBuffer, "\"field3\":");
  lengthT = String(temperature).length();
  String(temperature).toCharArray(temp,lengthT+1);
  strcat(jsonBuffer,temp);
  strcat(jsonBuffer,"}]}");
  httpRequest(jsonBuffer);
}
