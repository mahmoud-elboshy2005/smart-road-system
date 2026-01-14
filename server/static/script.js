

const socket = io("/webinterface");

// Get DOM elements
const imgElement = document.getElementById("videoFeed");
const btnStart = document.getElementById("btnStart");
const btnStop = document.getElementById("btnStop");
const btnEmergency = document.getElementById("btnEmergency");
const btnReset = document.getElementById("btnReset");
const carCountInput = document.getElementById("carCountInput");
const btnSendCarCount = document.getElementById("btnSendCarCount");
const currentCarCount = document.getElementById("currentCarCount");
const speedSlider = document.getElementById("speedSlider");
const speedValue = document.getElementById("speedValue");
const connectionStatus = document.getElementById("connectionStatus");
const emergencyStatus = document.getElementById("emergencyStatus");

// Socket event handlers
socket.on("connect", () => {
  console.log("Connected to the webinterface namespace");
  connectionStatus.textContent = "Connected";
  connectionStatus.className = "status-value status-connected";
});

socket.on("disconnect", () => {
  console.log("Disconnected from the webinterface namespace");
  connectionStatus.textContent = "Disconnected";
  connectionStatus.className = "status-value status-disconnected";
});

socket.on("frame", (data) => {
  // Create a Blob from the buffer and display in <img> element
  const blob = new Blob([data], { type: 'image/jpeg' });
  imgElement.src = URL.createObjectURL(blob);
});

// Button event listeners
btnStart.addEventListener("click", () => {
  console.log("Start button clicked");
  socket.emit("control_command", { action: "start" });
});

btnStop.addEventListener("click", () => {
  console.log("Stop button clicked");
  socket.emit("control_command", { action: "stop" });
});

btnEmergency.addEventListener("click", () => {
  console.log("Emergency button clicked");
  socket.emit("control_command", { action: "emergency" });
  emergencyStatus.textContent = "ACTIVE";
  emergencyStatus.style.background = "#fee2e2";
  emergencyStatus.style.color = "#991b1b";
});

btnReset.addEventListener("click", () => {
  console.log("Reset button clicked");
  socket.emit("control_command", { action: "reset" });
  emergencyStatus.textContent = "Normal";
  emergencyStatus.style.background = "#e0e0e0";
  emergencyStatus.style.color = "#555";
});

// Car count input listener
carCountInput.addEventListener("input", (e) => {
  const value = e.target.value;
  console.log("Car count input changed:", value);
  currentCarCount.textContent = value;
});

// Send car count button
btnSendCarCount.addEventListener("click", () => {
  const count = parseInt(carCountInput.value) || 0;
  console.log("Sending car count:", count);
  socket.emit("set_car_count", { car_count: count });
  alert(`Car count ${count} sent successfully!`);
});

// Speed slider listener
speedSlider.addEventListener("change", (e) => {
  const value = e.target.value;
  speedValue.textContent = value;
  console.log("Speed changed:", value);
  socket.emit("set_speed", { speed: parseInt(value) });
});

// Listen for detection updates from server
socket.on("detection_update", (data) => {
  console.log("Detection update received:", data);
  if (data.car_count !== undefined) {
    currentCarCount.textContent = data.car_count;
  }
  if (data.has_ambulance) {
    emergencyStatus.textContent = "AMBULANCE DETECTED";
    emergencyStatus.style.background = "#fee2e2";
    emergencyStatus.style.color = "#991b1b";
  } else {
    emergencyStatus.textContent = "Normal";
    emergencyStatus.style.background = "#e0e0e0";
    emergencyStatus.style.color = "#555";
  }
});