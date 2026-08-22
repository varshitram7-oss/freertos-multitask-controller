/**
 * @file main.c
 * @brief FreeRTOS Real-Time Multi-Tasking Edge Controller
 * @author Vila Ram Varshit (github.com/varshitram7-oss)
 */

#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "sensor_types.h"

/* ========================================================================== */
/*                          FREE_RTOS HANDLES                                 */
/* ========================================================================== */

static QueueHandle_t xSensorQueue = NULL;
static QueueHandle_t xEventQueue  = NULL;
static SemaphoreHandle_t xI2CMutex = NULL;
static TimerHandle_t xHeartbeatTimer = NULL;

/* Task Priority Definitions */
#define PRIORITY_EMERGENCY_TASK  ( tskIDLE_PRIORITY + 4 ) /* Highest Priority */
#define PRIORITY_SENSOR_TASK     ( tskIDLE_PRIORITY + 3 )
#define PRIORITY_TELEMETRY_TASK  ( tskIDLE_PRIORITY + 2 )
#define PRIORITY_DISPLAY_TASK    ( tskIDLE_PRIORITY + 1 )

/* Task Timing Definitions */
#define SENSOR_SAMPLE_PERIOD_MS  ( 200 )
#define TELEMETRY_SEND_PERIOD_MS ( 1000 )
#define HEARTBEAT_PERIOD_MS      ( 1000 )

/* ========================================================================== */
/*                      INTERRUPT SERVICE ROUTINE (ISR)                       */
/* ========================================================================== */

/**
 * @brief External GPIO Emergency Alarm ISR
 * Demonstrates deferred interrupt processing using xQueueSendFromISR and portYIELD_FROM_ISR
 */
void IRAM_ATTR Emergency_Button_ISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventMessage_t event = {
        .event_type = EVENT_BUTTON_PRESSED,
        .event_data = 0xFF
    };

    /* Non-blocking queue post from interrupt context */
    xQueueSendFromISR(xEventQueue, &event, &xHigherPriorityTaskWoken);

    /* Context switch if higher priority task was unblocked */
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/* ========================================================================== */
/*                             SOFTWARE TIMERS                                */
/* ========================================================================== */

/**
 * @brief 1-Second Periodic Heartbeat Timer Callback
 */
static void vHeartbeatTimerCallback(TimerHandle_t xTimer)
{
    /* Toggle System Health LED & increment tick heartbeat */
    printf("[HEARTBEAT] System Alive | Free Heap: %d bytes\n", (int)xPortGetFreeHeapSize());
}

/* ========================================================================== */
/*                              TASK DEFINITIONS                              */
/* ========================================================================== */

/**
 * @brief Task 1: Periodic Sensor Acquisition Task (High Priority)
 * Collects analog and digital sensor telemetry using precise vTaskDelayUntil
 */
static void vSensorAcquisitionTask(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    SensorPacket_t packet;
    float simulated_temp = 25.0f;

    while (1)
    {
        /* 1. Acquire I2C Bus Mutex to access simulated sensor */
        if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            /* Read Sensor data */
            simulated_temp += 0.2f;
            if (simulated_temp > 45.0f) simulated_temp = 25.0f;

            packet.timestamp_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
            packet.temperature_c = simulated_temp;
            packet.humidity_pct = 55.4f;
            packet.raw_adc_value = 1024;
            packet.sys_state = (packet.temperature_c > 40.0f) ? SYS_STATE_WARNING : SYS_STATE_NORMAL;

            /* Release I2C Bus Mutex */
            xSemaphoreGive(xI2CMutex);

            /* 2. Post sensor packet to Telemetry & Display Queue */
            xQueueOverwrite(xSensorQueue, &packet);
        }

        /* Enforce strict periodic execution (200ms) without jitter */
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(SENSOR_SAMPLE_PERIOD_MS));
    }
}

