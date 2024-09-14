#define TIME_SYNC_INTERVAL 5000000  // Mesh time resync period, in us. 5 seconds
#define TASK_RES_MULTIPLIER 1
#include "shika.h"
#include "gpio.h"
#include "leds.h"

#define BROADCAST_DELAY 200000  // 200 ms delay to handle latency across mesh (Adjust as needed)

painlessMesh mesh;
SimpleList<uint32_t> nodes;
Scheduler userScheduler;

static void main_loop(void);

Task taskLedAnimation(20UL, TASK_FOREVER, &main_loop);

uint32_t get_millisecond_timer(void) {
  return mesh.getNodeTime() / 1000;
}

void setup() {
  Serial.begin(115200);

  //mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
  mesh.setDebugMsgTypes(ERROR | SYNC | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6, 1);
  mesh.onReceive(&receivedCallback);
  mesh.setContainsRoot(false);

  userScheduler.addTask(taskLedAnimation);
  taskLedAnimation.enable();
  //WiFi.setTxPower(WIFI_POWER_2dBm);

  leds_init();
  gpio_init();
}

void loop() {
  mesh.update();
}

static int global_mode = GLOBAL_MODE_OFF;
static uint32_t queue_time = 0;
static int queued_mode = GLOBAL_MODE_NONE;
static int queued_data = 0;

static void main_loop(void) {

  int result = gpio_update(mesh.getNodeList(true).size());

  if (result == GPIO_EVENT_ALARM)
    mesh_announce_alarm(mesh.getNodeTime() + BROADCAST_DELAY);
  else if (result == GPIO_EVENT_PARTY)
    mesh_announce_party(mesh.getNodeTime() + BROADCAST_DELAY);

  if (queued_mode == GLOBAL_MODE_PARTY && check_queue_time()) {
    leds_set_intro();
    leds_effect_change_offset(queued_data + 1);  //increment the effect
    global_mode = GLOBAL_MODE_PARTY;
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (queued_mode == GLOBAL_MODE_ALARM && check_queue_time()) {
    leds_set_intro();
    global_mode = GLOBAL_MODE_ALARM;
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (leds_update(global_mode))
    global_mode = GLOBAL_MODE_OFF;
}

static inline bool check_queue_time(void) {
  if (mesh.getNodeTime() - queue_time < UINT32_MAX / 2)
    return true;
  return false;
}

static inline void logic_party(uint32_t time, int effect) {
  gpio_vibe_request(2, GPIO_VIBE_SHORT);
  queue_time = time;
  queued_mode = GLOBAL_MODE_PARTY;
  queued_data = effect;
}

static inline void logic_alarm(uint32_t time) {
  gpio_vibe_request(1, GPIO_VIBE_LONG);
  queue_time = time;
  queued_mode = GLOBAL_MODE_ALARM;
}

static inline void receivedCallback(uint32_t from, String& msg) {

  Serial.printf("Parsing: Received from %u msg=%s\n", from, msg.c_str());

  char mode_tmp;
  uint32_t start_time;
  int mode;

  if (sscanf(msg.c_str(), "%c %u %d", &mode_tmp, &start_time, &mode) == 3) {
    if (mode_tmp == 'A')
      logic_alarm(start_time);
    else if (mode_tmp == 'P') {
      logic_party(start_time, mode);
    }
    uint32_t headroom = start_time - mesh.getNodeTime();
    if (headroom > BROADCAST_DELAY)
      Serial.printf("Late: %u\n", UINT32_MAX - headroom);
    else
      Serial.printf("Early: %u\n", headroom);
  }
}

static inline void mesh_announce_alarm(uint32_t start_time) {
  char buffer[50];
  sprintf(buffer, "A %u %d", start_time, 0);
  Serial.println(buffer);
  mesh.sendBroadcast(buffer, true);
}

static inline void mesh_announce_party(uint32_t start_time) {
  char buffer[50];
  sprintf(buffer, "P %u %d", start_time, leds_get_effect_offset());
  Serial.println(buffer);
  mesh.sendBroadcast(buffer, true);
}