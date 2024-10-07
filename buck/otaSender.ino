//************************************************************
// this is an example that uses the painlessmesh library to
// upload firmware to another node on the network.
// This will upload to an ESP device running the otaReceiver.ino
//
// The naming convetions should be as follows for bin files
// firmware_<hardware>_<role>.bin
// To create your own binary files, export them and rename
// them to follow this format, where hardware will be
// "ESP8266" or "ESP32" (capitalized.)
// If sending to the otacreceiver sketch, role should be
// "otareceiver" (lowercase)
//
// This sketch assumes a Nodemcu-32s with an SD card
// connected as shown in the jpg included in the sketch folder
// This code may have to be reworked/hardware adjusted for
// other boards or ESP8266. The core code works well though
//
// MAKE SURE YOUR UPLOADED OTA FIRMWARE INCLUDES OTA SUPPORT
// OR YOU WILL LOSE THE ABILITY TO UPLOAD MORE FIRMWARE.
// THE INCLUDED .bin DOES NOT HAVE OTA SUPPORT SO THIS MUST BE
// REFLASHED
//************************************************************
#include "painlessMesh.h"
#include <list>
#include <FS.h>
#include "SD_MMC.h"
#include "SPI.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems

#define MESH_PREFIX ""
#define MESH_PASSWORD ""
#define MESH_PORT 5555

#define OTA_PART_SIZE 1024  //How many bytes to send per OTA data packet

#define MAX_NODES 30

Scheduler userScheduler;
painlessMesh mesh;

static int totalsize = 0;
static uint32_t clients[MAX_NODES] = { 0 };
static int progress[MAX_NODES] = { 0 };

void ChangedConnectionsCallback() {
  std::list<uint32_t> gqlist = mesh.getNodeList(false);
  for (auto g : gqlist) {
    for (int i = 0; i < MAX_NODES; i++) {
      if (clients[i] == 0) {
        clients[i] = g;
        break;
      }
      if (clients[i] == g) {
        break;
      }
    }
  }
}


void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  //disable brownout detector
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // set before init() so that you can see startup messages
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6, 1);
  mesh.setContainsRoot(false);
  mesh.onChangedConnections(&ChangedConnectionsCallback);

  if (!SD_MMC.begin("/sdcard", true)) {
    rebootEspWithReason("Could not mount SD card, restarting");
  }
  uint8_t cardType = SD_MMC.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  File dir = SD_MMC.open("/");
  if (!dir) {
    Serial.println("Failed to open root directory");
    return;
  }
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) {  //End of files
      rebootEspWithReason("Could not find valid firmware, please validate");
    }

    //This block of code parses the file name to make sure it is valid.
    //It will also get the role and hardware the firmware is targeted at.
    if (!entry.isDirectory()) {
      TSTRING name = entry.name();
      Serial.println(name);
      if (name.length() > 1 && name.indexOf('_') != -1 && name.indexOf('_') != name.lastIndexOf('_') && name.indexOf('.') != -1) {
        TSTRING firmware = name.substring(0, name.indexOf('_'));
        TSTRING hardware =
          name.substring(name.indexOf('_') + 1, name.lastIndexOf('_'));
        TSTRING role =
          name.substring(name.lastIndexOf('_') + 1, name.indexOf('.'));
        TSTRING extension =
          name.substring(name.indexOf('.') + 1, name.length());
        Serial.println(firmware);
        Serial.println(hardware);
        Serial.println(role);
        Serial.println(extension);
        if (firmware.equals("firmware") && (hardware.equals("ESP8266") || hardware.equals("ESP32")) && extension.equals("bin")) {

          Serial.println("OTA FIRMWARE FOUND, NOW BROADCASTING");

          //This is the important bit for OTA, up to now was just getting the file.
          //If you are using some other way to upload firmware, possibly from
          //mqtt or something, this is what needs to be changed.
          //This function could also be changed to support OTA of multiple files
          //at the same time, potentially through using the pkg.md5 as a key in
          //a map to determine which to send
          totalsize = ceil(((float)entry.size()) / OTA_PART_SIZE);

          mesh.initOTASend(
            [&entry](painlessmesh::plugin::ota::DataRequest pkg,
                     char* buffer) {
              //fill the buffer with the requested data packet from the node.
              entry.seek(OTA_PART_SIZE * pkg.partNo);
              entry.readBytes(buffer, OTA_PART_SIZE);

              for (int i = 0; i < MAX_NODES; i++) {

                if (clients[i] == 0) {
                  progress[i]++;
                  clients[i] = (uint32_t)pkg.from;
                  break;
                }
                if (clients[i] == (uint32_t)pkg.from) {
                  progress[i]++;
                  break;
                }
              }

              //The buffer can hold OTA_PART_SIZE bytes, but the last packet may
              //not be that long. Return the actual size of the packet.
              return min((unsigned)OTA_PART_SIZE, entry.size() - (OTA_PART_SIZE * pkg.partNo));
            },
            OTA_PART_SIZE);

          //Calculate the MD5 hash of the firmware we are trying to send. This will be used
          //to validate the firmware as well as tell if a node needs this firmware.
          MD5Builder md5;
          md5.begin();
          md5.addStream(entry, entry.size());
          md5.calculate();

          //Make it known to the network that there is OTA firmware available.
          //This will send a message every minute for an hour letting nodes know
          //that firmware is available.
          //This returns a task that allows you to do things on disable or more,
          //like closing your files or whatever.
          mesh.offerOTA(role, hardware, md5.toString(), ceil(((float)entry.size()) / OTA_PART_SIZE), false);

          while (true) {
            //This program will not reach loop() so we dont have to worry about
            //file scope.


            static uint32_t last_time = 0;
            if (millis() - last_time > 1000) {
              int nodecount = 0;
              Serial.println("Update Progress:");
              for (int i = 0; i < MAX_NODES; i++) { 
                mesh.update();
                if (clients[i] != 0) {
                  nodecount++;
                  Serial.print(i+1);
                  Serial.print(" ");
                  //Serial.print(clients[i]);
                  //Serial.print(" ");
                  //Serial.print(progress[i]);
                  Serial.print(" ");
                  Serial.print(((float)progress[i]) / totalsize);
                  Serial.println(" ");
                }
              }
              Serial.println(" ");

              Serial.print("Total Nodes Seen: ");
              Serial.println(nodecount);
              Serial.print("Nodes Online Now: ");
              Serial.println(mesh.getNodeList(false).size());
              Serial.println(" ");
              last_time = millis();
            }
          }
        }
      }
    }
  }
}

void rebootEspWithReason(String reason) {
  Serial.println(reason);
  delay(1000);
  ESP.restart();
}

void loop(){};