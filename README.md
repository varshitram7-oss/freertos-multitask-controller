# FreeRTOS Real-Time Multi-Tasking Edge Controller

[![Language: C](https://img.shields.io/badge/Language-C%2FC%2B%2B-blue.svg)](https://en.wikipedia.org/wiki/C_(programming_language))
[![RTOS: FreeRTOS](https://img.shields.io/badge/RTOS-FreeRTOS%20Kernel-orange.svg)](https://www.freertos.org/)
[![Target: ESP32 / ARM Cortex-M](https://img.shields.io/badge/Target-ESP32%20%2F%20ARM%20Cortex--M-green.svg)](https://www.espressif.com/)
[![Simulation: Wokwi](https://img.shields.io/badge/Simulator-Wokwi%20Live-purple.svg)](https://wokwi.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

An industrial-grade, multi-threaded embedded firmware engine built on the **FreeRTOS Real-Time Operating System**. Demonstrates preemptive priority scheduling, thread-safe inter-task communication (**Queues**), shared resource synchronization with priority inheritance (**Mutexes**), interrupt deferred processing from ISRs (`xQueueSendFromISR`), and auto-reload software timers.

---

## 📌 1. Architectural Overview

In mission-critical embedded systems, real-time operating systems guarantee that time-sensitive operations (such as emergency shutdowns and sensor acquisition) execute within deterministic deadlines without CPU starvation.

```text
                               +------------------------------------------+
                               |     Hardware GPIO Interrupt (EXTI)       |
                               +--------------------+---------------------+
                                                    | (xQueueSendFromISR)
                                                    v
+-------------------+      Queue       +--------------------+      UART   +--------------------+
| vSensorTask       | ---------------> | vDisplayTask       | ----------> | Serial Telemetry   |
| (Priority 3)      |  (Thread-Safe)   | (Priority 1)       |             | JSON Stream        |
+---------+---------+                  +---------+----------+             +--------------------+
          |                                      |
          +------------------+-------------------+
                             |
                             v
                [ Mutex: xI2CMutex ]
           (Guards Shared I2C Sensor/LCD)
```

---

## 📊 2. Task Priority & Resource Matrix

| Task Name | Priority | Period / Trigger | Stack Size | Function & Sync Primitives |
| :--- | :---: | :---: | :---: | :--- |
| **`vEmergencyTask`** | **4 (Highest)** | Event-Driven (ISR) | 2048 B | Blocks on `xEventQueue`. Woken up immediately via `xQueueSendFromISR` on emergency alarm. |
| **`vSensorTask`** | **3 (High)** | 200 ms Periodic | 2048 B | Samples environmental telemetry using `vTaskDelayUntil()`. Guards bus access via `xI2CMutex`. |
| **`vTelemetryTask`** | **2 (Medium)** | 1000 ms Periodic | 2048 B | Peeks latest telemetry data from `xSensorQueue` and outputs formatted JSON packets over UART. |
| **`vDisplayTask`** | **1 (Normal)** | Queue-Driven | 2048 B | Blocks on `xSensorQueue`. Updates LCD display safely under `xI2CMutex` protection. |
| **`vHeartbeatTimer`** | **Timer Daemon**| 1000 ms Auto-Reload | N/A | Software timer monitoring system uptime and calculating free heap memory (`xPortGetFreeHeapSize`). |

---

## 🔑 3. Key RTOS Engineering Concepts Implemented

### A. Priority Inversion Prevention with Mutexes
When low-priority `vDisplayTask` acquires the I2C bus mutex, medium-priority `vTelemetryTask` cannot preempt and delay it indefinitely because FreeRTOS applies **Priority Inheritance** to temporarily elevate `vDisplayTask` to Priority 3 until the mutex is released.

### B. Deferred Interrupt Processing (ISR-to-Task)
To keep hardware Interrupt Service Routines (ISRs) deterministic and ultra-short, the interrupt handler does not execute heavy blocking logic. Instead, it posts a message to `xEventQueue` using `xQueueSendFromISR` and immediately requests a context switch via `portYIELD_FROM_ISR`.

### C. Jitter-Free Periodic Execution
Instead of using standard `vTaskDelay()` (which causes cumulative timing drift), `vSensorAcquisitionTask` uses `vTaskDelayUntil()` to guarantee exactly 200 ms sampling intervals regardless of task execution duration.

---

## 💻 4. Core Implementation Snippets

### Inter-Task Synchronization & Queue Transfer
```c
/* Post telemetry to display queue */
if (xSemaphoreTake(xI2CMutex, pdMS_TO_TICKS(50)) == pdTRUE)
{
    packet.temperature_c = read_temperature();
    packet.timestamp_ms  = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    xSemaphoreGive(xI2CMutex);
    xQueueOverwrite(xSensorQueue, &packet);
}
```

### Context Switch Request from ISR
```c
void IRAM_ATTR Emergency_Button_ISR(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventMessage_t event = { .event_type = EVENT_BUTTON_PRESSED, .event_data = 0xFF };

    xQueueSendFromISR(xEventQueue, &event, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

---

## 🚀 5. How to Run in Free Wokwi Simulator

You can simulate this entire firmware without physical hardware:
1. Open [Wokwi ESP32 Simulator](https://wokwi.com/).
2. Load `main.c` and `diagram.json` from this repository.
3. Click the **Play (Start Simulation)** button.
4. Open the Serial Monitor to observe multi-tasking logs, JSON telemetry, and click the red button to trigger the Emergency ISR.

---

## 🎯 6. Technical Interview Questions & Answers

### Q1: What is the difference between a Binary Semaphore and a Mutex in FreeRTOS?
> A **Mutex** includes a **Priority Inheritance mechanism** to prevent unbounded Priority Inversion when sharing resources, and must be released by the same task that acquired it. A **Binary Semaphore** is used for task-to-task or ISR-to-task synchronization (signaling) and does not have priority inheritance or ownership.

### Q2: Why should you avoid calling standard FreeRTOS APIs inside an ISR?
> Standard APIs (e.g. `xQueueSend`) are blocking and can put the CPU into a wait state, which is illegal inside an interrupt context. FreeRTOS provides dedicated ISR-safe APIs (e.g. `xQueueSendFromISR`) that do not block and include the `pxHigherPriorityTaskWoken` parameter to perform immediate context switching on ISR exit.

### Q3: What is the difference between `vTaskDelay()` and `vTaskDelayUntil()`?
> `vTaskDelay(n)` delays the task for `n` ticks relative to when the function is called, leading to timing drift over time. `vTaskDelayUntil()` calculates the delay relative to the exact previous wake time, guaranteeing an exact periodic execution frequency without timing jitter.

---

## 👤 Author

**Vila Ram Varshit**  
- **LinkedIn:** [linkedin.com/in/ram-varshit-ece](https://www.linkedin.com/in/ram-varshit-ece/)  
- **GitHub:** [github.com/varshitram7-oss](https://github.com/varshitram7-oss)  
- **Email:** ramvarshit18@gmail.com
