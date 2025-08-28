
@echo off
REM Create venv and install deps, then run server on Windows
python -m venv .venv
call .venv\Scripts\activate
pip install -r requirements.txt
set BT_COM_PORT=
set BT_BAUD=115200
python run_server.py
pause
