#include "esp_camera.h"
#include "board_config.h"

#define _WEBSOCKETS_LOGLEVEL_ 0

#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h>
// #define WEBSOCKETS_NETWORK_TYPE   NETWORK_ESP32_ASYNC

#include <WebSocketsClient_Generic.h>
#include <SocketIOclient_Generic.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"

#define CHUNK_SIZE  1400

#define STREAM_FPS  30

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

WiFiUDP udp;

// Server (laptop) IP and Port number
// IPAddress serverIP(192, 168, 137, 1);
// IPAddress serverIP(192, 168, 1, 4);
IPAddress serverIP(10, 42, 0, 1);
uint16_t serverPort = 5000;
uint16_t updPort    = 3000;

const char *videoNS = "/video";

int status = WL_IDLE_STATUS;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h

// char ssid[] = "WEECEF49";  // your network SSID (name)
// char pass[] = "kb147576";  // your network password (use for WPA, or use as key for WEP), length must be 8+

char ssid[] = "mahmoud";  // your network SSID (name)
char pass[] = "12345678";  // your network password (use for WPA, or use as key for WEP), length must be 8+


volatile bool isStreaming = false;
SemaphoreHandle_t mutex = NULL;
TaskHandle_t socketIOTaskHandle = NULL;
UBaseType_t socketIOTaskPriority = 20;

void startStream();
void pauseStream();

void hexdump(const uint8_t *data, const size_t &length) {
  for (size_t i = 0; i < length; i++) {
    Serial.printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0) Serial.println();
  }
  Serial.println();

  Serial.write(data, length);

  Serial.println();
}

void socketIOEvent(const socketIOmessageType_t &type, uint8_t *payload, const size_t &length) {
  switch (type) {
    case sIOtype_DISCONNECT:
      Serial.println("[IOc] Disconnected");

      pauseStream();
      break;

    case sIOtype_CONNECT:
      Serial.print("[IOc] Connected to url: ");
      Serial.println((char *)payload);

      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, videoNS, strlen(videoNS));
      Serial.println("[IOc] Connected to the video namespace");

      break;

    case sIOtype_EVENT:
      {
        Serial.print("[IOc] Get event: ");
        Serial.println((char *)payload);

        char *packet = (char *)payload;

        // Skip to the beginning of array /video,["start"]
        while (packet[0] != '[') packet++;

        Serial.println(packet);

        JsonDocument doc;

        deserializeJson(doc, (char *)packet);

        String eventName = doc[0];

        if (eventName == "start") {
          startStream();
        } else if (eventName == "pause") {
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

      hexdump(payload, length);

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

void printWifiStatus() {
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

void setup() {
  Serial.begin(115200);

  while (!Serial)
    ;

  delay(200);

  camera_config_t cameraConfig;
  cameraConfig.ledc_channel = LEDC_CHANNEL_0;
  cameraConfig.ledc_timer = LEDC_TIMER_0;
  cameraConfig.pin_d0 = Y2_GPIO_NUM;
  cameraConfig.pin_d1 = Y3_GPIO_NUM;
  cameraConfig.pin_d2 = Y4_GPIO_NUM;
  cameraConfig.pin_d3 = Y5_GPIO_NUM;
  cameraConfig.pin_d4 = Y6_GPIO_NUM;
  cameraConfig.pin_d5 = Y7_GPIO_NUM;
  cameraConfig.pin_d6 = Y8_GPIO_NUM;
  cameraConfig.pin_d7 = Y9_GPIO_NUM;
  cameraConfig.pin_xclk = XCLK_GPIO_NUM;
  cameraConfig.pin_pclk = PCLK_GPIO_NUM;
  cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
  cameraConfig.pin_href = HREF_GPIO_NUM;
  cameraConfig.pin_sccb_sda = SIOD_GPIO_NUM;
  cameraConfig.pin_sccb_scl = SIOC_GPIO_NUM;
  cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
  cameraConfig.pin_reset = RESET_GPIO_NUM;
  cameraConfig.xclk_freq_hz = 24000000;
  cameraConfig.frame_size = FRAMESIZE_SVGA;
  cameraConfig.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //cameraConfig.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  cameraConfig.grab_mode = CAMERA_GRAB_LATEST;
  cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
  cameraConfig.jpeg_quality = 4;
  cameraConfig.fb_count = 6;

  // camera init
  esp_err_t err = esp_camera_init(&cameraConfig);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  delay(200);

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

void socket_io_task(void *pvParams) {
  while (true) {
    socketIO.loop();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

static int64_t lastFrameTime = 0;
static uint32_t frameNum = 0;

void loop() {
  if (isStreaming) {
    if (esp_camera_available_frames() == 0) {
      Serial.println("There are no available frames");
      return;
    }

    if (esp_timer_get_time() - lastFrameTime > (1000000 / STREAM_FPS))
    {
      lastFrameTime = esp_timer_get_time();

      camera_fb_t *frame = esp_camera_fb_get();

      size_t totalSize = frame->len;
      size_t totalChunks = (totalSize + CHUNK_SIZE - 1) / CHUNK_SIZE;

      // Serial.print("Frame Grap Time: ");
      // Serial.println((float)(esp_timer_get_time() - lastFrameTime) / 1000);

      frameNum++;

      // int64_t sum = 0;
      // int64_t timeBeforeEncoding = 0;
      // int64_t timeBeforeSending = esp_timer_get_time();
      
      for (size_t i = 0; i < totalChunks; i++) {
        size_t offset = i * CHUNK_SIZE;
        size_t chunkSize = min((size_t)CHUNK_SIZE, (size_t)(totalSize - offset));

        udp.beginPacket(serverIP, updPort);
        // timeBeforeEncoding = esp_timer_get_time();
        // String chunkBase64 = base64::encode(frame->buf + offset, chunkSize);
        // sum += esp_timer_get_time() - timeBeforeEncoding;
        
        // String output = videoNS;
        // output += ",[ \"frame_chunk\" , \"" + chunk64Encoded + "\" ]";
        udp.write(frame->buf + offset, chunkSize);
        udp.write((uint8_t*)&frameNum, sizeof(frameNum));
        udp.write((uint8_t*)&totalChunks, sizeof(totalChunks));
        udp.write((uint8_t*)&i, sizeof(i));
        udp.endPacket();
      }

      // Serial.print("Sending Time: ");
      // Serial.println((float)(esp_timer_get_time() - timeBeforeSending) / 1000.0);
      esp_camera_fb_return(frame);

      
      Serial.print("[Camera] Frame sent,    Total Time: ");
      Serial.println((float)(esp_timer_get_time() - lastFrameTime) / 1000.0);
      
      // String output = videoNS;
      // output += ",[ \"frame_end\" ]";

      // socketIO.sendEVENT(output);
      // Serial.print("    Encode Time: ");
      // Serial.println((float)sum / 1000);
      // vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
}

void startStream() {
  // Already Streaming, just return
  if (isStreaming == true) return;

  // Set isStreaming value to true
  isStreaming = true;

  Serial.println("[Camera] Start Stream");
}

void pauseStream() {
  // Already paused, just return
  if (isStreaming == false) return;

  // Set isStreaming value to false
  isStreaming = false;

  Serial.println("[Camera] Pause Stream");
}