#ifndef JOYSTICK_CONFIG_H
#define JOYSTICK_CONFIG_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// Μέγιστος αριθμός κουμπιών που μπορεί να υποστηρίξει το gamepad
constexpr int MAX_BUTTONS = 32;

// Μέγιστο μήκος για τα strings
constexpr int MAX_NAME_LENGTH = 20;
constexpr int MAX_CREDENTIALS_LENGTH = 20;

// Δομή για την αποθήκευση πληροφοριών κουμπιού
struct ButtonConfig {
    int pin;                // GPIO pin
    int buttonId;           // Αναγνωριστικό κουμπιού στο BleGamepad
    char label[MAX_NAME_LENGTH]; // Ετικέτα κουμπιού
    bool enabled;           // Ενεργοποιημένο/Απενεργοποιημένο
};

// Δομή για την αποθήκευση των ρυθμίσεων
struct JoystickConfig {
    char deviceName[MAX_NAME_LENGTH];         // Όνομα της συσκευής BLE
    char manufacturer[MAX_NAME_LENGTH];       // Κατασκευαστής
    int batteryLevel;                         // Επίπεδο μπαταρίας (0-100)
    unsigned long sleepTimeout;               // Χρονικό όριο για sleep mode σε ms
    ButtonConfig buttons[MAX_BUTTONS];        // Ρυθμίσεις κουμπιών
    int numButtons;                           // Αριθμός ρυθμισμένων κουμπιών
    char username[MAX_CREDENTIALS_LENGTH];    // Όνομα χρήστη για το web interface
    char password[MAX_CREDENTIALS_LENGTH];    // Κωδικός για το web interface
};

// Εξωτερική μεταβλητή config για πρόσβαση από άλλα αρχεία
extern JoystickConfig config;

// Διαδρομή του αρχείου ρυθμίσεων
const char* const CONFIG_FILE = "/config.json";

// Αρχικοποίηση του συστήματος αρχείων και φόρτωση της διαμόρφωσης
bool initConfig() {
    // Αρχικοποίηση του SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Αποτυχία αρχικοποίησης SPIFFS");
        return false;
    }

    // Έλεγχος αν υπάρχει το αρχείο ρυθμίσεων
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Το αρχείο ρυθμίσεων δεν υπάρχει, δημιουργία προεπιλογών");
        resetToDefaults();
        saveConfig();
        return true;
    }

    // Άνοιγμα του αρχείου για ανάγνωση
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        Serial.println("Αποτυχία ανοίγματος αρχείου ρυθμίσεων");
        return false;
    }

    // Κατανομή μνήμης για το έγγραφο JSON
    StaticJsonDocument<2048> doc;

    // Ανάλυση του JSON
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.print("Αποτυχία ανάλυσης JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // Φόρτωση της διαμόρφωσης από το έγγραφο JSON
    strlcpy(config.deviceName, doc["deviceName"] | "ESP32 BLE Gamepad", MAX_NAME_LENGTH);
    strlcpy(config.manufacturer, doc["manufacturer"] | "ESP32", MAX_NAME_LENGTH);
    config.batteryLevel = doc["batteryLevel"] | 100;
    config.sleepTimeout = doc["sleepTimeout"] | 600000; // Προεπιλογή: 10 λεπτά

    // Φόρτωση των ρυθμίσεων κουμπιών
    config.numButtons = doc["numButtons"] | 0;
    config.numButtons = min(config.numButtons, MAX_BUTTONS);

    for (int i = 0; i < config.numButtons; i++) {
        config.buttons[i].pin = doc["buttons"][i]["pin"] | 0;
        config.buttons[i].buttonId = doc["buttons"][i]["buttonId"] | (i + 1);
        strlcpy(config.buttons[i].label, doc["buttons"][i]["label"] | "", MAX_NAME_LENGTH);
        config.buttons[i].enabled = doc["buttons"][i]["enabled"] | true;
    }

    // Φόρτωση των διαπιστευτηρίων
    strlcpy(config.username, doc["username"] | "admin", MAX_CREDENTIALS_LENGTH);
    strlcpy(config.password, doc["password"] | "admin", MAX_CREDENTIALS_LENGTH);

    Serial.println("Ρυθμίσεις φορτώθηκαν επιτυχώς");
    return true;
}

// Αποθήκευση της τρέχουσας διαμόρφωσης στο αρχείο
bool saveConfig() {
    // Κατανομή μνήμης για το έγγραφο JSON
    StaticJsonDocument<2048> doc;

    // Αποθήκευση της διαμόρφωσης στο έγγραφο JSON
    doc["deviceName"] = config.deviceName;
    doc["manufacturer"] = config.manufacturer;
    doc["batteryLevel"] = config.batteryLevel;
    doc["sleepTimeout"] = config.sleepTimeout;
    doc["numButtons"] = config.numButtons;

    // Αποθήκευση των ρυθμίσεων κουμπιών
    JsonArray buttons = doc.createNestedArray("buttons");
    for (int i = 0; i < config.numButtons; i++) {
        JsonObject button = buttons.createNestedObject();
        button["pin"] = config.buttons[i].pin;
        button["buttonId"] = config.buttons[i].buttonId;
        button["label"] = config.buttons[i].label;
        button["enabled"] = config.buttons[i].enabled;
    }

    // Αποθήκευση των διαπιστευτηρίων
    doc["username"] = config.username;
    doc["password"] = config.password;

    // Άνοιγμα του αρχείου για εγγραφή
    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        Serial.println("Αποτυχία ανοίγματος αρχείου ρυθμίσεων για εγγραφή");
        return false;
    }

    // Εγγραφή του JSON στο αρχείο
    if (serializeJson(doc, file) == 0) {
        Serial.println("Αποτυχία εγγραφής στο αρχείο ρυθμίσεων");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Ρυθμίσεις αποθηκεύτηκαν επιτυχώς");
    return true;
}

