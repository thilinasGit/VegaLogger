///to do memory optimization , refining code
#include <Preferences.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <REG.h>
#include <wit_c_sdk.h>
#include <SoftwareSerial.h>


LiquidCrystal_I2C lcd(0x27, 16, 4);
Preferences sharedPreferences;

/// Network
WiFiClient espClient;
PubSubClient client(espClient);
const char* ssid = "ETX_DT7";
const char* password = "ETXP0001";
const char* mqttBroker = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
const char* mqttTopic0 = "tukLogClient";
bool cloudEnabled = true;
bool dataSaverEnabled = false;
const char* ntpServer = "pool.ntp.org";
unsigned long Epoch_Time;
byte networkTimeout = 0;

//HW RX,TX lines common for RS485 communication
String receivedString;// Variable to store the received RS485 data string


////VA meter stuff
const String RSCommands[3] = {":R50=1,2,1,\r\n", ":R50=2,2,1,\r\n", ":R50=3,2,1,\r\n"};
int commandCycle = 0, receivedDeviceID = -1;
byte deviceID = 1, statMode = 0;
bool RSDataAvailableFlag = false;
String voltage, current, power, wattHour, soc, temp;   //// reserved 20 for each

//UI stuff
extern String mainMenuItems[];
extern String subMenuItems[];
extern String statMenuItems[];
extern String gyroMenuItems[];
int mainMenuOrder[] = {0, 1, 2, 3, 4};
int subMenuOrder[] = {2, 0, 1};
int statMenuOrder[] = {2, 0, 1};
int gyroMenuOrder[] = {4, 0, 1, 2, 3};
int mode = 0, gyroMode = 0;
String r0 , r1 , r2 , r3 ;     /// reserved 20 for each
 
extern byte icon[] ;

//// IO
#define RS485_DE 15
#define DE_ActiveHigh 0
#define button0 26
#define button1 27
#define button2 14

/////////  gyro stuff
SoftwareSerial mySerial(13, 12); // RX, TX
bool SWSerialAvailableFlag = false;
#define ACC_UPDATE    0x01
#define GYRO_UPDATE   0x02
#define ANGLE_UPDATE  0x04
#define MAG_UPDATE    0x08
#define READ_UPDATE   0x80
static volatile char s_cDataUpdate = 0, s_cCmd = 0xff;
int i;
float fAcc[3], fGyro[3], fAngle[3];
int fMag[3] ;

///// Jason obj for mqtt
DynamicJsonDocument jsonDoc(350); // Adjust the capacity as needed





void callback(char* topic, byte* payload, unsigned int length) {
  char payloadStr[length + 1];
  memcpy(payloadStr, payload, length);
  payloadStr[length] = '\0';

  if (strcmp(topic, "setFlags") == 0) {
    if (strcmp(payloadStr, "fastMode=1") == 0) {
      dataSaverEnabled = false; // Set dataSaverFlag to false
    }
    else if (strcmp(payloadStr, "fastMode=0") == 0) {
      dataSaverEnabled = true; // Set dataSaverFlag to true
    }
  }
  else {
    // No other topics here
  }
}



void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);
}

uint32_t epochTime = 0; // Variable to store the epoch time
uint32_t getEpochTime() {
  time_t now;
  time(&now);
  if (now != -1) {
    // Get the current time (epoch time)
    return epochTime = static_cast<uint32_t>(now); // Store the epoch time as uint32_t
    // You can perform other tasks using the epochTime here
  } else {
    lcdUpdate("Retrieving time>", 1, 0);
    configTime(0, 0, "pool.ntp.org");
    time_t now;
    time(&now); // Get the current time (epoch time)
    if (now != -1) {
      lcdUpdate("NTP server down!", 2, 0);
      return epochTime = static_cast<uint32_t>(now) * 1000;
    }
    else return -1;
  }
}


