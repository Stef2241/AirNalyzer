document.addEventListener('DOMContentLoaded', function() {
  const form = document.getElementById('diagnostic-form');
  const progress = document.getElementById('progress');
  const feedbackMessage = document.createElement('div');
  feedbackMessage.id = 'feedback-message';
  form.appendChild(feedbackMessage);

  form.addEventListener('submit', function(e) {
    e.preventDefault();

    // Validate required fields
    const requiredFields = form.querySelectorAll('[required]');
    let isValid = true;
    
    requiredFields.forEach(field => {
      if (!field.value) {
        showError(field, 'This field is required');
        isValid = false;
      } else {
        clearError(field);
      }
      
      // Additional validation
      if (field.type === 'email' && !validateEmail(field.value)) {
        showError(field, 'Please enter a valid email address');
        isValid = false;
      }
    });

    if (isValid) {
      progress.value = 0;
      progress.style.display = 'block';
      
      const interval = setInterval(() => {
        progress.value += 10;
        feedbackMessage.textContent = `${progress.value}% completed`;
        progress.style.backgroundColor = progress.value < 100 ? '#4caf50' : '#d4edda'; // Color change

        if (progress.value >= 100) {
          clearInterval(interval);
          submitForm(); // Call to async form submission
        }
      }, 300);
    }
  });

  // Function to display error messages
  function showError(field, message) {
    field.style.borderColor = '#D11D1D';
    const errorMsg = field.closest('.form-row').querySelector('.error-message');
    if (errorMsg) errorMsg.textContent = message;
  }

  // Function to clear error messages
  function clearError(field) {
    field.style.borderColor = '#ddd';
    const errorMsg = field.closest('.form-row').querySelector('.error-message');
    if (errorMsg) errorMsg.textContent = '';
  }

  // Email validation
  function validateEmail(email) {
    const re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/;
    return re.test(String(email).toLowerCase());
  }

  // Async form submission using Fetch API
  function submitForm() {
    const formData = new FormData(form);
    fetch(form.action, {
      method: 'POST',
      body: formData,
    })
    .then(response => response.json())
    .then(data => {
      feedbackMessage.textContent = 'Form submitted successfully!';
      // Perform any further actions based on the response data
    })
    .catch(error => {
      feedbackMessage.textContent = 'An error occurred during submission.';
      console.error('Error:', error);
    });
  }

  // Clear error messages when typing
  form.querySelectorAll('input, select').forEach(field => {
    field.addEventListener('input', function() {
      clearError(this);
    });
  });
});
