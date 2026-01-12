
#define _WEBSOCKETS_LOGLEVEL_     0

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>

#include <ArduinoJson.h>

#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"

WiFiMulti       WiFiMulti;
SocketIOclient  socketIO;

// Server (laptop) IP and Port number
// IPAddress serverIP(192, 168, 137, 1);
IPAddress serverIP(192, 168, 1, 4);
uint16_t  serverPort = 5000;

const char * videoNS = "/video";

int status = WL_IDLE_STATUS;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h

char ssid[] = "WEECEF49";        // your network SSID (name)
char pass[] = "kb147576";         // your network password (use for WPA, or use as key for WEP), length must be 8+


volatile bool isStreaming = false;
SemaphoreHandle_t mutex = NULL;
TaskHandle_t socketIOTaskHandle = NULL;
UBaseType_t socketIOTaskPriority = 20;

void startStream();
void pauseStream();

void socketIOEvent(const socketIOmessageType_t& type, uint8_t * payload, const size_t& length)
{
  switch (type)
  {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected");
      
      pauseStream();
      break;

    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      Serial.println((char*) payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, videoNS, strlen(videoNS));
      Serial.println("[IOc] Connected to the video namespace");

      break;

    case sIOtype_EVENT:
    {
      Serial.print("[IOc] Get event: ");
      Serial.println((char*) payload);

      char *packet = (char*)payload;

      // Skip to the beginning of array /video,["start"]
      while (packet[0] != '[') packet++;

      Serial.println(packet);

      JsonDocument doc;

      deserializeJson(doc, (char*)packet);

      String eventName = doc[0];

      if (eventName == "start")
      {
        startStream();
      } else if (eventName == "pause")
      {
        pauseStream();
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

    default:
      break;
  }
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("ESP32-Camera IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void setup()
{
  Serial.begin(115200);

  while (!Serial);

  delay(200);

  WiFiMulti.addAP(ssid, pass);

  Serial.print("Connecting to ");
  Serial.println(ssid);

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }

  Serial.println();

  // Client address
  Serial.print("ESP32-Camera @ IP address: ");
  Serial.println(WiFi.localIP());

  // server address, port and URL
  Serial.print("Connecting to Server @ IP address: ");
  Serial.print(serverIP);
  Serial.print(", port: ");
  Serial.println(serverPort);

  // setReconnectInterval to 10s, new from v2.5.1 to avoid flooding server. Default is 0.5s
  socketIO.setReconnectInterval(5000);

  // server address, port and URL
  // void begin(IPAddress host, uint16_t port, String url = "/socket.io/?EIO=4", String protocol = "arduino");
  // To use default EIO=4 from v2.5.1
  socketIO.begin(serverIP, serverPort);

  // event handler
  socketIO.onEvent(socketIOEvent);
  xTaskCreatePinnedToCore(socket_io_task, "Socket.IO Client Task", 4096, NULL, socketIOTaskPriority, &socketIOTaskHandle, 1);
}

void socket_io_task(void *pvParams)
{
  while (true)
  {
    socketIO.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void loop()
{
  if (isStreaming)
  {
    static uint32_t frameCount = 1;
    Serial.print("Frame: ");
    Serial.println(frameCount++);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void startStream()
{
  // Already Streaming, just return
  if (isStreaming == true) return;

  // Set isStreaming value to true
  isStreaming = true;
  
  Serial.println("[Camera] Start Stream");
}

void pauseStream()
{
  // Already paused, just return
  if (isStreaming == false) return;

  // Set isStreaming value to false
  isStreaming = false;

  Serial.println("[Camera] Pause Stream");
}