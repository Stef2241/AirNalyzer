import os
import json
import time
import threading
from queue import Queue
from datetime import datetime
from collections import deque
from flask import Flask, Response, render_template, jsonify, request
from flask_cors import CORS
import serial
import serial.tools.list_ports
import numpy as np
import tensorflow as tf
import joblib

# ------------------ Flask ------------------
app = Flask(__name__)
CORS(app)

# ------------------ AI Model ------------------
model = tf.keras.models.load_model('airnalyzer_best_model.keras')
scaler = joblib.load('scaler.save')
classes = ['Normal', 'Diabetes', 'Lung Infection', 'Asthma', 'Liver Dysfunction', 'COPD', 'Unclear']

# ------------------ Config Serial ------------------
BAUD_RATE = int(os.environ.get("BT_BAUD", "115200"))
COM_PORT_ENV = os.environ.get("BT_COM_PORT", "").strip()
AUTO_DETECT_HINTS = ["Bluetooth", "Standard Serial over Bluetooth", "SPP", "RFCOMM", "BT"]

# ------------------ SSE & State ------------------
subscribers = set()
sub_lock = threading.Lock()
latest_snapshot = {}
latest_raw_lines = deque(maxlen=300)

# ------------------ Sensor Data ------------------
sensor_data = {
    'Temperature': 0.0,
    'Humidity': 0.0,
    'Acetone': 0.0,
    'Ammonia': 0.0,
    'CO': 0.0,
    'CO2': 0.0,
    'H2S': 0.0
}
last_sensor_update = 0
serial_lock = threading.Lock()

# ------------------ SSE Helper ------------------
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

# ------------------ Parsing Line ------------------
def parse_metric_line(line: str):
    """Încearcă să parseze JSON, altfel trimite raw line."""
    try:
        data = json.loads(line)
        with serial_lock:
            for k in sensor_data:
                if k in data:
                    sensor_data[k] = float(data[k])
        return data
    except json.JSONDecodeError:
        return {"raw_line": line}

# ------------------ Serial Port Detection ------------------
def list_all_ports():
    return list(serial.tools.list_ports.comports())

def try_peek_data(port_name: str, peek_seconds: float = 3.5) -> bool:
    try:
        with serial.Serial(port=port_name, baudrate=BAUD_RATE, timeout=1) as ser:
            start = time.time()
            while time.time() - start < peek_seconds:
                raw = ser.readline()
                if raw:
                    return True
    except Exception:
        return False
    return False

def find_esp_port_loop() -> str:
    """Caută ESP32 automat și loghează fiecare încercare."""
    while True:
        if COM_PORT_ENV:
            print(f"[Detect] Trying forced port {COM_PORT_ENV} ...")
            if try_peek_data(COM_PORT_ENV):
                print(f"[Detect] OK on {COM_PORT_ENV}")
                return COM_PORT_ENV
            print(f"[Detect] No data on {COM_PORT_ENV}. Retrying in 3s.")
            time.sleep(3)
            continue

        ports = list_all_ports()
        if not ports:
            print("[Detect] No serial ports found. Retrying in 3s...")
            time.sleep(3)
            continue

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

# ------------------ Serial Reader ------------------
def serial_reader_forever():
    global latest_snapshot
    while True:
        port = find_esp_port_loop()
        print(f"[Serial] Connecting to {port} @ {BAUD_RATE}")
        try:
            with serial.Serial(port=port, baudrate=BAUD_RATE, timeout=1) as ser:
                print(f"[Serial] Connected to {port}. Reading data...")
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
                    print(f"[ESP32] {line}")  # afișăm tot ce vine

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

# ------------------ Flask Routes ------------------
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/stream")
def stream():
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

# ------------------ AI Prediction with Valid Data Check ------------------
@app.route('/predict', methods=['POST'])
def predict():
    try:
        # --- Citire date de la frontend ---
        data = request.get_json(force=True)
        s = sensor_data.copy()

        # --- Verificare valori valide ---
        essential_keys = ['Temperature', 'Humidity', 'Acetone', 'Ammonia', 'CO', 'CO2', 'H2S']
        for k in essential_keys:
            val = s.get(k, None)
            if val is None or np.isnan(val) or np.isinf(val):
                return jsonify({
                    "error": f"Invalid sensor data for {k}",
                    "message": "Predicția nu se poate face până când toți senzorii au valori valide.",
                    "sensor_data": s
                }), 400

        # --- Calcule suplimentare ---
        eps = 1e-6
        acetone_ammonia_ratio = np.float32(s['Acetone'] / (s['Ammonia'] + eps))
        co_co2_ratio = np.float32(s['CO'] / (s['CO2'] + eps))
        h2s_ammonia_ratio = np.float32(s['H2S'] / (s['Ammonia'] + eps))
        total_voc = np.float32(s['Acetone'] + s['Ammonia'] + s['CO'] + s['H2S'])
        mean_gases = np.float32(np.mean([s['Acetone'], s['Ammonia'], s['CO'], s['CO2'], s['H2S']]))
        std_gases = np.float32(np.std([s['Acetone'], s['Ammonia'], s['CO'], s['CO2'], s['H2S']]))

        # --- Pregătire input pentru model ---
        input_values = [
            np.float32(data.get('Age', 0)),
            np.float32(data.get('Sex', 0)),
            np.float32(data.get('Smoker', 0)),
            1.0 if data.get('Cough', False) else 0.0,
            1.0 if data.get('Fatigue', False) else 0.0,
            1.0 if data.get('Fever', False) else 0.0,
            1.0 if data.get('Shortness_of_breath', False) else 0.0,
            np.float32(s['Temperature']),
            np.float32(s['Humidity']),
            np.float32(s['Acetone']),
            np.float32(s['Ammonia']),
            np.float32(s['CO']),
            np.float32(s['CO2']),
            np.float32(s['H2S']),
            acetone_ammonia_ratio,
            co_co2_ratio,
            h2s_ammonia_ratio,
            total_voc,
            mean_gases,
            std_gases,
            np.float32(data.get('Hour_Of_Day', 12))
        ]

        input_array = np.array([input_values], dtype=np.float32)
        input_scaled = scaler.transform(input_array)

        # --- Predictie ---
        pred_probs = model.predict(input_scaled, verbose=0)
        pred_class = int(np.argmax(pred_probs, axis=1)[0])
        diagnosis = classes[pred_class]
        confidence = float(pred_probs[0][pred_class])

        return jsonify({'diagnosis': diagnosis, 'confidence': confidence})

    except Exception as e:
        import traceback
        return jsonify({
            "error": "Error during prediction",
            "details": str(e),
            "trace": traceback.format_exc()
        }), 400

# ------------------ Main ------------------
def main():
    t = threading.Thread(target=serial_reader_forever, daemon=True)
    t.start()
    app.run(host="127.0.0.1", port=5000, debug=True, threaded=True)

if __name__ == "__main__":
    main()
