#include "WebServer.h"
#include <ArduinoJson.h>

// Constructor
JoystickWebServer::JoystickWebServer()
    : server(80), wifiAPStarted(false) {
}

// Εκκίνηση του web server
bool JoystickWebServer::begin(const String& ssid, const String& password) {
    apSSID = ssid;
    apPassword = password;
    
    // Διαμόρφωση του WiFi σε λειτουργία Access Point
    WiFi.mode(WIFI_AP);
    
    // Εκκίνηση του access point
    if (password.length() >= 8) {
        wifiAPStarted = WiFi.softAP(ssid.c_str(), password.c_str());
    } else {
        wifiAPStarted = WiFi.softAP(ssid.c_str());
    }
    
    if (!wifiAPStarted) {
        Serial.println("Αποτυχία εκκίνησης του WiFi AP");
        return false;
    }
    
    // Ορισμός στατικής IP διεύθυνσης για το access point
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Διεύθυνση IP του access point: ");
    Serial.println(IP);
    
    // Ρύθμιση των διαδρομών του server
    setupRoutes();
    
    // Εκκίνηση του web server
    server.begin();
    Serial.println("Ο Web Server ξεκίνησε");
    
    return true;
}

// Τερματισμός του web server
void JoystickWebServer::end() {
    server.end();
    if (wifiAPStarted) {
        WiFi.softAPdisconnect(true);
        wifiAPStarted = false;
    }
    Serial.println("Ο Web Server τερματίστηκε");
}

// Έλεγχος αν το web server είναι ενεργό
bool JoystickWebServer::isRunning() {
    return wifiAPStarted;
}

// Μέθοδος αυθεντικοποίησης για προστασία των σελίδων
bool JoystickWebServer::authenticate(AsyncWebServerRequest *request) {
    if (!request->authenticate(config.username, config.password)) {
        request->requestAuthentication();
        return false;
    }
    return true;
}

// Δημιουργία των διαδρομών (routes) του server
void JoystickWebServer::setupRoutes() {
    // Σερβίρισμα της κεντρικής σελίδας (index.html)
    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        request->send(SPIFFS, "/www/index.html", "text/html");
    });
    
    // Σερβίρισμα των σελίδων ρυθμίσεων
    server.on("/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        request->send(SPIFFS, "/www/settings.html", "text/html");
    });
    
    server.on("/buttons", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        request->send(SPIFFS, "/www/buttons.html", "text/html");
    });
    
    server.on("/credentials", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        request->send(SPIFFS, "/www/credentials.html", "text/html");
    });
    
    // Σερβίρισμα των static αρχείων (CSS, JavaScript)
    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/styles.css", "text/css");
    });
    
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/www/script.js", "application/javascript");
    });
    
    // API endpoints για την επικοινωνία με το frontend
    server.on("/api/buttons", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiButtonsRequest(request);
    });
    
    server.on("/api/buttons/add", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiAddButton(request);
    });
    
    server.on("/api/buttons/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiUpdateButton(request);
    });
    
    server.on("/api/buttons/delete", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiDeleteButton(request);
    });
    
    server.on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiGeneralSettings(request);
    });
    
    server.on("/api/settings/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiGeneralSettings(request);
    });
    
    server.on("/api/credentials/update", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiCredentials(request);
    });
    
    server.on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (!authenticate(request)) return;
        handleApiReset(request);
    });
    
    // Χειρισμός αιτημάτων για μη υπάρχουσες διαδρομές
    server.onNotFound([](AsyncWebServerRequest *request) {
        request->send(404, "text/plain", "Not found");
    });
}

// Διαχείριση των αιτημάτων API για όλα τα κουμπιά
void JoystickWebServer::handleApiButtonsRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    
    DynamicJsonDocument doc(4096);
    JsonArray buttonsArray = doc.createNestedArray("buttons");
    
    for (int i = 0; i < config.numButtons; i++) {
        JsonObject button = buttonsArray.createNestedObject();
        button["pin"] = config.buttons[i].pin;
        button["buttonId"] = config.buttons[i].buttonId;
        button["enabled"] = config.buttons[i].enabled;
        button["label"] = config.buttons[i].label;
    }
    
    serializeJson(doc, *response);
    request->send(response);
}

// Διαχείριση των αιτημάτων API για προσθήκη κουμπιού
void JoystickWebServer::handleApiAddButton(AsyncWebServerRequest *request) {
    if (!request->hasParam("pin", true) || !request->hasParam("buttonId", true) || !request->hasParam("label", true)) {
        request->send(400, "text/plain", "Missing parameters");
        return;
    }
    
    int pin = request->getParam("pin", true)->value().toInt();
    int buttonId = request->getParam("buttonId", true)->value().toInt();
    String label = request->getParam("label", true)->value();
    
    if (addButton(pin, buttonId, label.c_str())) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Failed to add button\"}");
    }
}

