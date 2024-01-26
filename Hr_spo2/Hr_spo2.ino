#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
MAX30105 particleSensor;
const double pi = 3.1459;
const int mod = 1e3;
const int N = 32;
#define debug Serial //Uncomment this line if you're using an Uno or ESP
//#define debug SerialUSB //Uncomment this line if you're using a SAMD21

int32_t bufferLength = 100; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

double* dft(uint32_t inp[]) {
  double* dft_mag = (double*)calloc(N,sizeof(double));
  for(int n = 0; n < N; n++) {
    double real = 0;
    double imag = 0;
    for(int k = 0;k < N;k ++) {
      real += inp[k]*cos((2*pi*n*k)/N);
      imag += -1*inp[k]*sin((2*pi*n*k)/N);
    }
    dft_mag[n] = sqrt(real*real + imag*imag)/mod;
  }
  return dft_mag;
}

void print() {
  
  Serial.print(F(", HR="));
  Serial.print(heartRate, DEC);

  Serial.print(F(", HRvalid="));
  Serial.print(validHeartRate, DEC);

  Serial.print(F(", SPO2="));
  Serial.print(spo2, DEC);

  Serial.print(F(", SPO2Valid="));
  Serial.println(validSPO2, DEC);
}
void setup()
{
  debug.begin(9600);
  debug.println("MAX30105 Basic Readings Example");

  // Initialize sensor
  if (particleSensor.begin() == false)
  {
    debug.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  particleSensor.setup(); //Configure sensor with
}

void loop()
{
    for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample
   //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
  //print();
  while(1) {
      for(byte i = N; i < bufferLength; i++) {
        redBuffer[i - N] = redBuffer[i];
        irBuffer[i - N] = irBuffer[i];
      }
      for(byte i = bufferLength - N ; i < bufferLength; i++) {
        while (particleSensor.available() == false) //do we have new data?
          particleSensor.check(); //Check the sensor for new data

        redBuffer[i] = particleSensor.getRed();
        irBuffer[i] = particleSensor.getIR();
        particleSensor.nextSample(); //We're finished with this sample so move to next sample
      }
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
      //print();
      if(validHeartRate == 1) {
        //calculate dft for both the set of samples
        double* red = dft(redBuffer);
        double* ir = dft(irBuffer);
       Serial.println("dft is ");
        for(int j = 0; j < N; j++) {
          Serial.print(red[j]);
          Serial.print(",");
          Serial.println(ir[j]);
        }
        double red_ac = red[1],ir_ac = ir[1];
        for(int i = 6; i <= 12; i++) {
          red_ac = max(red_ac,red[i]);
          ir_ac = max(ir_ac,ir[i]);
        }
        double r_a2c = red_ac/red[0];
        double ir_a2c = ir_ac/ir[0];
        double R = (r_a2c)/(ir_a2c);
        double hb = -3.626*R + 15.838;
        double RBC = 0.380*hb - 0.295;
        double spO2 = 99.99 - 5.475*R;
        Serial.print("hb = ");
        Serial.print(hb);
        Serial.print(" RBC = ");
        Serial.print(RBC);
        Serial.print(" spO2 = ");
        Serial.print(spO2);
        free(red);
        free(ir);
      }
      else Serial.println("No finger");

  }
  debug.println();
}
