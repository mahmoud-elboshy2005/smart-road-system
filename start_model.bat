@echo off
echo Installing required Python packages...

python -m pip install --upgrade pip
python -m pip install ultralytics opencv-python python-socketio numpy

echo.
echo Starting Detection Model...
python model.py

pause
