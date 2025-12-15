
#pragma once
#include <Arduino.h>

/**
 * @brief Maximum number of scheduled cron jobs.
 *
 * This limit ensures efficient memory usage on the ESP8266.
 */
#define MAX_CRON_JOBS 32

/**
 * @brief Default timezone string for UTC.
 *
 * UTC has no offset and no daylight-saving rules, making it the
 * simplest and most stable choice for systems that do not require
 * local civil time.
 *
 * Used by configTime() to configure the ESP8266 timezone.
 */
#define DEFAULT_TZ "UTC0"

/**
 * @brief Time window (in seconds) for cron job execution.
 *
 * Due to the non-deterministic nature of microcontroller loops,
 * a job is considered matched if the current time falls within
 * this window around the scheduled execution time.
 */
#define CRON_EXEC_WINDOW_SEC 2

/**
 * @brief Actions that can be performed by a cron job.
 *
 * - SetPinState: Set a GPIO pin to HIGH or LOW
 * - TogglePinState: Toggle the current state of a GPIO pin
 * - HttpRequest: Perform an HTTP GET request to a specified URL
 * - Reboot: Reboot the device
 */
enum CronAction { SetPinState = 0, TogglePinState, HttpRequest, Reboot };

/**
 * @brief Represents a scheduled cron job.
 *
 * Each job includes:
 * - active: Whether the job is enabled
 * - cron: The cron expression defining the schedule
 * - action: The action to perform
 * - pin: The target GPIO pin (if applicable)
 * - value: The value associated with the action (if applicable)
 *
 * The field `lastExecEpoch` stores the timestamp of the last execution.
 * It is required because, on a microcontroller like ESP8266, the main loop
 * does not run at deterministic intervals: WiFi operations, interrupts,
 * or internal delays may cause the scheduler to miss the exact trigger
 * second of a cron event.
 *
 * To avoid losing jobs, the scheduler uses a "time window" around the
 * target execution point. If the current time falls within this window,
 * the job is considered matched — but it must only run once.
 *
 * Storing `lastExecEpoch` allows the scheduler to:
 * - prevent multiple executions within the same window,
 * - detect whether the job has already run for the current cycle,
 * - remain robust even when time checks occur late or irregularly.
 *
 * The type is uint32_t (unsigned epoch time), which is not affected by
 * the Year 2038 problem and remains valid until the year 2106.
 */
struct CronJob {
  bool active;
  char cron[32];
  CronAction action;
  uint8_t pin;
  int value;
  uint32_t lastExecEpoch; // Y2038-safe (unsigned); valid until year 2106
};

/**
 * @brief Converts a CronAction enum to its string representation.
 *
 * @param action The CronAction enum value
 * @return The corresponding string representation
 */
String cronActionToString(CronAction action);

/**
 * @brief Initializes the cron scheduler.
 *
 * - Sets up internal data structures and prepares the scheduler
 * - Inizialization of cron jobs storage
 * - Inizialization of NTPClient for timekeeping
 *
 * @return true if initialization was successful, false otherwise
 */
bool cronSchedulerInit();

/**
 * @brief Sets a cron job at the specified index.
 *
 * @param index The index of the cron job to set (0 to MAX_CRON_JOBS-1)
 * @param job The CronJob structure containing the job details
 * @return true if the job was set successfully, false otherwise
 */
bool setCronJob(uint8_t index, const CronJob &job);

/**
 * @brief Retrieves all cron jobs.
 *
 * @return Pointer to the array of all cron jobs.
 */
CronJob *cronGetAll();

/**
 * @brief Retrieves a cron job by index.
 *
 * @param index The index of the cron job to retrieve (0 to MAX_CRON_JOBS-1)
 * @return Pointer to the CronJob structure at the specified index, or nullptr
 * if index is out of range
 */
CronJob *cronGet(uint8_t index);

/**
 * @brief Main loop function for the cron scheduler.
 */
void cronSchedulerLoop();
