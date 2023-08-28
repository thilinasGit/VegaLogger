#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <REG.h>
#include <wit_c_sdk.h>
#include <SoftwareSerial.h>

const char* ssid = "Nokia 8";
const char* password = "155155155";
const char* mqttBroker = "broker.mqtt-dashboard.com";
const int mqttPort = 1883;
const char* mqttTopic0 = "tuklogclient";

WiFiClient espClient;
PubSubClient client(espClient);

const char* ntpServer = "pool.ntp.org";
unsigned long Epoch_Time;

LiquidCrystal_I2C lcd(0x27, 16, 4);

//HW RX,TX lines common for RS485 communication

String receivedString = "";// Variable to store the received RS485 data string
String displayInitiaize = "";// Variable to store the display initialization message
String LCD_MSG = "";// Variable to store the display message

const String RSCommands[3] = {":R50=1,2,1,\r\n", ":R50=2,2,1,\r\n", ":R50=3,2,1,\r\n"};
int commandCycle = 0;

int deviceID = 1, statMode = 0;
bool RSDataAvailableFlag = false;
String voltage, current, power, wattHour, soc, temp;

//UI stuff
extern String mainMenuItems[];
extern String subMenuItems[];
extern String statMenuItems[];
int mainMenuOrder[] = {0, 1, 2, 3, 4};
int subMenuOrder[] = {2, 0, 1};
int statMenuOrder[] = {2, 0, 1};
int mode = 0;
bool cloudEnabled = false;
extern byte icon[] ;

#define RS485_DE 15
#define DE_ActiveHigh 0
#define button0 26
#define button1 27
#define button2 14

unsigned long lastProcessTime = 0;
const unsigned long processInterval = 20;  // tracking time

/////////  gyro stuff
SoftwareSerial mySerial(13, 12); // RX, TX
#define ACC_UPDATE    0x01
#define GYRO_UPDATE   0x02
#define ANGLE_UPDATE  0x04
#define MAG_UPDATE    0x08
#define READ_UPDATE   0x80
static volatile char s_cDataUpdate = 0, s_cCmd = 0xff;

static void CmdProcess(void);
static void RS485_IO_Init(void);
static void AutoScanSensor(void);
static void SensorUartSend(uint8_t *p_data, uint32_t uiSize);
static void CopeSensorData(uint32_t uiReg, uint32_t uiRegNum);
static void Delayms(uint16_t ucMs);

const uint32_t c_uiBaud[8] = { 0, 4800, 9600, 19200, 38400, 57600, 115200, 230400};

void callback(char* topic, byte* payload, unsigned int length) {
  ////to do


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
      return epochTime = static_cast<uint32_t>(now);
    }
    else return -1;
  }
}


void MQTTPub(String ss) {
  DynamicJsonDocument jsonDocument(200); // Adjust the capacity as needed

  // Populate the JSON object with the values
  jsonDocument["device_id"] = "SN001";
  jsonDocument["data"] = random(1022, 1523);
  jsonDocument["timestamp"] = getEpochTime();

  // Convert the JSON object to a JSON string
  String jsonString;
  serializeJson(jsonDocument, jsonString);
  char payload[jsonString.length() + 1]; // +1 for the null-terminator

  const char* cstr2 = jsonString.c_str();
  client.publish(mqttTopic0, cstr2);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}


int i;
float fAcc[3], fGyro[3], fAngle[3];


//                                                _____
//                                               / ____|
//                                              | (___  
//                                               \___ \
//                                               ____) |
//                                              |_____/ 


