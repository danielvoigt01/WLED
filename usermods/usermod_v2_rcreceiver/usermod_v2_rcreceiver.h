#pragma once

#include "wled.h"
#include <RCSwitch.h> // Assuming you're using RCSwitch library

// class name. Use something descriptive and leave the ": public Usermod" part :)
class UsermodRCReceiver : public Usermod
{

private:
  // Private class members. You can declare variables and functions only accessible to your usermod here
  bool enabled = true;
  bool initDone = false;
  unsigned long lastTime = 0;
  String MqttRCTopic = "/receiver/value";
  RCSwitch RCReceiver = RCSwitch();

  int8_t rcPins[1];

  static const char _name[];
  static const char _enabled[];

  // any private methods should go here (non-inline method should be defined out of class)
  void publishMqtt(const char *state, bool retain = false); // example for publishing MQTT message

public:
  inline void enable(bool enable) { enabled = enable; }

  inline bool isEnabled() { return enabled; }

  void setup() override
  {
    RCReceiver.enableReceive(rcPins[0]);
    initDone = true;
  }

  void connected() override
  {
    // Serial.println("Connected to WiFi!");
  }

  void loop() override
  {
    if (!enabled)
    {
      return;
    }

    if (RCReceiver.available())
    {
      unsigned long datevalue = RCReceiver.getReceivedValue();
      char msg[20]; // Buffer for the message
      snprintf(msg, sizeof(msg), "%lu", datevalue);
      publishMqtt(msg);
      RCReceiver.resetAvailable();
    }
  }

  void addToJsonInfo(JsonObject &root) override
  {
    // if "u" object does not exist yet wee need to create it
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");
  }

  void addToConfig(JsonObject &root) override
  {
    JsonObject top = root.createNestedObject(FPSTR(_name));
    top[FPSTR(_enabled)] = enabled;
    top["MqttRCTopic"] = MqttRCTopic;
    // save these vars persistently whenever settings are saved
    JsonArray pinArray = top.createNestedArray("pin");
    pinArray.add(rcPins[0]);
  }

  bool readFromConfig(JsonObject &root) override
  {
    JsonObject top = root[FPSTR(_name)];

    bool configComplete = !top.isNull();
    // "pin" fields have special handling in settings page (or some_pin as well)
    configComplete &= getJsonValue(top["MqttRCTopic"], MqttRCTopic);
    configComplete &= getJsonValue(top["pin"][0], rcPins[0], -1);

    return configComplete;
  }

  void appendConfigData() override
  {
    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_name)).c_str());
    oappend(SET_F(":MqttRCTopic"));
    oappend(SET_F("',1,'Sends received value to Topic');"));
  }

#ifndef WLED_DISABLE_MQTT
  void onMqttConnect(bool sessionPresent) override
  {
    publishMqtt("online");
  }
#endif

  uint16_t getId() override
  {
    return USERMOD_ID_RCRECEIVER;
  }
};

const char UsermodRCReceiver::_name[] PROGMEM = "RCReceiver";
const char UsermodRCReceiver::_enabled[] PROGMEM = "enabled";

void UsermodRCReceiver::publishMqtt(const char *state, bool retain)
{
#ifndef WLED_DISABLE_MQTT
  // Check if MQTT Connected, otherwise it will crash the 8266
  if (WLED_MQTT_CONNECTED)
  {
    char subuf[64];

    if (MqttRCTopic[0] == '/')
    {
      // Wenn an erster Stelle in MqttRCTopic ein "/" ist
      strcpy(subuf, mqttDeviceTopic);
      strncat(subuf, MqttRCTopic.c_str(), sizeof(subuf) - strlen(subuf) - 1);
    }
    else
    {
      // Ansonsten den kompletten String aus MqttRCTopic nehmen
      strncpy(subuf, MqttRCTopic.c_str(), sizeof(subuf) - 1);
      subuf[sizeof(subuf) - 1] = '\0'; // Nullterminierung sicherstellen
    }
    Serial.println(subuf);
    // Senden
    mqtt->publish(subuf, 0, retain, state);
  }
#endif
}
