
#define _WEBSOCKETS_LOGLEVEL_ 0

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "socket_io_manager.h"
#include "traffic_light.h"
#include "pin_config.h"
#include "motor.h"
#include "fsm.h"

WiFiMulti WiFiMulti;

// Server (laptop) IP and Port number
// IPAddress serverIP(192, 168, 137, 1);
IPAddress serverIP(192, 168, 1, 4);
// IPAddress serverIP(10, 42, 0, 1);
uint16_t serverPort = 5000;
uint16_t updPort    = 3000;

char ssid[] = "WEECEF49";  // your network SSID (name)
char pass[] = "kb147576";  // your network password (use for WPA, or use as key for WEP), length must be 8+


volatile bool system_initialized = false;
volatile uint32_t car_count = 0;
volatile bool has_ambulance = false;
volatile uint32_t duration = MIN_GREEN_DURATION_MS;
volatile uint32_t greenDuration = MIN_GREEN_DURATION_MS;
volatile uint32_t increaseInDuration = 0;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);


  WiFiMulti.addAP(ssid, pass);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  // Client address
  Serial.print("ESP32 @ IP address: ");
  Serial.println(WiFi.localIP());

  // server address, port and URL
  Serial.print("Connecting to Server @ IP address: ");
  Serial.print(serverIP);
  Serial.print(", port: ");
  Serial.println(serverPort);

  fsm_init(STATE_IDLE);

  // Register FSM transitions
  // Transitions for traffic light control normal mode
  fsm_register_transition(STATE_IDLE, STATE_STREET_1_GREEN_STREET_2_RED, EVENT_START, street_1_green_street_2_red_action);
  fsm_register_transition(STATE_STREET_1_GREEN_STREET_2_RED, STATE_STREET_1_YELLOW_STREET_2_RED, EVENT_SWITCH, street_1_yellow_street_2_red_action);
  fsm_register_transition(STATE_STREET_1_YELLOW_STREET_2_RED, STATE_STREET_1_RED_STREET_2_GREEN, EVENT_SWITCH, street_1_red_street_2_green_action);
  fsm_register_transition(STATE_STREET_1_RED_STREET_2_GREEN, STATE_STREET_1_RED_STREET_2_YELLOW, EVENT_SWITCH, street_1_red_street_2_yellow_action);
  fsm_register_transition(STATE_STREET_1_RED_STREET_2_YELLOW, STATE_STREET_1_GREEN_STREET_2_RED, EVENT_SWITCH, street_1_green_street_2_red_action);

  // Transitions for emergency handling
  fsm_register_transition(STATE_STREET_1_GREEN_STREET_2_RED, STATE_EMERGENCY, EVENT_EMERGENCY, emergency_action);
  fsm_register_transition(STATE_STREET_1_YELLOW_STREET_2_RED, STATE_EMERGENCY, EVENT_EMERGENCY, emergency_action);
  fsm_register_transition(STATE_STREET_1_RED_STREET_2_GREEN, STATE_EMERGENCY, EVENT_EMERGENCY, emergency_action);
  fsm_register_transition(STATE_STREET_1_RED_STREET_2_YELLOW, STATE_EMERGENCY, EVENT_EMERGENCY, emergency_action);

  // Transition to resume normal operation after emergency ends
  fsm_register_transition(STATE_EMERGENCY, STATE_STREET_1_GREEN_STREET_2_RED, EVENT_CLEAR_EMERGENCY, street_1_green_street_2_red_action);

  xTaskCreatePinnedToCore(motor_task, "Motor Task", 2048, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(traffic_light_task, "Traffic Task", 2048, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(socket_io_task, "Socket IO Task", 4096, NULL, 20, NULL, 1);  // Larger stack, run on core 0
  xTaskCreatePinnedToCore(traffic_loop_task, "Traffic Loop Task", 4096, NULL, 5, NULL, 1);
  xTaskCreatePinnedToCore(fsm_task, "FSM Task", 4096, NULL, 10, NULL, 1);

  // Wait for tasks to initialize their queues
  Serial.println("Waiting for tasks to initialize...");
  while (!motor_is_ready() || !traffic_light_is_ready() || !fsm_is_ready()) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  Serial.println("All tasks initialized!");

  // Mark system as initialized
  system_initialized = true;

  fsm_push_event(EVENT_START);

}

void fsm_task(void *pvParams) {
  // Wait for system initialization
  while (!system_initialized) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  
  while (true) {
    fsm_process_events();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void loop() {
  // Empty loop - everything runs in tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}

void emergency_stop(void) {
  // Stop motor
  motor_stop();

  // Turn off all traffic lights
  traffic_light_turn_off_all();
}

void traffic_loop_task(void *pvParams) {

  // Wait for system initialization to complete
  while (!system_initialized) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  uint32_t last_switch_time = millis();

  while (true) {
    if (millis() - last_switch_time > duration)
    {
      last_switch_time = millis();
      if (has_ambulance) {
        // Skip normal operation during emergency
        continue;
      }
  
      fsm_push_event(EVENT_SWITCH);
    }
    
    Serial.println((millis() - last_switch_time) / 1000);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Adjust delay as needed
  }
}

void street_1_green_street_2_red_action(void) {
  traffic_light_set(TRAFFIC_LIGHT_1, GREEN);
  traffic_light_set(TRAFFIC_LIGHT_2, RED);
  duration = greenDuration + increaseInDuration;
  Serial.println("Street 1 GREEN, Street 2 RED");
}

void street_1_yellow_street_2_red_action(void) {
  traffic_light_set(TRAFFIC_LIGHT_1, YELLOW);
  traffic_light_set(TRAFFIC_LIGHT_2, RED);
  duration = YELLOW_DURATION_MS;
  Serial.println("Street 1 YELLOW, Street 2 RED");
}

void street_1_red_street_2_green_action(void) {
  traffic_light_set(TRAFFIC_LIGHT_1, RED);
  traffic_light_set(TRAFFIC_LIGHT_2, GREEN);
  duration = greenDuration - increaseInDuration;
  Serial.println("Street 1 RED, Street 2 GREEN");
}

void street_1_red_street_2_yellow_action(void) {
  traffic_light_set(TRAFFIC_LIGHT_1, RED);
  traffic_light_set(TRAFFIC_LIGHT_2, YELLOW);
  duration = YELLOW_DURATION_MS;
  Serial.println("Street 1 RED, Street 2 YELLOW");
}

void emergency_action(void) {
  // In emergency, turn both lights to red
  traffic_light_set(TRAFFIC_LIGHT_1, GREEN);
  traffic_light_set(TRAFFIC_LIGHT_2, RED);
  close_pump();
  Serial.println("Street 1 GREEN, Street 2 RED, EMERGENCY!");
}

typedef enum {
  PUMP_OFF = 0,
  PUMP_ON  = 1
} pump_state_t;

pump_state_t pump_status = PUMP_OFF;

void open_pump(void) {
  if (pump_status == PUMP_ON) {
    return;
  }
  Serial.println("Opening pump...");
  pump_status = PUMP_ON;
  motor_turn_left();
  
  vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
  motor_stop();
}

void close_pump(void) {
  if (pump_status == PUMP_OFF) {
    return;
  }
  Serial.println("Closing pump...");
  pump_status = PUMP_OFF;
  motor_turn_right();
  vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_MS));
  motor_stop();
}