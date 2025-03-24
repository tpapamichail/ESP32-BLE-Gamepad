#include "JoystickConfig.h"

// Δήλωση της καθολικής μεταβλητής config
JoystickConfig config;

// Όνομα αρχείου για την αποθήκευση των ρυθμίσεων
const char* CONFIG_FILE = "/joystick_config.json";

// Αρχικοποίηση και φόρτωση των ρυθμίσεων από το SPIFFS
bool initConfig() {
    // Εκκίνηση του συστήματος αρχείων SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Αποτυχία εκκίνησης SPIFFS");
        return false;
    }

    // Έλεγχος για το αν υπάρχει το αρχείο ρυθμίσεων
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Δεν βρέθηκε αρχείο ρυθμίσεων, δημιουργία προεπιλεγμένων ρυθμίσεων");
        resetToDefaults();
        return saveConfig();
    }

    // Άνοιγμα του αρχείου για ανάγνωση
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Αποτυχία ανοίγματος αρχείου ρυθμίσεων");
        return false;
    }

    // Δέσμευση μνήμης για το JSON document
    DynamicJsonDocument doc(4096);

    // Ανάλυση του JSON
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("Αποτυχία ανάλυσης JSON: ");
        Serial.println(error.c_str());
        resetToDefaults();
        return false;
    }

    // Φόρτωση των τιμών στη δομή config
    config.numButtons = doc["numButtons"] | 0;
    
    strlcpy(config.username, doc["username"] | DEFAULT_USERNAME, sizeof(config.username));
    strlcpy(config.password, doc["password"] | DEFAULT_PASSWORD, sizeof(config.password));
    strlcpy(config.deviceName, doc["deviceName"] | "ESP32 Gamepad", sizeof(config.deviceName));
    strlcpy(config.manufacturer, doc["manufacturer"] | "Espressif", sizeof(config.manufacturer));
    
    config.batteryLevel = doc["batteryLevel"] | 100;
    config.sleepTimeout = doc["sleepTimeout"] | 600000; // 10 λεπτά προεπιλογή

    // Φόρτωση των κουμπιών
    JsonArray buttonsArray = doc["buttons"];
    int index = 0;
    
    for (JsonObject button : buttonsArray) {
        if (index < MAX_BUTTONS) {
            config.buttons[index].pin = button["pin"];
            config.buttons[index].buttonId = button["buttonId"];
            config.buttons[index].enabled = button["enabled"] | true;
            strlcpy(config.buttons[index].label, button["label"] | "", sizeof(config.buttons[index].label));
            index++;
        }
    }
    
    config.numButtons = index;
    
    Serial.println("Επιτυχής φόρτωση ρυθμίσεων");
    return true;
}

// Αποθήκευση των ρυθμίσεων στο SPIFFS
bool saveConfig() {
    // Δέσμευση μνήμης για το JSON document
    DynamicJsonDocument doc(4096);

    // Αποθήκευση των βασικών ρυθμίσεων
    doc["numButtons"] = config.numButtons;
    doc["username"] = config.username;
    doc["password"] = config.password;
    doc["deviceName"] = config.deviceName;
    doc["manufacturer"] = config.manufacturer;
    doc["batteryLevel"] = config.batteryLevel;
    doc["sleepTimeout"] = config.sleepTimeout;

    // Δημιουργία πίνακα για τα κουμπιά
    JsonArray buttonsArray = doc.createNestedArray("buttons");
    
    for (int i = 0; i < config.numButtons; i++) {
        JsonObject button = buttonsArray.createNestedObject();
        button["pin"] = config.buttons[i].pin;
        button["buttonId"] = config.buttons[i].buttonId;
        button["enabled"] = config.buttons[i].enabled;
        button["label"] = config.buttons[i].label;
    }

    // Άνοιγμα του αρχείου για εγγραφή
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Αποτυχία ανοίγματος αρχείου για εγγραφή");
        return false;
    }

    // Εγγραφή του JSON στο αρχείο
    if (serializeJson(doc, file) == 0) {
        Serial.println("Αποτυχία εγγραφής στο αρχείο");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Επιτυχής αποθήκευση ρυθμίσεων");
    return true;
}

