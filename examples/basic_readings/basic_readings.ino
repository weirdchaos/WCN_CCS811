#include "WCN_CCS811.h"

WCN_CCS811 iaq;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("CSS811 Air Quality sensor test...");

  // use begin(address, pin) to let the library manage the WAKE pin
  // or use begin(address) if you manage the WAKE pin in your sketch
  if (!iaq.begin(0x5A))
  {
    Serial.println("Error initializing CSS811 densor!");
    while (1);
  }
  else
  {
    iaq.sensorInfo();
  }
}

void loop() {
  Serial.println("Fetching values");
  byte err = iaq.readData();
  if (err == 0)
  {
    Serial.print("CO2: ");
    Serial.print(iaq.getCO2());
    Serial.println(" ppm");
    Serial.print("tVOC: ");
    Serial.print(iaq.getTVOC());
    Serial.println(" ppb");
  }
  else
  {
    Serial.print("Error ");
    Serial.println(err, BIN);
  }
  delay(5000);
}
