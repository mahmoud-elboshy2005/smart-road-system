
#include <Arduino.h>

#include "traffic_light.h"
#include "pin_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

typedef enum
{
  CHANGE_COLOR,
  SET_DURATION,
  TURN_OFF_ALL,
} traffic_light_command_type_t;

typedef struct
{
  traffic_light_id_t id;
  traffic_light_color_t color;
  uint32_t duration_ms;
  traffic_light_command_type_t type;
} traffic_light_command_t;

static QueueHandle_t traffic_light_queue = NULL;

static uint32_t traffic_light_durations[2] = {MIN_GREEN_DURATION_MS, MIN_GREEN_DURATION_MS}; // Default durations for TRAFFIC_LIGHT_1 and TRAFFIC_LIGHT_2

static void light(traffic_light_id_t id, traffic_light_color_t color);

void traffic_light_task(void *pvParams)
{
  traffic_light_queue = xQueueCreate(10, sizeof(traffic_light_command_t));

  if (traffic_light_queue == NULL)
  {
    // Queue creation failed
    Serial.println("[Traffic Light] Failed to create traffic light queue");
    vTaskDelete(NULL);
  }

  pinMode(TRAFFIC_1_RED, OUTPUT);
  pinMode(TRAFFIC_1_YELLOW, OUTPUT);
  pinMode(TRAFFIC_1_GREEN, OUTPUT);
  pinMode(TRAFFIC_2_RED, OUTPUT);
  pinMode(TRAFFIC_2_YELLOW, OUTPUT);
  pinMode(TRAFFIC_2_GREEN, OUTPUT);

  digitalWrite(TRAFFIC_1_RED, LOW);
  digitalWrite(TRAFFIC_1_YELLOW, LOW);
  digitalWrite(TRAFFIC_1_GREEN, LOW);
  digitalWrite(TRAFFIC_2_RED, LOW);
  digitalWrite(TRAFFIC_2_YELLOW, LOW);
  digitalWrite(TRAFFIC_2_GREEN, LOW);

  Serial.println("[Traffic Light] Traffic light task started");

  traffic_light_command_t command;

  while (true)
  {
    if (xQueueReceive(traffic_light_queue, &command, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      switch (command.type)
      {
      case CHANGE_COLOR:
        light(command.id, command.color);
        break;

      case SET_DURATION:
        if (command.id == TRAFFIC_LIGHT_1)
        {
          traffic_light_durations[TRAFFIC_LIGHT_1] = command.duration_ms;
        }
        else if (command.id == TRAFFIC_LIGHT_2)
        {
          traffic_light_durations[TRAFFIC_LIGHT_2] = command.duration_ms;
        }
        break;

      case TURN_OFF_ALL:
        digitalWrite(TRAFFIC_1_RED, LOW);
        digitalWrite(TRAFFIC_1_YELLOW, LOW);
        digitalWrite(TRAFFIC_1_GREEN, LOW);
        digitalWrite(TRAFFIC_2_RED, LOW);
        digitalWrite(TRAFFIC_2_YELLOW, LOW);
        digitalWrite(TRAFFIC_2_GREEN, LOW);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task hogging CPU
  }
}

void traffic_light_set(traffic_light_id_t id, traffic_light_color_t color)
{
  if (traffic_light_queue != NULL)
  {
    traffic_light_command_t command;
    command.id = id;
    command.color = color;
    command.type = CHANGE_COLOR;
    xQueueSend(traffic_light_queue, &command, pdMS_TO_TICKS(100));
  }
}

void traffic_light_set_duration(traffic_light_id_t id, uint32_t duration_ms)
{
  if (traffic_light_queue != NULL)
  {
    traffic_light_command_t command;
    command.id = id;
    command.duration_ms = duration_ms;
    command.type = SET_DURATION;
    xQueueSend(traffic_light_queue, &command, pdMS_TO_TICKS(100));
  }
}

void traffic_light_turn_off_all()
{
  if (traffic_light_queue != NULL)
  {
    traffic_light_command_t command;
    command.type = TURN_OFF_ALL;
    xQueueSend(traffic_light_queue, &command, pdMS_TO_TICKS(100));
  }
}

bool traffic_light_is_ready()
{
  return traffic_light_queue != NULL;
}

void light(traffic_light_id_t id, traffic_light_color_t color)
{
  switch (id)
  {
  case TRAFFIC_LIGHT_1:
    switch (color)
    {
    case RED:
      digitalWrite(TRAFFIC_1_RED, HIGH);
      digitalWrite(TRAFFIC_1_YELLOW, LOW);
      digitalWrite(TRAFFIC_1_GREEN, LOW);
      break;
    case YELLOW:
      digitalWrite(TRAFFIC_1_RED, LOW);
      digitalWrite(TRAFFIC_1_YELLOW, HIGH);
      digitalWrite(TRAFFIC_1_GREEN, LOW);
      break;
    case GREEN:
      digitalWrite(TRAFFIC_1_RED, LOW);
      digitalWrite(TRAFFIC_1_YELLOW, LOW);
      digitalWrite(TRAFFIC_1_GREEN, HIGH);
      break;
    }
    break;
  
  case TRAFFIC_LIGHT_2:
    switch (color)
    {
    case RED:
      digitalWrite(TRAFFIC_2_RED, HIGH);
      digitalWrite(TRAFFIC_2_YELLOW, LOW);
      digitalWrite(TRAFFIC_2_GREEN, LOW);
      break;
    case YELLOW:
      digitalWrite(TRAFFIC_2_RED, LOW);
      digitalWrite(TRAFFIC_2_YELLOW, HIGH);
      digitalWrite(TRAFFIC_2_GREEN, LOW);
      break;
    case GREEN:
      digitalWrite(TRAFFIC_2_RED, LOW);
      digitalWrite(TRAFFIC_2_YELLOW, LOW);
      digitalWrite(TRAFFIC_2_GREEN, HIGH);
      break;
    }
    break;
  }
}