void setup() {
  //LCD Initializatio
  lcd.begin();
  lcd.backlight();
  lcd.createChar(1, icon); // battery icon
  Serial.begin(115200);       /////// default baud for energy meter is 115200
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
  mySerial.begin(9600);
  WitInit(WIT_PROTOCOL_NORMAL, 0x50);
  WitSerialWriteRegister(SensorUartSend);
  WitRegisterCallBack(SensorDataUpdata);
  WitDelayMsRegister(Delayms);
  WitSetUartBaud(WIT_BAUD_115200);
  Serial.begin(c_uiBaud[WIT_BAUD_115200]);
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
  if (cloudEnabled && !client.connected()) {
    reconnect();
  }

  if (commandCycle > 2)commandCycle = 0;
  RsSend(RSCommands[commandCycle]);
  if(mode==4)deviceID = commandCycle + 1;
  delay(9);
  serialStuff();

  switch (mode) {
    case 0: mainMenuPg(); break;
    case 1: subMenuPg(); break;
    case 2: dashboardPg(); break;
    case 3: statMenuPg(); break;
    case 4:       
      statPg();
      break;
    case 5: gyroPg(); break;
    case 485: RSReadPg("", 1);
  }

  // unsigned long currentMillis = millis();
  //
  //  if (currentMillis - lastProcessTime >= processInterval) {// Pseudo thread every xx millis
  //    lastProcessTime = currentMillis;  
  //}

  commandCycle++;
  delay(20);
}

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



