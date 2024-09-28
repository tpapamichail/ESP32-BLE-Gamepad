#include <BleGamepad.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>  // Απαιτείται για την απομόνωση των GPIO

// Δημιουργία αντικειμένου BleGamepad
BleGamepad bleGamepad;

// Χρήση constexpr για τον ορισμό των GPIO pins για κάθε κουμπί
constexpr int BUTTON_A_PIN = 32;
constexpr int BUTTON_B_PIN = 33;
constexpr int BUTTON_X_PIN = 25;
constexpr int BUTTON_Y_PIN = 26;
constexpr int BUTTON_L1_PIN = 27;
constexpr int BUTTON_R1_PIN = 14;
constexpr int BUTTON_L2_PIN = 12;
constexpr int BUTTON_R2_PIN = 13;
constexpr int BUTTON_START_PIN = 23;
constexpr int BUTTON_SELECT_PIN = 22;
constexpr int BUTTON_THUMB_LEFT_PIN = 4;
constexpr int BUTTON_THUMB_RIGHT_PIN = 5;
constexpr int BUTTON_DPAD_UP_PIN = 21;
constexpr int BUTTON_DPAD_DOWN_PIN = 19;
constexpr int BUTTON_DPAD_LEFT_PIN = 18;
constexpr int BUTTON_DPAD_RIGHT_PIN = 17;

constexpr int LED_PIN = 2;  // Συνήθως το ενσωματωμένο LED είναι στο GPIO 2

constexpr int WAKE_BUTTON_PIN = BUTTON_A_PIN;

// Πίνακας με όλα τα pins των κουμπιών
constexpr int buttonPins[] = {
  BUTTON_A_PIN, BUTTON_B_PIN, BUTTON_X_PIN, BUTTON_Y_PIN,
  BUTTON_L1_PIN, BUTTON_R1_PIN, BUTTON_L2_PIN, BUTTON_R2_PIN,
  BUTTON_START_PIN, BUTTON_SELECT_PIN, BUTTON_THUMB_LEFT_PIN, BUTTON_THUMB_RIGHT_PIN,
  BUTTON_DPAD_UP_PIN, BUTTON_DPAD_DOWN_PIN, BUTTON_DPAD_LEFT_PIN, BUTTON_DPAD_RIGHT_PIN
};
constexpr int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

// Σταθερές για καθυστέρηση αποθορυβοποίησης, διάστημα polling και χρονικό όριο ύπνου
constexpr unsigned long DEBOUNCE_DELAY = 5;
constexpr unsigned long POLLING_INTERVAL = 5;
constexpr unsigned long SLEEP_TIMEOUT = 600000;  // 10 λεπτά
constexpr unsigned long CONNECTION_CHECK_INTERVAL = 60000;  // 1 λεπτό

// Μεταβλητές χρόνου
unsigned long previousMillis = 0;
unsigned long lastActivityTime = 0;
unsigned long lastConnectionCheck = 0;

// Πίνακες για την παρακολούθηση της κατάστασης των κουμπιών
unsigned long lastDebounceTime[numButtons];
bool buttonState[numButtons];
bool lastButtonState[numButtons];

// Μεταβλητή για την αποθήκευση της κατάστασης σύνδεσης
bool bleConnected = false;

void setup() {
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
}

void loop() {
  unsigned long currentMillis = millis();

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
    }
  } else {
    // Αναμονή για σύνδεση
    if (currentMillis - previousMillis >= 1000) {
      Serial.println("Waiting for connection...");
      previousMillis = currentMillis;
    }
  }
}

/**
 * Ελέγχει την κατάσταση ενός κουμπιού και ενημερώνει τη συσκευή BLE.
 * Επιστρέφει true αν υπήρξε αλλαγή στην κατάσταση του κουμπιού.
 */
bool checkButton(int buttonIndex) {
  int reading = digitalRead(buttonPins[buttonIndex]);
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
        bleGamepad.press(BUTTON_1 + buttonIndex);
      } else {
        bleGamepad.release(BUTTON_1 + buttonIndex);
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
