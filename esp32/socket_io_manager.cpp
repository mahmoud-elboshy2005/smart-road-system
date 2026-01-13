#include <stdint.h>
#include "esp_system.h"

#include <Arduino.h>

#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>

#include <ArduinoJson.h>

#include "socket_io_manager.h"
#include "traffic_light.h"
#include "pin_config.h"
#include "motor.h"

#define SOCKET_IO_STATUS_OK         "ok"
#define SOCKET_IO_STATUS_ERROR      "error"

const char *devicesNS = "/devices";

SocketIOclient socketIO;

extern IPAddress serverIP;
extern uint16_t serverPort;
extern uint16_t updPort;

void socket_io_exec_command(String command, JsonVariant requestData, JsonVariant responseData);
void socket_io_send_ack(JsonVariant requestData, JsonVariant responseData);
void socket_io_send_status(void);

void socketIOEvent(const socketIOmessageType_t type, const uint8_t * payload, const size_t length);

void socket_io_task(void * pvParams)
{

  // server address, port and URL
  socketIO.begin(serverIP, serverPort);


  // event handler
  socketIO.onEvent(socketIOEvent);

  unsigned long messageTimestamp = 0;
  while(true)
  {
    socketIO.loop();    
    
    if (millis() - messageTimestamp > STATUS_UPDATE_INTERVAL_MS)
    {
      messageTimestamp = millis();
      
      socket_io_send_status();
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to prevent task hogging CPU
  }
}


void socketIOEvent(const socketIOmessageType_t type, const uint8_t * payload, const size_t length) 
{
  switch(type) 
  {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected!\n");
      
      break;

    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      
      if (payload) {
        Serial.println((char*) payload);
      }

      Serial.println();

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, devicesNS);
      Serial.println("[IOc] Connected to the devices namespace");
      break;

    case sIOtype_EVENT:
      if (payload == NULL) {
        Serial.println("[IOc] Empty payload received for EVENT");
        break;
      }
      {
        Serial.print("[IOc] Get event: ");
        Serial.println((char *)payload);

        char *packet = (char *)payload;

        // Skip to the beginning of array /devices,["start"]
        while (packet[0] != '[') packet++;

        Serial.println(packet);

        // Json document to carry event name and data
        JsonDocument request;
        
        // Parse payload into json document
        deserializeJson(request, (char*) packet);
        
        // Extract event name
        String eventName = request[0];

        // Extract request data
        JsonVariant requestData = request[1];

        // Prepare response document
        JsonDocument response;
        JsonObject responseData = response.to<JsonObject>();

        if (eventName == "message")
        {
          String output = requestData["message"];

          Serial.print("[Server] ");
          Serial.println(output);
        } else if (eventName == "emergency_stop")
        {
          Serial.println("Emergency stop received from server!!");

          // Stop motor
          motor_stop();

          // Turn off all traffic lights
          traffic_light_turn_off_all();
        } 
        else if (eventName == "emergency")
        {
          Serial.println("Emergency state!!");

          // Resume normal operation
          motor_stop();
        }
      }
      break;

    case sIOtype_ACK:
      Serial.print("[IOc] Get ack: ");
      Serial.println(length);
      
      //hexdump(payload, length);

      break;
        
    case sIOtype_ERROR:
      Serial.print("[IOc] Get error: ");
      Serial.println(length);

      //hexdump(payload, length);

      break;

    case sIOtype_BINARY_EVENT:
      Serial.print("[IOc] Get binary: ");
      Serial.println(length);

      //hexdump(payload, length);

      break;

    case sIOtype_BINARY_ACK:
      Serial.print("[IOc] Get binary ack: ");
      Serial.println(length);

      //hexdump(payload, length);

      break;
    
    case sIOtype_PING:
      Serial.println("[IOc] Get PING");

      break;

    case sIOtype_PONG:
      Serial.println("[IOc] Get PONG");

      break;
  }
}

void socket_io_send_status(void)
{
  if (!socketIO.isConnected())
  {
    Serial.println("[IOc] Not connected, cannot send status");
    return;
  }

  uint32_t now = (uint32_t) millis();

  // Create JSON message for Socket.IO status event (camelCase fields)
  JsonDocument response;

  // event name
  response.add("status_update");

  // payload
  JsonObject data = response.createNestedObject();
  data["uptimeMs"] = now;

  // firmware, battery and rssi
  data["WiFi Dbm"] = WiFi.RSSI();

  // serialize and send
  String output;
  serializeJson(response, output);
  socketIO.sendEVENT(output);

  Serial.print("[IOc] Sent status: ");
  Serial.println(output);
}

void socket_io_exec_command(String command, JsonVariant requestData, JsonVariant responseData)
{
  uint64_t timestamp = millis();
  JsonVariant params = requestData["params"];
  
  if (command == "command")
  {
    
    responseData["status"] = SOCKET_IO_STATUS_OK;
    responseData["message"] = "Command successfully!";
    responseData["timestamp"] = timestamp;

    socket_io_send_ack(requestData, responseData);
  } else if (command == "restart")
  {
    Serial.println("Got restart command from server!!");
        
    responseData["status"] = SOCKET_IO_STATUS_OK;
    responseData["message"] = "ESP32 restarting...";
    responseData["timestamp"] = timestamp;

    socket_io_send_ack(requestData, responseData);
    esp_restart();
  }
}

void socket_io_send_ack(JsonVariant requestData, JsonVariant responseData)
{
  if (!socketIO.isConnected()) 
  {
    Serial.println("[IOc] Cannot send ack, not connected!");
    return;
  }

  JsonDocument response;

  response.add("ack");  // echo event name
  response.add(responseData.as<JsonObject>());  // placeholder for response data

  // JSON to String (serialization)
  String output;
  serializeJson(response, output);
  // Send ack event
  socketIO.sendEVENT(output);

  // Print JSON for debugging
  Serial.print("[IOc] Sent ack: ");
  Serial.println(output);
}
