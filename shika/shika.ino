
#define TASK_RES_MULTIPLIER 1
#include "shika.h"
#include "gpio.h"
#include "leds.h"

#define BROADCAST_DELAY 1000 // 500 ms delay to handle latency across mesh (Adjust as needed)

painlessMesh mesh;
SimpleList<uint32_t> nodes;
Scheduler userScheduler;

char buffer[50];
bool repeat = false;
uint32_t repeat_time = 0;

static void main_loop(void);

Task taskLedAnimation(20UL, TASK_FOREVER, &main_loop);

uint32_t get_millisecond_timer(void)
{
  // do not trust this timer for anything logic related
  // this will cause a visual glitch at rollover
  // every ~70 minutes or less, depending on the time sync
  return (mesh.getNodeTime());
}

static int global_mode = GLOBAL_MODE_OFF;
static uint32_t queue_time = 0;
static int queued_mode = GLOBAL_MODE_NONE;
static int queued_data = 0;

static int nodecount = 0;

#define BUCK_ID 2883351813 // fixed buck ID

static uint32_t my_node_id = 0;
static uint32_t node_place = 0;
static int node_count = 0;

static int queued_buck_color = 0;
static int queued_buck_mode = 0;

void ChangedConnectionsCallback()
{
  int temp = mesh.getNodeList(true).size();
  if (temp > nodecount)
  {
    Serial.println("New Connection");
    led_reset();
    leds_set_intro();
    global_mode = GLOBAL_MODE_PARTY;
    gpio_vibe_request(2, GPIO_VIBE_SHORT);
  }
  else
  {
    Serial.println("Dropped Connection");
  }
  nodecount = temp;

  std::list<uint32_t> gqlist = mesh.getNodeList(true);
  gqlist.sort();
  gqlist.remove(BUCK_ID);

  for (uint32_t g : gqlist)
  {
    Serial.println(g);
  }

  Serial.print(gqlist.size());
  Serial.print("  ");

  node_place = 0;
  for (uint32_t g : gqlist)
  {
    node_place++;
    if (g == my_node_id)
      break;
  }

  node_count = gqlist.size();
  Serial.print(node_place);
  Serial.print(" of ");
  Serial.println(node_count);
}

static inline void logic_party(uint32_t time, int effect)
{
  if (queue_time != time)
  {
    gpio_vibe_request(2, GPIO_VIBE_SHORT);
    queue_time = time;
    queued_mode = GLOBAL_MODE_PARTY;
    queued_data = effect;
  }
  else
  {
    Serial.println("Suppressing");
  }
}

static inline void logic_buck(uint32_t time, int effect)
{
  if (queue_time != time)
  {
    gpio_vibe_request(2, GPIO_VIBE_SHORT);
    queue_time = time;
    queued_buck_mode = effect;
    queued_mode = GLOBAL_MODE_BUCK;
  }
  else
  {
    Serial.println("Suppressing");
  }
}
static inline void logic_palette(uint32_t time, int effect)
{
  if (queue_time != time)
  {
    // gpio_vibe_request(2, GPIO_VIBE_SHORT);
    queue_time = time;
    queued_buck_mode = effect;
    queued_mode = GLOBAL_MODE_PALETTE;
  }
  else
  {
    Serial.println("Suppressing");
  }
}

static inline void logic_alarm(uint32_t time)
{
  if (queue_time != time)
  {
    gpio_vibe_request(1, GPIO_VIBE_LONG);
    queue_time = time;
    queued_mode = GLOBAL_MODE_ALARM;
  }
  else
  {
    Serial.println("Suppressing");
  }
}

