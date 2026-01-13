

socket = io("/webinterface");

const imgElement = document.getElementById("videoFeed");

socket.on("connect", () => {
  console.log("Connected to the webinterface namespace");
});

socket.on("disconnect", () => {
  console.log("Disconnected from the webinterface namespace");
});

socket.on("frame", (data) => {
  // Create a Blob from the buffer and display in <img> element
  const blob = new Blob([data], { type: 'image/jpeg' });
  imgElement.src = URL.createObjectURL(blob);
});