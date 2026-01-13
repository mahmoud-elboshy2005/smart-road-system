
import udp from 'dgram';
import http from 'http';
import express from 'express';
import { Server } from 'socket.io';
import { fileURLToPath } from 'url';
import { dirname, join } from 'path';

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

const PORT = process.env.SERVER_PORT || 5000;
const UDP_PORT = process.env.UPD_PORT || 3000;
const DETECTION_URL = process.env.DETECTION_URL || 'http://0.0.0.0:8000/detect';

const app = express();
const server = http.createServer(app);
const udpSocket = udp.createSocket('udp4',);
const io = new Server(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  },
  pingInterval: 10000,
  pingTimeout: 5000
});

app.use(express.static(join(__dirname, 'static')));

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

// Store frames as array of packets
// Structure: frames[frameNumber] = { packets: [], totalPackets: N, timestamp: Date, receivedCount: N }
const frames = {};
const FRAME_TIMEOUT = 3000; // 3 second timeout for incomplete frames

// Store latest complete frame for HTTP polling
let latestFrame = null;

// Middleware to parse JSON
app.use(express.json({ limit: '50mb' }));

// HTTP endpoint to receive detection results from model.py
app.post('/detection_results', (req, res) => {
  try {
    const { car_count, has_ambulance, frame } = req.body;
    
    // Update system status
    systemStatus.car_count = car_count || 0;
    systemStatus.has_ambulance = has_ambulance || false;
    console.log('Detection results received - Cars:', systemStatus.car_count, 'Ambulance:', systemStatus.has_ambulance);
    
    // Convert hex frame back to buffer and emit to web interface
    if (frame) {
      const processedFrame = Buffer.from(frame, 'hex');
      webInterfaceNS.emit('frame', processedFrame);
    }
    
    res.status(200).json({ status: 'success' });
  } catch (error) {
    console.error('Error processing detection results:', error);
    res.status(500).json({ error: error.message });
  }
});

async function sendFrameToDetection(frameBuffer) {
  try {
    const response = await fetch(DETECTION_URL, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/octet-stream'
      },
      body: frameBuffer
    });
    
    if (response.ok) {
      // const result = await response.json();
      
      // Update system status
      // systemStatus.car_count = result.car_count;
      // systemStatus.has_ambulance = result.has_ambulance;
      // console.log('Detection result - Cars:', result.car_count, 'Ambulance:', result.has_ambulance);
      
      // Convert hex frame back to buffer and emit to web interface
      // if (result.frame) {
      //   const processedFrame = Buffer.from(result.frame, 'hex');
      //   webInterfaceNS.emit('frame', processedFrame);
      // }
    } else {
      console.error('Detection server returned error:', response.status);
    }
  } catch (error) {
    console.error('Error sending frame to detection:', error.message);
  }
}

udpSocket.on('error', (err) => {
  console.log(`UDP server error:\n${err.stack}`);
  udpSocket.close();
});

udpSocket.on('message', (msg, rinfo) => {
  // Parse metadata from end of packet (last 12 bytes)
  if (msg.length < 12) {
    console.log('Invalid packet: too small');
    return;
  }

  const frameNumber = msg.readUInt32LE(msg.length - 12);
  const totalPackets = msg.readUInt32LE(msg.length - 8);
  const packetIndex = msg.readUInt32LE(msg.length - 4);
  const packetData = msg.subarray(0, msg.length - 12); // Actual image data

  // console.log(`Frame ${frameNumber}, Packet ${packetIndex + 1}/${totalPackets}, ${packetData.length} bytes`);

  // Initialize frame if it doesn't exist
  if (!frames[frameNumber]) {
    frames[frameNumber] = {
      packets: new Array(totalPackets),
      totalPackets: totalPackets,
      timestamp: Date.now(),
      receivedCount: 0
    };
  }

  // Store packet at correct position (only if not already received)
  if (!frames[frameNumber].packets[packetIndex]) {
    frames[frameNumber].packets[packetIndex] = packetData;
    frames[frameNumber].timestamp = Date.now(); // Update timestamp on each received packet
    frames[frameNumber].receivedCount++;
  }

  // Check if frame is complete: packetIndex + 1 == totalPackets means last packet received
  // But we also need to ensure ALL packets are received
  if (frames[frameNumber].receivedCount === totalPackets) {
    // console.log(`Frame ${frameNumber} complete! Concatenating ${totalPackets} packets...`);
    
    // Concatenate all packets in order
    const completeFrame = Buffer.concat(frames[frameNumber].packets);
    // console.log(`Complete frame size: ${completeFrame.length} bytes`);
    
    // Send to detection model
    sendFrameToDetection(completeFrame);
    
    // Clean up this frame
    delete frames[frameNumber];
  }
});

// Cleanup old incomplete frames periodically
setInterval(() => {
  const now = Date.now();
  for (const frameNumber in frames) {
    if (now - frames[frameNumber].timestamp > FRAME_TIMEOUT) {
      console.log(`Frame ${frameNumber} timeout - discarding ${frames[frameNumber].receivedCount}/${frames[frameNumber].totalPackets} packets`);
      delete frames[frameNumber];
    }
  }
}, FRAME_TIMEOUT);

udpSocket.bind(UDP_PORT);

videoNS.on('connection', (socket) => {
  console.log('A new ESP32-Camera connected to the video namespace', 'socketID:', socket.id);
  systemStatus.esp32camera_connected = true;
  connection_ids.esp32camera_id = socket.id;

  socket.on('disconnect', () => {
    console.log('ESP32-Camera disconnected from the video namespace', 'socketID:', socket.id);
    systemStatus.esp32camera_connected = false;
    connection_ids.esp32camera_id = null;
  });

  setTimeout(() => {
    socket.emit('start');
  }, 5000);
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
    systemStatus.has_ambulance = data.has_ambulance;
    console.log('Updated Car Count:', systemStatus.car_count);
    webInterfaceNS.emit('frame', data.frame);
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


server.listen(PORT, () => {
  console.log(`Server is running @ http://localhost:${PORT}`);
});


let isShuttingDown = false;

function shutdown(signal) {
  if (isShuttingDown) return;
  isShuttingDown = true;

  console.log(`\nGraceful shutdown started (${signal})`);

  // Stop accepting new HTTP connections
  if (server) {
    server.close(() => {
      console.log("HTTP server closed");
    });
  }

  // Close UDP socket
  if (udpSocket) {
    udpSocket.close(() => {
      console.log("UDP socket closed");
    });
  }

  // Allow time for cleanup then exit
  setTimeout(() => {
    console.log("Process exiting");
    process.exit(0);
  }, 1000);
}

// Signals
process.on("SIGINT", shutdown);   // Ctrl+C
process.on("SIGTERM", shutdown);  // kill
process.on("SIGQUIT", shutdown);