// Επαναφορά των προεπιλεγμένων ρυθμίσεων
void resetToDefaults() {
    Serial.println("Επαναφορά προεπιλεγμένων ρυθμίσεων");
    
    // Γενικές ρυθμίσεις
    strlcpy(config.deviceName, "ESP32 BLE Gamepad", MAX_NAME_LENGTH);
    strlcpy(config.manufacturer, "ESP32", MAX_NAME_LENGTH);
    config.batteryLevel = 100;
    config.sleepTimeout = 600000; // 10 λεπτά

    // Προεπιλεγμένη διαμόρφωση κουμπιών
    config.numButtons = 16;

    // Κουμπί Α
    config.buttons[0].pin = 32;
    config.buttons[0].buttonId = 1;
    strlcpy(config.buttons[0].label, "A", MAX_NAME_LENGTH);
    config.buttons[0].enabled = true;

    // Κουμπί B
    config.buttons[1].pin = 33;
    config.buttons[1].buttonId = 2;
    strlcpy(config.buttons[1].label, "B", MAX_NAME_LENGTH);
    config.buttons[1].enabled = true;

    // Κουμπί X
    config.buttons[2].pin = 25;
    config.buttons[2].buttonId = 3;
    strlcpy(config.buttons[2].label, "X", MAX_NAME_LENGTH);
    config.buttons[2].enabled = true;

    // Κουμπί Y
    config.buttons[3].pin = 26;
    config.buttons[3].buttonId = 4;
    strlcpy(config.buttons[3].label, "Y", MAX_NAME_LENGTH);
    config.buttons[3].enabled = true;

    // Κουμπί L1
    config.buttons[4].pin = 27;
    config.buttons[4].buttonId = 5;
    strlcpy(config.buttons[4].label, "L1", MAX_NAME_LENGTH);
    config.buttons[4].enabled = true;

    // Κουμπί R1
    config.buttons[5].pin = 14;
    config.buttons[5].buttonId = 6;
    strlcpy(config.buttons[5].label, "R1", MAX_NAME_LENGTH);
    config.buttons[5].enabled = true;

    // Κουμπί L2
    config.buttons[6].pin = 12;
    config.buttons[6].buttonId = 7;
    strlcpy(config.buttons[6].label, "L2", MAX_NAME_LENGTH);
    config.buttons[6].enabled = true;

    // Κουμπί R2
    config.buttons[7].pin = 13;
    config.buttons[7].buttonId = 8;
    strlcpy(config.buttons[7].label, "R2", MAX_NAME_LENGTH);
    config.buttons[7].enabled = true;

    // Κουμπί Start
    config.buttons[8].pin = 23;
    config.buttons[8].buttonId = 9;
    strlcpy(config.buttons[8].label, "Start", MAX_NAME_LENGTH);
    config.buttons[8].enabled = true;

    // Κουμπί Select
    config.buttons[9].pin = 22;
    config.buttons[9].buttonId = 10;
    strlcpy(config.buttons[9].label, "Select", MAX_NAME_LENGTH);
    config.buttons[9].enabled = true;

    // Κουμπί Left Thumb
    config.buttons[10].pin = 4;
    config.buttons[10].buttonId = 11;
    strlcpy(config.buttons[10].label, "Left Thumb", MAX_NAME_LENGTH);
    config.buttons[10].enabled = true;

    // Κουμπί Right Thumb
    config.buttons[11].pin = 5;
    config.buttons[11].buttonId = 12;
    strlcpy(config.buttons[11].label, "Right Thumb", MAX_NAME_LENGTH);
    config.buttons[11].enabled = true;

    // Κουμπί D-pad Up
    config.buttons[12].pin = 21;
    config.buttons[12].buttonId = 13;
    strlcpy(config.buttons[12].label, "D-pad Up", MAX_NAME_LENGTH);
    config.buttons[12].enabled = true;

    // Κουμπί D-pad Down
    config.buttons[13].pin = 19;
    config.buttons[13].buttonId = 14;
    strlcpy(config.buttons[13].label, "D-pad Down", MAX_NAME_LENGTH);
    config.buttons[13].enabled = true;

    // Κουμπί D-pad Left
    config.buttons[14].pin = 18;
    config.buttons[14].buttonId = 15;
    strlcpy(config.buttons[14].label, "D-pad Left", MAX_NAME_LENGTH);
    config.buttons[14].enabled = true;

    // Κουμπί D-pad Right
    config.buttons[15].pin = 17;
    config.buttons[15].buttonId = 16;
    strlcpy(config.buttons[15].label, "D-pad Right", MAX_NAME_LENGTH);
    config.buttons[15].enabled = true;

    // Διαπιστευτήρια
    strlcpy(config.username, "admin", MAX_CREDENTIALS_LENGTH);
    strlcpy(config.password, "admin", MAX_CREDENTIALS_LENGTH);
}

// Ορισμός της εξωτερικής μεταβλητής config
JoystickConfig config;

#endif // JOYSTICK_CONFIG_H 