// Επαναφορά προεπιλεγμένων ρυθμίσεων
void resetToDefaults() {
    // Καθαρισμός της δομής
    memset(&config, 0, sizeof(config));
    
    // Ορισμός προεπιλεγμένων τιμών
    strlcpy(config.username, DEFAULT_USERNAME, sizeof(config.username));
    strlcpy(config.password, DEFAULT_PASSWORD, sizeof(config.password));
    strlcpy(config.deviceName, "ESP32 Gamepad", sizeof(config.deviceName));
    strlcpy(config.manufacturer, "Espressif", sizeof(config.manufacturer));
    config.batteryLevel = 100;
    config.sleepTimeout = 600000; // 10 λεπτά
    
    // Προεπιλεγμένα κουμπιά (από το αρχικό joystick.ino)
    config.numButtons = 16;

    // BUTTON_A_PIN = 32
    config.buttons[0].pin = 32;
    config.buttons[0].buttonId = 1; // BUTTON_1
    config.buttons[0].enabled = true;
    strlcpy(config.buttons[0].label, "A", sizeof(config.buttons[0].label));

    // BUTTON_B_PIN = 33
    config.buttons[1].pin = 33;
    config.buttons[1].buttonId = 2; // BUTTON_2
    config.buttons[1].enabled = true;
    strlcpy(config.buttons[1].label, "B", sizeof(config.buttons[1].label));

    // BUTTON_X_PIN = 25
    config.buttons[2].pin = 25;
    config.buttons[2].buttonId = 3; // BUTTON_3
    config.buttons[2].enabled = true;
    strlcpy(config.buttons[2].label, "X", sizeof(config.buttons[2].label));

    // BUTTON_Y_PIN = 26
    config.buttons[3].pin = 26;
    config.buttons[3].buttonId = 4; // BUTTON_4
    config.buttons[3].enabled = true;
    strlcpy(config.buttons[3].label, "Y", sizeof(config.buttons[3].label));

    // BUTTON_L1_PIN = 27
    config.buttons[4].pin = 27;
    config.buttons[4].buttonId = 5; // BUTTON_5
    config.buttons[4].enabled = true;
    strlcpy(config.buttons[4].label, "L1", sizeof(config.buttons[4].label));

    // BUTTON_R1_PIN = 14
    config.buttons[5].pin = 14;
    config.buttons[5].buttonId = 6; // BUTTON_6
    config.buttons[5].enabled = true;
    strlcpy(config.buttons[5].label, "R1", sizeof(config.buttons[5].label));

    // BUTTON_L2_PIN = 12
    config.buttons[6].pin = 12;
    config.buttons[6].buttonId = 7; // BUTTON_7
    config.buttons[6].enabled = true;
    strlcpy(config.buttons[6].label, "L2", sizeof(config.buttons[6].label));

    // BUTTON_R2_PIN = 13
    config.buttons[7].pin = 13;
    config.buttons[7].buttonId = 8; // BUTTON_8
    config.buttons[7].enabled = true;
    strlcpy(config.buttons[7].label, "R2", sizeof(config.buttons[7].label));

    // BUTTON_START_PIN = 23
    config.buttons[8].pin = 23;
    config.buttons[8].buttonId = 9; // BUTTON_9
    config.buttons[8].enabled = true;
    strlcpy(config.buttons[8].label, "START", sizeof(config.buttons[8].label));

    // BUTTON_SELECT_PIN = 22
    config.buttons[9].pin = 22;
    config.buttons[9].buttonId = 10; // BUTTON_10
    config.buttons[9].enabled = true;
    strlcpy(config.buttons[9].label, "SELECT", sizeof(config.buttons[9].label));

    // BUTTON_THUMB_LEFT_PIN = 4
    config.buttons[10].pin = 4;
    config.buttons[10].buttonId = 11; // BUTTON_11
    config.buttons[10].enabled = true;
    strlcpy(config.buttons[10].label, "THUMB_L", sizeof(config.buttons[10].label));

    // BUTTON_THUMB_RIGHT_PIN = 5
    config.buttons[11].pin = 5;
    config.buttons[11].buttonId = 12; // BUTTON_12
    config.buttons[11].enabled = true;
    strlcpy(config.buttons[11].label, "THUMB_R", sizeof(config.buttons[11].label));

    // BUTTON_DPAD_UP_PIN = 21
    config.buttons[12].pin = 21;
    config.buttons[12].buttonId = 13; // BUTTON_13
    config.buttons[12].enabled = true;
    strlcpy(config.buttons[12].label, "DPAD_UP", sizeof(config.buttons[12].label));

    // BUTTON_DPAD_DOWN_PIN = 19
    config.buttons[13].pin = 19;
    config.buttons[13].buttonId = 14; // BUTTON_14
    config.buttons[13].enabled = true;
    strlcpy(config.buttons[13].label, "DPAD_DOWN", sizeof(config.buttons[13].label));

    // BUTTON_DPAD_LEFT_PIN = 18
    config.buttons[14].pin = 18;
    config.buttons[14].buttonId = 15; // BUTTON_15
    config.buttons[14].enabled = true;
    strlcpy(config.buttons[14].label, "DPAD_LEFT", sizeof(config.buttons[14].label));

    // BUTTON_DPAD_RIGHT_PIN = 17
    config.buttons[15].pin = 17;
    config.buttons[15].buttonId = 16; // BUTTON_16
    config.buttons[15].enabled = true;
    strlcpy(config.buttons[15].label, "DPAD_RIGHT", sizeof(config.buttons[15].label));

    Serial.println("Επαναφορά προεπιλεγμένων ρυθμίσεων");
}

