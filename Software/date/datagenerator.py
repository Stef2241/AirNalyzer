import pandas as pd
import numpy as np

np.random.seed(42)
N = 50000

# --- 1. ID-uri și demografice ---
ids = np.arange(1, N+1)
age = np.random.randint(20, 80, size=N)  # vârste între 20-80 ani
sex = np.random.choice([0,1], size=N)    # 0=fem,1=masc
smoker = np.array([np.random.choice([0,1], p=[0.7,0.3]) if a < 60 else np.random.choice([0,1], p=[0.6,0.4]) for a in age])

# --- 2. Diagnostice ---
diagnoses = ['Normal', 'Diabetes', 'Lung Infection', 'Asthma', 'Liver Dysfunction', 'COPD', 'Unclear']
diagnosis_probs = [0.65, 0.10, 0.07, 0.05, 0.05, 0.05, 0.03]
diagnosis = np.random.choice(diagnoses, size=N, p=diagnosis_probs)

# --- 3. Variabile clinice și simptome corelate ---
cough = np.zeros(N, dtype=int)
fatigue = np.zeros(N, dtype=int)
fever = np.zeros(N, dtype=int)
shortness_of_breath = np.zeros(N, dtype=int)
temp = np.zeros(N)
humidity = np.zeros(N)
hour_of_day = np.random.randint(0,24,size=N)

acetone = np.zeros(N)
ammonia = np.zeros(N)
co = np.zeros(N)
co2 = np.zeros(N)
h2s = np.zeros(N)

for i in range(N):
    d = diagnosis[i]
    a = age[i]
    s = sex[i]
    sm = smoker[i]
    
    # --- Temperatura și umiditatea ---
    base_temp = 36.7 + np.random.normal(0,0.2)
    base_hum = 30 + np.random.normal(0,5)
    
    # --- VOC-uri de bază ---
    base_acetone = 0.3 + np.random.normal(0,0.05)
    base_ammonia = 0.02 + np.random.normal(0,0.005)
    base_co = 0.5 + np.random.normal(0,0.1)
    base_co2 = 400 + np.random.normal(0,10)
    base_h2s = 0.004 + np.random.normal(0,0.001)

    # Setare valori în funcție de diagnostic (corelate cu simptomele)
    if d == 'Normal':
        temp[i] = base_temp
        humidity[i] = base_hum
        cough[i] = 0
        fatigue[i] = np.random.choice([0,1], p=[0.9,0.1])
        fever[i] = 0
        shortness_of_breath[i] = 0
        acetone[i] = base_acetone
        ammonia[i] = base_ammonia
        co[i] = base_co
        co2[i] = base_co2
        h2s[i] = base_h2s
    elif d == 'Diabetes':
        temp[i] = base_temp
        humidity[i] = base_hum
        cough[i] = 0
        fatigue[i] = 1
        fever[i] = 0
        shortness_of_breath[i] = 0
        acetone[i] = 1.5 + np.random.normal(0,0.1)
        ammonia[i] = base_ammonia
        co[i] = base_co
        co2[i] = base_co2
        h2s[i] = base_h2s
    elif d == 'Lung Infection':
        temp[i] = base_temp + np.random.uniform(0.5,2.0)
        humidity[i] = base_hum + np.random.uniform(-5,5)
        cough[i] = 1
        fatigue[i] = 1
        fever[i] = 1
        shortness_of_breath[i] = np.random.choice([0,1], p=[0.3,0.7])
        acetone[i] = base_acetone
        ammonia[i] = 0.05 + np.random.normal(0,0.01)
        co[i] = 2.0 + np.random.normal(0,0.3)
        co2[i] = 450 + np.random.normal(0,15)
        h2s[i] = base_h2s
    elif d == 'Asthma':
        temp[i] = base_temp
        humidity[i] = base_hum + np.random.uniform(-5,5)
        cough[i] = np.random.choice([0,1], p=[0.3,0.7])
        fatigue[i] = np.random.choice([0,1], p=[0.6,0.4])
        fever[i] = 0
        shortness_of_breath[i] = 1
        acetone[i] = base_acetone
        ammonia[i] = base_ammonia
        co[i] = 1.3 + np.random.normal(0,0.15)
        co2[i] = 430 + np.random.normal(0,10)
        h2s[i] = base_h2s
    elif d == 'Liver Dysfunction':
        temp[i] = base_temp
        humidity[i] = base_hum
        cough[i] = 0
        fatigue[i] = 1
        fever[i] = 0
        shortness_of_breath[i] = 0
        acetone[i] = base_acetone
        ammonia[i] = 0.2 + np.random.normal(0,0.02)
        co[i] = base_co
        co2[i] = base_co2
        h2s[i] = 0.015 + np.random.normal(0,0.004)
    elif d == 'COPD':
        temp[i] = base_temp + np.random.uniform(0,0.5)
        humidity[i] = base_hum + np.random.uniform(-5,5)
        cough[i] = 1
        fatigue[i] = 1
        fever[i] = 0
        shortness_of_breath[i] = 1
        acetone[i] = base_acetone
        ammonia[i] = base_ammonia
        co[i] = 1.8 + np.random.normal(0,0.2)
        co2[i] = 440 + np.random.normal(0,12)
        h2s[i] = base_h2s
    else:  # Unclear
        temp[i] = base_temp + np.random.uniform(0,1)
        humidity[i] = base_hum + np.random.uniform(-5,5)
        cough[i] = np.random.choice([0,1])
        fatigue[i] = np.random.choice([0,1])
        fever[i] = np.random.choice([0,1])
        shortness_of_breath[i] = np.random.choice([0,1])
        acetone[i] = base_acetone + np.random.uniform(0,0.2)
        ammonia[i] = base_ammonia + np.random.uniform(0,0.05)
        co[i] = base_co + np.random.uniform(0,0.5)
        co2[i] = base_co2 + np.random.uniform(0,20)
        h2s[i] = base_h2s + np.random.uniform(0,0.002)

