# ESP32 BLE Gamepad

## English

### Overview
This project implements a Bluetooth Low Energy (BLE) gamepad using an ESP32 microcontroller. It allows you to create a customizable wireless gamepad that can be connected to computers, smartphones, or other devices supporting Bluetooth controllers.

### Features
- BLE connectivity for wireless control
- Configurable buttons (up to 32) with customizable PINs
- Web interface for easy configuration
- Authentication system (default credentials: admin/admin)
- Sleep mode for energy conservation
- LED status indicators
- Simple wiring with direct button connections to ESP32 pins

### Hardware Requirements
- ESP32 development board
- Push buttons or microswitches
- 10k ohm resistors (optional, as the internal pull-up resistors are used)
- LED for status indication (optional)
- Power source (USB or battery)

### Installation
1. Clone this repository or download the source code
2. Install the required libraries in your Arduino IDE:
   - ESP32 BLE Arduino by Neil Kolban
   - AsyncTCP by Hristo Gochkov
   - ESPAsyncWebServer by Hristo Gochkov
   - ArduinoJson by Benoit Blanchon
3. Connect your buttons to the ESP32 as per your configuration
4. Upload the code to your ESP32
5. Upload the web interface files to SPIFFS using the "ESP32 Sketch Data Upload" tool

### Web Interface Usage
1. Turn on the web interface by pressing the BOOT button on your ESP32
2. Connect to the "ESP32-BLE-Gamepad" WiFi network with password "esp32gamepad"
3. Open a web browser and navigate to `http://192.168.4.1`
4. Log in with the default credentials (username: `admin`, password: `admin`)
5. Configure your gamepad through the intuitive web interface:
   - General Settings: Change device name, manufacturer, battery level, and sleep timeout
   - Buttons: Add, edit, or remove button configurations
   - Credentials: Change the username and password

### Power Management
The gamepad enters sleep mode after a configurable period of inactivity to conserve power. Press any configured button to wake it up. The LED indicates the current status:
- Solid ON: Connected via BLE
- OFF: Not connected
- Blinking: Web interface active

### Troubleshooting
- If buttons don't respond, check your wiring and PIN configurations
- If the gamepad doesn't connect, ensure Bluetooth is enabled on your host device
- If the web interface isn't accessible, try resetting the ESP32 and check your WiFi connection

### License
This project is released under the MIT License.

---

## Ελληνικά

### Επισκόπηση
Αυτό το έργο υλοποιεί ένα χειριστήριο Bluetooth Low Energy (BLE) χρησιμοποιώντας έναν μικροελεγκτή ESP32. Σας επιτρέπει να δημιουργήσετε ένα προσαρμόσιμο ασύρματο χειριστήριο που μπορεί να συνδεθεί με υπολογιστές, smartphones ή άλλες συσκευές που υποστηρίζουν ελεγκτές Bluetooth.

### Χαρακτηριστικά
- Συνδεσιμότητα BLE για ασύρματο έλεγχο
- Διαμορφώσιμα κουμπιά (έως 32) με προσαρμόσιμα PIN
- Διαδικτυακή διεπαφή για εύκολη διαμόρφωση
- Σύστημα αυθεντικοποίησης (προεπιλεγμένα διαπιστευτήρια: admin/admin)
- Λειτουργία αναστολής για εξοικονόμηση ενέργειας
- Ενδείξεις κατάστασης LED
- Απλή καλωδίωση με απευθείας συνδέσεις κουμπιών στα PIN του ESP32

### Απαιτήσεις Υλικού
- Αναπτυξιακή πλακέτα ESP32
- Κουμπιά πίεσης ή μικροδιακόπτες
- Αντιστάσεις 10k ohm (προαιρετικά, καθώς χρησιμοποιούνται οι εσωτερικές αντιστάσεις pull-up)
- LED για ένδειξη κατάστασης (προαιρετικά)
- Πηγή τροφοδοσίας (USB ή μπαταρία)

### Εγκατάσταση
1. Κλωνοποιήστε αυτό το αποθετήριο ή κατεβάστε τον πηγαίο κώδικα
2. Εγκαταστήστε τις απαιτούμενες βιβλιοθήκες στο Arduino IDE σας:
   - ESP32 BLE Arduino από τον Neil Kolban
   - AsyncTCP από τον Hristo Gochkov
   - ESPAsyncWebServer από τον Hristo Gochkov
   - ArduinoJson από τον Benoit Blanchon
3. Συνδέστε τα κουμπιά σας στον ESP32 σύμφωνα με τη διαμόρφωσή σας
4. Ανεβάστε τον κώδικα στον ESP32 σας
5. Ανεβάστε τα αρχεία διαδικτυακής διεπαφής στο SPIFFS χρησιμοποιώντας το εργαλείο "ESP32 Sketch Data Upload"

### Χρήση της Διαδικτυακής Διεπαφής
1. Ενεργοποιήστε τη διαδικτυακή διεπαφή πατώντας το κουμπί BOOT στον ESP32 σας
2. Συνδεθείτε στο δίκτυο WiFi "ESP32-BLE-Gamepad" με κωδικό πρόσβασης "esp32gamepad"
3. Ανοίξτε ένα πρόγραμμα περιήγησης ιστού και μεταβείτε στο `http://192.168.4.1`
4. Συνδεθείτε με τα προεπιλεγμένα διαπιστευτήρια (όνομα χρήστη: `admin`, κωδικός πρόσβασης: `admin`)
5. Διαμορφώστε το χειριστήριό σας μέσω της διαισθητικής διαδικτυακής διεπαφής:
   - Γενικές Ρυθμίσεις: Αλλάξτε το όνομα της συσκευής, τον κατασκευαστή, το επίπεδο μπαταρίας και το χρονικό όριο αναστολής
   - Κουμπιά: Προσθέστε, επεξεργαστείτε ή αφαιρέστε διαμορφώσεις κουμπιών
   - Διαπιστευτήρια: Αλλάξτε το όνομα χρήστη και τον κωδικό πρόσβασης

### Διαχείριση Ενέργειας
Το χειριστήριο εισέρχεται σε λειτουργία αναστολής μετά από μια διαμορφώσιμη περίοδο αδράνειας για εξοικονόμηση ενέργειας. Πατήστε οποιοδήποτε διαμορφωμένο κουμπί για να το αφυπνίσετε. Το LED υποδεικνύει την τρέχουσα κατάσταση:
- Σταθερά ΑΝΑΜΜΕΝΟ: Συνδεδεμένο μέσω BLE
- ΣΒΗΣΤΟ: Μη συνδεδεμένο
- Αναβοσβήνει: Ενεργή διαδικτυακή διεπαφή

### Αντιμετώπιση Προβλημάτων
- Αν τα κουμπιά δεν αποκρίνονται, ελέγξτε την καλωδίωσή σας και τις διαμορφώσεις PIN
- Αν το χειριστήριο δεν συνδέεται, βεβαιωθείτε ότι το Bluetooth είναι ενεργοποιημένο στη συσκευή υποδοχής σας
- Αν η διαδικτυακή διεπαφή δεν είναι προσβάσιμη, δοκιμάστε να επαναφέρετε τον ESP32 και ελέγξτε τη σύνδεση WiFi σας

### Άδεια Χρήσης
Αυτό το έργο κυκλοφορεί υπό την Άδεια MIT.