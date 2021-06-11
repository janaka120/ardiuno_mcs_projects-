#include <SoftwareSerial.h>
#define WIFI_SSID        "DESKTOP-DM5SI49 4457" //WIFI SSID
#define WIFI_PASS        "master17040" // Wifi Password
#define SERVER_IP     "192.168.137.1" // IP of the target server.
#define TIMEOUT     5000 //Timeout for the serial communication
#define CONTINUE    false
#define HALT        true
#define NO_OF_PARAMETERS 0 //No of parameters sending to the server.

SoftwareSerial esp8266SerialPort(10, 11); // RX, TX

//------------------ MPU650 ------- variable initialization --------- START -------
#include "Wire.h" // This library allows you to communicate with I2C devices.
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.
int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t gyro_x, gyro_y, gyro_z; // variables for gyro raw data
int16_t temperature; // variables for temperature data
char tmp_str[7]; // temporary variable used in convert function
char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}
//------------------ MPU650 ------- variable initialization --------- END -------

void exception(String msg){ //Exception occured. Print msg and stops.
  Serial.println(msg);
  Serial.println("HALT");
  while(true){
   readResponseData("");
   delay(60000);
  }
}

boolean readResponseData(String keyword){ //Receive data from the serial port. Returns true if keyword matches.
  String response;
  long deadline = millis() + TIMEOUT;
  while(millis() < deadline){
    if (esp8266SerialPort.available()){
      char ch = esp8266SerialPort.read(); // Read one character from serial port and append to a string.
      response+=ch;
      if(keyword!=""){
        if(response.indexOf(keyword)>0){ //Searched keyword found.
          Serial.println(response);
          return true;
        }
      }
    }
  }
  Serial.println(response);
  return false;
}

boolean sendCommand(String command, String acknowledgement, boolean stopOnError)
{
  esp8266SerialPort.println(command);
  if (!readResponseData(acknowledgement)) 
    if (stopOnError)
      exception(command+" Failed to execute.");
    else
      return false;            // Let the caller handle it.
  return true;                   // ack blank or ack found
}

boolean initializeESP8266Module(){
  esp8266SerialPort.begin(9600);  
  esp8266SerialPort.setTimeout(TIMEOUT);
  delay(2000);

  //sendCommand("AT+RST", "ready", HALT);    // Reset & test if the module is ready  
  sendCommand("AT+GMR", "OK", CONTINUE);   // Retrieves the firmware ID (version number) of the module. 
  sendCommand("AT+CWMODE?","OK", CONTINUE);// Get module access mode. 
  sendCommand("AT+CWMODE=1","no change", HALT);    // Station mode
//  sendCommand("AT+CWMODE=1","OK", HALT);    // Station mode
  sendCommand("AT+CIPMUX=1","OK", HALT);    // Allow multiple connections (we'll only use the first).

  String cmd = "AT+CWJAP=\""; cmd += WIFI_SSID; cmd += "\",\""; cmd += WIFI_PASS; cmd += "\"";
  for(int counter=0;counter<5;counter++){
    if (sendCommand(cmd, "OK", CONTINUE)){ // Join Access Point
      Serial.println("Connected to WiFi.");
      break;
    }else if(counter==4)
      exception("Connection to Wi-Fi failed. Please Check");
  }
  
  delay(5000);

  sendCommand("AT+CWSAP=?", "OK", CONTINUE); // Test connection
  sendCommand("AT+CIFSR","OK", HALT);         // Echo IP address. (Firmware bug - should return "OK".)

  cmd = "AT+CIPSTART=0,\"UDP\",\""; cmd += SERVER_IP; cmd += "\",8125"; //Start a TCP connection.  to server SERVER_IP on port 80
  if (!sendCommand(cmd, "OK", CONTINUE)) 
    return;
  delay(2000);

  if (!sendCommand("AT+CIPSTATUS", "OK", CONTINUE))// Check for TCP Connection status.
    return;

}

void setup()  
{
  Serial.begin(9600);
  Serial.println("ESP8266 Demo");
  initializeESP8266Module();
  Serial.println("ESP8266 Module is ready.");

  Serial.println("MPU6050 Demo");
  initializeMPU6050Module();
  Serial.println("MPU6050 Module is ready.");
}

void loop() 
{
  // ----------- read MPU6050 Module --------- START ----------
   Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7*2, true); // request a total of 7*2=14 registers
  
  // "Wire.read()<<8 | Wire.read();" means two registers are read and stored in the same variable
  accelerometer_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  temperature = Wire.read()<<8 | Wire.read(); // reading registers: 0x41 (TEMP_OUT_H) and 0x42 (TEMP_OUT_L)
  gyro_x = Wire.read()<<8 | Wire.read(); // reading registers: 0x43 (GYRO_XOUT_H) and 0x44 (GYRO_XOUT_L)
  gyro_y = Wire.read()<<8 | Wire.read(); // reading registers: 0x45 (GYRO_YOUT_H) and 0x46 (GYRO_YOUT_L)
  gyro_z = Wire.read()<<8 | Wire.read(); // reading registers: 0x47 (GYRO_ZOUT_H) and 0x48 (GYRO_ZOUT_L)
  String str_acc_x = convert_int16_to_str(accelerometer_x);
  String str_acc_y = convert_int16_to_str(accelerometer_y);
  String str_acc_z = convert_int16_to_str(accelerometer_z);
  // print out data
  Serial.print("aX = "); Serial.print(str_acc_x);
  Serial.print(" | aY = "); Serial.print(str_acc_y);
  Serial.print(" | aZ = "); Serial.print(str_acc_z);
  // the following equation was taken from the documentation [MPU-6000/MPU-6050 Register Map and Description, p.30]
  Serial.print(" | tmp = "); Serial.print(temperature/340.00+36.53);
  
  String str_gyro_x = convert_int16_to_str(gyro_x);
  String str_gyro_y = convert_int16_to_str(gyro_y);
  String str_gyro_z = convert_int16_to_str(gyro_z);
  Serial.print(" | gX = "); Serial.print(str_gyro_x);
  Serial.print(" | gY = "); Serial.print(str_gyro_y);
  Serial.print(" | gZ = "); Serial.print(str_gyro_z);
  Serial.println();
  // ----------- read MPU6050 Module --------- END ----------
  
//  String  cmd = "UDPTest";
String  cmd = str_acc_x + "," + str_acc_y + "," + str_acc_z + "," + str_gyro_x + "," + str_gyro_y + "," + str_gyro_z;
  if (!sendCommand("AT+CIPSEND=0,"+String(cmd.length()), ">", CONTINUE)){
    sendCommand("AT+CIPCLOSE", "", CONTINUE);
    Serial.println("Connection timeout.");
    return;
  }

  sendCommand(cmd, "OK", CONTINUE);// Send data to server.
//  readResponseData("");// Read response data received from server.
//  exception("ONCE ONLY");
}


// ----------- initializeMPU6050Module --------- START ----------
boolean initializeMPU6050Module() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);
}