# --- Clip valori negative ---
acetone = np.clip(acetone,0,None)
ammonia = np.clip(ammonia,0,None)
co = np.clip(co,0,None)
co2 = np.clip(co2,350,None)
h2s = np.clip(h2s,0,None)

# --- Feature engineering ---
eps = 1e-6
acetone_ammonia_ratio = acetone / (ammonia + eps)
co_co2_ratio = co / (co2 + eps)
h2s_ammonia_ratio = h2s / (ammonia + eps)
total_voc = acetone + ammonia + co + h2s
mean_gases = (acetone + ammonia + co + co2 + h2s)/5
std_gases = np.std(np.vstack([acetone, ammonia, co, co2, h2s]), axis=0)

# --- Creează DataFrame ---
df = pd.DataFrame({
    "ID": ids,
    "Age": age,
    "Sex": sex,
    "Smoker": smoker,
    "Cough": cough,
    "Fatigue": fatigue,
    "Fever": fever,
    "Shortness_of_breath": shortness_of_breath,
    "Temperature": temp.round(1),
    "Humidity": humidity.round(1),
    "Acetone": acetone.round(3),
    "Ammonia": ammonia.round(3),
    "CO": co.round(3),
    "CO2": co2.round(1),
    "H2S": h2s.round(4),
    "Acetone_Ammonia_Ratio": acetone_ammonia_ratio.round(3),
    "CO_CO2_Ratio": co_co2_ratio.round(3),
    "H2S_Ammonia_Ratio": h2s_ammonia_ratio.round(4),
    "Total_VOC": total_voc.round(3),
    "Mean_Gases": mean_gases.round(3),
    "Std_Gases": std_gases.round(4),
    "Hour_Of_Day": hour_of_day,
    "Diagnosis": diagnosis
})

df.to_csv("airnalyzer_simulated_data_realistic_full_correlated.csv", index=False)
print("File 'airnalyzer_simulated_data_realistic_full_correlated.csv' generated successfully!")
