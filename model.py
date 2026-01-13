from ultralytics import YOLO
import cv2
import json
import os
import numpy as np
from flask import Flask, request, jsonify
from queue import Queue
import threading
import requests

DETECTION_PORT = 8000
YOLO_MODEL_PATH = "model/best.pt"
SERVER_URL = "http://localhost:5000"

ENV_FILE = '.env'
if os.path.exists(ENV_FILE):
    with open(ENV_FILE) as f:
        for line in f:
            if '=' not in line:
                continue
            key, value = line.strip().split('=', 1)
            if key == "DETECTION_PORT":
                DETECTION_PORT = int(value)
            elif key == "YOLO_MODEL_PATH":
                YOLO_MODEL_PATH = value
            elif key == "SERVER_URL":
                SERVER_URL = value

app = Flask(__name__)
model = YOLO(YOLO_MODEL_PATH)
print(f"Loaded YOLO model from {YOLO_MODEL_PATH}")

# Queue for incoming frames
frame_queue = Queue(maxsize=10)

# HTTP client to send detection results to server
def send_detection_results(results):
    """Send detection results to server via HTTP POST"""
    try:
        response = requests.post(
            f"{SERVER_URL}/detection_results",
            json=results,
            timeout=5
        )
        if response.ok:
            print(f"Detection results sent successfully")
        else:
            print(f"Failed to send detection results: {response.status_code}")
    except Exception as e:
        print(f"Error sending detection results: {e}")

# Detection worker thread
def detection_worker():
    """Worker thread that processes frames from the queue"""
    print("Detection worker thread started")
    while True:
        try:
            # Get frame from queue (blocks until frame is available)
            frame_data = frame_queue.get(timeout=1)
            if frame_data is None:  # Poison pill to stop thread
                break
            
            # Process the frame
            results = detect_vehicles(frame_data)
            
            # Send results to server
            send_detection_results(results)
            
            # Mark task as done
            frame_queue.task_done()
        except Exception as e:
            if "Empty" not in str(type(e)):
                print(f"Error in detection worker: {e}")

@app.route('/detect', methods=['POST'])
def detect_endpoint():
  """HTTP endpoint to receive frame and add it to the queue"""
  try:
    frame_data = request.get_data()
    if not frame_data:
      return jsonify({"error": "No frame data received"}), 400
    
    # Add frame to queue (non-blocking)
    if not frame_queue.full():
      frame_queue.put(frame_data)
      return jsonify({"status": "queued"}), 200
    else:
      return jsonify({"status": "queue_full", "message": "Frame dropped"}), 503
  except Exception as e:
    print(f"Error queueing frame: {e}")
    return jsonify({"error": str(e)}), 500

def detect_vehicles(frame_data):
  """Detect vehicles in the frame and return detection results"""
  frame_array = np.frombuffer(frame_data, dtype=np.uint8)
  image = cv2.imdecode(frame_array, cv2.IMREAD_COLOR)
  if image is None:
    raise ValueError("Could not decode image")
  
  # Object Detection
  results = model.track(
    source=image, 
    show=False, 
    stream=True, 
    conf=0.5, 
    persist=True 
  )
  r = list(results)[0]
  targets = []
  boxes = r.boxes
  
  # Reset detection flags
  has_ambulance = False
  car_count = 0
  
  for box in boxes:
    # Bounding Box
    if box.is_track:
      x1, y1, x2, y2 = map(int, box.xyxy[0])
      cls_id = int(box.cls[0])
      track_id = int(box.id[0])
      targets.append((x1, y1, x2, y2, cls_id, track_id))
      
      # Check for special vehicles
      class_name = r.names[cls_id].lower()

      if 'ambulance' in class_name:
        has_ambulance = True

  # Counting Logic  
  car_count = len(targets)

  for target in targets:
    x1, y1, x2, y2, cls_id, track_id = target

    # Draw bounding box and label
    cv2.rectangle(image, (x1, y1), (x2, y2), (255, 0, 255), 2)

    label = f'ID:{track_id} {r.names[cls_id].capitalize()}'
    cv2.putText(image, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (255, 0, 255), 2)

  # Display counts
  label = f'Count: {car_count}'
  cv2.putText(image, label, (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.9, (0, 255, 0), 2)
  
  # Display special vehicle indicators
  status_text = f'Ambulance: {has_ambulance}'
  cv2.putText(image, status_text, (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)

  _, frame_encoded = cv2.imencode('.jpg', image)
  frame_base64 = frame_encoded.tobytes().hex()
  return {
    "car_count": car_count,
    "has_ambulance": has_ambulance,
    "frame": frame_base64
  }

if __name__ == '__main__':
  print(f"Starting detection server on port {DETECTION_PORT}...")
  
  # Start detection worker thread
  worker_thread = threading.Thread(target=detection_worker, daemon=True)
  worker_thread.start()
  
  app.run(host='0.0.0.0', port=DETECTION_PORT, debug=False, threaded=True)