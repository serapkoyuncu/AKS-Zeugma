#include <SPI.h>
#include <mcp2515.h>

struct can_frame Received_Data;
struct can_frame Transmitted_Data;



MCP2515 mcp2515(53);


void setup() {
  Serial.begin(9600);

  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  Transmitted_Data.can_id = 123;
  Transmitted_Data.can_dlc = 8;
  Transmitted_Data.data[1] = 23;
  Transmitted_Data.data[2] = 14;
  Transmitted_Data.data[3] = 53;

  Transmitted_Data.data[4] = 10;
  Transmitted_Data.data[5] = 10;
  Transmitted_Data.data[6] = 10;
  Transmitted_Data.data[7] = 10;

  Serial.println("------- CAN Read ----------");
}


bool a = 0;
int temps[5];
int sumVoltage = 0;
int sumVoltageBytes[2];
int soc = 0;
int socBytes[2];
int powerWatt = 0;
int powerWattBytes[2];
int cells[20];
int cellsBytes[40];

bool al(int _data) {
  unsigned long previousTime = millis();
  while (1) {
    if (millis() - previousTime > 3000) {
      Serial.println("*********************************************************************************************************");
      Transmitted_Data.data[0] = _data;
      mcp2515.sendMessage(&Transmitted_Data);
      previousTime = millis();
      delay(50);
      a = _data;
    }


    if (mcp2515.readMessage(&Received_Data) != MCP2515::ERROR_OK) {
      if (_data == Received_Data.data[0]) {
        // Serial.print("ID: ");
        // Serial.println(Received_Data.can_id, HEX);
        // Serial.print("DLC: ");
        // Serial.println(Received_Data.can_dlc, DEC);
        // Serial.print(" ");


        if (Received_Data.data[0] == 101) {
          temps[0] = Received_Data.data[1];
          temps[1] = Received_Data.data[2];
          temps[2] = Received_Data.data[3];
          temps[3] = Received_Data.data[4];
          temps[5] = max(temps[0], temps[1]);
          temps[5] = max(temps[5], temps[2]);
          temps[5] = max(temps[5], temps[3]);
        }


        // TOPLAM VOLTAJ, SOC, HARCANAN ENERJI
        if (Received_Data.data[0] == 102) {
          sumVoltage = Received_Data.data[1] * 256 + Received_Data.data[2];
          sumVoltageBytes[0] = Received_Data.data[1];
          sumVoltageBytes[1] = Received_Data.data[2];


          soc = Received_Data.data[3] * 256 + Received_Data.data[4];
          socBytes[0] = Received_Data.data[3];
          socBytes[1] = Received_Data.data[4];

          powerWatt = Received_Data.data[3] * 256 + Received_Data.data[4];
          powerWattBytes[0] = Received_Data.data[5];
          powerWattBytes[1] = Received_Data.data[6];
        }


        // BATARYA CELLERI
        if (Received_Data.data[0] == 103) {
          for (int i = 0; i < 6; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 0; i < 3; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }


        if (Received_Data.data[0] == 104) {
          for (int i = 0; i < 6; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 0; i < 3; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }

        if (Received_Data.data[0] == 105) {
          for (int i = 6; i < 12; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 3; i < 6; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }


        if (Received_Data.data[0] == 106) {
          for (int i = 12; i < 18; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 6; i < 9; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }

        if (Received_Data.data[0] == 106) {
          for (int i = 18; i < 24; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 9; i < 12; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }



        if (Received_Data.data[0] == 106) {
          for (int i = 24; i < 30; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 12; i < 15; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }
        
        if (Received_Data.data[0] == 106) {
          for (int i = 30; i < 36; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 15; i < 18; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }
        
        if (Received_Data.data[0] == 106) {
          for (int i = 36; i < 40; i++) {
            cellsBytes[i] = Received_Data.data[i + 1];
          }

          for (int i = 18; i < 20; i++) {
            cells[i] = Received_Data.data[(2 * i) + 1 * 256 + Received_Data.data[(2 * i) + 2];
          }
        }

        for (int i = 0; i < Received_Data.can_dlc; i++) {  // print the data
          Serial.print(Received_Data.data[i], DEC);
          Serial.print(" ");
        }
        Serial.println();
        return true;
      }
    }
  }
}

void loop() {

  Serial.print("");
  if (a) {
    a = 0;
    al(a * 100);
  }
  Transmitted_Data.data[0] = 1;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(101);

  delay(90);

  Transmitted_Data.data[0] = 2;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(102);
  delay(90);

  Transmitted_Data.data[0] = 3;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(103);
  delay(90);

  Transmitted_Data.data[0] = 4;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(104);
  delay(90);

  Transmitted_Data.data[0] = 5;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(105);
  delay(90);

  Transmitted_Data.data[0] = 6;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(106);
  delay(90);

  Transmitted_Data.data[0] = 7;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(107);
  delay(90);

  Transmitted_Data.data[0] = 8;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(108);

  delay(90);

  Transmitted_Data.data[0] = 9;
  mcp2515.sendMessage(&Transmitted_Data);
  delay(90);
  al(109);
  delay(90);

  delay(1000);
  Serial.println();
}
