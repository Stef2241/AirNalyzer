from flask import Flask, render_template, request, jsonify
import numpy as np
import tensorflow as tf

app = Flask(__name__)

# Încarcă modelul TensorFlow
model = tf.keras.models.load_model('airnalyzer_best_model.keras')

# Lista claselor (diagnosticelor) în ordine, să fie aceeași cu a modelului
classes = ['Normal', 'Diabetes', 'Lung Infection', 'Asthma', 'Liver Dysfunction', 'COPD', 'Unclear']

@app.route('/', methods=['GET', 'POST'])
def index():
    diagnosis = None
    if request.method == 'POST':
        try:
            # Extrage datele trimise din formular
            data = {
                'Age': float(request.form['Age']),
                'Sex': float(request.form['Sex']),
                'Smoker': float(request.form['Smoker']),
                'Cough': float(request.form['Cough']),
                'Fatigue': float(request.form['Fatigue']),
                'Temperature': float(request.form['Temperature']),
                'Humidity': float(request.form['Humidity']),
                'Acetone': float(request.form['Acetone']),
                'Ammonia': float(request.form['Ammonia']),
                'CO': float(request.form['CO']),
                'CO2': float(request.form['CO2']),
                'H2S': float(request.form['H2S']),
            }

            # Transformă într-un array 2D pentru model (batch size = 1)
            input_data = np.array([[
                data['Age'], data['Sex'], data['Smoker'], data['Cough'], data['Fatigue'],
                data['Temperature'], data['Humidity'], data['Acetone'], data['Ammonia'],
                data['CO'], data['CO2'], data['H2S']
            ]], dtype=np.float32)

            # Predict
            pred_probs = model.predict(input_data)
            pred_class = np.argmax(pred_probs, axis=1)[0]
            diagnosis = classes[pred_class]
            confidence = pred_probs[0][pred_class]

            return render_template('index.html', diagnosis=diagnosis, confidence=confidence, formdata=data)

        except Exception as e:
            diagnosis = f"Error: {str(e)}"
            return render_template('index.html', diagnosis=diagnosis)

    # GET request => doar pagina goală
    return render_template('index.html', diagnosis=diagnosis)


if __name__ == '__main__':
    app.run(debug=True)
