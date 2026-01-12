
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

videoNS.on('connection', (socket) => {
  console.log('A new ESP32-Camera connected to the video namespace', 'socketID:', socket.id);

  socket.on('disconnect', () => {
    console.log('ESP32-Camera disconnected from the video namespace', 'socketID:', socket.id);
  });
});


detectionNS.on('connection', (socket) => {
  console.log('A new Detection Model connected to the detection namespace', 'socketID:', socket.id);
  
  socket.on('disconnect', () => {
    console.log('Detection Model disconnected from the detection namespace', 'socketID:', socket.id);
  });
});


devicesNS.on('connection', (socket) => {
  console.log('A new ESP32 connected to the devices namespace', 'socketID:', socket.id);
  
  socket.on('disconnect', () => {
    console.log('ESP32 disconnected from the devices namespace', 'socketID:', socket.id);
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