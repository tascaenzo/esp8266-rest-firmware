#include "CronScheduler.h"
#include <BinaryStorage.h>
#include <DeviceController.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
static HTTPClient http;

#define STORAGE_PATH "/cron_state.bin"
#define FILE_SIZE sizeof(CronJob) * MAX_CRON_JOBS

static CronJob cronJobsState[MAX_CRON_JOBS];

/**
 * Converts a CronAction enum to its string representation.
 */
String cronActionToString(CronAction action) {
  switch (action) {
  case SetPinState:
    return "Set";
  case TogglePinState:
    return "Toggle";
  case Reboot:
    return "Reboot";
  default:
    return "Unknown";
  }
}

/**
 * @brief Checks if a cron field matches a given value.
 *
 * Supports:
 *  - "*"
 *  - numeric value (e.g. "5")
 *  - ranges (e.g. "1-5")
 *  - lists (e.g. "1,3,5" or "5,10-20")
 *
 * @param expr Cron field expression
 * @param value Value to match against
 * @return true if matches, false otherwise
 */
bool cronFieldMatch(const char *expr, int value) {
  // wildcard "*"
  if (strcmp(expr, "*") == 0)
    return true;

  // copy field
  char buf[16];
  strncpy(buf, expr, sizeof(buf));
  buf[sizeof(buf) - 1] = '\0';

  // split by comma (lists)
  char *tok = strtok(buf, ",");
  while (tok) {
    // range or single?
    char *dash = strchr(tok, '-');
    if (!dash) {
      if (atoi(tok) == value)
        return true;
    } else {
      int a = atoi(tok);
      int b = atoi(dash + 1);
      if (value >= a && value <= b)
        return true;
    }
    tok = strtok(nullptr, ",");
  }

  return false;
}

/**
 * @brief Checks whether a cron job should be executed at the given timestamp.
 *
 * This function:
 *  - evaluates the 5 cron fields (m h dom mon dow)
 *  - computes the target execution second for this minute
 *  - applies a time window (CRON_EXEC_WINDOW_SEC)
 *  - prevents double execution using lastExecEpoch
 *
 * @return true if the job should run now.
 */
bool cronMatch(const CronJob &job, uint32_t nowEpoch) {
  // Convert epoch → local time
  time_t now = nowEpoch;
  struct tm *t = localtime(&now);

  if (!t)
    return false;

  // Split cron fields
  char expr[32];
  strncpy(expr, job.cron, sizeof(expr));
  expr[sizeof(expr) - 1] = '\0';

  char *fields[5];
  int n = 0;
  char *tok = strtok(expr, " ");
  while (tok && n < 5) {
    fields[n++] = tok;
    tok = strtok(nullptr, " ");
  }
  if (n != 5)
    return false;

  // Check each cron field
  if (!cronFieldMatch(fields[0], t->tm_min))
    return false;
  if (!cronFieldMatch(fields[1], t->tm_hour))
    return false;
  if (!cronFieldMatch(fields[2], t->tm_mday))
    return false;
  if (!cronFieldMatch(fields[3], t->tm_mon + 1))
    return false;
  if (!cronFieldMatch(fields[4], t->tm_wday))
    return false;

  // EXECUTION WINDOW CHECK

  // expected second = 0 (cron standard)
  uint32_t targetEpoch = nowEpoch - t->tm_sec;

  // Window match?
  if (abs((int32_t)(nowEpoch - targetEpoch)) > CRON_EXEC_WINDOW_SEC)
    return false;

  // avoid double execution
  if ((uint32_t)(nowEpoch - job.lastExecEpoch) <= CRON_EXEC_WINDOW_SEC)
    return false;

  return true;
}

/**
 * Initializes the cron scheduler.
 * - Sets up internal data structures and prepares the scheduler
 * - Inizialization of cron jobs storage
 * - Inizialization of NTPClient for timekeeping
 */
bool cronSchedulerInit() {
  timeClient.begin();
  timeClient.update();

  // Timezone Europa/Italia: CET/CEST
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  // setenv("TZ", DEFAULT_TZ, 1);

  tzset();

  // Load cron jobs state from storage
  bool storageOk =
      storageRead(STORAGE_PATH, (uint8_t *)cronJobsState, FILE_SIZE);

  return storageOk;
}

/**
 * Retrieves a cron job by index.
 */
bool setCronJob(uint8_t index, const CronJob &job) {
  if (index >= MAX_CRON_JOBS)
    return false;

  cronJobsState[index] = job;

  // Save to storage
  return storageWrite(STORAGE_PATH, (uint8_t *)cronJobsState, FILE_SIZE);
}

/**
 * Retrieves all cron jobs.
 */
CronJob *cronGetAll() { return cronJobsState; }

/**
 * Retrieves a cron job by index.
 */
CronJob *cronGet(uint8_t index) {
  if (index >= MAX_CRON_JOBS)
    return nullptr;
  return &cronJobsState[index];
}

void cronSchedulerLoop() {
  static unsigned long lastPrint = 0;

  // Aggiorna NTP
  timeClient.update();

  // Stampa solo una volta al secondo
  unsigned long now = millis();
  if (now - lastPrint >= 1000) {
    lastPrint = now;

    for (int i = 0; i < MAX_CRON_JOBS; i++) {

      if (cronJobsState[i].active &&
          cronMatch(cronJobsState[i], timeClient.getEpochTime())) {
        GpioConfig *existing = deviceGet(cronJobsState[i].pin);
        GpioConfig newCfg = *existing;

        // Esegui l'azione
        switch (cronJobsState[i].action) {
        case SetPinState:
          newCfg.state = cronJobsState[i].value;
          deviceSet(newCfg);
          break;
        case TogglePinState:
          newCfg.state = newCfg.state ? 0 : 1;
          deviceSet(newCfg);
          break;
        case HttpRequest:
          break;
        case Reboot:
          ESP.restart();
          break;
        }
        // Aggiorna lastExecEpoch
        cronJobsState[i].lastExecEpoch = timeClient.getEpochTime();
      }
    }
  }
}