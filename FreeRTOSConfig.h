/**
 * @file FreeRTOSConfig.h
 * @brief FreeRTOS Kernel Configuration Header
 * @author Vila Ram Varshit (github.com/varshitram7-oss)
 *
 * Tuned for the FreeRTOS Real-Time Multi-Tasking Edge Controller
 * (see main.c): sensor acquisition, JSON telemetry, LCD display,
 * emergency-stop ISR and a 1 s heartbeat software timer.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* ------------------------------------------------------------------ */
/* Core Scheduling                                                     */
/* ------------------------------------------------------------------ */

/* Preemptive scheduling: a higher-priority task (e.g. the emergency   */
/* handler woken by the button ISR) immediately preempts lower-priority */
/* work. Required for the safety-shutdown path in main.c.              */
#define configUSE_PREEMPTION 1

/* Round-robin time slicing between equal-priority tasks so ready tasks */
/* at the same level share the CPU fairly.                             */
#define configUSE_TIME_SLICING 1

/* Use the port's optimised ready-task selection instead of the generic */
/* C implementation.                                                   */
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

/* Tickless idle disabled: the steady 1 ms tick keeps the 200 ms sensor */
/* loop (vTaskDelayUntil) and the 1 s heartbeat timer deterministic.    */
#define configUSE_TICKLESS_IDLE 0

/* CPU clock the port's tick timer is derived from. Must match the clock */
/* the firmware is actually built to run at.                            */
#define configCPU_CLOCK_HZ ( ( unsigned long ) 80000000 )

/* 1000 Hz tick = 1 ms resolution. Fine-grained enough for the 200 ms   */
/* sensor sampling period and the 1000 ms telemetry/heartbeat periods    */
/* used with pdMS_TO_TICKS() in main.c.                                */
#define configTICK_RATE_HZ ( ( TickType_t ) 1000 )

/* 5 priority levels, exactly matching the priority map in main.c:      */
/* idle(0), Display(1), Telemetry(2), Sensor(3), Emergency(4, highest). */
#define configMAX_PRIORITIES ( 5 )

/* 128 words is the FreeRTOS minimum; tasks that need more request a   */
/* larger stack at creation (see the xTaskCreate calls in main.c).      */
#define configMINIMAL_STACK_SIZE ( ( unsigned short ) 128 )

/* 17 KB heap: covers the four tasks, the sensor + event queues, the    */
/* I2C mutex and the heartbeat software timer with headroom.            */
#define configTOTAL_HEAP_SIZE ( ( size_t ) ( 17 * 1024 ) )

/* 16 chars fits names like "EmergencyTask"/"TelemetryTask".            */
#define configMAX_TASK_NAME_LEN ( 16 )

/* 32-bit tick counts: no 16-bit wraparound to worry about.             */
#define configUSE_16_BIT_TICKS 0

/* Let the idle task yield so background cleanup never starves when it  */
/* shares priority 0.                                                  */
#define configIDLE_SHOULD_YIELD 1

/* ------------------------------------------------------------------ */
/* Synchronisation & Communication                                     */
/* ------------------------------------------------------------------ */

/* Mutexes on: xI2CMutex in main.c guards the I2C bus shared by the     */
/* sensor reader and the LCD display task.                             */
#define configUSE_MUTEXES 1

/* Recursive mutexes on: display helpers may re-lock the same mutex.    */
#define configUSE_RECURSIVE_MUTEXES 1

/* Counting semaphores on: available for event-counting extensions.     */
#define configUSE_COUNTING_SEMAPHORES 1

/* Queue sets on: lets a task block on multiple queues/event sources.  */
#define configUSE_QUEUE_SETS 1

/* Up to 8 queues/semaphores can be registered for kernel-aware         */
/* debugging.                                                          */
#define configQUEUE_REGISTRY_SIZE 8

/* ------------------------------------------------------------------ */
/* Software Timers                                                     */
/* ------------------------------------------------------------------ */

/* Software timers on: drives the 1 s auto-reload [HEARTBEAT] timer.    */
#define configUSE_TIMERS 1

/* Timer service task at priority 2 (same level as telemetry): timer    */
/* callbacks stay responsive without preempting sensor/emergency work.  */
#define configTIMER_TASK_PRIORITY ( 2 )

/* Timer command queue depth of 10: absorbs bursts of timer commands.   */
#define configTIMER_QUEUE_LENGTH 10

/* 256-word stack for the timer service task: comfortably fits its      */
/* callback processing.                                                */
#define configTIMER_TASK_STACK_DEPTH ( configMINIMAL_STACK_SIZE * 2 )

/* ------------------------------------------------------------------ */
/* Memory Allocation & Debug Hooks                                     */
/* ------------------------------------------------------------------ */

/* Both allocation schemes on: tasks use dynamic creation, with static  */
/* allocation available where determinism matters.                     */
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 1

/* Stack overflow checking method 2 (canary words + bounds check):      */
/* catches overflows during development; keep enabled in test builds.   */
#define configCHECK_FOR_STACK_OVERFLOW 2

/* Call the malloc-failed hook so heap exhaustion is loud instead of a  */
/* silent NULL-handle failure.                                         */
#define configUSE_MALLOC_FAILED_HOOK 1

/* ------------------------------------------------------------------ */
/* Optional API functions used by the application                      */
/* ------------------------------------------------------------------ */

#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskCleanUpResources 0
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1

/* High-water-mark API on: lets the application verify task stacks      */
/* still have headroom at runtime.                                     */
#define INCLUDE_uxTaskGetStackHighWaterMark 1

#endif /* FREERTOS_CONFIG_H */
