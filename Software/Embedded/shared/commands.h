#ifndef COMMANDS_H
#define COMMANDS_H

constexpr int CMD_PROCESSING_TIME_MS = 100;

// Exchange Scheme
// 1.Byte: CMD
// 2.Byte: Data (Only for CMD_SET) 

// Define all shared commands here
// All CMDs expect or return uint8_t unless otherwise specified
#define CMD_GET_DEBUG 0x01
#define CMD_GET_HIT 0x02

#define CMD_GET_BRIGHTNESS 0x03
#define CMD_SET_BRIGHTNESS 0x04

#define CMD_GET_THRESHOLD 0x05
#define CMD_SET_THRESHOLD 0x06

#endif // COMMANDS_H