static inline void receivedCallback(uint32_t from, String &msg)
{

  char mode_tmp;
  uint32_t start_time;
  int mode;

  Serial.printf("Parsing: Received from %u msg=%s\n", from, msg.c_str());

  int h;

  if (sscanf(msg.c_str(), "%c %u %d %d", &mode_tmp, &start_time, &mode, &h) == 4)
  {

    if (mode_tmp == 'B')
    {
      queued_buck_color = h;
      logic_buck(start_time, mode);
    }
    if (mode_tmp == 'C')
    {

      logic_palette(start_time, mode);
    }
    return;
  }

  if (sscanf(msg.c_str(), "%c %u %d", &mode_tmp, &start_time, &mode) == 3)
  {
    if (mode_tmp == 'A' || mode_tmp == 'P')
    {
      if (mode_tmp == 'A')
        logic_alarm(start_time);
      else if (mode_tmp == 'P')
      {
        logic_party(start_time, mode);
      }
      uint32_t headroom = start_time - mesh.getNodeTime();

      if (headroom > BROADCAST_DELAY)
        Serial.printf("Late: %u\n", UINT32_MAX - headroom);
      else
        Serial.printf("Early: %u\n", headroom);
    }
  }
}

void setup()
{
  nodecount = mesh.getNodeList(true).size();
  Serial.begin(115200);

  // mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE);
  mesh.setDebugMsgTypes(ERROR | SYNC | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA, 6, 1);
  mesh.onReceive(&receivedCallback);
  mesh.setContainsRoot(false);
  mesh.onChangedConnections(&ChangedConnectionsCallback);
  mesh.initOTAReceive("otareceiver");
  userScheduler.addTask(taskLedAnimation);
  taskLedAnimation.enable();
  // WiFi.setTxPower(WIFI_POWER_2dBm);

  leds_init();
  gpio_init();

  my_node_id = mesh.getNodeId();
}

void loop()
{
  mesh.update();
}

static inline void mesh_announce_alarm(uint32_t start_time)
{
  sprintf(buffer, "A %u %d", start_time, 0);
  Serial.println(buffer);
  mesh.sendBroadcast(buffer, true);
  repeat = true;
  repeat_time = millis();
}

static inline void mesh_announce_party(uint32_t start_time)
{
  sprintf(buffer, "P %u %d", start_time, leds_get_effect_offset());
  Serial.println(buffer);
  mesh.sendBroadcast(buffer, true);
  repeat = true;
  repeat_time = millis();
}

static inline bool check_queue_time(void)
{
  if (mesh.getNodeTime() - queue_time < UINT32_MAX / 2)
    return true;
  return false;
}

static void main_loop(void)
{

  int result = gpio_update(mesh.getNodeList(true).size());

  if (result == GPIO_EVENT_ALARM)
    mesh_announce_alarm(mesh.getNodeTime() + BROADCAST_DELAY);
  else if (result == GPIO_EVENT_PARTY)
    mesh_announce_party(mesh.getNodeTime() + BROADCAST_DELAY);

  if (queued_mode == GLOBAL_MODE_PARTY && check_queue_time())
  {
    leds_set_intro();
    leds_effect_change_offset(queued_data + 1); // increment the effect
    global_mode = GLOBAL_MODE_PARTY;
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (queued_mode == GLOBAL_MODE_BUCK && check_queue_time())
  {
    // dont set intro if only a color change?

    uint32_t node_speed = 1000;
    if (queued_buck_mode >= 100)
    {
      node_speed = queued_buck_mode / 100;
      queued_buck_mode = queued_buck_mode % 100;
    }

    leds_set_buck(queued_buck_mode, node_place, node_count, queued_buck_color, node_speed);

    global_mode = GLOBAL_MODE_BUCK;
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (queued_mode == GLOBAL_MODE_ALARM && check_queue_time())
  {
    leds_set_intro();
    global_mode = GLOBAL_MODE_ALARM;
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (queued_mode == GLOBAL_MODE_PALETTE && check_queue_time())
  {
    if (queued_buck_mode == 66)
      ESP.restart();
    leds_set_intro();
    leds_palette(queued_buck_mode);
    queued_mode = GLOBAL_MODE_NONE;
  }

  if (leds_update(global_mode))
  {
    global_mode = GLOBAL_MODE_OFF;
    gpio_vibe_request(1, GPIO_VIBE_SHORT);
  }

  if (repeat && millis() - repeat_time > 50)
  {
    mesh.sendBroadcast(buffer, false);
    repeat = false;
  }
}
