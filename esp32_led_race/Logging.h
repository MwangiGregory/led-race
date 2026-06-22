#ifndef LOGGING_H
#define LOGGING_H

#include <Arduino.h>

/**
 * @file Logging.h
 * @brief Standard and time-throttled serial logging macros to control console printing.
 */

// Standard system event logging macro (wraps Serial.printf)
#define SYS_LOG(...) Serial.printf(__VA_ARGS__)

// Throttled logging macro that executes Serial.printf at most once every interval_ms
#define SYS_LOG_THROTTLED(interval_ms, ...) \
    do { \
        static uint32_t _lastLogTime = 0; \
        uint32_t _now = millis(); \
        if (_now - _lastLogTime >= (interval_ms)) { \
            _lastLogTime = _now; \
            Serial.printf(__VA_ARGS__); \
        } \
    } while (0)

#endif // LOGGING_H
