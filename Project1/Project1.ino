#include <WiFi.h>
#include <time.h>
#include <FreeRTOS.h>
#include <task.h>

const char *ssid = "--";
const char *password = "--"; // Wifi Password
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600*7;  // Your timezone offset in seconds thailand gmt +7hours = 3600*7 seconds

// Daylight saving time offset in seconds is the practice of advancing clocks (typically by one hour) 
// during warmer months so that darkness falls at a later clock time.

// Thailand currently observes Indochina Time (ICT) all year. Daylight Saving Time has never been used here.
// Clocks do not change in Thailand. There is no previous DST change in Thailand.
const int daylightOffset_sec = 0; // 1 hour = 3600 sec

// Global variables for time
struct tm timeinfo;

// Function for display Local time
void DisplayLocalTime(void *parameter) {
  for (;;) {

    if (getLocalTime(&timeinfo)) {
      // Print Local time
      Serial.print("Local Time: ");
      Serial.printf("%02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
      Serial.println("Failed to display Time");
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // delay for 1 second
  }
}

// Function for updating Time with NTP time
void UpdateNTPTime(void *parameter) {
  for (;;) {

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    if (getLocalTime(&timeinfo)) {
      Serial.println("Time updated with NTP server");
    } else {
      Serial.println("Failed to update time with NTP server");
    }
    vTaskDelay(pdMS_TO_TICKS(60000)); // Delay for 1 minute
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Connect to Wi-Fi
  Serial.print("Connecting to :");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..."); // Wait for connecting Wifi
  }
  Serial.println("Connected to WiFi"); // Show when Wifi already Connect
  Serial.println("");

  //create task Name "DisplayLocalTime"
  xTaskCreate(
    DisplayLocalTime,        // Call Function
    "DisplayTimeTask",      // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter
    0,                       // Task priority
    NULL                    // Task handle
  );

  //create task Name "UpdateNTPTime"
  xTaskCreate(
    UpdateNTPTime,          // Call Function
    "UpdateNTPTimeTask",    // Task name
    10000,                   // Stack size (bytes)
    NULL,                    // Parameter
    1,                       // Task priority (higher priority)
    NULL                    // Task handle
  );

}

void loop() {
  // put your main code here, to run repeatedly:
}