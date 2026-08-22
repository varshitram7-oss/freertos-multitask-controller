/**
 * @file sensor_types.h
 * @brief Data structures and type definitions for FreeRTOS Edge Controller
 * @author Vila Ram Varshit (github.com/varshitram7-oss)
 */

#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief System Operating and Alarm States
 */
typedef enum {
    SYS_STATE_NORMAL = 0,
    SYS_STATE_WARNING,
    SYS_STATE_EMERGENCY_SHUTDOWN
} SystemState_t;

/**
 * @brief Telemetry Data Packet transmitted across FreeRTOS Queues
 */
typedef struct {
    uint32_t timestamp_ms;   /* System uptime tick in milliseconds */
    float temperature_c;     /* Temperature in Celsius */
    float humidity_pct;      /* Relative humidity percentage */
    uint16_t raw_adc_value;  /* Raw ADC sensor reading */
    SystemState_t sys_state; /* Current operational status */
} SensorPacket_t;

/**
 * @brief Event message types for the Event Queue
 */
typedef enum {
    EVENT_BUTTON_PRESSED = 1,
    EVENT_SENSOR_THRESHOLD_EXCEEDED,
    EVENT_HEARTBEAT_TIMEOUT
} EventType_t;

typedef struct {
    EventType_t event_type;
    uint32_t event_data;
} EventMessage_t;

#endif /* SENSOR_TYPES_H */