void MQTTPub() {

  // Convert the JSON object to a JSON string
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  char payload[jsonString.length() + 1]; // +1 for the null-terminator

  const char* cstr2 = jsonString.c_str();
  client.publish(mqttTopic0, cstr2);
}

void reconnect() {
  if (!client.connected()) {
    networkTimeout++;
    if (networkTimeout > 2) {
      lcdUpdate("Connection Lost", 1, 1);
      lcdUpdate("", 0, 1);
      lcdUpdate("", 2, 1);
      lcdUpdate("", 3, 0);
    }
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      //Serial.println(" retrying in 5 seconds");
      delay(50);
    }
  }
}

//                                                _____
//                                               / ____|
//                                              | (___
//                                               \___ \
//                                               ____) |
//                                              |_____/


void setup() {
  lcd.begin();
  lcd.backlight();
  lcd.createChar(1, icon); // battery icon
  Serial.begin(115200);       /////// default baud for energy meter is 115200
  Serial.setRxBufferSize(256);
  
  stringStuff();
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  pinMode(RS485_DE , OUTPUT);
  pinMode(button0 , INPUT_PULLUP);
  pinMode(button1 , INPUT_PULLUP);
  pinMode(button2 , INPUT_PULLUP);
  // digitalWrite(RS485_DE,DE_ActiveHigh^LOW);
  if (cloudEnabled) {
    lcdUpdate("Connecting to", 1, 1);
    lcdUpdate(ssid, 2, 0);
    initWiFi();
    configTime(0, 0, ntpServer);
    delay(2500);
  }

  sharedPreferences.begin("settings", false);                      /// retrieve settings
  dataSaverEnabled = sharedPreferences.getBool("dataSaverFlag", true);
  dataSaverEnabled = sharedPreferences.getBool("clientEnabledFlag", true);
  sharedPreferences.end();
  
  mySerial.begin(9600);
  WitInit(WIT_PROTOCOL_NORMAL, 0x50);
  WitSerialWriteRegister(SensorUartSend);
  WitRegisterCallBack(SensorDataUpdata);
  WitDelayMsRegister(Delayms); WitSetUartBaud(WIT_BAUD_115200);
  lcdUpdate("RS485 down", 1, 1); ////////////////////////////////////////////////not required
  lcdUpdate("--", 2, 0); ////////////////////////////////////////////////not required

  
}

//                                                                             _
//                                                                            | |
//                                                                            | |
//                                                                            | |
//                                                                            | |____
//                                                                            |______|

void loop() {

  gyroStuff();

  networkTimeout = 0;
  if (cloudEnabled)
    if (!client.connected()) {
      reconnect();

    }

  if (commandCycle > 2)commandCycle = 0;

  RsSend(RSCommands[commandCycle]);
  if (mode == 4)deviceID = commandCycle + 1;
  delay(10);
  RSDataAvailableFlag = false;
  serialStuff();

  if(cloudEnabled)jsonStuff();


  //// UI Render
  switch (mode) {
    case 0: mainMenuPg(); break;
    case 1: subMenuPg(); break;
    case 2: dashboardPg(); break;
    case 3: statMenuPg(); break;
    case 4: statPg();   break;
    case 5: gyroMenuPg(); break;
    case 6: gyroPg(); break;
    case 7: mqttSettingsPg(); break;
    case 485: RSReadPg("", 1);
  }

  commandCycle++;
}

//                                                                ____
//                                                               / ___|
//                                                              | |  _
//                                                              | |_| |
//                                                               \____|
//                                                                       ____
//                                                                      |  _ \ 
//                                                                      | | | |
//                                                                      | |_| |
//                                                                      |____/

