import pandas as pd
import numpy as np

np.random.seed(42)
N = 50000

# ID
ids = np.arange(1, N+1)

# Age (20-80)
age = np.random.randint(20, 80, size=N)

# Sex (0=female,1=male)
sex = np.random.choice([0,1], size=N)

# Smoker (0=no,1=yes)
smoker = np.random.choice([0,1], size=N, p=[0.7,0.3])

# Symptoms binary (cough, fatigue)
cough = np.random.choice([0,1], size=N, p=[0.7,0.3])
fatigue = np.random.choice([0,1], size=N, p=[0.75,0.25])

# Temperature (normal 36.5-37.5, add fever 37.5-39 for some)
temp = np.random.normal(36.8, 0.3, size=N)
temp += (np.random.rand(N) < 0.15) * np.random.uniform(0.8,2.2,size=N) # fever 15%

# Humidity (30-70)
humidity = np.random.uniform(30,70,size=N)

# Diagnoses and probabilities
diagnoses = ['Normal', 'Diabetes', 'Lung Infection', 'Asthma', 'Liver Dysfunction', 'COPD', 'Unclear']
diagnosis_probs = [0.65, 0.10, 0.07, 0.05, 0.05, 0.05, 0.03]
diagnosis = np.random.choice(diagnoses, size=N, p=diagnosis_probs)

acetone = np.zeros(N)
ammonia = np.zeros(N)
co = np.zeros(N)
co2 = np.zeros(N)
h2s = np.zeros(N)

for i in range(N):
    d = diagnosis[i]
    base_acetone = 0.3 + np.random.normal(0,0.05)
    base_ammonia = 0.02 + np.random.normal(0,0.005)
    base_co = 0.5 + np.random.normal(0,0.1)
    base_co2 = 400 + np.random.normal(0,10)
    base_h2s = 0.004 + np.random.normal(0,0.001)

    if d == 'Normal':
        acetone[i] = base_acetone + np.random.normal(0,0.02)
        ammonia[i] = base_ammonia + np.random.normal(0,0.001)
        co[i] = base_co + np.random.normal(0,0.05)
        co2[i] = base_co2 + np.random.normal(0,5)
        h2s[i] = base_h2s + np.random.normal(0,0.0005)
    elif d == 'Diabetes':
        acetone[i] = 1.5 + np.random.normal(0,0.1)
        ammonia[i] = base_ammonia + np.random.normal(0,0.002)
        co[i] = base_co + np.random.normal(0,0.05)
        co2[i] = base_co2 + np.random.normal(0,7)
        h2s[i] = base_h2s + np.random.normal(0,0.0007)
    elif d == 'Lung Infection':
        acetone[i] = base_acetone + np.random.normal(0,0.04)
        ammonia[i] = 0.05 + np.random.normal(0,0.01)
        co[i] = 2.5 + np.random.normal(0,0.3)
        co2[i] = 450 + np.random.normal(0,15)
        h2s[i] = base_h2s + np.random.normal(0,0.001)
    elif d == 'Asthma':
        acetone[i] = base_acetone + np.random.normal(0,0.03)
        ammonia[i] = base_ammonia + np.random.normal(0,0.002)
        co[i] = 1.3 + np.random.normal(0,0.15)
        co2[i] = 430 + np.random.normal(0,10)
        h2s[i] = base_h2s + np.random.normal(0,0.0008)
    elif d == 'Liver Dysfunction':
        acetone[i] = base_acetone + np.random.normal(0,0.02)
        ammonia[i] = 0.2 + np.random.normal(0,0.02)
        co[i] = base_co + np.random.normal(0,0.07)
        co2[i] = base_co2 + np.random.normal(0,6)
        h2s[i] = 0.015 + np.random.normal(0,0.004)
    elif d == 'COPD':
        acetone[i] = base_acetone + np.random.normal(0,0.03)
        ammonia[i] = base_ammonia + np.random.normal(0,0.005)
        co[i] = 1.8 + np.random.normal(0,0.2)
        co2[i] = 440 + np.random.normal(0,12)
        h2s[i] = base_h2s + np.random.normal(0,0.0009)
    else:
        acetone[i] = base_acetone + np.random.normal(0,0.1)
        ammonia[i] = base_ammonia + np.random.normal(0,0.01)
        co[i] = base_co + np.random.normal(0,0.15)
        co2[i] = base_co2 + np.random.normal(0,15)
        h2s[i] = base_h2s + np.random.normal(0,0.0015)

# Clip values to not be negative
acetone = np.clip(acetone, 0, None)
ammonia = np.clip(ammonia, 0, None)
co = np.clip(co, 0, None)
co2 = np.clip(co2, 350, None)
h2s = np.clip(h2s, 0, None)

# -- New features --

# Avoid division by zero with a tiny epsilon
eps = 1e-6

acetone_ammonia_ratio = acetone / (ammonia + eps)
co_co2_ratio = co / (co2 + eps)
h2s_ammonia_ratio = h2s / (ammonia + eps)

# Total VOC (sum of all gases)
total_voc = acetone + ammonia + co + h2s

# Mean and std dev of gases
mean_gases = (acetone + ammonia + co + co2 + h2s) / 5
std_gases = np.std(np.vstack([acetone, ammonia, co, co2, h2s]), axis=0)

# Hour of day (simulate between 0-23)
hour_of_day = np.random.randint(0, 24, size=N)

# Exposure to pollution (random float between 0 and 1)
pollution_exposure = np.random.uniform(0, 1, size=N)

# Creează DataFrame cu toate datele
df = pd.DataFrame({
    "ID": ids,
    "Age": age,
    "Sex": sex,
    "Smoker": smoker,
    "Cough": cough,
    "Fatigue": fatigue,
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
    "Pollution_Exposure": pollution_exposure.round(3),
    "Diagnosis": diagnosis
})

df.to_csv("airnalyzer_simulated_data_with_features.csv", index=False)

print("File 'airnalyzer_simulated_data_with_features.csv' generated successfully!")