// Προσθήκη νέου κουμπιού
bool addButton(int pin, int buttonId, const char* label) {
    if (config.numButtons >= MAX_BUTTONS) {
        Serial.println("Έχετε φτάσει τον μέγιστο αριθμό κουμπιών");
        return false;
    }

    // Έλεγχος για το αν υπάρχει ήδη κουμπί με το ίδιο ID ή pin
    for (int i = 0; i < config.numButtons; i++) {
        if (config.buttons[i].buttonId == buttonId) {
            Serial.println("Υπάρχει ήδη κουμπί με αυτό το ID");
            return false;
        }
        
        if (config.buttons[i].pin == pin) {
            Serial.println("Υπάρχει ήδη κουμπί σε αυτό το PIN");
            return false;
        }
    }

    // Προσθήκη του νέου κουμπιού
    int index = config.numButtons;
    config.buttons[index].pin = pin;
    config.buttons[index].buttonId = buttonId;
    config.buttons[index].enabled = true;
    strlcpy(config.buttons[index].label, label, sizeof(config.buttons[index].label));
    
    config.numButtons++;
    
    // Αποθήκευση των αλλαγών
    return saveConfig();
}

// Αφαίρεση κουμπιού
bool removeButton(int buttonId) {
    int index = -1;
    
    // Εύρεση του κουμπιού με το συγκεκριμένο ID
    for (int i = 0; i < config.numButtons; i++) {
        if (config.buttons[i].buttonId == buttonId) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        Serial.println("Δεν βρέθηκε κουμπί με αυτό το ID");
        return false;
    }
    
    // Μετακίνηση όλων των κουμπιών μετά από αυτό το κουμπί κατά μία θέση πίσω
    for (int i = index; i < config.numButtons - 1; i++) {
        config.buttons[i] = config.buttons[i + 1];
    }
    
    config.numButtons--;
    
    // Αποθήκευση των αλλαγών
    return saveConfig();
}

// Ενημέρωση ρυθμίσεων κουμπιού
bool updateButton(int buttonId, int pin, const char* label, bool enabled) {
    int index = -1;
    
    // Εύρεση του κουμπιού με το συγκεκριμένο ID
    for (int i = 0; i < config.numButtons; i++) {
        if (config.buttons[i].buttonId == buttonId) {
            index = i;
            break;
        }
    }
    
    if (index == -1) {
        Serial.println("Δεν βρέθηκε κουμπί με αυτό το ID");
        return false;
    }
    
    // Έλεγχος για το αν υπάρχει άλλο κουμπί με το ίδιο PIN
    for (int i = 0; i < config.numButtons; i++) {
        if (i != index && config.buttons[i].pin == pin) {
            Serial.println("Υπάρχει ήδη άλλο κουμπί σε αυτό το PIN");
            return false;
        }
    }
    
    // Ενημέρωση των ρυθμίσεων του κουμπιού
    config.buttons[index].pin = pin;
    config.buttons[index].enabled = enabled;
    strlcpy(config.buttons[index].label, label, sizeof(config.buttons[index].label));
    
    // Αποθήκευση των αλλαγών
    return saveConfig();
} 