void gyroStuff() {
  while (mySerial.available())
  {
    WitSerialDataIn(mySerial.read());
    SWSerialAvailableFlag = true;
  }

  //CmdProcess();
  if (s_cDataUpdate)
  {
    for (i = 0; i < 3; i++)
    {
      fAcc[i] = sReg[AX + i] / 32768.0f * 16.0f;
      fGyro[i] = sReg[GX + i] / 32768.0f * 2000.0f;
      fAngle[i] = sReg[Roll + i] / 32768.0f * 180.0f;
    }

    if (s_cDataUpdate & ACC_UPDATE)
    {
      //      jsonDoc["imu_AccX"] = fAcc[0];
      //      jsonDoc["imu_AccY"] = fAcc[1];
      //      jsonDoc["imu_AccZ"] = fAcc[2];
      s_cDataUpdate &= ~ACC_UPDATE;
    }
    if (s_cDataUpdate & GYRO_UPDATE)
    {
      //      jsonDoc["imu_GyroX"] = fGyro[0];
      //      jsonDoc["imu_GyroY"] = fGyro[1];
      //      jsonDoc["imu_GyroZ"] = fGyro[2];
      s_cDataUpdate &= ~GYRO_UPDATE;
    }
    if (s_cDataUpdate & ANGLE_UPDATE)
    {
      //      jsonDoc["imu_AngleX"] = fAngle[0];
      //      jsonDoc["imu_AngleY"] = fAngle[1];
      //      jsonDoc["imu_AngleZ"] = fAngle[2];

      s_cDataUpdate &= ~ANGLE_UPDATE;
    }
    if (s_cDataUpdate & MAG_UPDATE)
    {
      fMag[0] = sReg[HX];
      fMag[1] = sReg[HY];
      fMag[2] = sReg[HZ];

      //      jsonDoc["imu_MagX"] = fMag[0];
      //      jsonDoc["imu_MagY"] = fMag[1];
      //      jsonDoc["imu_MagZ"] = fMag[2];
      s_cDataUpdate &= ~MAG_UPDATE;
    }

    s_cDataUpdate = 0;

  }
}



void mainMenuPg() {                         ///0                  ////////////////////////////////////////////MMENNUUUUUUUUUUUUUUUUUUU

  lcdUpdate("      Menu", 0, 1);
  lcdUpdate(" " + mainMenuItems[mainMenuOrder[0]], 1, 1);
  lcdUpdate(">" + mainMenuItems[mainMenuOrder[1]], 2, 1);
  lcdUpdate(" " + mainMenuItems[mainMenuOrder[2]], 3, 0);


  switch (getBtnStatus()) {

    case 1:
      rotateArray(mainMenuOrder, 5, false); break;

    case 2:
      if (mainMenuOrder[1] == 0)mode = 3;
      else if (mainMenuOrder[1] == 1)mode = 1;
      else if (mainMenuOrder[1] == 2)mode = 5;
      else if (mainMenuOrder[1] == 3)mode = 7;
      else if (mainMenuOrder[1] == 4) {
        mode = 485;
        //RSDataAvailableFlag = false;
        RSSplashPg();
      }
      break;

    case 3:
      rotateArray(mainMenuOrder, 5, true);  break;

  }
}



void subMenuPg() {                         ///2                   ////////////////////////////////////////////ENERGY Meter
  lcdUpdate(" V/A Meters", 0, 1);
  lcdUpdate(" " + subMenuItems[subMenuOrder[0]], 1, 1);
  lcdUpdate(">" + subMenuItems[subMenuOrder[1]], 2, 1);
  lcdUpdate(" " + subMenuItems[subMenuOrder[2]], 3, 0);

  switch (getBtnStatus()) {

    case 1:
      rotateArray(subMenuOrder, 3 , false); break;

    case 2:
      if (subMenuOrder[1] == 0)deviceID = 1;
      else if (subMenuOrder[1] == 1)deviceID = 2;
      else if (subMenuOrder[1] == 2)deviceID = 3;
      mode = 2;
      // RSDataAvailableFlag = false;
      RSSplashPg();
      break;

    case 200:
      mode = 0;
      break;

    case 3:
      rotateArray(subMenuOrder, 3, true);  break;

  }

}



