#include "esphome.h"

class samildata : public PollingComponent, public UARTDevice {
 public:
// *** Setup variables *** 
  #define RS485_RX D5
  #define RS485_TX D4
  #define RS485_enable D7

  Sensor *samil_temp = new Sensor();
  Sensor *samil_vpv = new Sensor();
  Sensor *samil_ipv = new Sensor();
  Sensor *samil_workMode = new Sensor();
  Sensor *samil_iac = new Sensor();
  Sensor *samil_vac = new Sensor();
  Sensor *samil_fac = new Sensor();
  Sensor *samil_pac = new Sensor();
  Sensor *samil_eTotal = new Sensor();
  Sensor *samil_eDay = new Sensor();
  Sensor *samil_online = new Sensor();
  
  float temp = 0.0;
  float vpv = 0.0;
  float ipv = 0.0;
  short workMode=0;
  float iac = 0.0;
  float vac = 0.0;
  float fac = 0.0;
  short pac=0;
  float eTotal = 0.0;
  float eDay = 0.0;
  
  String hexstr = "";
  String tempid = "";
  static const int BufferSize = 96;
  static const int bufferlength = 9;
  static const int datalength = 11;
  uint8_t headerBuffer[bufferlength];
  char inputBuffer[BufferSize];
  
  unsigned long lastvaliddata = 0; // last time data was recieved from inverter
  unsigned long Offline_Timeout = 120000;
  int registeredSamil = 1; 
  unsigned long packettimeout = 500; 
  
  unsigned long lastReceived = 0;     //timeout detection
  bool startPacketReceived = false;   //start packet marker
  char lastReceivedByte = 0;        //packet start consist of 2 bytes to test. This holds the previous byte
  int curReceivePtr = 0;          //the ptr in our OutputBuffer when reading
  int numToRead = 0;            //number of bytes to read after the header is read.
  char lastUsedAddress = 1;  

// *** variables done ***

  samildata(UARTComponent *parent) : PollingComponent(30000), UARTDevice(parent) {}

  void setup() override {
    pinMode(RS485_enable, OUTPUT);
    headerBuffer[0] = 0x55;
    headerBuffer[1] = 0xAA;
    headerBuffer[2] = 0X00;
    headerBuffer[3] = 0X00;
    samil_online->publish_state(false);
  }

  void update() {
//    ESP_LOGD("Samil", "Polling");
    inverterComms();
//    yield();
  }
  
  void inverterComms() {
    if (registeredSamil == 1) {
        getfreshdata();
 //       checkIncomingData();
    }    
    if (registeredSamil == 0) {
      connecttoinverter();
//      checkIncomingData();
    }
    
    if (millis() > (lastvaliddata + Offline_Timeout)) {
      registeredSamil = 0;
      samil_online->publish_state(false);
    }  
//    getfreshdata();
    checkIncomingData();
  } 
  
  int sendData(unsigned int address, char controlCode, char functionCode, char dataLength, char * data) {
    digitalWrite(RS485_enable, HIGH);
    delay(30);
    //send the header first
    headerBuffer[4] = address >> 8;
    headerBuffer[5] = address & 0xFF;
    headerBuffer[6] = controlCode;
    headerBuffer[7] = functionCode;
    headerBuffer[8] = dataLength;
    write_array(headerBuffer, bufferlength);
//check if we need to write the data part and send it.
    if (dataLength) {
//temp hard coded
        write_byte(0x53);
        write_byte(0x33);
        write_byte(0x33);
        write_byte(0x31);
        write_byte(0x31);
        write_byte(0x35);
        write_byte(0x53);
        write_byte(0x34);
        write_byte(0x36);
        write_byte(0x39);
        write_byte(0x01);
//      write_array(data, dataLength);
    }
//need to send out the crc which is the addition of all previous values.
    uint16_t crc = 0;
    for (int cnt = 0; cnt < 9; cnt++)
    {
      crc += headerBuffer[cnt];
    }
    if (dataLength) {
      for (int cnt = 0; cnt < dataLength; cnt++)
      {
        crc += data[cnt];
      }
    }
    //write out the high and low
    auto high = (crc >> 8) & 0xff;
    auto low = crc & 0xff;
    write_byte(high);
    write_byte(low);
//    delay(15);
    digitalWrite (RS485_enable, LOW);
    return 9 + dataLength + 2; //header, data, crc
  }

  void getfreshdata(){
    sendData(0x01, 0x01, 0x02, 0x00, nullptr);
    ESP_LOGD("Samil", "Get Fresh Data");
    
  }

