"""
Flask + SSE server that reads ESP32 Bluetooth/Serial (COM) and serves a live-updating webpage.

How to run (Windows, Python 3.10+):
1) pip install -r requirements.txt   (sau: pip install flask pyserial)
2) python run_server.py
3) Open http://localhost:5000 in your browser

NOTE: HTML/CSS/JS sunt luate din templates/index.html și static/* (exact ca în exemplul tău).
"""

import os
import sys
import json
import time
import threading
from queue import Queue
from datetime import datetime
from collections import deque

from flask import Flask, Response, render_template, jsonify
import serial
import serial.tools.list_ports

# ----------------- Configuration -----------------
# Poți forța COM-ul prin variabilă de mediu, ex.:
#   set BT_COM_PORT=COM9
COM_PORT_ENV = os.environ.get("BT_COM_PORT", "").strip()
BAUD_RATE = int(os.environ.get("BT_BAUD", "115200"))

# indicii după care „miroase” a Bluetooth (doar pt. afișaj/prioritizare, nu obligatoriu)
AUTO_DETECT_HINTS = ["Bluetooth", "Standard Serial over Bluetooth", "SPP", "RFCOMM", "BT"]

# câte linii brute păstrăm în memorie pt. UI
MAX_RAW_LINES = 300
# -------------------------------------------------

app = Flask(__name__)

# --- SSE pub/sub ---
subscribers = set()
sub_lock = threading.Lock()

# --- starea curentă ---
latest_snapshot = {}
latest_raw_lines = deque(maxlen=MAX_RAW_LINES)

def publish(message: dict):
    """Trimite mesajul către toți abonații SSE."""
    with sub_lock:
        dead = []
        for q in subscribers:
            try:
                q.put_nowait(message)
            except Exception:
                dead.append(q)
        for q in dead:
            subscribers.discard(q)

def add_raw_line(line: str):
    latest_raw_lines.append(line)

# ------- parsing linii (heuristic, compatibil cu sketch-ul tău) -------
def parse_metric_line(line: str):
    """
    Extrage perechi cheie/valoare din liniile tipice:
      SGP30 → eCO2: 400 ppm, TVOC: 0 ppb
      BME688 → Temp: 23.1 °C, Hum: 45.2 %, Press: 1008.5 hPa, Gas: 7.8 KΩ
      MQ-3 analog: 1323
      MH-Z19B → CO2 raw: 612 ppm | avg: 590 ppm
      MQ-135: 2048
    """
    result = {}
    norm = line.replace("→", "->").replace("|", ",")
    parts = [p.strip() for p in norm.split(",")]

    def _num_from_text(txt: str):
        buf = ""
        for ch in txt:
            if ch.isdigit() or ch in ".-":
                buf += ch
            else:
                break
        if not buf:
            return None
        try:
            return float(buf) if "." in buf else int(buf)
        except ValueError:
            return None

    # ex: "MQ-3 analog: 1323"
    if ":" in parts[0] and ("->" not in parts[0] or parts[0].startswith("MQ-")):
        k, v = parts[0].split(":", 1)
        num = _num_from_text(v.strip())
        if num is not None:
            result[k.strip()] = num
        return result

    # ex: "BME688 -> Temp: xx, Hum: yy, ..."
    if "->" in parts[0]:
        head = parts[0]
        dev, rest = head.split("->", 1)
        dev = dev.strip()
        fields = [rest.strip()] + parts[1:]
        for f in fields:
            if ":" in f:
                k, v = f.split(":", 1)
                key = f"{dev} {k.strip()}"
                num = _num_from_text(v.strip())
                if num is not None:
                    result[key] = num
        return result

    return result
# ----------------------------------------------------------------------

# --------- detectare + citire serială cu reconectare automată ---------
def list_all_ports():
    return list(serial.tools.list_ports.comports())

def try_peek_data(port_name: str, peek_seconds: float = 3.5) -> bool:
    """Deschide portul, așteaptă scurt și verifică dacă apare vreo linie."""
    try:
        with serial.Serial(port=port_name, baudrate=BAUD_RATE, timeout=1) as ser:
            start = time.time()
            while time.time() - start < peek_seconds:
                raw = ser.readline()
                if raw:
                    return True
    except Exception:
        pass
    return False

