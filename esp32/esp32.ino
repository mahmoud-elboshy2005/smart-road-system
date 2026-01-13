
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define MOTOR_PIN_1  18
#define MOTOR_PIN_2  19

#define TRAFFIC_1_RED     21
#define TRAFFIC_1_YELLOW  22
#define TRAFFIC_1_GREEN   23

#define TRAFFIC_2_RED     25
#define TRAFFIC_2_YELLOW  26
#define TRAFFIC_2_GREEN   27

void motor_task(void *pvParams)
{
  while (true)
  {
    pinMode(MOTOR_PIN_1, HIGH);
    vTaskDelay(pdMS_TO_TICKS(5000));
    pinMode(MOTOR_PIN_1, LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));

    pinMode(MOTOR_PIN_2, HIGH);
    vTaskDelay(pdMS_TO_TICKS(5000));
    pinMode(MOTOR_PIN_2, LOW);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void traffic_1_task(void *pvParams)
{
  while (true)
  {
    pinMode(TRAFFIC_1_RED, HIGH);
    pinMode(TRAFFIC_1_YELLOW, HIGH);
    pinMode(TRAFFIC_1_GREEN, HIGH);

    vTaskDelay(pdMS_TO_TICKS(1000));

    pinMode(TRAFFIC_1_RED, LOW);
    pinMode(TRAFFIC_1_YELLOW, LOW);
    pinMode(TRAFFIC_1_GREEN, LOW);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void traffic_2_task(void *pvParams)
{
  while (true)
  {
    pinMode(TRAFFIC_2_RED, HIGH);
    pinMode(TRAFFIC_2_YELLOW, HIGH);
    pinMode(TRAFFIC_2_GREEN, HIGH);

    vTaskDelay(pdMS_TO_TICKS(1000));

    pinMode(TRAFFIC_2_RED, LOW);
    pinMode(TRAFFIC_2_YELLOW, LOW);
    pinMode(TRAFFIC_2_GREEN, LOW);

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void setup() {
  pinMode(MOTOR_PIN_1, OUTPUT);
  pinMode(MOTOR_PIN_2, OUTPUT);

  pinMode(TRAFFIC_1_RED,    OUTPUT);
  pinMode(TRAFFIC_1_YELLOW, OUTPUT);
  pinMode(TRAFFIC_1_GREEN,  OUTPUT);

  pinMode(TRAFFIC_2_RED,    OUTPUT);
  pinMode(TRAFFIC_2_YELLOW, OUTPUT);
  pinMode(TRAFFIC_2_GREEN,  OUTPUT);

  pinMode(MOTOR_PIN_1, LOW);
  pinMode(MOTOR_PIN_2, LOW);

  pinMode(TRAFFIC_1_RED, LOW);
  pinMode(TRAFFIC_1_YELLOW, LOW);
  pinMode(TRAFFIC_1_GREEN, LOW);
  
  pinMode(TRAFFIC_2_RED, LOW);
  pinMode(TRAFFIC_2_YELLOW, LOW);
  pinMode(TRAFFIC_2_GREEN, LOW);


  xTaskCreate(motor_task, "Motor Task", 2048, NULL, 5, NULL, 1);

  vTaskDelay(pdMS_TO_TICKS(3000));

  xTaskCreate(traffic_1_task, "Traffic 1 Task", 2048, NULL, 5, NULL, 1);

  vTaskDelay(pdMS_TO_TICKS(3000));

  xTaskCreate(traffic_2_task, "Traffic 2 Task", 2048, NULL, 5, NULL, 1);
}

void loop() {
  
}