  void checkIncomingData() {
    if (available()) {
//      ESP_LOGD("Samil", "Data found!");
      while (available()) {
        byte incomingData = read();
        if (!startPacketReceived && (lastReceivedByte == 0x55 && incomingData == 0xAA)) {
          startPacketReceived = true;
          curReceivePtr = 0;
          numToRead = 0;
          lastReceivedByte = 0x00; //reset last received for next packet
          ESP_LOGD("Samil", "Start Packet recieved");
        }
      else if (startPacketReceived)
      {
        if (numToRead > 0 || curReceivePtr < 7)
        {
          inputBuffer[curReceivePtr] = incomingData;
          curReceivePtr++;
          if (curReceivePtr == 7)
          {
            numToRead = inputBuffer[6] + 2;
          }
          else if (curReceivePtr > 5)
            numToRead--;
        }
        if (curReceivePtr >= 7 && numToRead == 0)
        {
          startPacketReceived = false;
          parseIncomingData(curReceivePtr);
        }
      }
      else if (!startPacketReceived)
        lastReceivedByte = incomingData; //keep track of the last incoming byte so we detect the packet start
    }
    lastReceived = millis();
  }
  else if (startPacketReceived && millis() - lastReceived > packettimeout) // 0.5 sec timoeut
  {
    //there is an open packet timeout. 
    startPacketReceived = false; //wait for start packet again
    ESP_LOGD("Samil", "Comms Timeout");
  }
}

void connecttoinverter() {
  ESP_LOGD("Samil", "Connecting to inverter");
  sendData(0x00, 0x00, 0x00, 0x00, nullptr);
//  checkIncomingData();
  
}

void parseIncomingData(char incomingDataLength) //
{
  uint16_t crc = 0x55 + 0xAA;
  for (int cnt = 0; cnt < incomingDataLength - 2; cnt++)
    crc += inputBuffer[cnt];

  auto high = (crc >> 8) & 0xff;
  auto low = crc & 0xff;

  //match the crc
  if (!(high == inputBuffer[incomingDataLength - 2] && low == inputBuffer[incomingDataLength - 1]))
    return;

  //check the control code and function code to see what to do
  if (inputBuffer[4] == 0x00 && inputBuffer[5] == 0x80){
    ESP_LOGD("Samil", "Registration data!");
    handleRegistration(inputBuffer + 7, 10);
  }    
  else if (inputBuffer[1] == 0x01 && inputBuffer[5] == 0x81) {
   registeredSamil = 1;
   samil_online->publish_state(true);
   ESP_LOGD("Samil", "Successfully Registered");
  }
  else if (inputBuffer[1] == 0x01 && inputBuffer[5] == 0x82) {
    handleIncomingInformation(inputBuffer[6], inputBuffer + 7);
  }
}

void handleRegistration(char * serialNumber, char length)
{
  //check if the serialnumber isn't listed yet. If it is use that one
  //Add the serialnumber, generate an address and send it to the inverter
  if (length != 10)
    return;
  
  char serialNumb[17];
  strncpy(serialNumb, serialNumber, 10);
  serialNumb[10] = '\0';
//  sendAllocateRegisterAddress(serialNumb, lastUsedAddress);
  sendAllocateRegisterAddress(lastUsedAddress);

}

// void sendAllocateRegisterAddress(char * serialNumber, char address)
void sendAllocateRegisterAddress(char address)
{
  char RegisterData[17];
  RegisterData[0] = 0x53;
  RegisterData[1] = 0x33;
  RegisterData[2] = 0x33;
  RegisterData[3] = 0x31;
  RegisterData[4] = 0x31;
  RegisterData[5] = 0x35;
  RegisterData[6] = 0x53;
  RegisterData[7] = 0x34;
  RegisterData[8] = 0x36;
  RegisterData[9] = 0x39;
  RegisterData[10] = address;
  

  sendData(0x00, 0x00, 0x01, datalength, RegisterData);
  ESP_LOGD("Samil","Registering Inverter");
//  checkIncomingData();
}  


void handleIncomingInformation(char dataLength, char * data)
{
  if (dataLength < 36) { //minimum for non dt series
    ESP_LOGD("Samil","Not enough data");
    return;
  }
//  ESP_LOGD("Samil","Lets break up the data");
  
  char dtPtr = 0;
  samil_temp->publish_state(bytesToFloat(data, 10));          dtPtr += 2;
  samil_vpv->publish_state(bytesToFloat(data + dtPtr, 10));          dtPtr += 2;
  samil_ipv->publish_state(bytesToFloat(data + dtPtr, 10));          dtPtr += 8;
  samil_eDay->publish_state(bytesToFloat(data + dtPtr, 100));          dtPtr += 12;
  samil_iac->publish_state(bytesToFloat(data + dtPtr, 10));          dtPtr += 2;
  samil_vac->publish_state(bytesToFloat(data + dtPtr, 10));          dtPtr += 2;
  samil_fac->publish_state(bytesToFloat(data + dtPtr, 100));          dtPtr += 2;
  samil_pac->publish_state(bytesToFloat(data + dtPtr, 1));          dtPtr += 2;

  lastvaliddata = millis();
  if (registeredSamil == 0) {
    registeredSamil = 1;
    samil_online->publish_state(true);

  }
}

float bytesToFloat(char * bt, char factor) {
  return float(((unsigned short)bt[0] << 8) | bt[1]) / factor;
}


};