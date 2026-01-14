
#include <Arduino.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "pin_config.h"
#include "motor.h"

typedef enum {
  MOTOR_LEFT,
  MOTOR_RIGHT,
  MOTOR_STOP
} motor_command_t;

QueueHandle_t motor_queue = NULL;

void motor_task(void *pvParams)
{
  motor_queue = xQueueCreate(10, sizeof(motor_command_t));

  if (motor_queue == NULL)
  {
    // Queue creation failed
    Serial.println("[Motor] Failed to create motor queue");
    vTaskDelete(NULL);
    return;
  }

  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);

  digitalWrite(MOTOR_PIN_1, LOW);
  digitalWrite(MOTOR_PIN_2, LOW);

  Serial.println("[Motor] Motor task started");
  
  motor_command_t command;
  
  while (true)
  {
    if (xQueueReceive(motor_queue, &command, pdMS_TO_TICKS(10)) == pdTRUE)
    {
      // Process motor command
      switch (command)
      {
        case MOTOR_LEFT:
          digitalWrite(MOTOR_PIN_1, HIGH);
          digitalWrite(MOTOR_PIN_2, LOW);
          break;
        case MOTOR_RIGHT:
          digitalWrite(MOTOR_PIN_1, LOW);
          digitalWrite(MOTOR_PIN_2, HIGH);
          break;
        case MOTOR_STOP:
          digitalWrite(MOTOR_PIN_1, LOW);
          digitalWrite(MOTOR_PIN_2, LOW);
          break;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent task hogging CPU
  }
}

void motor_turn_left()
{
  if (motor_queue == NULL)
    return;

  motor_command_t command = MOTOR_LEFT;
  xQueueSend(motor_queue, &command, pdMS_TO_TICKS(100));
}

void motor_turn_right()
{
  if (motor_queue == NULL)
    return;

  motor_command_t command = MOTOR_RIGHT;
  xQueueSend(motor_queue, &command, pdMS_TO_TICKS(100));
}

void motor_stop()
{
  if (motor_queue == NULL)
    return;

  motor_command_t command = MOTOR_STOP;
  xQueueSend(motor_queue, &command, pdMS_TO_TICKS(100));
}

bool motor_is_ready()
{
  return motor_queue != NULL;
}