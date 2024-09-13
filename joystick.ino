#include <BleGamepad.h>
#include <esp_sleep.h>

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
constexpr unsigned long CONNECTION_CHECK_INTERVAL = 5000;  // 5 δευτερόλεπτα

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

  // Ενεργοποίηση αφύπνισης από βαθύ ύπνο με το κουμπί A
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

      // Αν περάσει το χρονικό όριο χωρίς δραστηριότητα, εισέρχεται σε βαθύ ύπνο
      if ((currentMillis - lastActivityTime) > SLEEP_TIMEOUT) {
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