void statMenuPg() {                         ///3                   ////////////////////////////////////////////Stat Menu
  lcdUpdate("   Statistics", 0, 1);
  lcdUpdate(" " + statMenuItems[statMenuOrder[0]], 1, 1);
  lcdUpdate(">" + statMenuItems[statMenuOrder[1]], 2, 1);
  lcdUpdate(" " + statMenuItems[statMenuOrder[2]], 3, 0);

  switch (getBtnStatus()) {
    case 1:
      rotateArray(statMenuOrder, 3 , false); break;

    case 2:
      if (statMenuOrder[1] == 0)statMode = 0;
      else if (statMenuOrder[1] == 1)statMode = 1;
      else if (statMenuOrder[1] == 2)statMode = 2;
      mode = 4;
      //RSDataAvailableFlag = false;
      RSSplashPg();
      break;

    case 200:
      mode = 0;
      break;

    case 3:
      rotateArray(statMenuOrder, 3, true);  break;
  }
}

void gyroMenuPg() {                         ///7                   ////////////////////////////////////////////Gyro Menu
  lcdUpdate("   IMU Data", 0, 1);
  lcdUpdate(" " + gyroMenuItems[gyroMenuOrder[0]], 1, 1);
  lcdUpdate(">" + gyroMenuItems[gyroMenuOrder[1]], 2, 1);
  lcdUpdate(" " + gyroMenuItems[gyroMenuOrder[2]], 3, 0);

  switch (getBtnStatus()) {
    case 1:
      rotateArray(gyroMenuOrder, 5 , false); break;

    case 2:
      if (gyroMenuOrder[1] == 0)gyroMode = 0;
      else if (gyroMenuOrder[1] == 1)gyroMode = 1;
      else if (gyroMenuOrder[1] == 2)gyroMode = 2;
      else if (gyroMenuOrder[1] == 3)gyroMode = 3;
      else if (gyroMenuOrder[1] == 4) {
        WitStartAccCali();
        gyroMode = 0;
        RSSplashPg();
        delay(5000);
      }
      mode = 6;
      RSSplashPg();

      break;

    case 200:
      mode = 0;
      break;

    case 3:
      rotateArray(gyroMenuOrder, 5, true);  break;
  }
}

//                                                                       ____
//                                                                      |  _ \ 
//                                                                      | | | |
//                                                                      | |_| |
//                                                                      |____/

int disconnectedTime;
void dashboardPg() {                               /////////////////////////////////////////////
  char row1[20];
  char row2[20];
  char row3[20];
  char row4[20];

  if (RSDataAvailableFlag && deviceID == receivedDeviceID) {
    disconnectedTime = 0;
    sprintf(row1, "P0%s   %s*C  %s%%", String(deviceID), temp.c_str(), soc.c_str());
    sprintf(row2, "%sV  %sA", voltage.c_str(), current.c_str());
    sprintf(row3, "Power :%sW", power.c_str());
    sprintf(row4, "Energy:%skWh", wattHour.c_str());

    lcdUpdate(row1, 0, 1);
    lcdUpdate(row2, 1, 1);
    lcdUpdate(row3, 2, 1);
    lcdUpdate(row4, 3, 0);
  }
  else {
    disconnectedTime++;
    if (disconnectedTime > 200)RSSplashPg();
  }

  switch (getBtnStatus()) {

    case 1:
      rotateArray(subMenuOrder, 3 , false);
      if (subMenuOrder[1] == 0)deviceID = 1;
      else if (subMenuOrder[1] == 1)deviceID = 2;
      else if (subMenuOrder[1] == 2)deviceID = 3;
      //RSDataAvailableFlag = false;
      RSSplashPg();
      break;

    case 2:

      break;

    case 200:
      mode = 1;
      break;

    case 3:
      rotateArray(subMenuOrder, 3, true);
      if (subMenuOrder[1] == 0)deviceID = 1;
      else if (subMenuOrder[1] == 1)deviceID = 2;
      else if (subMenuOrder[1] == 2)deviceID = 3;
      //RSDataAvailableFlag = false;
      RSSplashPg();
      break;

  }
}

