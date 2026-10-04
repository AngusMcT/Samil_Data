#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace samil {

class SamilData : public PollingComponent, public uart::UARTDevice {
 public:
  SamilData() : PollingComponent(30000), UARTDevice() {}

  // Sensors
  void set_samil_temp_sensor(sensor::Sensor *sensor) { this->samil_temp_ = sensor; }
  void set_samil_vpv_sensor(sensor::Sensor *sensor) { this->samil_vpv_ = sensor; }
  void set_samil_ipv_sensor(sensor::Sensor *sensor) { this->samil_ipv_ = sensor; }
  void set_samil_e_day_sensor(sensor::Sensor *sensor) { this->samil_eDay_ = sensor; }
  void set_samil_iac_sensor(sensor::Sensor *sensor) { this->samil_iac_ = sensor; }
  void set_samil_vac_sensor(sensor::Sensor *sensor) { this->samil_vac_ = sensor; }
  void set_samil_fac_sensor(sensor::Sensor *sensor) { this->samil_fac_ = sensor; }
  void set_samil_pac_sensor(sensor::Sensor *sensor) { this->samil_pac_ = sensor; }
  void set_samil_online_sensor(sensor::Sensor *sensor) { this->samil_online_ = sensor; }

  void setup() override;
  void update() override;
  void dump_config() override;

 protected:
  sensor::Sensor *samil_temp_{nullptr};
  sensor::Sensor *samil_vpv_{nullptr};
  sensor::Sensor *samil_ipv_{nullptr};
  sensor::Sensor *samil_eDay_{nullptr};
  sensor::Sensor *samil_iac_{nullptr};
  sensor::Sensor *samil_vac_{nullptr};
  sensor::Sensor *samil_fac_{nullptr};
  sensor::Sensor *samil_pac_{nullptr};
  sensor::Sensor *samil_online_{nullptr};

  static const int BufferSize = 96;
  static const int bufferlength = 9;
  static const int datalength = 11;

  uint8_t headerBuffer[bufferlength];
  char inputBuffer[BufferSize];

  unsigned long lastvaliddata{0};
  unsigned long Offline_Timeout{120000};
  int registeredSamil{1};
  unsigned long packettimeout{500};

  unsigned long lastReceived{0};
  bool startPacketReceived{false};
  char lastReceivedByte{0};
  int curReceivePtr{0};
  int numToRead{0};
  char lastUsedAddress{1};

  void inverterComms();
  int sendData(unsigned int address, char controlCode, char functionCode,
               char dataLength, char *data);
  void getfreshdata();
  void checkIncomingData();
  void connecttoinverter();
  void parseIncomingData(char incomingDataLength);
  void handleRegistration(char *serialNumber, char length);
  void sendAllocateRegisterAddress(char address);
  void handleIncomingInformation(char dataLength, char *data);

  float bytesToFloat(char *bt, char factor);
};

}  // namespace samil
}  // namespace esphome
