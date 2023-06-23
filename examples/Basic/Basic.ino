#include "ESPHelper.h"
#include <ArduinoJson.h>

#define DBG_OUTPUT  (*ESPHelper.dbg_out)
#define content ESPHelper.content

ESPHelper ESPHelper(&Serial);
//ESPHelper ESPHelper;

// Implementation of Station Mode
void stHandler(void) {

  ESPHelper.on("/", HTTP_GET, []() {
    ESPHelper.handleFileRead("/index.html");
  });

  ESPHelper.on("/setting", HTTP_GET, []() {
    ESPHelper.handleFileRead("/setting.html");
  });
}


// Implementation of Access Point Mode
void apHandler(void) {
  ESPHelper.on("/setting", HTTP_GET, []() {
    ESPHelper.handleFileRead("/setting.html");
  });
}

ESPHelper::Config_t config;

String load_json_file(String name) {
  Serial.println("Loading " + name);
  String json;
  File file = SPIFFS.open(name, "r");
  if (file) {
    char temp_json[file.size()];
    for (int i = 0; i < file.size(); i++)
    {
      temp_json[i] = file.read();
    }
    file.close();
    json = temp_json;
  }

  Serial.println(json);
  Serial.println();

  return json;
}


bool load_config() {
  String json = load_json_file("/config.json");
  if (json) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
    } else {
      config.deviceName = (String)doc["deviceName"];
      config.stSSID = (String)doc["stSSID"];
      config.stPASS = (String)doc["stPASS"];
      config.ip = (String)doc["ip"];
      config.gateway = (String)doc["gateway"];
      config.subnet = (String)doc["subnet"];
      config.dns = (String)doc["dns"];
      config.www_username = (String)doc["www_username"];
      config.www_password = (String)doc["www_password"];
      config.apSSID = (String)doc["apSSID"];
      config.apPASS = (String)doc["apPASS"];
      return true;
    }
  }
  return false;
}

void print_config() {
  Serial.println("deviceName:\t" + config.deviceName);
  Serial.println("stSSID:\t\t" + config.stSSID);
  Serial.println("stPASS:\t\t" + config.stPASS);
  Serial.println("ip:\t\t" + config.ip);
  Serial.println("gateway:\t\t" + config.gateway);
  Serial.println("subnet:\t\t" + config.subnet);
  Serial.println("dns:\t\t" + config.dns);
  Serial.println("www_username:\t" + config.www_username);
  Serial.println("www_password:\t" + config.www_password);
  Serial.println("apSSID:\t\t" + config.apSSID);
  Serial.println("apPASS:\t\t" + config.apPASS);
}

void setup() {
  Serial.begin(115200);
  Serial.println();

  ESPHelper.setHandlers(stHandler, apHandler);
  ESPHelper.server.enableCORS(true);
  

  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS Mount failed");
  } else {
    Serial.println("SPIFFS Mount succesfull");
    if (load_config()) {
      Serial.println("Config:\t\tLoaded");
      print_config();
    } else {
      Serial.println("Config:\t\tFailed");
    }
  }

  ESPHelper.setup(config);

}

void loop() {
  ESPHelper.loop();
}