void gyroPg(){
 
   while (mySerial.available())
    {
      WitSerialDataIn(mySerial.read());
    }

    CmdProcess();
    if(s_cDataUpdate)
    {
      for(i = 0; i < 3; i++)
      {
        fAcc[i] = sReg[AX+i] / 32768.0f * 16.0f;
        fGyro[i] = sReg[GX+i] / 32768.0f * 2000.0f;
        fAngle[i] = sReg[Roll+i] / 32768.0f * 180.0f;
      }
      if(s_cDataUpdate & ACC_UPDATE)
      {
        lcdUpdate("  Acceleration", 0, 1);
  lcdUpdate(String(fAcc[0]), 1, 1);
  lcdUpdate(String(fAcc[1]), 2, 1);
  lcdUpdate(String(fAcc[2]), 3, 0);
        s_cDataUpdate &= ~ACC_UPDATE;

      s_cDataUpdate = 0;
    }}
  switch (getBtnStatus()) {
    case 2:
      mode = 0; break;
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
      else if (mainMenuOrder[1] == 4) {
        mode = 485;
        RSDataAvailableFlag = false;
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
      RSDataAvailableFlag = false;
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
      RSDataAvailableFlag = false;
      RSSplashPg();
      break;

    case 200:
      mode = 0;
      break;

    case 3:
      rotateArray(statMenuOrder, 3, true);  break;
  }
}

//                                                                       ____  
//                                                                      |  _ \ 
//                                                                      | | | |
//                                                                      | |_| |
//                                                                      |____/ 

void dashboardPg() {                               /////////////////////////////////////////////
  char row1[20];
  char row2[20];
  char row3[20];
  char row4[20];

  if (RSDataAvailableFlag) {
    sprintf(row1, "P0%s   %s  %s", String(deviceID), temp.c_str(), soc.c_str());
    sprintf(row2, "%s  %s", voltage.c_str(), current.c_str());
    sprintf(row3, "Power :%s", power.c_str());
    sprintf(row4, "Energy:%s", wattHour.c_str());

    lcdUpdate(row1, 0, 1);
    lcdUpdate(row2, 1, 1);
    lcdUpdate(row3, 2, 1);
    lcdUpdate(row4, 3, 0);
  }
  else RSSplashPg();

  switch (getBtnStatus()) {

    case 1:
      rotateArray(subMenuOrder, 3 , false);
      if (subMenuOrder[1] == 0)deviceID = 1;
      else if (subMenuOrder[1] == 1)deviceID = 2;
      else if (subMenuOrder[1] == 2)deviceID = 3;
      RSDataAvailableFlag = false;
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
      RSDataAvailableFlag = false;
      RSSplashPg();
      break;

  }
}

//                                                          _____
//                                                         / ____|
//                                                        | (___  
//                                                         \___ \
//                                                         ____) |
//                                                        |_____/ 


void statPg() {                         ///4                   ////////////////////////////////////////////StatPg
  String title[] = {"Current", "Power", "Energy"};
  String value = "--";
  char row[20];

  if (RSDataAvailableFlag) {
    RSDataAvailableFlag = false;
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
    case 1: sprintf(row,"MPPT :%s" , value);
      lcdUpdate(row,2, 0); break;
    case 2: sprintf(row,"Batt.:%s" , value);
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


//void gyroPg() {

//  char row2[20];
//  char row3[20];
//  char row4[20];
//  sprintf(row2, "ax :  %.3f", fAcc[0]);
//    sprintf(row3, "ay :  %.3f", fAcc[1]);
//    sprintf(row4, "az :  %.3f", fAcc[2]);
//  if (s_cDataUpdate & ACC_UPDATE) {
//    sprintf(row2, "ax :  %.3f", fAcc[0]);
//    sprintf(row3, "ay :  %.3f", fAcc[1]);
//    sprintf(row4, "az :  %.3f", fAcc[2]);
//    s_cDataUpdate &= ~ACC_UPDATE;
//  }
//  
//  s_cDataUpdate &= ~GYRO_UPDATE;
//  s_cDataUpdate &= ~ANGLE_UPDATE;
//  s_cDataUpdate &= ~MAG_UPDATE;
//
//  lcdUpdate("Acceleration", 0, 1);
//  lcdUpdate(row2, 1, 1);
//  lcdUpdate(row3, 2, 1);
//  lcdUpdate(row4, 3, 0);
//  s_cDataUpdate = 0;

//  switch (getBtnStatus()) {
//    case 2:
//      mode = 0; break;
//  }
//}



String r0 = "                ", r1 = "                ", r2 = "                ", r3 = "                ";
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
      if(longPress==50)lcd.clear();
      longPress++;      
      delay(12);
    }
    if (longPress >50) {
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
  while (Serial.available() && ccc < 100) {
    ccc++;
    char inChar = (char)Serial.read();
    if (inChar == '\n' && receivedString.length() > 1) {
      const char* string1 = receivedString.c_str();
      decodeMsg(receivedString);
      if (mode == 485)RSReadPg(receivedString, 0);
      if (cloudEnabled)client.publish("RSDATA", string1);
      delay(30);
      receivedString = "";
    } else {
      receivedString += inChar;
      
    }
  }
}


void decodeMsg(String msg) {
  int indexToExtract = msg.indexOf("r50=");
  if (indexToExtract > 0) {
    msg = msg.substring(indexToExtract);
    // Split the comma-separated values into an array
    String values[16];
    int valueCount = splitString(msg, ',', values);
    int receivedDeviceID = msg.substring(4, 5).toInt();
    if (valueCount >= 13 && receivedDeviceID == deviceID) {
      RSDataAvailableFlag = true;
      voltage = formatValue(values[2], 0.01, 2) + "V";
      current = getCurrentDir(values[11].toInt()) + formatValue(values[3], 0.01, 2) + "A";
      wattHour = formatValue(values[6], 0.00001, 4) + "kWh";
      soc = calculateSOC(values[5], values[4]) + "%";
      temp = values[8].substring(1) + "*C"; // Temperature at 8th position
      power = calculatePwr(voltage, current) + "W"; // Power at 13th position

    }
  }
}


int splitString(String input, char separator, String values[]) {
  int valueCount = 0;
  int startIndex = 0;
  while (startIndex >= 0) {
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
  int socPercentage = int((remainingAh / 120000.0 * 100));
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
  if(s_ucRxCnt<3)return;                    //Less than three data returned
  if(s_ucRxCnt >= 50) s_ucRxCnt = 0;
  if(s_ucRxCnt >= 3)
  {
    if((s_ucData[1] == '\r') && (s_ucData[2] == '\n'))
    {
      s_cCmd = s_ucData[0];
      memset(s_ucData,0,50);
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

static void ShowHelp(void)
{
  Serial.println("WIT_SDK_DEMO");
//  Serial.print("\r\n************************          HELP           ************************\r\n");
//  Serial.print("UART SEND:a\\r\\n   Acceleration calibration.\r\n");
//  Serial.print("UART SEND:m\\r\\n   Magnetic field calibration,After calibration send:   e\\r\\n   to indicate the end\r\n");
//  Serial.print("UART SEND:U\\r\\n   Bandwidth increase.\r\n");
//  Serial.print("UART SEND:u\\r\\n   Bandwidth reduction.\r\n");
//  Serial.print("UART SEND:B\\r\\n   Baud rate increased to 115200.\r\n");
//  Serial.print("UART SEND:b\\r\\n   Baud rate reduction to 9600.\r\n");
//  Serial.print("UART SEND:R\\r\\n   The return rate increases to 10Hz.\r\n");
//  Serial.print("UART SEND:r\\r\\n   The return rate reduction to 1Hz.\r\n");
//  Serial.print("UART SEND:C\\r\\n   Basic return content: acceleration, angular velocity, angle, magnetic field.\r\n");
//  Serial.print("UART SEND:c\\r\\n   Return content: acceleration.\r\n");
//  Serial.print("UART SEND:h\\r\\n   help.\r\n");
//  Serial.print("******************************************************************************\r\n");
}



static void CmdProcess(void)
{
  switch(s_cCmd)
  {
    case 'a': if(WitStartAccCali() != WIT_HAL_OK) Serial.print("\r\nSet AccCali Error\r\n");
      break;
    case 'm': if(WitStartMagCali() != WIT_HAL_OK) Serial.print("\r\nSet MagCali Error\r\n");
      break;
    case 'e': if(WitStopMagCali() != WIT_HAL_OK) Serial.print("\r\nSet MagCali Error\r\n");
      break;
    case 'u': if(WitSetBandwidth(BANDWIDTH_5HZ) != WIT_HAL_OK) Serial.print("\r\nSet Bandwidth Error\r\n");
      break;
    case 'U': if(WitSetBandwidth(BANDWIDTH_256HZ) != WIT_HAL_OK) Serial.print("\r\nSet Bandwidth Error\r\n");
      break;
    case 'B': if(WitSetUartBaud(WIT_BAUD_115200) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else 
              {
                mySerial.begin(c_uiBaud[WIT_BAUD_115200]);
                Serial.print(" 115200 Baud rate modified successfully\r\n");
              }
      break;
    case 'b': if(WitSetUartBaud(WIT_BAUD_9600) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else 
              {
                mySerial.begin(c_uiBaud[WIT_BAUD_9600]); 
                Serial.print(" 9600 Baud rate modified successfully\r\n");
              }
      break;
    case 'r': if(WitSetOutputRate(RRATE_1HZ) != WIT_HAL_OK)  Serial.print("\r\nSet Baud Error\r\n");
              else Serial.print("\r\nSet Baud Success\r\n");
      break;
    case 'R': if(WitSetOutputRate(RRATE_10HZ) != WIT_HAL_OK) Serial.print("\r\nSet Baud Error\r\n");
              else Serial.print("\r\nSet Baud Success\r\n");
      break;
    case 'C': if(WitSetContent(RSW_ACC|RSW_GYRO|RSW_ANGLE|RSW_MAG) != WIT_HAL_OK) Serial.print("\r\nSet RSW Error\r\n");
      break;
    case 'c': if(WitSetContent(RSW_ACC) != WIT_HAL_OK) Serial.print("\r\nSet RSW Error\r\n");
      break;
    case 'h': ShowHelp();
      break;
    default :break;
  }
  s_cCmd = 0xff;
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
    for(i = 0; i < uiRegNum; i++)
    {
        switch(uiReg)
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
