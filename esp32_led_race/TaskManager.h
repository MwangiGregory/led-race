#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>

/**
 * @file TaskManager.h
 * @brief Declarations of the FreeRTOS background tasks managing physics, rendering, and UI.
 */

/**
 * @brief Task managing the high-frequency physics calculation loop.
 *        Runs on Core 1 at 60Hz.
 */
void vPhysicsEngineTask(void* pvParameters);

/**
 * @brief Task managing the track display LED animations and dirty-flag rendering.
 *        Runs on Core 1 at ~50fps (20ms interval).
 */
void vTrackRenderTask(void* pvParameters);

/**
 * @brief Task managing the web server telemetry and physical LCD/button UI.
 *        Runs on Core 0 at ~20Hz (50ms interval).
 */
void vAdminUiTask(void* pvParameters);

#endif // TASK_MANAGER_H