//                                                          _____   __________
//                                                         / ____|  __________
//                                                        | (___       |  |
//                                                         \___ \      |  |
//                                                         ____) |     |  |
//                                                        |_____/      |  |


void statPg() {                         ///4                   ////////////////////////////////////////////StatPg
  String title[] = {"Current (A)", "Power (W)", "Energy (kWh)"};
  String value = "--";
  char row[20];

  if (RSDataAvailableFlag) {
    //RSDataAvailableFlag = false;
    switch (statMode) {
      case 0:     value = current.c_str(); break;
      case 1:     value = power.c_str(); break;
      case 2:     value = wattHour.c_str(); break;
    }
  }
  lcdUpdate(title[statMode], 0, 1);

  switch (commandCycle) {
    case 0: sprintf(row, "Solar:%s", value);
      lcdUpdate(row, 1, 0); break;
    case 1: sprintf(row, "MPPT :%s" , value);
      lcdUpdate(row, 2, 0); break;
    case 2: sprintf(row, "Batt.:%s" , value);
      lcdUpdate(row, 3, 0); break;
  }


  switch (getBtnStatus()) {

    case 1:
      statMode--;
      if (statMode == -1)statMode = 2;
      break;

    case 2:
      /// todo hold feature
      break;

    case 200:
      mode = 3;
      break;

    case 3:
      statMode++;
      if (statMode == 3)statMode = 0; break;
  }

}


void RSReadPg(String sss, bool btnsOnly) {

  if (!btnsOnly) {
    lcdUpdate("   RS485 read", 0, 1);
    lcdUpdate(sss.substring(0, 16), 1, 1);
    lcdUpdate(sss.substring(16, 32), 2, 1);
    lcdUpdate(sss.substring(32), 3, 0);
  }

  switch (getBtnStatus()) {
    case 2:
      mode = 0; break;
  }
}


void RSSplashPg() {
  lcdUpdate("Waiting for the", 0, 1);
  lcdUpdate("device to", 1, 1);
  lcdUpdate("respond..." , 2, 1);
  char row4[20];
  //sprintf(row4, "Device ID : 0%s", String(deviceID));
  lcdUpdate(" " , 3, 0);
}


////////////////////////////////////////////////////////////////////////////////////GGGGGGGGGGGGGGGYYYYYYYYYYYRRRRRRRRROOOOOOOOOOOOOOOOOOO PPPPPPPPGGGGGGGGGGGGGG
//                                                                ____
//                                                               / ___|
//                                                              | |  _
//                                                              | |_| |
//                                                               \____|
//
//                                                                  \\   //
//                                                                   \\ //
//                                                                    \v/
//                                                                    //

void gyroPg() {

  String title[] = {"Acceleration", "Gyro Data", "Angle", "Compass"};
  float valueX , valueY, valueZ ;
  char row[20];

  if (SWSerialAvailableFlag) {
    SWSerialAvailableFlag = false;
    switch (gyroMode) {
      case 0:
        valueX = fAcc[0];
        valueY = fAcc[1];
        valueZ = fAcc[2];
        break;

      case 1:
        valueX = fGyro[0];
        valueY = fGyro[1];
        valueZ = fGyro[2];
        break;

      case 2:
        valueX = fAngle[0];
        valueY = fAngle[1];
        valueZ = fAngle[2];
        break;

      case 3:
        valueX = fMag[0];
        valueY = fMag[1];
        valueZ = fMag[2];
        break;
    }
  }
  lcdUpdate(title[gyroMode], 0, 1);

  sprintf(row, "X :%.3f", valueX);
  lcdUpdate(row, 1, 1);
  sprintf(row, "Y :%.3f" , valueY);
  lcdUpdate(row, 2, 1);
  sprintf(row, "Z :%.3f" , valueZ);
  lcdUpdate(row, 3, 0);


  switch (getBtnStatus()) {

    case 1:
      gyroMode--;
      if (gyroMode == -1)gyroMode = 3;
      break;

    case 2:
      /// todo hold feature
      break;

    case 200:
      mode = 5;
      break;

    case 3:
      gyroMode++;
      if (gyroMode == 4)gyroMode = 0; break;
  }

}



