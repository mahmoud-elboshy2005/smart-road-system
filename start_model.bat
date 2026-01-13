@echo off
echo Installing required Python packages...

python -m pip install --upgrade pip
python -m pip install -r requirements.txt

echo.
echo Starting Detection Model...
python model.py

pause
