# Hardware Code for Iot
## File structure:
There is one 'main' file inside the 'main_code' folder. This is the code that will run on the ESP32. The other folder have examples that work with individual sensors
for reference.
## Connections:
There are three sensors that the code is written for. They are, 
- MPU6050
- MLX9614
- MAX30100
  
All of these sensors work with I2C, and are therefore connected to the I2C lines of the ESP32. Vcc and GND connections are also made.
