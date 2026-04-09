#ifndef COMMANDS_H
#define COMMANDS_H

constexpr int CMD_PROCESSING_TIME_MS = 100;

// Exchange Scheme
// 1.Byte: CMD
// 2.Byte: Data (Only for CMD_SET) 

// Define all shared commands here
// All CMDs expect or return uint8_t unless otherwise specified
constexpr uint8_t VERSION_CMD = 1;

#define CMD_GET_VERSION 0x01

#define CMD_GET_DEBUG 0x02
#define CMD_SET_DEBUG 0x03

#define CMD_GET_HIT 0x04
#define CMD_GET_THRESHOLD 0x05
#define CMD_SET_THRESHOLD 0x06
#define CMD_GET_VIBRATION 0x07

#define CMD_GET_COLOR 0x08
#define CMD_SET_COLOR 0x09 // 0: Off, 1: Alternating
#define CMD_GET_BRIGHTNESS 0x0A
#define CMD_SET_BRIGHTNESS 0x0B

#define CMD_SET_AUTO_TURN_OFF 0x0C // 0: Off, 1: On (Automatically turn lights off when threshold is met/target is hit)

#endif // COMMANDS_H