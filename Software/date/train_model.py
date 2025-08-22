import pandas as pd
import numpy as np
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import LabelEncoder, StandardScaler
from sklearn.metrics import classification_report, confusion_matrix
import matplotlib.pyplot as plt

# 1. Încarcă datele
df = pd.read_csv("airnalyzer_simulated_data_with_features.csv")

# 2. Pregătește datele
X = df.drop(columns=["ID", "Diagnosis"])
y = df["Diagnosis"]

# Encodează label-urile în cifre
le = LabelEncoder()
y_encoded = le.fit_transform(y)

# Scalează datele numerice
scaler = StandardScaler()
X_scaled = scaler.fit_transform(X)

# Imparte în seturi de antrenare și testare
X_train, X_test, y_train, y_test = train_test_split(
    X_scaled, y_encoded, test_size=0.2, random_state=42, stratify=y_encoded)

# 3. Construiește modelul - rețea neuronală profundă cu BatchNorm și Dropout
def build_model(input_dim, num_classes):
    inputs = tf.keras.Input(shape=(input_dim,))

    x = tf.keras.layers.Dense(256)(inputs)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.Activation('relu')(x)
    x = tf.keras.layers.Dropout(0.4)(x)

    x = tf.keras.layers.Dense(128)(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.Activation('relu')(x)
    x = tf.keras.layers.Dropout(0.3)(x)

    x = tf.keras.layers.Dense(64)(x)
    x = tf.keras.layers.BatchNormalization()(x)
    x = tf.keras.layers.Activation('relu')(x)
    x = tf.keras.layers.Dropout(0.2)(x)

    outputs = tf.keras.layers.Dense(num_classes, activation='softmax')(x)

    model = tf.keras.Model(inputs=inputs, outputs=outputs)
    return model

model = build_model(X_train.shape[1], len(le.classes_))

model.compile(
    optimizer=tf.keras.optimizers.Adam(learning_rate=0.001),
    loss='sparse_categorical_crossentropy',
    metrics=['accuracy']
)

# Callbacks pentru oprirea timpurie și reducerea LR
early_stop = tf.keras.callbacks.EarlyStopping(monitor='val_loss', patience=10, restore_best_weights=True)
reduce_lr = tf.keras.callbacks.ReduceLROnPlateau(monitor='val_loss', factor=0.5, patience=5, min_lr=1e-6)

# 4. Antrenează modelul
history = model.fit(
    X_train, y_train,
    validation_split=0.15,
    epochs=2000,
    batch_size=128,
    callbacks=[early_stop, reduce_lr],
    verbose=2
)

# 5. Evaluează modelul pe test set
test_loss, test_acc = model.evaluate(X_test, y_test, verbose=2)
print(f"\nTest Accuracy: {test_acc:.4f}")

# 6. Predicții și rapoarte
y_pred_probs = model.predict(X_test)
y_pred = np.argmax(y_pred_probs, axis=1)

print("\nConfusion Matrix:")
print(confusion_matrix(y_test, y_pred))

print("\nClassification Report:")
print(classification_report(y_test, y_pred, target_names=le.classes_))

# 7. Salvează modelul și scaler-ul

model.save("airnalyzer_best_model.keras")
import joblib
joblib.dump(scaler, "scaler.save")
print("\nModel and scaler saved!")

# 8. Plot training history
plt.figure(figsize=(12,5))

plt.subplot(1,2,1)
plt.plot(history.history['accuracy'], label='Train Accuracy')
plt.plot(history.history['val_accuracy'], label='Validation Accuracy')
plt.title('Model Accuracy over Epochs')
plt.xlabel('Epoch')
plt.ylabel('Accuracy')
plt.legend()

plt.subplot(1,2,2)
plt.plot(history.history['loss'], label='Train Loss')
plt.plot(history.history['val_loss'], label='Validation Loss')
plt.title('Model Loss over Epochs')
plt.xlabel('Epoch')
plt.ylabel('Loss')
plt.legend()

plt.tight_layout()
plt.savefig("training_history.png")  # salvează graficul
plt.show()

# 9. Afișează grafic matricea de confuzie
import seaborn as sns
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix

# Generează matricea
cm = confusion_matrix(y_test, y_pred)
class_names = le.classes_

# Creează figura
plt.figure(figsize=(8, 6))
sns.heatmap(cm, annot=True, fmt='d', cmap='Blues',
            xticklabels=class_names,
            yticklabels=class_names)

plt.title('Confusion Matrix')
plt.xlabel('Predicted Label')
plt.ylabel('True Label')
plt.tight_layout()
plt.savefig("confusion_matrix.png")  # Salvează imaginea
plt.show()
