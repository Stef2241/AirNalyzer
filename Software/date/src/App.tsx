import React, { useState } from 'react';
import { BrowserRouter as Router, Routes, Route, useNavigate, useLocation } from 'react-router-dom';
import { Stethoscope, User, AlertCircle, CheckCircle2, Loader2, Wind } from 'lucide-react';

interface FormData {
  age: string;
  sex: string;
  smoker: string;
  cough: boolean;
  fatigue: boolean;
  fever: boolean;
  shortness_of_breath: boolean;
}

interface FormErrors {
  [key: string]: string;
}

interface ResultData {
  diagnosis: string;
  confidence: number;
}

// ----------------------------
// DiagnosticForm Component
// ----------------------------
function DiagnosticForm() {
  const [formData, setFormData] = useState<FormData>({
    age: '',
    sex: '',
    smoker: '',
    cough: false,
    fatigue: false,
    fever: false,
    shortness_of_breath: false,
  });

  const [errors, setErrors] = useState<FormErrors>({});
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [progress, setProgress] = useState(0);
  const [showProgress, setShowProgress] = useState(false);
  const [feedbackMessage, setFeedbackMessage] = useState('');
  const navigate = useNavigate();

  const validateForm = (): boolean => {
    const newErrors: FormErrors = {};
    if (!formData.age) newErrors.age = 'This field is required';
    if (!formData.sex) newErrors.sex = 'This field is required';
    if (!formData.smoker) newErrors.smoker = 'This field is required';
    setErrors(newErrors);
    return Object.keys(newErrors).length === 0;
  };

  const handleInputChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value, type, checked } = e.target;
    if (type === 'checkbox') setFormData(prev => ({ ...prev, [name]: checked }));
    else setFormData(prev => ({ ...prev, [name]: value }));
    if (errors[name]) setErrors(prev => ({ ...prev, [name]: '' }));
  };

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!validateForm()) return;

    setIsSubmitting(true);
    setShowProgress(true);
    setProgress(0);
    setFeedbackMessage('');

    const interval = setInterval(() => {
      setProgress(prev => {
        const newProgress = prev + 10;
        setFeedbackMessage(`${newProgress}% completed`);
        if (newProgress >= 100) clearInterval(interval);
        return newProgress;
      });
    }, 200);

    try {
      const response = await fetch('http://127.0.0.1:5000/predict', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          Age: parseFloat(formData.age),
          Sex: parseInt(formData.sex),
          Smoker: parseInt(formData.smoker),
          Cough: formData.cough ? 1 : 0,
          Fatigue: formData.fatigue ? 1 : 0,
          Fever: formData.fever ? 1 : 0,
          Shortness_of_breath: formData.shortness_of_breath ? 1 : 0,
          Hour_Of_Day: new Date().getHours()
        }),
      });

      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);

      const data: ResultData = await response.json();
      clearInterval(interval);
      setFeedbackMessage('');
      setIsSubmitting(false);

      navigate('/result', { state: data });
    } catch (err) {
      console.error(err);
      clearInterval(interval);
      setIsSubmitting(false);
      setFeedbackMessage('Error during prediction.');
    }
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 via-white to-green-50">
      <header className="bg-white shadow-sm border-b border-blue-100 py-6">
        <div className="max-w-6xl mx-auto px-4 flex items-center justify-center space-x-4">
          <div className="w-12 h-12 bg-gradient-to-br from-blue-600 to-green-600 rounded-xl flex items-center justify-center">
            <Wind className="w-6 h-6 text-white" />
          </div>
          <div className="text-center">
            <h1 className="text-3xl font-bold bg-gradient-to-r from-blue-600 to-green-600 bg-clip-text text-transparent flex items-center">
              <Stethoscope className="w-8 h-8 text-blue-600 mr-2" />
              Airnalyzer
            </h1>
            <p className="text-gray-600 text-sm font-medium">Air in, air out, find out.</p>
          </div>
        </div>
      </header>

      <main className="max-w-4xl mx-auto px-4 py-8">
        <form onSubmit={handleSubmit} className="space-y-8">
          <div className="bg-white rounded-2xl shadow-lg p-8 border border-blue-100">
            <div className="flex items-center mb-8">
              <div className="w-10 h-10 bg-blue-100 rounded-lg flex items-center justify-center mr-4">
                <User className="w-5 h-5 text-blue-600" />
              </div>
              <h2 className="text-2xl font-bold text-gray-900">Patient Diagnostic Form</h2>
            </div>

            {/* Age */}
            <div className="mb-6">
              <label className="block text-sm font-semibold text-gray-700 mb-3">
                Age <span className="text-red-500">*</span>
              </label>
              <input
                type="number"
                name="age"
                value={formData.age}
                onChange={handleInputChange}
                min="1"
                max="120"
                placeholder="Enter your age"
                required
                className={`w-full px-4 py-3 border-2 rounded-xl focus:ring-2 focus:ring-blue-500 focus:border-transparent transition-all ${
                  errors.age ? 'border-red-500 bg-red-50' : 'border-gray-200 hover:border-blue-300'
                }`}
              />
              {errors.age && (
                <p className="text-red-600 text-sm mt-2 flex items-center">
                  <AlertCircle className="w-4 h-4 mr-1" /> {errors.age}
                </p>
              )}
            </div>

            {/* Sex */}
            <div className="mb-6">
              <label className="block text-sm font-semibold text-gray-700 mb-3">
                Sex <span className="text-red-500">*</span>
              </label>
              <div className="space-y-3">
                <label className="flex items-center p-3 border-2 rounded-xl cursor-pointer hover:bg-blue-50 transition-colors">
                  <input
                    type="radio"
                    name="sex"
                    value="0"
                    checked={formData.sex === '0'}
                    onChange={handleInputChange}
                    required
                    className="w-4 h-4 text-blue-600 border-gray-300 focus:ring-blue-500"
                  />
                  <span className="ml-3 text-gray-700 font-medium">Female</span>
                </label>
                <label className="flex items-center p-3 border-2 rounded-xl cursor-pointer hover:bg-blue-50 transition-colors">
                  <input
                    type="radio"
                    name="sex"
                    value="1"
                    checked={formData.sex === '1'}
                    onChange={handleInputChange}
                    className="w-4 h-4 text-blue-600 border-gray-300 focus:ring-blue-500"
                  />
                  <span className="ml-3 text-gray-700 font-medium">Male</span>
                </label>
              </div>
            </div>

            {/* Smoker */}
            <div className="mb-6">
              <label className="block text-sm font-semibold text-gray-700 mb-3">
                Smoker <span className="text-red-500">*</span>
              </label>
              <div className="flex space-x-6">
                <label className="flex items-center p-3 border-2 rounded-xl cursor-pointer hover:bg-blue-50 transition-colors">
                  <input
                    type="radio"
                    name="smoker"
                    value="0"
                    checked={formData.smoker === '0'}
                    onChange={handleInputChange}
                    required
                    className="w-4 h-4 text-blue-600 border-gray-300 focus:ring-blue-500"
                  />
                  <span className="ml-3 text-gray-700 font-medium">No</span>
                </label>
                <label className="flex items-center p-3 border-2 rounded-xl cursor-pointer hover:bg-blue-50 transition-colors">
                  <input
                    type="radio"
                    name="smoker"
                    value="1"
                    checked={formData.smoker === '1'}
                    onChange={handleInputChange}
                    className="w-4 h-4 text-blue-600 border-gray-300 focus:ring-blue-500"
                  />
                  <span className="ml-3 text-gray-700 font-medium">Yes</span>
                </label>
              </div>
            </div>

            {/* Symptoms */}
            <div className="mb-6">
              <label className="block text-sm font-semibold text-gray-700 mb-3">
                Symptoms
              </label>
              <div className="grid grid-cols-2 gap-4">
                {['cough', 'fatigue', 'fever', 'shortness_of_breath'].map(symp => (
                  <label
                    key={symp}
                    className="flex items-center p-3 border-2 rounded-xl cursor-pointer hover:bg-blue-50 transition-colors"
                  >
                    <input
                      type="checkbox"
                      name={symp}
                      checked={(formData as any)[symp]}
                      onChange={handleInputChange}
                      className="w-4 h-4 text-blue-600 border-gray-300 focus:ring-blue-500"
                    />
                    <span className="ml-3 text-gray-700 font-medium">
                      {symp.replace(/_/g, ' ').replace(/\b\w/g, l => l.toUpperCase())}
                    </span>
                  </label>
                ))}
              </div>
            </div>

            <button
              type="submit"
              disabled={isSubmitting}
              className="w-full bg-gradient-to-r from-blue-600 to-green-600 text-white font-bold py-3 rounded-2xl hover:opacity-90 transition-opacity flex justify-center items-center"
            >
              {isSubmitting ? <Loader2 className="w-5 h-5 animate-spin mr-2" /> : 'Submit'}
            </button>

            {showProgress && (
              <div className="mt-4">
                <div className="h-4 bg-gray-200 rounded-full overflow-hidden">
                  <div className="h-4 bg-blue-600 rounded-full transition-all" style={{ width: `${progress}%` }}></div>
                </div>
                <p className="text-sm mt-2 text-gray-700">{feedbackMessage}</p>
              </div>
            )}
          </div>
        </form>
      </main>
    </div>
  );
}

// ----------------------------
// ResultPage Component
// ----------------------------
function ResultPage() {
  const location = useLocation();
  const data = location.state as ResultData;

  if (!data) {
    return <div className="p-10 text-center">No data available</div>;
  }

  return (
    <div className="min-h-screen flex flex-col items-center justify-center bg-gradient-to-br from-blue-50 via-white to-green-50">
      <div className="bg-white shadow-lg p-10 rounded-2xl border border-blue-100 max-w-md w-full text-center">
        <CheckCircle2 className="w-12 h-12 text-green-600 mx-auto mb-4" />
        <h2 className="text-2xl font-bold mb-4">Diagnostic Result</h2>
        <p className="text-lg font-medium mb-2">
          Diagnosis: <span className="text-blue-600">{data.diagnosis}</span>
        </p>
        <p className="text-lg font-medium">
          Confidence: <span className="text-green-600">{(data.confidence * 100).toFixed(2)}%</span>
        </p>
      </div>
    </div>
  );
}

// ----------------------------
// Main App
// ----------------------------
function App() {
  return (
    <Router>
      <Routes>
        <Route path="/" element={<DiagnosticForm />} />
        <Route path="/result" element={<ResultPage />} />
      </Routes>
    </Router>
  );
}

export default App;
