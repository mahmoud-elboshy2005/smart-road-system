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

QueueHandle_t motor_queue = xQueueCreate(10, sizeof(motor_command_t));

void motor_task(void *pvParams)
{
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);

  motor_command_t command = MOTOR_STOP;

  while (true)
  {
    if (xQueueReceive(motor_queue, &command, portMAX_DELAY) == pdTRUE)
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
  motor_command_t command = MOTOR_LEFT;
  xQueueSend(motor_queue, &command, portMAX_DELAY);
}

void motor_turn_right()
{
  motor_command_t command = MOTOR_RIGHT;
  xQueueSend(motor_queue, &command, portMAX_DELAY);
}

void motor_stop()
{
  motor_command_t command = MOTOR_STOP;
  xQueueSend(motor_queue, &command, portMAX_DELAY);
}