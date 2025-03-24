// script.js - Κύριο JavaScript αρχείο για το ESP32 BLE Gamepad Web Interface

// Κοινές λειτουργίες
function showAlert(message, type = 'success') {
    const alertBox = document.createElement('div');
    alertBox.className = `alert alert-${type}`;
    alertBox.textContent = message;
    
    // Εύρεση του στοιχείου με class alerts ή δημιουργία ενός νέου
    let alertsContainer = document.querySelector('.alerts');
    if (!alertsContainer) {
        alertsContainer = document.createElement('div');
        alertsContainer.className = 'alerts';
        document.querySelector('main').prepend(alertsContainer);
    }
    
    // Προσθήκη του alert
    alertsContainer.appendChild(alertBox);
    
    // Αφαίρεση του alert μετά από 5 δευτερόλεπτα
    setTimeout(() => {
        alertBox.remove();
    }, 5000);
}

// Διαχείριση των γενικών ρυθμίσεων
function setupSettingsPage() {
    const form = document.getElementById('settingsForm');
    if (!form) return; // Αν δεν βρίσκεται στη σελίδα ρυθμίσεων
    
    // Φόρτωση των τρεχουσών ρυθμίσεων
    fetch('/api/settings')
        .then(response => response.json())
        .then(data => {
            document.getElementById('deviceName').value = data.deviceName;
            document.getElementById('manufacturer').value = data.manufacturer;
            document.getElementById('batteryLevel').value = data.batteryLevel;
            document.getElementById('sleepTimeout').value = data.sleepTimeout / 60000; // Μετατροπή ms σε λεπτά
        })
        .catch(error => {
            console.error('Error loading settings:', error);
            showAlert('Αποτυχία φόρτωσης ρυθμίσεων', 'danger');
        });
    
    // Χειρισμός της υποβολής της φόρμας
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const deviceName = document.getElementById('deviceName').value;
        const manufacturer = document.getElementById('manufacturer').value;
        const batteryLevel = document.getElementById('batteryLevel').value;
        const sleepTimeout = document.getElementById('sleepTimeout').value * 60000; // Μετατροπή λεπτών σε ms
        
        const formData = new FormData();
        formData.append('deviceName', deviceName);
        formData.append('manufacturer', manufacturer);
        formData.append('batteryLevel', batteryLevel);
        formData.append('sleepTimeout', sleepTimeout);
        
        fetch('/api/settings/update', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showAlert('Οι ρυθμίσεις αποθηκεύτηκαν επιτυχώς!');
            } else {
                showAlert(data.error || 'Σφάλμα κατά την αποθήκευση των ρυθμίσεων', 'danger');
            }
        })
        .catch(error => {
            console.error('Error saving settings:', error);
            showAlert('Αποτυχία αποθήκευσης ρυθμίσεων', 'danger');
        });
    });

    // Χειρισμός του κουμπιού επαναφοράς προεπιλογών
    const resetButton = document.getElementById('resetButton');
    if (resetButton) {
        resetButton.addEventListener('click', function() {
            if (confirm('Είστε σίγουροι ότι θέλετε να επαναφέρετε τις προεπιλεγμένες ρυθμίσεις; Όλες οι τρέχουσες ρυθμίσεις θα χαθούν.')) {
                fetch('/api/reset', {
                    method: 'POST'
                })
                .then(response => response.json())
                .then(data => {
                    if (data.success) {
                        showAlert('Επιτυχής επαναφορά προεπιλεγμένων ρυθμίσεων. Η σελίδα θα ανανεωθεί...');
                        setTimeout(() => {
                            window.location.reload();
                        }, 2000);
                    } else {
                        showAlert(data.error || 'Σφάλμα κατά την επαναφορά των ρυθμίσεων', 'danger');
                    }
                })
                .catch(error => {
                    console.error('Error resetting settings:', error);
                    showAlert('Αποτυχία επαναφοράς προεπιλεγμένων ρυθμίσεων', 'danger');
                });
            }
        });
    }
}

