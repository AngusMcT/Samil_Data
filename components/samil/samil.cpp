#include "samil.h"

namespace esphome {
namespace samil {

static const char *const TAG = "samil";

void SamilData::setup() {
  // RS485 enable pin
  pinMode(D7, OUTPUT);

  headerBuffer[0] = 0x55;
  headerBuffer[1] = 0xAA;
  headerBuffer[2] = 0x00;
  headerBuffer[3] = 0x00;

  if (this->samil_online_ != nullptr) {
    this->samil_online_->publish_state(false);
  }
}

void SamilData::update() {
  this->inverterComms();
}

void SamilData::dump_config() {
  ESP_LOGCONFIG(TAG, "Samil Inverter");
  ESP_LOGCONFIG(TAG, "  Update interval: 30s");
  ESP_LOGCONFIG(TAG, "  UART: 9600 baud");
  ESP_LOGCONFIG(TAG, "  RS485 enable pin: D7");
}

void SamilData::inverterComms() {
  if (registeredSamil == 1) {
    getfreshdata();
  }

  if (registeredSamil == 0) {
    connecttoinverter();
  }

  if (millis() > (lastvaliddata + Offline_Timeout)) {
    registeredSamil = 0;

    if (samil_online_ != nullptr) {
      samil_online_->publish_state(false);
    }
  }

  checkIncomingData();
}

int SamilData::sendData(unsigned int address, char controlCode,
                        char functionCode, char dataLength, char *data) {
  digitalWrite(D7, HIGH);
  delay(30);

  headerBuffer[4] = address >> 8;
  headerBuffer[5] = address & 0xFF;
  headerBuffer[6] = controlCode;
  headerBuffer[7] = functionCode;
  headerBuffer[8] = dataLength;

  write_array(headerBuffer, bufferlength);

  if (dataLength) {
    // Samil registration data
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
  }

  uint16_t crc = 0;

  for (int cnt = 0; cnt < 9; cnt++) {
    crc += headerBuffer[cnt];
  }

  if (dataLength) {
    for (int cnt = 0; cnt < dataLength; cnt++) {
      crc += data[cnt];
    }
  }

  auto high = (crc >> 8) & 0xff;
  auto low = crc & 0xff;

  write_byte(high);
  write_byte(low);

  digitalWrite(D7, LOW);

  return 9 + dataLength + 2;
}

void SamilData::getfreshdata() {
  sendData(0x01, 0x01, 0x02, 0x00, nullptr);
  ESP_LOGD(TAG, "Get Fresh Data");
}

void SamilData::checkIncomingData() {
  if (available()) {
    while (available()) {
      uint8_t incomingData = read();

      if (!startPacketReceived &&
          (lastReceivedByte == 0x55 && incomingData == 0xAA)) {

        startPacketReceived = true;
        curReceivePtr = 0;
        numToRead = 0;
        lastReceivedByte = 0x00;

        ESP_LOGD(TAG, "Start Packet received");

      } else if (startPacketReceived) {

        if (numToRead > 0 || curReceivePtr < 7) {
          inputBuffer[curReceivePtr] = incomingData;
          curReceivePtr++;

          if (curReceivePtr == 7) {
            numToRead = inputBuffer[6] + 2;
          } else if (curReceivePtr > 5) {
            numToRead--;
          }
        }

        if (curReceivePtr >= 7 && numToRead == 0) {
          startPacketReceived = false;
          parseIncomingData(curReceivePtr);
        }

      } else {
        lastReceivedByte = incomingData;
      }
    }

    lastReceived = millis();

  } else if (startPacketReceived &&
             millis() - lastReceived > packettimeout) {

    startPacketReceived = false;
    ESP_LOGD(TAG, "Comms Timeout");
  }
}

void SamilData::connecttoinverter() {
  ESP_LOGD(TAG, "Connecting to inverter");

  sendData(0x00, 0x00, 0x00, 0x00, nullptr);
}

void SamilData::parseIncomingData(char incomingDataLength) {
  uint16_t crc = 0x55 + 0xAA;

  for (int cnt = 0; cnt < incomingDataLength - 2; cnt++) {
    crc += inputBuffer[cnt];
  }

  auto high = (crc >> 8) & 0xff;
  auto low = crc & 0xff;

  if (!(high == inputBuffer[incomingDataLength - 2] &&
        low == inputBuffer[incomingDataLength - 1])) {
    return;
  }

  if (inputBuffer[4] == 0x00 && inputBuffer[5] == 0x80) {
    ESP_LOGD(TAG, "Registration data!");
    handleRegistration(inputBuffer + 7, 10);

  } else if (inputBuffer[1] == 0x01 &&
             inputBuffer[5] == 0x81) {

    registeredSamil = 1;

    if (samil_online_ != nullptr) {
      samil_online_->publish_state(true);
    }

    ESP_LOGD(TAG, "Successfully Registered");

  } else if (inputBuffer[1] == 0x01 &&
             inputBuffer[5] == 0x82) {

    handleIncomingInformation(inputBuffer[6], inputBuffer + 7);
  }
}

void SamilData::handleRegistration(char *serialNumber, char length) {
  if (length != 10) {
    return;
  }

  char serialNumb[17];

  strncpy(serialNumb, serialNumber, 10);
  serialNumb[10] = '\0';

  sendAllocateRegisterAddress(lastUsedAddress);
}

void SamilData::sendAllocateRegisterAddress(char address) {
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

  ESP_LOGD(TAG, "Registering Inverter");
}

void SamilData::handleIncomingInformation(char dataLength, char *data) {
  if (dataLength < 36) {
    ESP_LOGD(TAG, "Not enough data");
    return;
  }

  char dtPtr = 0;

  if (samil_temp_ != nullptr)
    samil_temp_->publish_state(bytesToFloat(data, 10));

  dtPtr += 2;

  if (samil_vpv_ != nullptr)
    samil_vpv_->publish_state(bytesToFloat(data + dtPtr, 10));

  dtPtr += 2;

  if (samil_ipv_ != nullptr)
    samil_ipv_->publish_state(bytesToFloat(data + dtPtr, 10));

  dtPtr += 8;

  if (samil_eDay_ != nullptr)
    samil_eDay_->publish_state(bytesToFloat(data + dtPtr, 100));

  dtPtr += 12;

  if (samil_iac_ != nullptr)
    samil_iac_->publish_state(bytesToFloat(data + dtPtr, 10));

  dtPtr += 2;

  if (samil_vac_ != nullptr)
    samil_vac_->publish_state(bytesToFloat(data + dtPtr, 10));

  dtPtr += 2;

  if (samil_fac_ != nullptr)
    samil_fac_->publish_state(bytesToFloat(data + dtPtr, 100));

  dtPtr += 2;

  if (samil_pac_ != nullptr)
    samil_pac_->publish_state(bytesToFloat(data + dtPtr, 1));

  lastvaliddata = millis();

  if (registeredSamil == 0) {
    registeredSamil = 1;

    if (samil_online_ != nullptr) {
      samil_online_->publish_state(true);
    }
  }
}

float SamilData::bytesToFloat(char *bt, char factor) {
  return float(((unsigned short) bt[0] << 8) | bt[1]) / factor;
}

}  // namespace samil
}  // namespace esphome
