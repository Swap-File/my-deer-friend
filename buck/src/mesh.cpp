
#include "painlessMesh.h"
#include "mesh.h"
#include <list>
#include <SPIFFS.h>

#define MESH_PREFIX ""
#define MESH_PASSWORD ""
#define MESH_PORT 5555

#define OTA_PART_SIZE 1024 // How many bytes to send per OTA data packet

#define BROADCAST_DELAY 500 // 500 ms delay to handle latency across mesh (Adjust as needed)

painlessMesh mesh;
Scheduler userScheduler;
int totalsize = 0;
uint32_t clients[MAX_NODES] = {0};
int progress[MAX_NODES] = {0};

int mesh_nodes(void)
{
    return mesh.getNodeList(false).size();
}

void mesh_update(void)
{
    mesh.update();
}

char buffer[50];

void mesh_announce_buck(char letter,char *ch)
{
    sprintf(buffer, "%c %u %s",letter, mesh.getNodeTime() + BROADCAST_DELAY, ch);
    Serial.println(buffer);
    mesh.sendBroadcast(buffer, false);
}

void ChangedConnectionsCallback()
{
    std::list<uint32_t> gqlist = mesh.getNodeList(false);
    for (auto g : gqlist)
    {
        for (int i = 0; i < MAX_NODES; i++)
        {
            if (clients[i] == 0)
            {
                clients[i] = g;
                break;
            }
            if (clients[i] == g)
            {
                break;
            }
        }
    }
}

File entry;

bool mesh_init(void)
{

    mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6, 1);
    mesh.setContainsRoot(false);
    mesh.onChangedConnections(&ChangedConnectionsCallback);

    if (!SPIFFS.begin(true))
    {
        Serial.println("Not able to mount SPIFFS");
        return false;
    }

    entry = SPIFFS.open("/firmware.bin", "r");
    if (!entry)
        return false;

    TSTRING hardware = "ESP32";
    TSTRING role = "otareceiver";

    Serial.println("OTA FIRMWARE FOUND, NOW BROADCASTING");

    // This is the important bit for OTA, up to now was just getting the file.
    // If you are using some other way to upload firmware, possibly from
    // mqtt or something, this is what needs to be changed.
    // This function could also be changed to support OTA of multiple files
    // at the same time, potentially through using the pkg.md5 as a key in
    // a map to determine which to send
    totalsize = ceil(((float)entry.size()) / OTA_PART_SIZE);

    Serial.println(totalsize);

    mesh.initOTASend(
        [](painlessmesh::plugin::ota::DataRequest pkg,
           char *buffer)
        {
            // fill the buffer with the requested data packet from the node.
            entry.seek(OTA_PART_SIZE * pkg.partNo, SeekSet);
            entry.readBytes(buffer, OTA_PART_SIZE);

            for (int i = 0; i < MAX_NODES; i++)
            {

                if (clients[i] == 0)
                {
                    progress[i]++;
                    clients[i] = (uint32_t)pkg.from;
                    break;
                }
                if (clients[i] == (uint32_t)pkg.from)
                {
                    progress[i]++;
                    break;
                }
            }

            // The buffer can hold OTA_PART_SIZE bytes, but the last packet may
            // not be that long. Return the actual size of the packet.
            return min((unsigned)OTA_PART_SIZE, entry.size() - (OTA_PART_SIZE * pkg.partNo));
        },
        OTA_PART_SIZE);

    // Calculate the MD5 hash of the firmware we are trying to send. This will be used
    // to validate the firmware as well as tell if a node needs this firmware.
    MD5Builder md5;
    md5.begin();
    md5.addStream(entry, entry.size());
    md5.calculate();

    // Make it known to the network that there is OTA firmware available.
    // This will send a message every minute for an hour letting nodes know
    // that firmware is available.
    // This returns a task that allows you to do things on disable or more,
    // like closing your files or whatever.
    mesh.offerOTA(role, hardware, md5.toString(), ceil(((float)entry.size()) / OTA_PART_SIZE), false);
    return true;
}