// Διαχείριση των κουμπιών
function setupButtonsPage() {
    const buttonsTable = document.getElementById('buttonsTable');
    const buttonForm = document.getElementById('buttonForm');
    if (!buttonsTable || !buttonForm) return; // Αν δεν βρίσκεται στη σελίδα κουμπιών
    
    let editingButtonId = null;
    
    // Φόρτωση των υπαρχόντων κουμπιών
    function loadButtons() {
        fetch('/api/buttons')
            .then(response => response.json())
            .then(data => {
                const tbody = buttonsTable.querySelector('tbody');
                tbody.innerHTML = '';
                
                data.buttons.forEach(button => {
                    const row = document.createElement('tr');
                    row.innerHTML = `
                        <td>${button.buttonId}</td>
                        <td>${button.pin}</td>
                        <td>${button.label}</td>
                        <td>${button.enabled ? 'Ναι' : 'Όχι'}</td>
                        <td>
                            <button class="button button-primary edit-button" data-id="${button.buttonId}">Επεξεργασία</button>
                            <button class="button button-danger delete-button" data-id="${button.buttonId}">Διαγραφή</button>
                        </td>
                    `;
                    tbody.appendChild(row);
                });
                
                // Προσθήκη event listeners για τα κουμπιά επεξεργασίας και διαγραφής
                document.querySelectorAll('.edit-button').forEach(button => {
                    button.addEventListener('click', function() {
                        const buttonId = parseInt(this.getAttribute('data-id'));
                        editButton(buttonId, data.buttons);
                    });
                });
                
                document.querySelectorAll('.delete-button').forEach(button => {
                    button.addEventListener('click', function() {
                        const buttonId = parseInt(this.getAttribute('data-id'));
                        deleteButton(buttonId);
                    });
                });
            })
            .catch(error => {
                console.error('Error loading buttons:', error);
                showAlert('Αποτυχία φόρτωσης κουμπιών', 'danger');
            });
    }
    
    // Επεξεργασία ενός κουμπιού
    function editButton(buttonId, buttons) {
        const button = buttons.find(b => b.buttonId === buttonId);
        if (!button) return;
        
        document.getElementById('buttonPin').value = button.pin;
        document.getElementById('buttonId').value = button.buttonId;
        document.getElementById('buttonLabel').value = button.label;
        document.getElementById('buttonEnabled').checked = button.enabled;
        
        document.getElementById('formTitle').textContent = 'Επεξεργασία Κουμπιού';
        document.getElementById('buttonSubmit').textContent = 'Ενημέρωση';
        
        editingButtonId = buttonId;
        
        // Απενεργοποίηση του πεδίου buttonId κατά την επεξεργασία
        document.getElementById('buttonId').disabled = true;
        
        // Εμφάνιση του κουμπιού ακύρωσης
        document.getElementById('buttonCancel').style.display = 'inline-block';
    }
    
    // Διαγραφή ενός κουμπιού
    function deleteButton(buttonId) {
        if (confirm(`Είστε σίγουροι ότι θέλετε να διαγράψετε το κουμπί ${buttonId};`)) {
            const formData = new FormData();
            formData.append('buttonId', buttonId);
            
            fetch('/api/buttons/delete', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showAlert('Το κουμπί διαγράφηκε επιτυχώς');
                    loadButtons();
                } else {
                    showAlert(data.error || 'Σφάλμα κατά τη διαγραφή του κουμπιού', 'danger');
                }
            })
            .catch(error => {
                console.error('Error deleting button:', error);
                showAlert('Αποτυχία διαγραφής κουμπιού', 'danger');
            });
        }
    }
    
    // Ακύρωση επεξεργασίας
    document.getElementById('buttonCancel').addEventListener('click', function() {
        resetForm();
    });
    
    // Επαναφορά της φόρμας στην αρχική κατάσταση (για προσθήκη)
    function resetForm() {
        buttonForm.reset();
        document.getElementById('formTitle').textContent = 'Προσθήκη Νέου Κουμπιού';
        document.getElementById('buttonSubmit').textContent = 'Προσθήκη';
        document.getElementById('buttonId').disabled = false;
        document.getElementById('buttonCancel').style.display = 'none';
        editingButtonId = null;
    }
    
    // Χειρισμός της υποβολής της φόρμας κουμπιού
    buttonForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const pin = document.getElementById('buttonPin').value;
        const buttonId = document.getElementById('buttonId').value;
        const label = document.getElementById('buttonLabel').value;
        const enabled = document.getElementById('buttonEnabled').checked;
        
        const formData = new FormData();
        formData.append('pin', pin);
        formData.append('buttonId', buttonId);
        formData.append('label', label);
        formData.append('enabled', enabled);
        
        let url = '/api/buttons/add';
        if (editingButtonId !== null) {
            url = '/api/buttons/update';
        }
        
        fetch(url, {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showAlert(editingButtonId !== null ? 'Το κουμπί ενημερώθηκε επιτυχώς' : 'Το κουμπί προστέθηκε επιτυχώς');
                loadButtons();
                resetForm();
            } else {
                showAlert(data.error || 'Σφάλμα κατά την επεξεργασία του κουμπιού', 'danger');
            }
        })
        .catch(error => {
            console.error('Error saving button:', error);
            showAlert('Αποτυχία αποθήκευσης κουμπιού', 'danger');
        });
    });
    
    // Αρχική φόρτωση των κουμπιών
    loadButtons();
}

// Διαχείριση των διαπιστευτηρίων
function setupCredentialsPage() {
    const form = document.getElementById('credentialsForm');
    if (!form) return; // Αν δεν βρίσκεται στη σελίδα διαπιστευτηρίων
    
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirmPassword').value;
        
        if (password !== confirmPassword) {
            showAlert('Οι κωδικοί πρόσβασης δεν ταιριάζουν', 'danger');
            return;
        }
        
        if (username.length < 3 || password.length < 3) {
            showAlert('Το όνομα χρήστη και ο κωδικός πρέπει να έχουν τουλάχιστον 3 χαρακτήρες', 'danger');
            return;
        }
        
        const formData = new FormData();
        formData.append('username', username);
        formData.append('password', password);
        formData.append('confirmPassword', confirmPassword);
        
        fetch('/api/credentials/update', {
            method: 'POST',
            body: formData
        })
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                showAlert('Τα διαπιστευτήρια ενημερώθηκαν επιτυχώς. Θα χρειαστεί να συνδεθείτε ξανά...');
                setTimeout(() => {
                    window.location.href = '/';
                }, 2000);
            } else {
                showAlert(data.error || 'Σφάλμα κατά την ενημέρωση των διαπιστευτηρίων', 'danger');
            }
        })
        .catch(error => {
            console.error('Error updating credentials:', error);
            showAlert('Αποτυχία ενημέρωσης διαπιστευτηρίων', 'danger');
        });
    });
}

// Εκτέλεση του κώδικα όταν φορτωθεί το DOM
document.addEventListener('DOMContentLoaded', function() {
    setupSettingsPage();
    setupButtonsPage();
    setupCredentialsPage();
}); 