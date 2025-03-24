#include <BleGamepad.h>
#include <esp_sleep.h>
<<<<<<< Updated upstream:joystick/joystick.ino
#include <driver/rtc_io.h>  // Απαιτείται για την απομόνωση των GPIO
=======
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "JoystickConfig.h"
#include "WebServer.h"
>>>>>>> Stashed changes:joystick.ino

// Δημιουργία αντικειμένου BleGamepad
BleGamepad bleGamepad;

// Δημιουργία του Web Server
JoystickWebServer webServer;

// Πίνακες για την παρακολούθηση της κατάστασης των κουμπιών
unsigned long* lastDebounceTime;
bool* buttonState;
bool* lastButtonState;

// Σταθερές για καθυστέρηση αποθορυβοποίησης, διάστημα polling
constexpr unsigned long DEBOUNCE_DELAY = 5;
constexpr unsigned long POLLING_INTERVAL = 5;
<<<<<<< Updated upstream:joystick/joystick.ino
constexpr unsigned long SLEEP_TIMEOUT = 600000;  // 10 λεπτά
constexpr unsigned long CONNECTION_CHECK_INTERVAL = 60000;  // 1 λεπτό
=======
constexpr unsigned long CONNECTION_CHECK_INTERVAL = 5000;  // 5 δευτερόλεπτα
>>>>>>> Stashed changes:joystick.ino

// Μεταβλητές χρόνου
unsigned long previousMillis = 0;
unsigned long lastActivityTime = 0;
unsigned long lastConnectionCheck = 0;

// Μεταβλητή για την αποθήκευση της κατάστασης σύνδεσης
bool bleConnected = false;

// Pin για το LED
constexpr int LED_PIN = 2;  // Συνήθως το ενσωματωμένο LED είναι στο GPIO 2

// Pin για το κουμπί ενεργοποίησης/απενεργοποίησης του web interface
constexpr int WEB_TOGGLE_PIN = 0;  // Το κουμπί BOOT του ESP32

// Κατάσταση του Web Interface
bool webInterfaceEnabled = false;

void setup() {
<<<<<<< Updated upstream:joystick/joystick.ino
  Serial.begin(115200);
  Serial.println("Starting BLE Gamepad...");
  bleGamepad.begin();

  // Ρύθμιση των pins των κουμπιών ως είσοδοι με εσωτερική αντίσταση pull-up
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
    lastButtonState[i] = HIGH;
    buttonState[i] = HIGH;
    lastDebounceTime[i] = 0;
  }

  // Ρύθμιση του LED pin ως έξοδος
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  // Ενεργοποίηση αφύπνισης από Hibernation mode με το κουμπί A (EXT0 wakeup source)
  esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(WAKE_BUTTON_PIN), LOW);

  // Αρχικοποίηση του χρόνου τελευταίας δραστηριότητας
  lastActivityTime = millis();
=======
    Serial.begin(115200);
    Serial.println("Έναρξη ESP32 BLE Gamepad με Web Interface...");
    
    // Αρχικοποίηση του συστήματος αρχείων
    if (!initConfig()) {
        Serial.println("Αποτυχία αρχικοποίησης ρυθμίσεων, επαναφορά στις προεπιλογές");
        resetToDefaults();
        saveConfig();
    }
    
    // Ρύθμιση των πινς των κουμπιών και των μεταβλητών παρακολούθησης
    setupButtons();
    
    // Ρύθμιση του LED pin ως έξοδος
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Ρύθμιση του κουμπιού web toggle
    pinMode(WEB_TOGGLE_PIN, INPUT_PULLUP);
    
    // Εκκίνηση του BLE Gamepad
    startBleGamepad();
    
    // Ενεργοποίηση αφύπνισης από βαθύ ύπνο με το κουμπί A
    if (config.numButtons > 0) {
        esp_sleep_enable_ext0_wakeup(static_cast<gpio_num_t>(config.buttons[0].pin), LOW);
    }
    
    // Αρχικοποίηση του χρόνου τελευταίας δραστηριότητας
    lastActivityTime = millis();
>>>>>>> Stashed changes:joystick.ino
}

