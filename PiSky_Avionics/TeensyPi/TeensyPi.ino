 /*
 * Raspberry Pi <-> Arduino Communication: Rocket Avionics
 * SSI 2015
 *
 * © SSI 2015
 */

/*
 * General layout of code:
 *
 * This code is the framework of the comunication protocol
 * between the Pi and the Arduino. The Arduino is in state 0 when
 * turned on. When it recieves a 1 from the pi, it enters state 1.
 * When it receives a read request while in state 1, it enters
 * sends a 1 back to the pi and enters state 2. When it gets a read request
 * in state 2, it sends the data that it has saved. If at any time it
 * recieves a 1, it enters state 1 agian, sends the 1 back to the pi, and
 * enters state 2.
 *
 *
 * The Pi will cycle through all connected i2c addresses
 * to obtain data from multiple boards. It will log and
 * process this data.
 */

#include <i2c_t3.h>
#include <Adafruit_10DOF.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>
#include <Adafruit_BMP085_U.h>
#include <Adafruit_L3GD20_U.h>

// Initialization for sensors
Adafruit_10DOF                dof   = Adafruit_10DOF();
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(30301);
Adafruit_LSM303_Mag_Unified   mag   = Adafruit_LSM303_Mag_Unified(30302);
Adafruit_BMP085_Unified       bmp   = Adafruit_BMP085_Unified(18001);
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;

//Assinging adresses for I2C
#define SLAVE_ADDRESS0 0x05
#define SLAVE_ADDRESS1 0x07
#define DEC_PRECISION  100

int state = 0;               //This is the state, which starts at 0
int sensorData[21];          //Array for storing sensor data
unsigned int index1 = 0;     //Index for sending sendor data

//union between a array of four bytes and a float, used for sending data
union u_tag{
  byte b[4];
  float fval;
} u;

//Initializatoin of Sensors
void initSensors()
{
  if (!accel.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println(F("Ooops, no LSM303 detected ... Check your wiring!"));
    while (1);
  }
  if (!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no LSM303 detected ... Check your wiring!");
    while (1);
  }
  if (!bmp.begin())
  {
    /* There was a problem detecting the BMP180 ... check your connections */
    Serial.println("Ooops, no BMP180 detected ... Check your wiring!");
    while (1);
  }
}

void setup() {
  sensorData[0] = 20; //The first value in the array gives the length of the arry 
  pinMode(LED_BUILTIN, OUTPUT); // LED
  Serial.begin(9600); // start serial for output
  // Init i2c given address
  Wire.begin(I2C_SLAVE, SLAVE_ADDRESS0, I2C_PINS_18_19, I2C_PULLUP_EXT, I2C_RATE_400);
  // i2c #2 on teensy
  Wire1.begin(I2C_SLAVE, SLAVE_ADDRESS1, I2C_PINS_29_30, I2C_PULLUP_EXT, I2C_RATE_400);

  Wire1.onReceive(receiveData);
  Wire1.onRequest(sendData);
  initSensors();
  Serial.println("Ready!");
}


/*
 * Collect the latest sensor data.
 */
void collectData() {
  sensors_event_t accel_event;
  sensors_event_t mag_event;
  sensors_event_t bmp_event;
  sensors_vec_t   orientation;

  /* Calculate pitch and roll from the raw accelerometer data */
  accel.getEvent(&accel_event);
  if (dof.accelGetOrientation(&accel_event, &orientation)) {
    /* 'orientation' should have valid .roll and .pitch fields */
    Serial.print(F("Roll: "));
    Serial.print(orientation.roll);
    Serial.print(F("; "));
    u.fval = orientation.roll;
    for (int ii = 0; ii < 4; ii++ ){
      sensorData[ii+1] = u.b[ii];
    }

    Serial.print(F("Pitch: "));
    Serial.print(orientation.pitch);
    Serial.print(F("; "));
    u.fval = orientation.pitch;
    for (int ii = 0; ii < 4; ii++ ){
      sensorData[ii+5] = u.b[ii];
    }
  }

  /* Calculate the heading using the magnetometer */
  mag.getEvent(&mag_event);
  if (dof.magGetOrientation(SENSOR_AXIS_Z, &mag_event, &orientation))
  {
    /* 'orientation' should have valid .heading data now */
    Serial.print(F("Heading: "));
    Serial.print(orientation.heading);
    Serial.print(F("; "));
    u.fval = orientation.heading;
    for (int ii = 0; ii < 4; ii++ ){
      sensorData[ii+9] = u.b[ii];
    }
  }

  /* Calculate the altitude using the barometric pressure sensor */
  bmp.getEvent(&bmp_event);
  if (bmp_event.pressure)
  {
    /* Get ambient temperature in C */
    float temperature;
    bmp.getTemperature(&temperature);
    /* Convert atmospheric pressure, SLP and temp to altitude    */
    Serial.print(F("Alt: "));
    Serial.print(bmp.pressureToAltitude(seaLevelPressure,
                                        bmp_event.pressure,
                                        temperature));
    float alt = bmp.pressureToAltitude(seaLevelPressure,
                                           bmp_event.pressure,
                                           temperature);
    Serial.print(F(" m; "));
    u.fval = bmp_event.pressure;
    for (int ii = 0; ii < 4; ii++ ){
      sensorData[ii+13] = u.b[ii];
    }
    
    /* Display the temperature */
    Serial.print(F("Temp: "));
    Serial.print(temperature);
    Serial.print(F(" C"));
    u.fval = temperature;
    for (int ii = 0; ii < 4; ii++ ){
      sensorData[ii+17] = u.b[ii];
    }
  }
  sensorData[0] = 20;
  Serial.println(F(""));
  delay(1);


  return;
}


void loop() {
  collectData();
}

/*
 * Read data from Pi
 */
void receiveData(size_t byteCount) {
  int number = 0;
  while (Wire1.available()) {
    number = Wire1.read();
    Serial.print("data received: ");
    Serial.println(number);

    if (number == 1) {
      state = 1;
      Serial.println("I HAVE BEEN ACTIVATED");
    }
    else {
      // figure out what we want to do here
    }
  }
}

/*
 * Send data to pi.
 * Either sends a '1' to activate or actual data.
 * 
 */
void sendData() {
  if (state == 1) {  //This is the initialization phase, after the pi sends a 1, now the teensy responds with a 1
    Wire1.write(1);
    digitalWrite(13, LOW);
    state = 2;
    return;
  }
  else if (state == 2) {
    Wire1.write(sensorData[index1]);
    index1 ++;
    // for loop for sending data
    if (index1 >= sizeof(sensorData) / sizeof(int)) {
      index1 = 0;
    }
    return;
  }
}