def find_esp_port_loop() -> str:
    """
    Încearcă în buclă să găsească un port de unde vin date.
    1) Dacă BT_COM_PORT este setat, încearcă acela repetat.
    2) Altfel scanează toate COM-urile până găsește unul cu date.
    """
    while True:
        # 1) Forțat de utilizator
        if COM_PORT_ENV:
            print(f"[Detect] Trying forced port {COM_PORT_ENV} ...")
            if try_peek_data(COM_PORT_ENV):
                print(f"[Detect] OK on {COM_PORT_ENV}")
                return COM_PORT_ENV
            print(f"[Detect] No data on {COM_PORT_ENV}. Will retry in 3s.")
            time.sleep(3)
            continue

        # 2) Enumerare toate porturile
        ports = list_all_ports()
        if not ports:
            print("[Detect] No serial ports found. Retrying in 3s...")
            time.sleep(3)
            continue

        # prioritizăm ce „sună” a Bluetooth
        bt_ports = []
        other_ports = []
        for p in ports:
            desc = f"{p.description or ''} {p.hwid or ''} {p.name or ''}"
            if any(h.lower() in desc.lower() for h in AUTO_DETECT_HINTS):
                bt_ports.append(p.device)
            else:
                other_ports.append(p.device)

        candidates = bt_ports + other_ports
        print("[Detect] Candidates:", candidates or "[]")

        for dev in candidates:
            print(f"[Detect] Probing {dev} ...")
            if try_peek_data(dev):
                print(f"[Detect] Found data on {dev}")
                return dev

        print("[Detect] No data detected on any port. Retrying in 3s...")
        time.sleep(3)

def serial_reader_forever():
    """
    Fir principal de citire:
    - caută portul cu date (în buclă);
    - citește continuu;
    - la eroare, reia detectarea și reconectează.
    """
    global latest_snapshot
    while True:
        port = find_esp_port_loop()
        print(f"[Serial] Connecting to {port} @ {BAUD_RATE}")
        try:
            with serial.Serial(port=port, baudrate=BAUD_RATE, timeout=1) as ser:
                while True:
                    raw = ser.readline()
                    if not raw:
                        continue
                    try:
                        line = raw.decode("utf-8", errors="ignore").strip()
                    except Exception:
                        line = str(raw).strip()
                    if not line:
                        continue

                    add_raw_line(line)
                    parsed = parse_metric_line(line)
                    if parsed:
                        latest_snapshot.update(parsed)

                    publish({
                        "ts": datetime.utcnow().isoformat() + "Z",
                        "line": line,
                        "parsed": parsed,
                        "snapshot": latest_snapshot,
                    })

        except serial.SerialException as e:
            print(f"[Serial] Lost connection on {port}: {e}. Reconnecting...")
            time.sleep(1)
        except Exception as e:
            print(f"[Serial] Unexpected error on {port}: {e}. Reconnecting...")
            time.sleep(1)
# ----------------------------------------------------------------------

# ------------------------- Flask routes -------------------------------
@app.route("/")
def index():
    # folosește templates/index.html din proiectul tău (exact ca în exemplu)
    return render_template("index.html")

@app.route("/stream")
def stream():
    """SSE endpoint."""
    def gen(q: Queue):
        try:
            while True:
                msg = q.get()
                yield f"data: {json.dumps(msg)}\n\n"
        except GeneratorExit:
            pass

    q = Queue(maxsize=1000)
    with sub_lock:
        subscribers.add(q)

    # hello imediat
    q.put({
        "ts": datetime.utcnow().isoformat() + "Z",
        "line": "*** connected to server ***",
        "parsed": {},
        "snapshot": latest_snapshot,
    })

    return Response(gen(q), mimetype="text/event-stream")

@app.route("/api/latest")
def api_latest():
    return jsonify({
        "snapshot": latest_snapshot,
        "raw": list(latest_raw_lines)[-100:],
        "time": datetime.utcnow().isoformat() + "Z",
    })

@app.route("/api/ports")
def api_ports():
    ports = [
        {"device": p.device, "name": p.name, "description": p.description, "hwid": p.hwid}
        for p in list_all_ports()
    ]
    return jsonify({"ports": ports})
# ----------------------------------------------------------------------

def main():
    # pornește thread-ul de citire/reconectare
    t = threading.Thread(target=serial_reader_forever, daemon=True)
    t.start()
    # server web
    app.run(host="127.0.0.1", port=5000, debug=False, threaded=True)

if __name__ == "__main__":
    main()