void loop() {
    unsigned long currentMillis = millis();
    
    // Έλεγχος του κουμπιού ενεργοποίησης/απενεργοποίησης του web interface
    checkWebToggleButton();
    
    // Έλεγχος κατάστασης σύνδεσης περιοδικά
    if (currentMillis - lastConnectionCheck >= CONNECTION_CHECK_INTERVAL) {
        bool currentConnectionStatus = bleGamepad.isConnected();
        
        if (currentConnectionStatus != bleConnected) {
            bleConnected = currentConnectionStatus;
            Serial.print("BLE Connection Status: ");
            Serial.println(bleConnected ? "Connected" : "Disconnected");
        }
        
        lastConnectionCheck = currentMillis;
    }
<<<<<<< Updated upstream:joystick/joystick.ino

    lastConnectionCheck = currentMillis;
  }

  // Διαχείριση του LED ανάλογα με την κατάσταση σύνδεσης
  if (bleConnected) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if (bleConnected) {
    if (currentMillis - previousMillis >= POLLING_INTERVAL) {
      bool activityDetected = false;

      // Έλεγχος της κατάστασης κάθε κουμπιού
      for (int i = 0; i < numButtons; i++) {
        activityDetected |= checkButton(i);
      }

      // Αν εντοπιστεί δραστηριότητα, ενημέρωσε τον χρόνο τελευταίας δραστηριότητας
      if (activityDetected) {
        lastActivityTime = currentMillis;
      }

      // Αν περάσει το χρονικό όριο χωρίς δραστηριότητα, εισέρχεται σε Hibernation mode
      if ((currentMillis - lastActivityTime) > SLEEP_TIMEOUT) {
        Serial.println("Entering hibernation mode...");
        digitalWrite(LED_PIN, LOW);
        prepareForHibernation(); // Κλήση της συνάρτησης προετοιμασίας
        esp_deep_sleep_start();
      }

      previousMillis = currentMillis;
=======
    
    // Διαχείριση του LED ανάλογα με την κατάσταση
    updateLed();
    
    if (bleConnected) {
        if (currentMillis - previousMillis >= POLLING_INTERVAL) {
            bool activityDetected = false;
            
            // Έλεγχος της κατάστασης κάθε κουμπιού
            for (int i = 0; i < config.numButtons; i++) {
                if (config.buttons[i].enabled) {
                    activityDetected |= checkButton(i);
                }
            }
            
            // Αν εντοπιστεί δραστηριότητα, ενημέρωσε τον χρόνο τελευταίας δραστηριότητας
            if (activityDetected) {
                lastActivityTime = currentMillis;
            }
            
            // Αν περάσει το χρονικό όριο χωρίς δραστηριότητα και το web interface είναι απενεργοποιημένο, 
            // εισέρχεται σε βαθύ ύπνο
            if ((currentMillis - lastActivityTime) > config.sleepTimeout && !webInterfaceEnabled) {
                Serial.println("Entering sleep mode...");
                bleGamepad.end();
                digitalWrite(LED_PIN, LOW);
                esp_deep_sleep_start();
            }
            
            previousMillis = currentMillis;
        }
    } else {
        // Αναμονή για σύνδεση
        if (currentMillis - previousMillis >= 1000) {
            Serial.println("Waiting for connection...");
            previousMillis = currentMillis;
        }
>>>>>>> Stashed changes:joystick.ino
    }
}

// Ρύθμιση των pins των κουμπιών και των μεταβλητών παρακολούθησης
void setupButtons() {
    // Δέσμευση μνήμης για τους πίνακες
    lastDebounceTime = new unsigned long[config.numButtons];
    buttonState = new bool[config.numButtons];
    lastButtonState = new bool[config.numButtons];
    
    // Αρχικοποίηση των pins και των μεταβλητών
    for (int i = 0; i < config.numButtons; i++) {
        pinMode(config.buttons[i].pin, INPUT_PULLUP);
        lastButtonState[i] = HIGH;
        buttonState[i] = HIGH;
        lastDebounceTime[i] = 0;
    }
}

// Έναρξη του BLE Gamepad με τις ρυθμίσεις από το config
void startBleGamepad() {
    // Αρχικοποίηση του BLE Gamepad με το όνομα συσκευής και τον κατασκευαστή από το config
    bleGamepad = BleGamepad(config.deviceName, config.manufacturer, config.batteryLevel);
    bleGamepad.begin();
    
    Serial.print("BLE Gamepad started with name: ");
    Serial.println(config.deviceName);
}

// Έλεγχος του κουμπιού ενεργοποίησης/απενεργοποίησης του web interface
void checkWebToggleButton() {
    static bool lastToggleState = HIGH;
    static unsigned long lastToggleDebounceTime = 0;
    
    // Διάβασε την τρέχουσα κατάσταση του κουμπιού
    bool toggleReading = digitalRead(WEB_TOGGLE_PIN);
    
    // Αν η κατάσταση άλλαξε, επανέφερε το χρόνο αποθορυβοποίησης
    if (toggleReading != lastToggleState) {
        lastToggleDebounceTime = millis();
    }
    
    // Αν έχει περάσει αρκετός χρόνος από την τελευταία αλλαγή
    if ((millis() - lastToggleDebounceTime) > 50) {
        // Αν η τρέχουσα κατάσταση είναι διαφορετική από την προηγούμενη
        if (toggleReading == LOW && lastToggleState == HIGH) {
            // Εναλλαγή κατάστασης web interface
            webInterfaceEnabled = !webInterfaceEnabled;
            
            if (webInterfaceEnabled) {
                // Εκκίνηση του web interface
                Serial.println("Ενεργοποίηση Web Interface...");
                if (webServer.begin()) {
                    Serial.println("Web Interface ενεργοποιήθηκε");
                } else {
                    Serial.println("Αποτυχία εκκίνησης Web Interface");
                    webInterfaceEnabled = false;
                }
            } else {
                // Τερματισμός του web interface
                Serial.println("Απενεργοποίηση Web Interface...");
                webServer.end();
                Serial.println("Web Interface απενεργοποιήθηκε");
            }
        }
    }
    
    lastToggleState = toggleReading;
}

// Ενημέρωση του LED ανάλογα με την κατάσταση
void updateLed() {
    if (webInterfaceEnabled) {
        // Αν το web interface είναι ενεργοποιημένο, κάνε το LED να αναβοσβήνει
        digitalWrite(LED_PIN, (millis() / 500) % 2);
    } else {
        // Αλλιώς, άναψε το LED μόνο αν έχει γίνει σύνδεση BLE
        digitalWrite(LED_PIN, bleConnected ? HIGH : LOW);
    }
}

/**
 * Ελέγχει την κατάσταση ενός κουμπιού και ενημερώνει τη συσκευή BLE.
 * Επιστρέφει true αν υπήρξε αλλαγή στην κατάσταση του κουμπιού.
 */
bool checkButton(int buttonIndex) {
    int reading = digitalRead(config.buttons[buttonIndex].pin);
    bool activity = false;
    
    // Έλεγχος για αποθορυβοποίηση
    if (reading != lastButtonState[buttonIndex]) {
        lastDebounceTime[buttonIndex] = millis();
    }
    
    if ((millis() - lastDebounceTime[buttonIndex]) > DEBOUNCE_DELAY) {
        if (reading != buttonState[buttonIndex]) {
            buttonState[buttonIndex] = reading;
            activity = true; // Υπήρξε αλλαγή κατάστασης κουμπιού
            
            // Ενημέρωση της κατάστασης του κουμπιού στη συσκευή BLE
            if (buttonState[buttonIndex] == LOW) {
                bleGamepad.press(config.buttons[buttonIndex].buttonId);
            } else {
                bleGamepad.release(config.buttons[buttonIndex].buttonId);
            }
        }
    }
    
    lastButtonState[buttonIndex] = reading;
    return activity;
}

/**
 * Προετοιμάζει το ESP32 για Hibernation mode για να μειώσει την κατανάλωση ενέργειας.
 */
void prepareForHibernation() {
  // Απενεργοποίηση του Bluetooth για μείωση της κατανάλωσης ενέργειας
  bleGamepad.end();
  btStop();

  // Απομόνωση των ακροδεκτών κουμπιών, εκτός από το WAKE_BUTTON_PIN
  for (int i = 0; i < numButtons; i++) {
    if (buttonPins[i] != WAKE_BUTTON_PIN) {
      gpio_num_t gpioNum = static_cast<gpio_num_t>(buttonPins[i]);
      gpio_pullup_dis(gpioNum);
      gpio_pulldown_dis(gpioNum);
      rtc_gpio_isolate(gpioNum);
    }
  }

  // Απομόνωση του LED_PIN
  gpio_num_t ledGpioNum = static_cast<gpio_num_t>(LED_PIN);
  gpio_pullup_dis(ledGpioNum);
  gpio_pulldown_dis(ledGpioNum);
  rtc_gpio_isolate(ledGpioNum);

  // Απενεργοποίηση των RTC περιφερειακών για ελαχιστοποίηση της κατανάλωσης ενέργειας
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
}