bool curPos = false;
void mqttSettingsPg() {                               /////////////////////////////////////////////  MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMSSSSSSSSSSSSSSSSSSSS
  char row1[20];
  char row2[20];

  char cursorPlaceHolder[2] = {'>', ' '};
  if (curPos) {
    cursorPlaceHolder[0] = ' ';
    cursorPlaceHolder[1] = '>';
  }

  if (cloudEnabled)
    sprintf(row1, "%c%s" , cursorPlaceHolder[0], "Disable client");
  else sprintf(row1, "%c%s" , cursorPlaceHolder[0], "Enable client");

if(cloudEnabled){
  if (dataSaverEnabled)
    sprintf(row2, "%c%s" , cursorPlaceHolder[1], "Enable fastmode");
  else sprintf(row2, "%c%s" , cursorPlaceHolder[1], "Disable fastmode");
} else sprintf(row2, "--" );

  lcdUpdate(" MQTT Settings", 0, 1);
  lcdUpdate(row1, 1, 1);
  lcdUpdate(row2, 2, 1);
  lcdUpdate("Resets on boot!", 3, 0);

  switch (getBtnStatus()) {

    case 1:
      curPos = !curPos;
      break;

    case 2:
      if (!curPos){
        cloudEnabled = !cloudEnabled;
        writeBoolToNVS(0,cloudEnabled);
      }
      else {
        dataSaverEnabled = !dataSaverEnabled;
        writeBoolToNVS(1,dataSaverEnabled);
      }    
      
      break;

    case 200:
      mode = 0;
      break;

    case 3:
      curPos = !curPos;
      break;

  }
}



void lcdUpdate(String ss, int row , bool postUpdate) {

  if (ss.length() > 16) {
    ss = ss.substring(0, 14) + "..";
  } else if (ss.length() < 16) {
    for (int c = ss.length(); c < 16; c++) {
      ss += " ";
    }
  }

  switch (row) {
    case 0: r0 = ss; break;
    case 1: r1 = ss; break;
    case 2: r2 = ss; break;
    case 3: r3 = ss;
  }
  if (!postUpdate) {
    lcd.setCursor(0, 0);
    String allRows = r0 + r2 + r1 + r3;
    lcd.print(allRows);
  }
}



void rotateArray(int arr[], int arSize, bool reverse) {
  int temp = 0;
  arSize--;
  if (reverse) {
    temp = arr[0];
    for (int i = 0; i < arSize; i++) {
      arr[i] = arr[i + 1];
    }
    arr[arSize] = temp;
  } else {
    temp = arr[arSize];
    for (int i = arSize; i > 0; i--) {
      arr[i] = arr[i - 1];
    }
    arr[0] = temp;
  }
}


int getBtnStatus() {
  int longPress = 0;
  if (!digitalRead(button0)) {
    delay(40);
    while (!digitalRead(button0)) {
      longPress++;
      delay(12);
    }
    if (longPress > 60) {
      return 100;
    } else return 1;
  } else if (!digitalRead(button1)) {
    delay(40);
    while (!digitalRead(button1)) {
      if (longPress == 50)lcd.clear();
      longPress++;
      delay(12);
    }
    if (longPress > 50) {
      return 200;
    } else return 2;
  } else if (!digitalRead(button2)) {
    delay(40);
    while (!digitalRead(button2)) {
      longPress++;
      delay(12);
    }
    if (longPress > 60) {
      return 300;
    } else return 3;
  }
  return -1;
}


