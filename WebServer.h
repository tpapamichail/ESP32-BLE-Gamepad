#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "JoystickConfig.h"

class JoystickWebServer {
private:
    // Όνομα και κωδικός πρόσβασης του AP
    const char* ssid = "ESP32-BLE-Gamepad";
    const char* password = "esp32gamepad";

    // Διακομιστής ιστού που εκτελείται στη θύρα 80
    AsyncWebServer server;

    // Σημειακό αντικείμενο για τη διαχείριση των συνδέσεων WebSocket
    AsyncWebSocket ws;

    // Μεταβλητή που δείχνει αν ο διακομιστής είναι ενεργός
    bool isActive;

    // Συνάρτηση για τον έλεγχο των διαπιστευτηρίων
    bool checkCredentials(AsyncWebServerRequest *request) {
        if (!request->authenticate(config.username, config.password)) {
            request->requestAuthentication();
            return false;
        }
        return true;
    }

    // Διαχείριση των αιτημάτων API
    void handleApiRequest(AsyncWebServerRequest *request, JsonVariant &json) {
        String requestPath = request->url();

        // Έλεγχος διαπιστευτηρίων για όλα τα API endpoints
        if (!checkCredentials(request)) {
            return;
        }

        if (requestPath == "/api/settings") {
            // Διαχείριση αιτήματος για τις γενικές ρυθμίσεις
            handleSettingsRequest(request, json);
        } else if (requestPath == "/api/buttons") {
            // Διαχείριση αιτήματος για τα κουμπιά
            handleButtonsRequest(request, json);
        } else if (requestPath == "/api/credentials") {
            // Διαχείριση αιτήματος για τα διαπιστευτήρια
            handleCredentialsRequest(request, json);
        } else if (requestPath == "/api/reset") {
            // Διαχείριση αιτήματος για επαναφορά εργοστασιακών ρυθμίσεων
            resetToDefaults();
            saveConfig();
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Οι ρυθμίσεις επαναφέρθηκαν στις προεπιλογές\"}");
        } else {
            // Μη έγκυρο API endpoint
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Άγνωστο API endpoint\"}");
        }
    }

    // Διαχείριση των αιτημάτων για τις γενικές ρυθμίσεις
    void handleSettingsRequest(AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() == HTTP_GET) {
            // Επιστροφή των τρεχουσών ρυθμίσεων
            StaticJsonDocument<512> doc;
            doc["deviceName"] = config.deviceName;
            doc["manufacturer"] = config.manufacturer;
            doc["batteryLevel"] = config.batteryLevel;
            doc["sleepTimeout"] = config.sleepTimeout / 1000 / 60; // Μετατροπή σε λεπτά
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else if (request->method() == HTTP_POST) {
            // Ενημέρωση των ρυθμίσεων
            if (json.is<JsonObject>()) {
                JsonObject obj = json.as<JsonObject>();
                
                if (obj.containsKey("deviceName")) {
                    strlcpy(config.deviceName, obj["deviceName"] | config.deviceName, MAX_NAME_LENGTH);
                }
                
                if (obj.containsKey("manufacturer")) {
                    strlcpy(config.manufacturer, obj["manufacturer"] | config.manufacturer, MAX_NAME_LENGTH);
                }
                
                if (obj.containsKey("batteryLevel")) {
                    config.batteryLevel = constrain(obj["batteryLevel"] | config.batteryLevel, 0, 100);
                }
                
                if (obj.containsKey("sleepTimeout")) {
                    int timeoutMinutes = obj["sleepTimeout"] | (config.sleepTimeout / 1000 / 60);
                    config.sleepTimeout = max(1, timeoutMinutes) * 60 * 1000; // Τουλάχιστον 1 λεπτό, μετατροπή σε ms
                }
                
                // Αποθήκευση των ρυθμίσεων
                saveConfig();
                
                request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Οι ρυθμίσεις ενημερώθηκαν επιτυχώς\"}");
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Μη έγκυρη μορφή JSON\"}");
            }
        } else {
            request->send(405, "application/json", "{\"status\":\"error\",\"message\":\"Η μέθοδος δεν επιτρέπεται\"}");
        }
    }

    // Διαχείριση των αιτημάτων για τα κουμπιά
    void handleButtonsRequest(AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() == HTTP_GET) {
            // Επιστροφή όλων των κουμπιών
            StaticJsonDocument<2048> doc;
            JsonArray buttons = doc.createNestedArray("buttons");
            
            for (int i = 0; i < config.numButtons; i++) {
                JsonObject button = buttons.createNestedObject();
                button["pin"] = config.buttons[i].pin;
                button["buttonId"] = config.buttons[i].buttonId;
                button["label"] = config.buttons[i].label;
                button["enabled"] = config.buttons[i].enabled;
            }
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        } else if (request->method() == HTTP_POST) {
            // Προσθήκη ή ενημέρωση κουμπιού
            if (json.is<JsonObject>()) {
                JsonObject obj = json.as<JsonObject>();
                
                if (obj.containsKey("buttonId") && obj.containsKey("pin")) {
                    int buttonId = obj["buttonId"];
                    int pin = obj["pin"];
                    const char* label = obj["label"] | "";
                    bool enabled = obj["enabled"] | true;
                    
                    // Έλεγχος αν υπάρχει ήδη αυτό το buttonId
                    int existingIndex = -1;
                    for (int i = 0; i < config.numButtons; i++) {
                        if (config.buttons[i].buttonId == buttonId) {
                            existingIndex = i;
                            break;
                        }
                    }
                    
                    if (existingIndex >= 0) {
                        // Ενημέρωση υπάρχοντος κουμπιού
                        config.buttons[existingIndex].pin = pin;
                        strlcpy(config.buttons[existingIndex].label, label, MAX_NAME_LENGTH);
                        config.buttons[existingIndex].enabled = enabled;
                        
                        saveConfig();
                        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Το κουμπί ενημερώθηκε επιτυχώς\"}");
                    } else if (config.numButtons < MAX_BUTTONS) {
                        // Προσθήκη νέου κουμπιού
                        config.buttons[config.numButtons].pin = pin;
                        config.buttons[config.numButtons].buttonId = buttonId;
                        strlcpy(config.buttons[config.numButtons].label, label, MAX_NAME_LENGTH);
                        config.buttons[config.numButtons].enabled = enabled;
                        config.numButtons++;
                        
                        saveConfig();
                        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Το κουμπί προστέθηκε επιτυχώς\"}");
                    } else {
                        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Έχει συμπληρωθεί ο μέγιστος αριθμός κουμπιών\"}");
                    }
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Λείπουν υποχρεωτικά πεδία (buttonId, pin)\"}");
                }
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Μη έγκυρη μορφή JSON\"}");
            }
        } else if (request->method() == HTTP_DELETE) {
            // Διαγραφή κουμπιού
            if (json.is<JsonObject>()) {
                JsonObject obj = json.as<JsonObject>();
                
                if (obj.containsKey("buttonId")) {
                    int buttonId = obj["buttonId"];
                    bool found = false;
                    
                    // Εύρεση και διαγραφή του κουμπιού
                    for (int i = 0; i < config.numButtons; i++) {
                        if (config.buttons[i].buttonId == buttonId) {
                            found = true;
                            
                            // Μετακίνηση όλων των κουμπιών μετά από αυτό μία θέση πίσω
                            for (int j = i; j < config.numButtons - 1; j++) {
                                config.buttons[j] = config.buttons[j + 1];
                            }
                            
                            config.numButtons--;
                            break;
                        }
                    }
                    
                    if (found) {
                        saveConfig();
                        request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Το κουμπί διαγράφηκε επιτυχώς\"}");
                    } else {
                        request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"Το κουμπί δεν βρέθηκε\"}");
                    }
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Λείπει το πεδίο buttonId\"}");
                }
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Μη έγκυρη μορφή JSON\"}");
            }
        } else {
            request->send(405, "application/json", "{\"status\":\"error\",\"message\":\"Η μέθοδος δεν επιτρέπεται\"}");
        }
    }

    // Διαχείριση των αιτημάτων για τα διαπιστευτήρια
    void handleCredentialsRequest(AsyncWebServerRequest *request, JsonVariant &json) {
        if (request->method() == HTTP_POST) {
            // Ενημέρωση των διαπιστευτηρίων
            if (json.is<JsonObject>()) {
                JsonObject obj = json.as<JsonObject>();
                
                if (obj.containsKey("username") && obj.containsKey("password")) {
                    const char* newUsername = obj["username"];
                    const char* newPassword = obj["password"];
                    
                    // Έλεγχος μήκους
                    if (strlen(newUsername) < 3 || strlen(newUsername) > MAX_CREDENTIALS_LENGTH - 1) {
                        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Το όνομα χρήστη πρέπει να έχει μεταξύ 3 και 19 χαρακτήρες\"}");
                        return;
                    }
                    
                    if (strlen(newPassword) < 3 || strlen(newPassword) > MAX_CREDENTIALS_LENGTH - 1) {
                        request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Ο κωδικός πρόσβασης πρέπει να έχει μεταξύ 3 και 19 χαρακτήρες\"}");
                        return;
                    }
                    
                    // Αποθήκευση των νέων διαπιστευτηρίων
                    strlcpy(config.username, newUsername, MAX_CREDENTIALS_LENGTH);
                    strlcpy(config.password, newPassword, MAX_CREDENTIALS_LENGTH);
                    saveConfig();
                    
                    request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"Τα διαπιστευτήρια ενημερώθηκαν επιτυχώς\"}");
                } else {
                    request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Λείπουν υποχρεωτικά πεδία (username, password)\"}");
                }
            } else {
                request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Μη έγκυρη μορφή JSON\"}");
            }
        } else {
            request->send(405, "application/json", "{\"status\":\"error\",\"message\":\"Η μέθοδος δεν επιτρέπεται\"}");
        }
    }

