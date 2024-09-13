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

// Προσθήκη του REVERSE_BUTTON_PIN
constexpr int REVERSE_BUTTON_PIN = 16;

// Πίνακας με όλα τα pins των κουμπιών
constexpr int buttonPins[] = {
  BUTTON_A_PIN, BUTTON_B_PIN, BUTTON_X_PIN, BUTTON_Y_PIN,
  BUTTON_L1_PIN, BUTTON_R1_PIN, BUTTON_L2_PIN, BUTTON_R2_PIN,
  BUTTON_START_PIN, BUTTON_SELECT_PIN, BUTTON_THUMB_LEFT_PIN, BUTTON_THUMB_RIGHT_PIN,
  BUTTON_DPAD_UP_PIN, BUTTON_DPAD_DOWN_PIN, BUTTON_DPAD_LEFT_PIN, BUTTON_DPAD_RIGHT_PIN
};
constexpr int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

// Σταθερές για καθυστέρηση αποθορυβοποίησης, διάστημα polling και χρονικό όριο ύπνου
constexpr unsigned long DEBOUNCE_DELAY = 10;
constexpr unsigned long POLLING_INTERVAL = 10;
constexpr unsigned long SLEEP_TIMEOUT = 300000;  // 5 λεπτά

// Μεταβλητές χρόνου
unsigned long previousMillis = 0;
unsigned long lastActivityTime = 0;

// Πίνακες για την παρακολούθηση της κατάστασης των κουμπιών
unsigned long lastDebounceTime[numButtons];
bool buttonState[numButtons];
bool lastButtonState[numButtons];

// Μεταβλητές για το push button αντιστροφής
unsigned long reverseButtonLastDebounceTime = 0;
bool reverseButtonState = HIGH;
bool lastReverseButtonState = HIGH;
bool reverseMode = false;

// Μεταβλητές για την παρακολούθηση των D-Pad κουμπιών
bool dpadUpPressed = false;
bool dpadDownPressed = false;
bool dpadLeftPressed = false;
bool dpadRightPressed = false;

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

  // Ρύθμιση του REVERSE_BUTTON_PIN
  pinMode(REVERSE_BUTTON_PIN, INPUT_PULLUP);
  reverseButtonLastDebounceTime = 0;

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

  // Διαχείριση του LED ανάλογα με την κατάσταση σύνδεσης
  if (bleGamepad.isConnected()) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  if (bleGamepad.isConnected()) {
    if (currentMillis - previousMillis >= POLLING_INTERVAL) {
      bool activityDetected = false;

      // Έλεγχος της κατάστασης του κουμπιού αντιστροφής
      activityDetected |= checkReverseButton();

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

      // Έλεγχος αν είναι D-Pad κουμπί
      if (buttonIndex >= 12 && buttonIndex <= 15) {
        // Ενημέρωση της κατάστασης των D-Pad κουμπιών
        updateDpadState(buttonIndex, buttonState[buttonIndex] == LOW);
      } else {
        // Ενημέρωση της κατάστασης των απλών κουμπιών
        int bleButtonIndex = BUTTON_1 + buttonIndex;

        // Ενημέρωση της κατάστασης του κουμπιού στη συσκευή BLE
        if (buttonState[buttonIndex] == LOW) {
          bleGamepad.press(bleButtonIndex);
        } else {
          bleGamepad.release(bleButtonIndex);
        }
      }
    }
  }

  lastButtonState[buttonIndex] = reading;
  return activity;
}

/**
 * Ενημερώνει την κατάσταση των D-Pad κουμπιών και ορίζει την κατεύθυνση του D-Pad.
 */
void updateDpadState(int buttonIndex, bool isPressed) {
  switch (buttonIndex) {
    case 12: // BUTTON_DPAD_UP_PIN
      dpadUpPressed = isPressed;
      break;
    case 13: // BUTTON_DPAD_DOWN_PIN
      dpadDownPressed = isPressed;
      break;
    case 14: // BUTTON_DPAD_LEFT_PIN
      dpadLeftPressed = isPressed;
      break;
    case 15: // BUTTON_DPAD_RIGHT_PIN
      dpadRightPressed = isPressed;
      break;
  }

  // Υπολογισμός της κατεύθυνσης του D-Pad
  uint8_t dpadDirection = DPAD_CENTERED;

  if (reverseMode) {
    // Αντιστροφή των κατευθύνσεων
    if (dpadUpPressed && !dpadDownPressed) {
      dpadDirection = DPAD_DOWN;
    }
    if (dpadDownPressed && !dpadUpPressed) {
      dpadDirection = DPAD_UP;
    }
    if (dpadLeftPressed && !dpadRightPressed) {
      dpadDirection = DPAD_RIGHT;
    }
    if (dpadRightPressed && !dpadLeftPressed) {
      dpadDirection = DPAD_LEFT;
    }

    // Συνδυασμός κατευθύνσεων για διαγώνιες κινήσεις
    if (dpadUpPressed && dpadRightPressed) dpadDirection = DPAD_DOWN_RIGHT;
    if (dpadUpPressed && dpadLeftPressed) dpadDirection = DPAD_DOWN_LEFT;
    if (dpadDownPressed && dpadRightPressed) dpadDirection = DPAD_UP_RIGHT;
    if (dpadDownPressed && dpadLeftPressed) dpadDirection = DPAD_UP_LEFT;

  } else {
    if (dpadUpPressed && !dpadDownPressed) {
      dpadDirection = DPAD_UP;
    }
    if (dpadDownPressed && !dpadUpPressed) {
      dpadDirection = DPAD_DOWN;
    }
    if (dpadLeftPressed && !dpadRightPressed) {
      dpadDirection = DPAD_LEFT;
    }
    if (dpadRightPressed && !dpadLeftPressed) {
      dpadDirection = DPAD_RIGHT;
    }

    // Συνδυασμός κατευθύνσεων για διαγώνιες κινήσεις
    if (dpadUpPressed && dpadRightPressed) dpadDirection = DPAD_UP_RIGHT;
    if (dpadUpPressed && dpadLeftPressed) dpadDirection = DPAD_UP_LEFT;
    if (dpadDownPressed && dpadRightPressed) dpadDirection = DPAD_DOWN_RIGHT;
    if (dpadDownPressed && dpadLeftPressed) dpadDirection = DPAD_DOWN_LEFT;
  }

  bleGamepad.setHat(dpadDirection);
}

/**
 * Ελέγχει το κουμπί αντιστροφής και εναλλάσσει το reverse mode αν πατηθεί.
 * Επιστρέφει true αν το κουμπί αντιστροφής πατήθηκε.
 */
bool checkReverseButton() {
  int reading = digitalRead(REVERSE_BUTTON_PIN);
  bool activity = false;

  // Αποθορυβοποίηση για το κουμπί αντιστροφής
  if (reading != lastReverseButtonState) {
    reverseButtonLastDebounceTime = millis();
  }

  if ((millis() - reverseButtonLastDebounceTime) > DEBOUNCE_DELAY) {
    if (reading != reverseButtonState) {
      reverseButtonState = reading;
      if (reverseButtonState == LOW) {
        // Εναλλαγή του reverse mode όταν το κουμπί πατηθεί
        reverseMode = !reverseMode;
        Serial.print("Reverse mode is now ");
        Serial.println(reverseMode ? "ON" : "OFF");
        activity = true; // Το κουμπί αντιστροφής πατήθηκε
      }
    }
  }

  lastReverseButtonState = reading;
  return activity;
}