void RsSend(String ss) {
  digitalWrite(RS485_DE, HIGH);
  delayMicroseconds(10);
  Serial.print(ss);
  delayMicroseconds(1150);
  digitalWrite(RS485_DE, LOW);
}

//                                                                    _____
//                                                                   / ____|
//                                                                  | (___
//                                                                   \___ \
//                                                                   ____) |
//                                                                  |_____/
//                                                                    _____
//                                                                   / ____|
//                                                                  | (___
//                                                                   \___ \
//                                                                   ____) |
//                                                                  |_____/

void serialStuff() {
  int ccc = 0;
  while (Serial.available() && ccc < 255) {
    ccc++;
    char inChar = (char)Serial.read();
    if (receivedString.length()>250|inChar == '\n' && receivedString.length() > 1) {
      RSDataAvailableFlag = true;
      const char* string1 = receivedString.c_str();
      decodeMsg();
      if (mode == 485)RSReadPg(receivedString, 0);
      if (cloudEnabled)client.publish("RSData", string1);     
      receivedString = ""; 
      delay(30);
    } else {
      receivedString += inChar;
      RSDataAvailableFlag = false;
    }
  }
}


void decodeMsg() {
  int indexToExtract = receivedString.indexOf("r50=");
  if (indexToExtract > 0) {
    receivedString = receivedString.substring(indexToExtract);
    // Split the comma-separated values into an array
    String values[18];
    int valueCount = splitString(receivedString, ',', values,18);
    receivedDeviceID = receivedString.substring(4, 5).toInt();
    if (valueCount >= 12 && receivedDeviceID == commandCycle + 1) {
      voltage = formatValue(values[2], 0.01, 2) ;
      current = getCurrentDir(values[11].toInt()) + formatValue(values[3], 0.01, 2) ;
      wattHour = formatValue(values[6], 0.00001, 4) ;
      soc = calculateSOC(values[5], values[4]) ;
      temp = values[8].substring(1) ; // Temperature at 8th position
      power = calculatePwr(voltage, current) ; // Power at 13th position
    } else {
      RSDataAvailableFlag = false;
    }
  }
}

//String jsonKeys[];//"D1","A1","","","","","","","","","","","","","","","","","","",""
bool mqttSentFlag;   ///////////////////// data saver mode requiment
void jsonStuff() {
  if (!RSDataAvailableFlag) {
    voltage = "--" ;
    current = "--" ;
    wattHour = "--" ;
    soc = "--" ;
    temp = "--" ;
    power = "--" ;
  }

  int idKey = commandCycle + 1;
  jsonDoc["A" + String(idKey)] = current;
  jsonDoc["V" + String(idKey)] = voltage;
  jsonDoc["E" + String(idKey)] = wattHour;
  jsonDoc["P" + String(idKey)] = power;
  jsonDoc["S" + String(idKey)] = soc;
  jsonDoc["T" + String(idKey)] = temp;

  if (commandCycle == 2) {
    jsonDoc["ax"] = String(fAcc[0], 3);
    jsonDoc["ay"] = String(fAcc[1], 3);
    jsonDoc["az"] = String(fAcc[2], 3);
    jsonDoc["X"] = String(fAngle[0], 3);
    jsonDoc["Y"] = String(fAngle[1], 3);
    jsonDoc["Z"] = String(fAngle[2], 3);
    jsonDoc["mx"] = String(fMag[0]);
    jsonDoc["my"] = String(fMag[1]);
    jsonDoc["mz"] = String(fMag[2]);
    if (dataSaverEnabled) {      
      if (getEpochTime() % 2 == 0){
        if(!mqttSentFlag)MQTTPub();
        mqttSentFlag=true;} 
        else mqttSentFlag=false;
      } else MQTTPub();
    jsonDoc = DynamicJsonDocument(680);
    jsonDoc["ID"] = "VLX01";
  }
}


