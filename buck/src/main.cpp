
#include <Arduino.h>
#include "painlessMesh.h"

#include "ui.h"
#include "mesh.h"

extern Scheduler userScheduler;
extern int totalsize;
extern uint32_t clients[];
extern int progress[];

static int ui_fps = 0;
static int main_fps = 0;

void fps_counter(void)
{
  int nodecount = 0;
  Serial.println("Update Progress:");
  for (int i = 0; i < MAX_NODES; i++)
  {
    if (clients[i] != 0)
    {
      nodecount++;
      Serial.print(i + 1);
      Serial.print(" ");
      // Serial.print(clients[i]);
      // Serial.print(" ");
      // Serial.print(progress[i]);
      Serial.print(" ");
      Serial.print(((float)progress[i]) / totalsize);
      Serial.println(" ");
    }
  }

  Serial.print("Total Nodes Seen: ");
  Serial.println(nodecount);
  Serial.print("Nodes Online Now: ");
  Serial.println(mesh_nodes());

  Serial.print("UI FPS: ");
  Serial.println(ui_fps);
  Serial.print("Main FPS: ");
  Serial.println(main_fps);
  Serial.println(" ");
  ui_fps = 0;
  main_fps = 0;
}

char serial_text[100] = "";
int serial_index = 0;

void ui_loop(void)
{
  ui_update();
  ui_fps++;

  if (Serial.available())
  {
    serial_text[serial_index] = Serial.read();
    if (serial_text[serial_index] == '\n')
    {
      mesh_announce_buck(serial_text);
      serial_index = 0;
    }
    else
    {
      serial_index++;
    }
  }
}

Task taskFPS(1000UL, TASK_FOREVER, &fps_counter);
Task taskUI(20UL, TASK_FOREVER, &ui_loop);

void setup()
{
  ui_init();
  Serial.begin(115200);
  mesh_init();
  
  userScheduler.addTask(taskFPS);
  taskFPS.enable();
  userScheduler.addTask(taskUI);
  taskUI.enable();
}

void loop()
{
  mesh_update();
  main_fps++;
};