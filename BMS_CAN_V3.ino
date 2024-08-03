#include <SPI.h>
#include <mcp2515.h>

struct can_frame Received_Data;
struct can_frame Transmitted_Data;

MCP2515 mcp2515(53);

// Data structures to store BMS information
struct {
  uint8_t temps[5];
  uint8_t maxTemp;
  uint16_t sumVoltage;
  uint16_t soc;
  uint16_t powerWatt;
  uint16_t cellVoltages[20];
  int16_t current;
} bmsData;

void setup() {
  Serial.begin(9600);
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  Transmitted_Data.can_id = 123;
  Transmitted_Data.can_dlc = 1;
  
  Serial.println("------- CAN Read ----------");
}

bool requestData(uint8_t requestId) {
  unsigned long startTime = millis();
  
  while (millis() - startTime < 3000) {
    Transmitted_Data.data[0] = requestId;
    mcp2515.sendMessage(&Transmitted_Data);
    
    delay(90);
    
    if (mcp2515.readMessage(&Received_Data) == MCP2515::ERROR_OK) {
      if (Received_Data.data[0] == requestId + 100) {
        processReceivedData(requestId + 100);
        return true;
      }
    }
  }
  
  Serial.println("Timeout waiting for response");
  return false;
}

void processReceivedData(uint8_t responseId) {
  switch (responseId) {
    case 101:
      for (int i = 0; i < 4; i++) {
        bmsData.temps[i] = Received_Data.data[i + 1];
      }
      bmsData.maxTemp = Received_Data.data[5];
      break;
    case 102:
      bmsData.sumVoltage = (Received_Data.data[1] << 8) | Received_Data.data[2];
      bmsData.soc = (Received_Data.data[3] << 8) | Received_Data.data[4];
      bmsData.powerWatt = (Received_Data.data[5] << 8) | Received_Data.data[6];
      break;
    case 103:
    case 104:
    case 105:
    case 106:
    case 107:
    case 108:
      for (int i = 0; i < 3; i++) {
        int index = (responseId - 103) * 3 + i;
        bmsData.cellVoltages[index] = (Received_Data.data[i*2 + 1] << 8) | Received_Data.data[i*2 + 2];
      }
      break;
    case 109:
      bmsData.cellVoltages[18] = (Received_Data.data[1] << 8) | Received_Data.data[2];
      bmsData.cellVoltages[19] = (Received_Data.data[3] << 8) | Received_Data.data[4];
      break;
  }
}

void printBmsData() {
  Serial.println("BMS Data:");
  
  Serial.print("Temperatures: ");
  for (int i = 0; i < 4; i++) {
    Serial.print(bmsData.temps[i]); Serial.print(" ");
  }
  Serial.println();
  
  Serial.print("Max Temperature: "); Serial.println(bmsData.maxTemp);
  Serial.print("Sum Voltage: "); Serial.println(bmsData.sumVoltage);
  Serial.print("SOC: "); Serial.println(bmsData.soc);
  Serial.print("Power: "); Serial.println(bmsData.powerWatt);
  
  Serial.println("Cell Voltages:");
  for (int i = 0; i < 20; i++) {
    Serial.print(bmsData.cellVoltages[i]); Serial.print(" ");
    if ((i + 1) % 5 == 0) Serial.println();
  }
  
  Serial.print("Current: "); Serial.println(bmsData.current);
  Serial.println();
}

void loop() {
  for (int i = 1; i <= 9; i++) {
    requestData(i);
  }
  
  printBmsData();
  
}
