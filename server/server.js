
import http from 'http';
import express from 'express';
import { Server } from 'socket.io';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const PORT = process.env.SERVER_PORT || 3000; 

const app = express();
const server = http.createServer(app);
const io = new Server(server);


const videoNS = io.of('/video');
const detectionNS = io.of('/detection');
const devicesNS = io.of('/devices');
const webInterfaceNS = io.of('/webinterface');

const systemStatus = {
  esp32camera_connected: false,
  detectionModel_connected: false,
  esp32_connected: false,
  car_count: 0
}

const connection_ids = {
  esp32camera_id: null,
  detectionModel_id: null,
  esp32_id: null
}

videoNS.on('connection', (socket) => {
  console.log('A new ESP32-Camera connected to the video namespace', 'socketID:', socket.id);
  systemStatus.esp32camera_connected = true;
  connection_ids.esp32camera_id = socket.id;

  socket.on('disconnect', () => {
    console.log('ESP32-Camera disconnected from the video namespace', 'socketID:', socket.id);
    systemStatus.esp32camera_connected = false;
    connection_ids.esp32camera_id = null;
  });

  socket.on('frame', (data) => {
    // Relay the frame data to all connected clients in the detection namespace
    detectionNS.volatile.emit('frame', data);
  });
});


detectionNS.on('connection', (socket) => {
  console.log('A new Detection Model connected to the detection namespace', 'socketID:', socket.id);
  systemStatus.detectionModel_connected = true;
  connection_ids.detectionModel_id = socket.id;

  socket.on('disconnect', () => {
    console.log('Detection Model disconnected from the detection namespace', 'socketID:', socket.id);
    systemStatus.detectionModel_connected = false;
    connection_ids.detectionModel_id = null;
  });

  socket.on('detection_result', (data) => {
    // Update car count and log it
    systemStatus.car_count = data.car_count;
    console.log('Updated Car Count:', systemStatus.car_count);
  });
});


devicesNS.on('connection', (socket) => {
  console.log('A new ESP32 connected to the devices namespace', 'socketID:', socket.id);
  systemStatus.esp32_connected = true;
  connection_ids.esp32_id = socket.id;

  socket.on('disconnect', () => {
    console.log('ESP32 disconnected from the devices namespace', 'socketID:', socket.id);
    systemStatus.esp32_connected = false;
    connection_ids.esp32_id = null;
  });
});


webInterfaceNS.on('connection', (socket) => {
  console.log('A new Web Interface connected to the webinterface namespace', 'socketID:', socket.id);
  
  socket.on('disconnect', () => {
    console.log('Web Interface disconnected from the webinterface namespace', 'socketID:', socket.id);
  });
});

app.use(express.static(join(__dirname, 'static')));

server.listen(PORT, () => {
  console.log(`Server is running @ http://localhost:${PORT}`);
});
