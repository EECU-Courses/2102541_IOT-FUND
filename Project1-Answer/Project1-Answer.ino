#include <WiFi.h>         // WiFi
#include <esp_wpa2.h>     // wpa2 library for connections to Enterprise networks
#include <ESPDateTime.h>  // DateTime lib for ESP32/ESP8266
#include <DateTimeTZ.h>   // Timezone definition

struct AP_Info {
  String SSID_Name, userName, passPhase;
  AP_Info(String sName, String pPhase, String uName=String("")) : SSID_Name(sName), userName(uName), passPhase(pPhase) {}
};
//AP_Info SchoolAP("ChulaWiFi", "CUNetPassword", "StudentID");
//AP_Info HomeAP("myHomeSSID", "HomePassphase");
//AP_Info MobileAP("hsp", "HotspotPassword");

//if you define your SchoolAP, HomeAP and MobileAP above, don't to include "mySSID.h"
#include "mySSID.h"  // define SSID names, user names & passwords

// print out sth if only verbose is true
const bool verbose {true};

// declare WIFI functions
bool connectWiFi(bool verbose = false);

// define two tasks for RTC display & NTP update
void TaskRTC(void *pvParameters);
void TaskNTP(void *pvParameters);


inline void lock(SemaphoreHandle_t &aMutex) {
  while (1)
    if (xSemaphoreTake(aMutex, 10) == true) break;
}

inline void unlock(SemaphoreHandle_t &aMutex) 
{
  xSemaphoreGive(aMutex);
}

inline void Sprint(const String &toPrint, SemaphoreHandle_t &SMutex) {
  lock(SMutex); 
  Serial.print(toPrint);
  unlock(SMutex); 
}
inline void Sprint(String &toPrint, SemaphoreHandle_t &SMutex) {
  lock(SMutex); 
  Serial.print(toPrint);
  unlock(SMutex); 
}

// the setup function runs once when you press reset or power the board
void setup() {
  // Create a mutex for Serial Object
  //SemaphoreHandle_t serialMutex = xSemaphoreCreateMutex();
  SemaphoreHandle_t *serialMutexPtr = new SemaphoreHandle_t {xSemaphoreCreateMutex()};
  if (verbose) {
  //  Serial.begin(115200, SERIAL_8N1, 44, 43);
    Serial.begin(115200);
    // Check if the serial port is ready
    while (!Serial) delay(10);
  }

  // connect to WiFi, status == true if connected
  bool status = connectWiFi(true);

  // update NTP Time for the firsttime if WiFi is connected
  if (status) {
    DateTime.setServer("th.pool.ntp.org");
    DateTime.setTimeZone(TZ_Asia_Bangkok);
    Serial.print("Date/time was " + DateTime.toISOString() +"\n");
    DateTime.begin();
    Serial.print("NTP sets Date/time to " + DateTime.toISOString() +"\n");
  }
  // otherwise do nothing 
  else {
    Sprint("\nNo WiFi Available, the END!\n", *serialMutexPtr);
    while(1);  // wait here forever!!!
  }
  
  // Now set up two tasks to run independently.
  xTaskCreate(
    TaskRTC,                     // function name to run the task
    "Display the RTC Time.",     // A name just for humans
    2048,                        // This stack size can be checked & adjusted by reading the Stack Highwater
    (void *)serialMutexPtr,      // pass lock as a parameter
    1,                           // (configMAX_PRIORITIES - 1) -> 0  high -> low
    NULL                         // No handle
  );

  xTaskCreate(
    TaskNTP, 
    "Update NTP clock", 
    2048,  // Stack size
    (void *)serialMutexPtr,                   // pass lock as a parameter
    2,                              // Priority
    NULL
  );
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop() {
  // Empty. Nothing is done in this loop task.
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskRTC(void *pvParameters)  // This is a task.
{
  // setup() part of the thread
  SemaphoreHandle_t *serialMutexPtr = (SemaphoreHandle_t *)pvParameters;
 // Initialise the xLastWakeTime variable with the current time.
  TickType_t xLastWakeTime = xTaskGetTickCount();
  // loop() part of the thread
  while(1) {
    if (verbose) {
        Sprint("Local date/time is " + DateTime.toISOString() +"\n", *serialMutexPtr);
    }
    vTaskDelayUntil( &xLastWakeTime, 1000);
  }
}

void TaskNTP(void *pvParameters)  // This is a task.
{
  // setup() part of the thread
  SemaphoreHandle_t *serialMutexPtr = (SemaphoreHandle_t *)pvParameters;
  // Initialise the xLastWakeTime variable with the current time.
  TickType_t xLastWakeTime = xTaskGetTickCount();

  // loop() part of the thread
  while(1)
  {
    // Wait for the next cycle. 
    // To test, let's update the clock every 15 sec, 
    // But in real life, once a day is usually freuqent enough. 
    vTaskDelayUntil( &xLastWakeTime, 15000);

    // Perform action here. 
    // Try to connect WiFi if not connected already
    if (WiFi.status() != WL_CONNECTED) connectWiFi();

    // Try to connect to NTP if WiFi is connected only
    if (WiFi.status() == WL_CONNECTED) {
      // re-sync the system time with the NTP server
      DateTime.begin(); 
      //-------------------------------------------
      if (!DateTime.isTimeValid()) {
        if (verbose)
          Sprint("Failed to get time from server.\n", *serialMutexPtr);
      }
      else {
        if (verbose)
          Sprint("NTP time is updated; Date/time is now " + DateTime.toISOString() +"\n", *serialMutexPtr);
      }
    }
  } // loop
} // Task



bool connectWiFi(bool verbose) {
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();

  for (int i = 0; i < n; i++) {
    if (verbose)
      Serial.printf("Found AP: %s with %d RSSI\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    // First, check if ChulaWiFi is available   
    if ((SchoolAP.SSID_Name == WiFi.SSID(i)) && (WiFi.RSSI(i) > -80)) {
      if (verbose)
        Serial.printf("Connecting to School Network.");
      WiFi.begin(WiFi.SSID(i).c_str(), WPA2_AUTH_PEAP, SchoolAP.userName.c_str(), SchoolAP.userName.c_str(), SchoolAP.passPhase.c_str());
      for (int j = 0; j < 20; j++) {
        if (WiFi.status() == WL_CONNECTED) {
          if (verbose) Serial.println(" !connected.");
          return true;
        }
        if (verbose)
          Serial.print(".");
        delay(500);
      }
    }

    // If we are not at CU, check if we are at home
    if ((HomeAP.SSID_Name == WiFi.SSID(i)) && (WiFi.RSSI(i) > -80)) {
      if (verbose)
        Serial.print("Connecting to Home Network.");
      WiFi.begin(WiFi.SSID(i).c_str(), HomeAP.passPhase.c_str());
      for (int j = 0; j < 20; j++) {
        if (WiFi.status() == WL_CONNECTED) {
          if (verbose) Serial.println(" !connected.");
          return true;
        }
        if (verbose)
          Serial.print(".");
        delay(500);
      }
    }
  }
  // If we are neither at home nor at CU, try our personal hotspot.
  if (verbose)
    Serial.printf("Trying to connect to %s.", MobileAP.SSID_Name.c_str());
  WiFi.begin(MobileAP.SSID_Name.c_str(), MobileAP.passPhase.c_str());
  for (int j = 0; j < 20; j++) {
    if (WiFi.status() == WL_CONNECTED) {
      if (verbose) Serial.println(" !connected.");
      return true;
    }
    if (verbose)
      Serial.print(".");
    delay(500);
  }
  return false;
}

