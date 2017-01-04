#include <I2Cdev.h>
#include <MPU6050.h>

#include <Wire.h>
#include <MS5611.h>
#include <SD.h>

MS5611 ms5611;

double referencePressure;
const int chipSelect = 4;
unsigned long time;
float alt;   
File dataFile;
String xmlFile;
int buzzer = 6;
MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

bool err = false;

bool armCharges = false;
bool apogeeFire = false;
bool mainLock = false;
int mainCount = 0;
bool mainFire = false;
int apogeeFireTime;
int mainFireTime;
int apogeeCount = 0;
bool apogeeLock = false;
int stoc = 0;
bool sensorTimeout = false;
float lastAlt;

int apogeePin = 9;
int mainPin = 3;
int mainAlt = 250 * 0.3048;

int stage2Pin = 8;

void setup() 
{
  pinMode(mainPin, OUTPUT);
  pinMode(apogeePin, OUTPUT);
  pinMode(stage2Pin, OUTPUT);
  Serial.begin(9600); // start serial for output
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("Setup");
  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    err = true;
  }
  if(!loadSDFile()){
    Serial.println("File Failure.");
    err = true;
  }else{
    Serial.println("Done.");
  }

  // Initialize MS5611 sensor
  Serial.println("Initialize MS5611 Sensor");

  while(!ms5611.begin() && !sensorTimeout)
  {
    Serial.println("Could not find a valid MS5611 sensor, check wiring!");
    delay(500);
    stoc++;
    if(stoc = 5){
      sensorTimeout = true;
    }
    
  }

  // Get reference pressure for relative altitude
  referencePressure = ms5611.readPressure();

  // Check settings
  checkSettings();

  accelgyro.initialize();
  if(!accelgyro.testConnection()){
    Serial.println("Gyro Failed!");
    err = true;
  }

  if(err){
    analogWrite(buzzer, 97);
    delay(3000);
    analogWrite(buzzer, 0);
    Serial.println("Err!");
  }else{
    int j = 0;
    while(j<3){
      analogWrite(buzzer, 97);
      delay(500);
      analogWrite(buzzer, 0);
      delay(500);
      j++;
    }
    
  }
  
}

void checkSettings()
{
  Serial.print("Oversampling: ");
  Serial.println(ms5611.getOversampling());
}

void loop()
{
  uint32_t rawPressure = ms5611.readRawPressure();
  long realPressure = ms5611.readPressure();
  alt = ms5611.getAltitude(realPressure, referencePressure);
  accelgyro.getAcceleration(&ax, &ay, &az);
  Serial.print(alt);    
  Serial.println(" m");
  time = millis();
  writeSD(alt,time,ay,mainFire,apogeeLock);  //WRITE
  if(time > 120000){
    soundBuzzer(time);
    Serial.println("buzz");
  }
  if(armCharges){
    apogeeCheck();
  }else{
    if(alt > 67){
      armCharges = true;
      writeSD(0,0,0,mainFire,apogeeLock);
    }
  }
  lastAlt = alt;
  
}

void apogeeCheck(){
  if(alt < lastAlt){
    apogeeCount = apogeeCount + 1;
  } else {
    apogeeCount = 0;
  }
  if(apogeeCount > 2 || apogeeLock){
    apogeeLock = true;
    charges();
  }
  
}

void charges(){
  if(!apogeeFire){
    apogeeFire = true;
    apogeeFireTime = time;
    digitalWrite(apogeePin, HIGH);
  }else{
    if(apogeeFireTime + 3000 < time){
        digitalWrite(apogeePin, LOW);
    }
  }
  if(mainAlt > alt){
    mainCount++;
    if(mainCount == 3){
      mainLock = true;
    }
  }else{
    mainCount = 0;
  }
  if(mainLock){
    if(!mainFire){
      mainFire = true;
      mainFireTime = time;
      digitalWrite(mainPin, HIGH);
    }else{
      if(mainFireTime + 3000 < time){
        digitalWrite(mainPin, LOW);
      }
    }
  }
}

boolean loadSDFile(){
    int i = 0;
    boolean file = false;
    while(!file && i < 1024){
       xmlFile = (String)i + ".awd";
       if (!SD.exists(xmlFile)){
          dataFile = SD.open(xmlFile, FILE_WRITE);
          delay(1000);          
          dataFile.close();
          file = true;
       }
       i++;
    }
    return file;
}
void writeSD(float a, long t, int16_t x, bool mf, bool al){
  int m = 0;
  if(mf){
    m = 1;
  }
  int alck = 0;
  if(al){
    alck =  1;
  }
    dataFile = SD.open(xmlFile, FILE_WRITE);
     dataFile.println((String)a + "," + (String)t + "," + (String)x + "," + (String)m + "," + (String)alck);
     dataFile.close();
    
}

void soundBuzzer(long t){
  int seconds = t/1000;
  Serial.println(seconds);
  if((seconds % 2) == 0){
    analogWrite(buzzer, 97);
    Serial.println("buzz on");
  }else{
    analogWrite(buzzer, 0);
    Serial.println("buzz off");
  }
}
