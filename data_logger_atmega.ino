 #include <SPI.h>
#include <SD.h>
#include <Wire.h>

const int chipSelect = 10; // Pin 10 for SD card CS

// FSR Pins
const int fsrPin1 = A0;
const int fsrPin2 = A1;
const int fsrPin3 = A2;

// Timing variables
const unsigned long interval = 120000;  // 2 minutes in milliseconds
const unsigned int sampleInterval = 10; // 10 ms for 100 samples per second
unsigned long startMillis;
unsigned long currentMillis;
unsigned long lastSampleMillis = 0;
int sampleCount = 0;

// Buffer for SD card writes
const int bufferSize = 512;
char buffer[bufferSize];
int bufferIndex = 0;

// File name buffer
char fileName[20];
File myFile;

// MPU6050 Registers and values
#define MPU6050_ADDR 0x68
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40

void setup() {
  Serial.begin(115200);
  while (!Serial);

  // Initialize SD card
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1);
  }
  Serial.println("card initialized.");

  // Generate a unique file name
  int fileNumber = getNextFileNumber();
  sprintf(fileName, "DATA_%d.CSV", fileNumber);

  // Create a new file
  myFile = SD.open(fileName, FILE_WRITE);
  if (myFile) {
    Serial.print("Created new file: ");
    Serial.println(fileName);
    // Write header to the file
    myFile.println("Timestamp,FSR1,FSR2,FSR3,ax,ay,az");
    myFile.flush();  // Ensure header is written immediately
  } else {
    Serial.print("Error creating file: ");
    Serial.println(fileName);
  }

  // Initialize MPU6050
  Wire.begin();
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // Wake up the MPU6050
  Wire.endTransmission(true);

  startMillis = millis();
}

void loop() {
  currentMillis = millis();

  if (currentMillis - startMillis >= interval || sampleCount >= 12000) {
    if (bufferIndex > 0) {
      myFile.write(buffer, bufferIndex);
      bufferIndex = 0;  // Reset buffer index after writing
    }
    myFile.flush();  // Ensure all data is written
    myFile.close();
    Serial.println("Finished writing to file");
    Serial.print("Total samples written: ");
    Serial.println(sampleCount);
    Serial.print("File name: ");
    Serial.println(fileName);
    while (1);  // Stop further execution
  }

  if (currentMillis - lastSampleMillis >= sampleInterval) {
    lastSampleMillis = currentMillis;
    int fsrReading1 = analogRead(fsrPin1);
    int fsrReading2 = analogRead(fsrPin2);
    int fsrReading3 = analogRead(fsrPin3);

    // Read MPU6050 data
    int16_t ax = readMPU6050(ACCEL_XOUT_H) << 8 | readMPU6050(ACCEL_XOUT_L);
    int16_t ay = readMPU6050(ACCEL_YOUT_H) << 8 | readMPU6050(ACCEL_YOUT_L);
    int16_t az = readMPU6050(ACCEL_ZOUT_H) << 8 | readMPU6050(ACCEL_ZOUT_L);

    // Print MPU6050 data to Serial Monitor for debugging
 

    // Generate data string with timestamp, FSR readings, and accelerometer readings
    char dataString[64];
    int charsWritten = snprintf(dataString, sizeof(dataString), "%lu,%d,%d,%d,%d,%d,%d\n",
                                currentMillis, fsrReading1, fsrReading2, fsrReading3,
                                ax, ay, az);

    if (charsWritten < 0) {
      Serial.println("Error formatting data string.");
      return;  // Skip this iteration if there's an error
    }

    if (bufferIndex + charsWritten >= bufferSize) {
      myFile.write(buffer, bufferIndex);
      bufferIndex = 0;
    }

    memcpy(&buffer[bufferIndex], dataString, charsWritten);
    bufferIndex += charsWritten;

    sampleCount++;

    if (sampleCount % 1000 == 0) {
      Serial.print("Samples collected: ");
      Serial.println(sampleCount);
    }
  }
}

uint8_t readMPU6050(uint8_t reg) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1);
  return Wire.read();
}

int getNextFileNumber() {
  int fileNumber = 1;
  sprintf(fileName, "DATA_%d.CSV", fileNumber);
  while (SD.exists(fileName)) {
    fileNumber++;
    sprintf(fileName, "DATA_%d.CSV", fileNumber);
  }
  return fileNumber;
}