int splitString(String input, char separator, String values[], int arraySize) {
  int valueCount = 0;
  int startIndex = 0;
  while (startIndex >= 0 && valueCount < arraySize) {
    int endIndex = input.indexOf(separator, startIndex);
    if (endIndex == -1) {
      values[valueCount++] = input.substring(startIndex);
      break;
    }
    values[valueCount++] = input.substring(startIndex, endIndex);
    startIndex = endIndex + 1;
  }
  return valueCount;
}


String formatValue(String value, float multiplier, int decPoint) {
  float floatValue = value.toFloat() * multiplier;
  return String(floatValue, decPoint);
}


char getCurrentDir(int d) {
  if (d == 0)return ' '; else return '-';
}


String calculateSOC(String elapsedCapacity, String remainingCapacity) {
  float elapsedAh = elapsedCapacity.toFloat() ;
  float remainingAh = remainingCapacity.toFloat();
  int socPercentage = int((remainingAh / 100000.0 * 100));
  return String(socPercentage);
}



String calculatePwr(String v, String c) {
  float volts = v.toFloat() ;
  float amps = c.toFloat();
  float watts = amps * volts;
  return String(watts);
}


void CopeCmdData(unsigned char ucData)
{
  static unsigned char s_ucData[50], s_ucRxCnt = 0;

  s_ucData[s_ucRxCnt++] = ucData;
  if (s_ucRxCnt < 3)return;                 //Less than three data returned
  if (s_ucRxCnt >= 50) s_ucRxCnt = 0;
  if (s_ucRxCnt >= 3)
  {
    if ((s_ucData[1] == '\r') && (s_ucData[2] == '\n'))
    {
      s_cCmd = s_ucData[0];
      memset(s_ucData, 0, 50);
      s_ucRxCnt = 0;
    }
    else
    {
      s_ucData[0] = s_ucData[1];
      s_ucData[1] = s_ucData[2];
      s_ucRxCnt = 2;

    }
  }
}


static void SensorUartSend(uint8_t *p_data, uint32_t uiSize)
{
  mySerial.write(p_data, uiSize);
  mySerial.flush();
}



static void Delayms(uint16_t ucMs)
{
  delay(ucMs);
}


static void SensorDataUpdata(uint32_t uiReg, uint32_t uiRegNum)
{
  int i;
  for (i = 0; i < uiRegNum; i++)
  {
    switch (uiReg)
    {
      case AZ:
        s_cDataUpdate |= ACC_UPDATE;
        break;
      case GZ:
        s_cDataUpdate |= GYRO_UPDATE;
        break;
      case HZ:
        s_cDataUpdate |= MAG_UPDATE;
        break;
      case Yaw:
        s_cDataUpdate |= ANGLE_UPDATE;
        break;
      default:
        s_cDataUpdate |= READ_UPDATE;
        break;
    }
    uiReg++;
  }
}

void stringStuff(){
  receivedString.reserve(250);
  r0.reserve(20);
  r1.reserve(20);
  r2.reserve(20);
  r3.reserve(20);
  r0= "                ";
  r1= "                ";
  r2= "                ";
  r3= "                ";
  voltage.reserve(20);
  current.reserve(20);
  power.reserve(20);
  wattHour.reserve(20);
  soc.reserve(20);
  temp.reserve(20);
}


void writeBoolToNVS(int c,bool torf) {    //// write sharedPreferences  
  sharedPreferences.begin("settings", false);
  switch(c){
    case 0:sharedPreferences.putBool("clientEnabledFlag", torf); break;
    case 1:sharedPreferences.putBool("dataSaverFlag", torf); break;
  }  
  sharedPreferences.end();
}
