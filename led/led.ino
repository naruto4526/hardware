#define LED 23

void setup() {
  // Set pin mode
  Serial.begin(9600);
  pinMode(LED,INPUT);
}

void loop() {
  delay(500);
  Serial.print("Pin value is: ");
  Serial.println(digitalRead(LED));
}