public:
    // Κατασκευαστής
    JoystickWebServer() : server(80), ws("/ws"), isActive(false) {}

    // Έναρξη του web server
    bool begin() {
        // Έλεγχος αν είναι ήδη ενεργός
        if (isActive) {
            return true;
        }

        // Ρύθμιση του ESP32 ως σημείο πρόσβασης (AP)
        if (!WiFi.softAP(ssid, password)) {
            Serial.println("Αποτυχία δημιουργίας σημείου πρόσβασης WiFi");
            return false;
        }

        IPAddress IP = WiFi.softAPIP();
        Serial.print("Διεύθυνση IP του σημείου πρόσβασης: ");
        Serial.println(IP);

        // Έλεγχος αν το SPIFFS είναι διαθέσιμο
        if (!SPIFFS.begin(true)) {
            Serial.println("Αποτυχία αρχικοποίησης SPIFFS, δεν είναι δυνατή η φόρτωση του web interface");
            return false;
        }

        // Προετοιμασία του WebSocket
        ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, 
                      void *arg, uint8_t *data, size_t len) {
            // Διαχείριση γεγονότων WebSocket αν χρειάζεται
        });
        
        server.addHandler(&ws);

        // Χειρισμός αιτημάτων για τα API endpoints
        server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
            if (!checkCredentials(request)) return;
            
            StaticJsonDocument<512> doc;
            doc["deviceName"] = config.deviceName;
            doc["manufacturer"] = config.manufacturer;
            doc["batteryLevel"] = config.batteryLevel;
            doc["sleepTimeout"] = config.sleepTimeout / 1000 / 60; // Μετατροπή σε λεπτά
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });

        // Body parser για τα API endpoints
        AsyncCallbackJsonWebHandler *settingsHandler = new AsyncCallbackJsonWebHandler(
            "/api/settings",
            [this](AsyncWebServerRequest *request, JsonVariant &json) {
                this->handleApiRequest(request, json);
            }
        );
        server.addHandler(settingsHandler);

        AsyncCallbackJsonWebHandler *buttonsHandler = new AsyncCallbackJsonWebHandler(
            "/api/buttons",
            [this](AsyncWebServerRequest *request, JsonVariant &json) {
                this->handleApiRequest(request, json);
            }
        );
        server.addHandler(buttonsHandler);

        AsyncCallbackJsonWebHandler *credentialsHandler = new AsyncCallbackJsonWebHandler(
            "/api/credentials",
            [this](AsyncWebServerRequest *request, JsonVariant &json) {
                this->handleApiRequest(request, json);
            }
        );
        server.addHandler(credentialsHandler);

        AsyncCallbackJsonWebHandler *resetHandler = new AsyncCallbackJsonWebHandler(
            "/api/reset",
            [this](AsyncWebServerRequest *request, JsonVariant &json) {
                this->handleApiRequest(request, json);
            }
        );
        server.addHandler(resetHandler);

        // Αίτημα GET για τα κουμπιά
        server.on("/api/buttons", HTTP_GET, [this](AsyncWebServerRequest *request) {
            if (!checkCredentials(request)) return;
            
            StaticJsonDocument<2048> doc;
            JsonArray buttons = doc.createNestedArray("buttons");
            
            for (int i = 0; i < config.numButtons; i++) {
                JsonObject button = buttons.createNestedObject();
                button["pin"] = config.buttons[i].pin;
                button["buttonId"] = config.buttons[i].buttonId;
                button["label"] = config.buttons[i].label;
                button["enabled"] = config.buttons[i].enabled;
            }
            
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });

        // Χειρισμός στατικών αρχείων από το SPIFFS
        server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");

        // Χειρισμός μη υπαρχόντων URL
        server.onNotFound([](AsyncWebServerRequest *request) {
            request->send(404, "text/plain", "Η σελίδα δεν βρέθηκε");
        });

        // Ξεκίνημα του διακομιστή
        server.begin();
        isActive = true;
        Serial.println("Ο web server ξεκίνησε");
        return true;
    }

    // Τερματισμός του web server
    void end() {
        if (!isActive) {
            return;
        }

        // Τερματισμός του WebSocket
        ws.closeAll();
        
        // Τερματισμός του διακομιστή
        server.end();
        
        // Απενεργοποίηση του AP
        WiFi.softAPdisconnect(true);
        
        isActive = false;
        Serial.println("Ο web server τερματίστηκε");
    }

    // Έλεγχος αν ο web server είναι ενεργός
    bool isRunning() const {
        return isActive;
    }
};

#endif // WEB_SERVER_H 