/**
 * @brief Task 2: Display & UI Task (Normal Priority)
 * Reads latest sensor packet and updates display under Mutex protection
 */
static void vDisplayTask(void *pvParameters)
{
    SensorPacket_t received_packet;

    while (1)
    {
        /* Wait for new data from queue */
        if (xQueueReceive(xSensorQueue, &received_packet, portMAX_DELAY) == pdTRUE)
        {
            /* Acquire I2C Bus Mutex to update LCD screen */
            if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                printf("[LCD DISPLAY] Temp: %.1f C | Hum: %.1f %% | State: %d\n",
                       received_packet.temperature_c,
                       received_packet.humidity_pct,
                       received_packet.sys_state);

                xSemaphoreGive(xI2CMutex);
            }
        }
    }
}

/**
 * @brief Task 3: Telemetry & Logging Task
 * Transmits formatted JSON telemetry over serial stream
 */
static void vTelemetryTask(void *pvParameters)
{
    SensorPacket_t packet;

    while (1)
    {
        if (xQueuePeek(xSensorQueue, &packet, pdMS_TO_TICKS(500)) == pdTRUE)
        {
            printf("{\"uptime_ms\": %u, \"temp\": %.2f, \"hum\": %.2f, \"status\": %d}\n",
                   packet.timestamp_ms,
                   packet.temperature_c,
                   packet.humidity_pct,
                   packet.sys_state);
        }
        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_SEND_PERIOD_MS));
    }
}

/**
 * @brief Task 4: Emergency Handling Task (Highest Priority)
 * Blocks until woken by ISR event message to execute immediate safety shutdown
 */
static void vEmergencyHandlingTask(void *pvParameters)
{
    EventMessage_t event;

    while (1)
    {
        if (xQueueReceive(xEventQueue, &event, portMAX_DELAY) == pdTRUE)
        {
            if (event.event_type == EVENT_BUTTON_PRESSED)
            {
                printf("\n[EMERGENCY SHUTDOWN] Hardware Alarm Triggered! Halting actuators...\n\n");
            }
        }
    }
}

/* ========================================================================== */
/*                             APPLICATION ENTRY                              */
/* ========================================================================== */

int app_main(void)
{
    printf("=====================================================\n");
    printf("   FreeRTOS Multi-Tasking Edge Controller Starting   \n");
    printf("   Author: Vila Ram Varshit (github.com/varshitram7-oss)\n");
    printf("=====================================================\n");

    /* 1. Create Queues */
    xSensorQueue = xQueueCreate(1, sizeof(SensorPacket_t));
    xEventQueue  = xQueueCreate(5, sizeof(EventMessage_t));

    /* 2. Create Mutex with Priority Inheritance */
    xI2CMutex = xSemaphoreCreateMutex();

    /* 3. Create Software Heartbeat Timer */
    xHeartbeatTimer = xTimerCreate("HeartbeatTimer",
                                   pdMS_TO_TICKS(HEARTBEAT_PERIOD_MS),
                                   pdTRUE, /* Auto-reload */
                                   (void *)0,
                                   vHeartbeatTimerCallback);

    if (xHeartbeatTimer != NULL)
    {
        xTimerStart(xHeartbeatTimer, 0);
    }

    /* 4. Spawn Real-Time Tasks */
    xTaskCreate(vEmergencyHandlingTask, "EmergencyTask", 2048, NULL, PRIORITY_EMERGENCY_TASK, NULL);
    xTaskCreate(vSensorAcquisitionTask, "SensorTask",    2048, NULL, PRIORITY_SENSOR_TASK,    NULL);
    xTaskCreate(vDisplayTask,           "DisplayTask",   2048, NULL, PRIORITY_DISPLAY_TASK,   NULL);
    xTaskCreate(vTelemetryTask,         "TelemetryTask", 2048, NULL, PRIORITY_TELEMETRY_TASK, NULL);

    return 0;
}