// Διαχείριση των αιτημάτων API για ενημέρωση κουμπιού
void JoystickWebServer::handleApiUpdateButton(AsyncWebServerRequest *request) {
    if (!request->hasParam("buttonId", true) || !request->hasParam("pin", true) || 
        !request->hasParam("label", true) || !request->hasParam("enabled", true)) {
        request->send(400, "text/plain", "Missing parameters");
        return;
    }
    
    int buttonId = request->getParam("buttonId", true)->value().toInt();
    int pin = request->getParam("pin", true)->value().toInt();
    String label = request->getParam("label", true)->value();
    bool enabled = request->getParam("enabled", true)->value() == "true";
    
    if (updateButton(buttonId, pin, label.c_str(), enabled)) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Failed to update button\"}");
    }
}

// Διαχείριση των αιτημάτων API για διαγραφή κουμπιού
void JoystickWebServer::handleApiDeleteButton(AsyncWebServerRequest *request) {
    if (!request->hasParam("buttonId", true)) {
        request->send(400, "text/plain", "Missing buttonId parameter");
        return;
    }
    
    int buttonId = request->getParam("buttonId", true)->value().toInt();
    
    if (removeButton(buttonId)) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Failed to delete button\"}");
    }
}

// Διαχείριση των αιτημάτων API για τις γενικές ρυθμίσεις
void JoystickWebServer::handleApiGeneralSettings(AsyncWebServerRequest *request) {
    // Για αιτήματα GET, επιστρέφουμε τις τρέχουσες ρυθμίσεις
    if (request->method() == HTTP_GET) {
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        
        DynamicJsonDocument doc(1024);
        doc["deviceName"] = config.deviceName;
        doc["manufacturer"] = config.manufacturer;
        doc["batteryLevel"] = config.batteryLevel;
        doc["sleepTimeout"] = config.sleepTimeout;
        
        serializeJson(doc, *response);
        request->send(response);
        return;
    }
    
    // Για αιτήματα POST, ενημερώνουμε τις ρυθμίσεις
    if (!request->hasParam("deviceName", true) || !request->hasParam("manufacturer", true) || 
        !request->hasParam("batteryLevel", true) || !request->hasParam("sleepTimeout", true)) {
        request->send(400, "text/plain", "Missing parameters");
        return;
    }
    
    String deviceName = request->getParam("deviceName", true)->value();
    String manufacturer = request->getParam("manufacturer", true)->value();
    int batteryLevel = request->getParam("batteryLevel", true)->value().toInt();
    unsigned long sleepTimeout = request->getParam("sleepTimeout", true)->value().toInt();
    
    // Ενημέρωση των ρυθμίσεων
    strlcpy(config.deviceName, deviceName.c_str(), sizeof(config.deviceName));
    strlcpy(config.manufacturer, manufacturer.c_str(), sizeof(config.manufacturer));
    config.batteryLevel = batteryLevel;
    config.sleepTimeout = sleepTimeout;
    
    // Αποθήκευση των ρυθμίσεων
    if (saveConfig()) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save settings\"}");
    }
}

// Διαχείριση των αιτημάτων API για τα credentials
void JoystickWebServer::handleApiCredentials(AsyncWebServerRequest *request) {
    if (!request->hasParam("username", true) || !request->hasParam("password", true) || !request->hasParam("confirmPassword", true)) {
        request->send(400, "text/plain", "Missing parameters");
        return;
    }
    
    String username = request->getParam("username", true)->value();
    String password = request->getParam("password", true)->value();
    String confirmPassword = request->getParam("confirmPassword", true)->value();
    
    // Έλεγχος αν τα passwords ταιριάζουν
    if (password != confirmPassword) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Passwords do not match\"}");
        return;
    }
    
    // Έλεγχος αν τα credentials είναι έγκυρα
    if (username.length() < 3 || password.length() < 3) {
        request->send(400, "application/json", "{\"success\":false,\"error\":\"Username and password must be at least 3 characters long\"}");
        return;
    }
    
    // Ενημέρωση των credentials
    strlcpy(config.username, username.c_str(), sizeof(config.username));
    strlcpy(config.password, password.c_str(), sizeof(config.password));
    
    // Αποθήκευση των ρυθμίσεων
    if (saveConfig()) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to save credentials\"}");
    }
}

// Διαχείριση των αιτημάτων API για επαναφορά εργοστασιακών ρυθμίσεων
void JoystickWebServer::handleApiReset(AsyncWebServerRequest *request) {
    resetToDefaults();
    
    if (saveConfig()) {
        request->send(200, "application/json", "{\"success\":true}");
    } else {
        request->send(500, "application/json", "{\"success\":false,\"error\":\"Failed to reset to defaults\"}");
    }
} 