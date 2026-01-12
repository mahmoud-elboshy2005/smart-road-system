

socket = io("/webinterface");

socket.on("connect", () => {
  console.log("Connected to the webinterface namespace");
});

socket.on("disconnect", () => {
  console.log("Disconnected from the webinterface namespace");
});