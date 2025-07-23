#ifndef _RADIOLIB_EX_LORAWAN_CONFIG_H
#define _RADIOLIB_EX_LORAWAN_CONFIG_H

#include <RadioLib.h>

// TTGO LoRa32 SX1276 PINS & SPI:
SPIClass mySPI(VSPI);
SX1276 radio = new Module(18, 26, 14, -1, mySPI); // CS, DIO0, RST, DIO1, SPI

const uint32_t uplinkIntervalSeconds = 20L;

// TODO: Enter your own LoRaWAN OTAA EUIs & Keys below:
uint64_t joinEUI = 0x0000000000000000; // <-- Replace with your Join EUI
uint64_t devEUI  = 0x0000000000000000; // <-- Replace with your Dev EUI
uint8_t nwkKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // <-- Replace with your NwkKey
uint8_t appKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; // <-- Replace with your AppKey

const LoRaWANBand_t Region = EU868;
const uint8_t subBand = 0;

LoRaWANNode node(&radio, &Region, subBand);

// --- Helpers ---
String stateDecode(const int16_t result) {
  switch (result) {
  case RADIOLIB_ERR_NONE: return "ERR_NONE";
  case RADIOLIB_ERR_CHIP_NOT_FOUND: return "ERR_CHIP_NOT_FOUND";
  case RADIOLIB_ERR_PACKET_TOO_LONG: return "ERR_PACKET_TOO_LONG";
  case RADIOLIB_ERR_RX_TIMEOUT: return "ERR_RX_TIMEOUT";
  case RADIOLIB_ERR_MIC_MISMATCH: return "ERR_MIC_MISMATCH";
  case RADIOLIB_ERR_INVALID_BANDWIDTH: return "ERR_INVALID_BANDWIDTH";
  case RADIOLIB_ERR_INVALID_SPREADING_FACTOR: return "ERR_INVALID_SPREADING_FACTOR";
  case RADIOLIB_ERR_INVALID_CODING_RATE: return "ERR_INVALID_CODING_RATE";
  case RADIOLIB_ERR_INVALID_FREQUENCY: return "ERR_INVALID_FREQUENCY";
  case RADIOLIB_ERR_INVALID_OUTPUT_POWER: return "ERR_INVALID_OUTPUT_POWER";
  case RADIOLIB_ERR_NETWORK_NOT_JOINED: return "RADIOLIB_ERR_NETWORK_NOT_JOINED";
  case RADIOLIB_ERR_DOWNLINK_MALFORMED: return "RADIOLIB_ERR_DOWNLINK_MALFORMED";
  case RADIOLIB_ERR_INVALID_REVISION: return "ERR_INVALID_REVISION";
  case RADIOLIB_ERR_INVALID_PORT: return "ERR_INVALID_PORT";
  case RADIOLIB_ERR_NO_RX_WINDOW: return "ERR_NO_RX_WINDOW";
  case RADIOLIB_ERR_INVALID_CID: return "ERR_INVALID_CID";
  case RADIOLIB_ERR_UPLINK_UNAVAILABLE: return "ERR_UPLINK_UNAVAILABLE";
  case RADIOLIB_ERR_COMMAND_QUEUE_FULL: return "ERR_COMMAND_QUEUE_FULL";
  case RADIOLIB_ERR_COMMAND_QUEUE_ITEM_NOT_FOUND: return "ERR_COMMAND_QUEUE_ITEM_NOT_FOUND";
  case RADIOLIB_ERR_JOIN_NONCE_INVALID: return "ERR_JOIN_NONCE_INVALID";
  case RADIOLIB_ERR_DWELL_TIME_EXCEEDED: return "ERR_DWELL_TIME_EXCEEDED";
  case RADIOLIB_ERR_CHECKSUM_MISMATCH: return "ERR_CHECKSUM_MISMATCH";
  case RADIOLIB_ERR_NO_JOIN_ACCEPT: return "ERR_NO_JOIN_ACCEPT";
  case RADIOLIB_LORAWAN_SESSION_RESTORED: return "RADIOLIB_LORAWAN_SESSION_RESTORED";
  case RADIOLIB_LORAWAN_NEW_SESSION: return "RADIOLIB_LORAWAN_NEW_SESSION";
  case RADIOLIB_ERR_NONCES_DISCARDED: return "ERR_NONCES_DISCARDED";
  case RADIOLIB_ERR_SESSION_DISCARDED: return "ERR_SESSION_DISCARDED";
  }
  return "See https://jgromes.github.io/RadioLib/group__status__codes.html";
}

void debug(bool failed, const __FlashStringHelper* message, int state, bool halt) {
  if(failed) {
    Serial.print(message);
    Serial.print(" - ");
    Serial.print(stateDecode(state));
    Serial.print(" (");
    Serial.print(state);
    Serial.println(")");
    while(halt) { delay(1); }
  }
}

void arrayDump(uint8_t *buffer, uint16_t len) {
  for(uint16_t c = 0; c < len; c++) {
    char b = buffer[c];
    if(b < 0x10) { Serial.print('0'); }
    Serial.print(b, HEX);
  }
  Serial.println();
}

#endif
