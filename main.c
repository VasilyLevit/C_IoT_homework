#include <pgmspace.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

const char *const ssidAP PROGMEM = "ESP_Relay";
const char *const passwordAP PROGMEM = "password";

const byte pinBuiltinLed = 2; // Светодиод для отображения статуса работы контроллера

// Блок для работы с EEPROM
const char configSign[4] PROGMEM = {'#', 'R', 'E', 'L'};
const byte maxStrParamLength = 32;
const char *const ssidArg PROGMEM = "ssid";
const char *const passwordArg PROGMEM = "pass";
const char *const domainArg PROGMEM = "domain";
const char *const serverArg PROGMEM = "server";
const char *const portArg PROGMEM = "port";
const char *const userArg PROGMEM = "user";
const char *const mqttpswdArg PROGMEM = "mqttpswd";
const char *const clientArg PROGMEM = "client";
const char *const topicArg PROGMEM = "topic";
const char *const gpioArg PROGMEM = "gpio";
const char *const levelArg PROGMEM = "level";
const char *const onbootArg PROGMEM = "onboot";
const char *const rebootArg PROGMEM = "reboot";
const char *const tempArg PROGMEM = "temp";
const char *const wifimodeArg PROGMEM = "wifimode";
const char *const mqttconnectedArg PROGMEM = "mqttconnected";

// Блок датчика температуры
const int gpioDHT22 = D7;
DHT dht(gpioDHT22, DHT22);
float Temperature;
float Humidity;
const bool mqttSensorRetained = false;

// Блок сервера, wi-fi и mqtt
String ssid, password, domain;
String mqttServer, mqttUser, mqttPassword, mqttClient = "ESP_Relay", mqttTopic = "/Relay";
uint16_t mqttPort = 1883;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient espClient;
PubSubClient pubsubClient(espClient);

/*
 * Функции для работы с EEPROM
 */

uint16_t readEEPROMString(uint16_t offset, String &str);

uint16_t writeEEPROMString(uint16_t offset, const String &str);

bool readConfig();

void writeConfig();

/*
 * Функции для работы с WiFi
 */

bool setupWiFiAsStation();

void setupWiFiAsAP();

void setupWiFi();

/*
 * Функции для работы с HTTP
 */

String quoteEscape(const String &str);

void handleRoot();

void handleWiFiConfig();

void handleMQTTConfig();

void handleStoreConfig();

void handleReboot();

void handleData();

/* Функции для работы с MQTT */

bool mqttReconnect();
bool mqtt_subscribe(PubSubClient &client, const String &topic);
void readSendTemperature(const String &topic);

void setup()
{
  Serial.begin(115200);
  Serial.println();
  pinMode(pinBuiltinLed, OUTPUT);

  EEPROM.begin(1024);
  if (!readConfig())
    Serial.println(F("EEPROM is empty!"));

  setupWiFi();
  dht.begin();

  httpUpdater.setup(&httpServer);
  httpServer.onNotFound([]()
                        { httpServer.send(404, F("text/plain"), F("FileNotFound")); });
  httpServer.on("/", handleRoot);
  httpServer.on("/index.html", handleRoot);
  httpServer.on("/wifi", handleWiFiConfig);
  httpServer.on("/mqtt", handleMQTTConfig);
  httpServer.on("/store", handleStoreConfig);
  httpServer.on("/reboot", handleReboot);
  httpServer.on("/data", handleData);

  if (mqttServer.length())
  {
    pubsubClient.setServer(mqttServer.c_str(), mqttPort);
  }
}


void loop()
{
  if ((WiFi.getMode() == WIFI_STA) && (WiFi.status() != WL_CONNECTED))
  {
    setupWiFi();
  }

  httpServer.handleClient();

  if (mqttServer.length() && (WiFi.getMode() == WIFI_STA))
  {
    if (!pubsubClient.connected())
      mqttReconnect();
    if (pubsubClient.connected())
      pubsubClient.loop();
    static unsigned long lastTempRead = 0;
    if ((millis() - lastTempRead) >= 30000)
    {
      lastTempRead = millis();
      readSendTemperature(mqttTopic);
    }
  }

  delay(1);
}

// Чтение заначения из EEPROM
uint16_t readEEPROMString(uint16_t offset, String &str)
{
  char buffer[maxStrParamLength + 1];

  buffer[maxStrParamLength] = 0;
  for (byte i = 0; i < maxStrParamLength; i++)
  {
    if (!(buffer[i] = EEPROM.read(offset + i)))
      break;
  }
  str = String(buffer);

  return offset + maxStrParamLength;
}

// Запись заначения в EEPROM
uint16_t writeEEPROMString(uint16_t offset, const String &str)
{
  for (byte i = 0; i < maxStrParamLength; i++)
  {
    if (i < str.length())
      EEPROM.write(offset + i, str[i]);
    else
      EEPROM.write(offset + i, 0);
  }

  return offset + maxStrParamLength;
}

// Чтение всех параметров из EEPROM
bool readConfig()
{
  uint16_t offset = 0;

  Serial.println(F("Reading config from EEPROM"));
  for (byte i = 0; i < sizeof(configSign); i++)
  {
    char c = pgm_read_byte(configSign + i);
    if (EEPROM.read(offset + i) != c)
      return false;
  }
  offset += sizeof(configSign);
  offset = readEEPROMString(offset, ssid);
  offset = readEEPROMString(offset, password);
  offset = readEEPROMString(offset, domain);
  offset = readEEPROMString(offset, mqttServer);
  EEPROM.get(offset, mqttPort);
  offset += sizeof(mqttPort);
  offset = readEEPROMString(offset, mqttUser);
  offset = readEEPROMString(offset, mqttPassword);
  offset = readEEPROMString(offset, mqttClient);
  offset = readEEPROMString(offset, mqttTopic);

  return true;
}

// Перезапись параметров в EEPROM
void writeConfig()
{
  uint16_t offset = 0;

  Serial.println(F("Writing config to EEPROM"));
  for (byte i = 0; i < sizeof(configSign); i++)
  {
    char c = pgm_read_byte(configSign + i);
    EEPROM.write(offset + i, c);
  }
  offset += sizeof(configSign);
  offset = writeEEPROMString(offset, ssid);
  offset = writeEEPROMString(offset, password);
  offset = writeEEPROMString(offset, domain);
  offset = writeEEPROMString(offset, mqttServer);
  EEPROM.put(offset, mqttPort);
  offset += sizeof(mqttPort);
  offset = writeEEPROMString(offset, mqttUser);
  offset = writeEEPROMString(offset, mqttPassword);
  offset = writeEEPROMString(offset, mqttClient);
  offset = writeEEPROMString(offset, mqttTopic);
  EEPROM.commit();
}

// Подключение к сети WiFi
bool setupWiFiAsStation()
{
  const uint32_t timeout = 60000;
  uint32_t maxtime = millis() + timeout;

  Serial.print(F("Connecting to "));
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(pinBuiltinLed, LOW);
    delay(500);
    digitalWrite(pinBuiltinLed, HIGH);
    Serial.print(".");
    if (millis() >= maxtime)
    {
      Serial.println(F(" fail!"));

      return false;
    }
  }
  Serial.println();
  Serial.println(F("WiFi connected"));
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  return true;
}

// Создание точки доступа WiFi
void setupWiFiAsAP()
{
  Serial.print(F("Configuring access point "));
  Serial.println(ssidAP);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssidAP, passwordAP);

  Serial.print(F("IP address: "));
  Serial.println(WiFi.softAPIP());
}

// Выбор режима работы с WiFi клиент или точка доступа
void setupWiFi()
{
  if ((!ssid.length()) || (!setupWiFiAsStation()))
    setupWiFiAsAP();

  if (domain.length())
  {
    if (MDNS.begin(domain.c_str()))
    {
      MDNS.addService("http", "tcp", 80);
      Serial.println(F("mDNS responder started"));
    }
    else
    {
      Serial.println(F("Error setting up mDNS responder!"));
    }
  }

  httpServer.begin();
  Serial.println(F("HTTP server started (use '/update' url to OTA update)"));
}

// Парсинг данных
String quoteEscape(const String &str)
{
  String result = "";
  unsigned int start = 0;
  int pos;

  while (start < str.length())
  {
    pos = str.indexOf('"', start);
    if (pos != -1)
    {
      result += str.substring(start, pos) + F("&quot;");
      start = pos + 1;
    }
    else
    {
      result += str.substring(start);
      break;
    }
  }
  return result;
}

// Отображение главной web страницы
void handleRoot()
{
  String message =
      F("<!DOCTYPE html>\n\
      <html>\n\
      <head>\n\
      <title>ESP </title>\n\
      <style type=\"text/css\">\n\
      .checkbox {\n\
      vertical-align:top;\n\
      margin:0 3px 0 0;\n\
      width:17px;\n\
      height:17px;\n\
      }\n\
      .checkbox + label {\n\
      cursor:pointer;\n\
      }\n\
      checkbox:not(checked) {\n\
      position:absolute;\n\
      opacity:0;\n\
      }\n\
    .checkbox:not(checked) + label {\n\
      position:relative;\n\
      padding:0 0 0 60px;\n\
    }\n\
    .checkbox:not(checked) + label:before {\n\
      content:'';\n\
      position:absolute;\n\
      top:-4px;\n\
      left:0;\n\
      width:50px;\n\
      height:26px;\n\
      border-radius:13px;\n\
      background:#CDD1DA;\n\
      box-shadow:inset 0 2px 3px rgba(0,0,0,.2);\n\
    }\n\
    .checkbox:not(checked) + label:after {\n\
      content:'';\n\
      position:absolute;\n\
      top:-2px;\n\
      left:2px;\n\
      width:22px;\n\
      height:22px;\n\
      border-radius:10px;\n\
      background:#FFF;\n\
      box-shadow:0 2px 5px rgba(0,0,0,.3);\n\
      transition:all .2s;\n\
    }\n\
    .checkbox:checked + label:before {\n\
      background:#9FD468;\n\
    }\n\
    .checkbox:checked + label:after {\n\
      left:26px;\n\
    }\n\
  </style>\n\
  <script type=\"text/javascript\">\n\
    function openUrl(url) {\n\
      var request = new XMLHttpRequest();\n\
      request.open('GET', url, true);\n\
      request.send(null);\n\
    }\n\
    function refreshData() {\n\
      var request = new XMLHttpRequest();\n\
      request.open('GET', '/data', true);\n\
      request.onreadystatechange = function() {\n\
        if (request.readyState == 4) {\n\
          var data = JSON.parse(request.responseText);\n\
          document.getElementById('");
  message += FPSTR(tempArg);
  message += F("').innerHTML = data.");
  message += FPSTR(tempArg);
  message += F(";\n\
          document.getElementById('");
  message += FPSTR(wifimodeArg);
  message += F("').innerHTML = data.");
  message += FPSTR(wifimodeArg);
  message += F(";\n\
          document.getElementById('");
  message += FPSTR(mqttconnectedArg);
  message += F("').innerHTML = (data.");
  message += FPSTR(mqttconnectedArg);
  message += F(" != true ) ? \"Not connected \" :  \"Connected\";");
  message += F("\n\
        }\n\
      }\n\
      request.send(null);\n\
    }\n\
    setInterval(refreshData, 500);\n\
  </script>\n\
</head>\n\
<body>\n\
  <form>\n\
    <h3>ESP </h3>\n\
    <p>\n\
    Temp : <span id=\"");
  message += FPSTR(tempArg);
  message += F("\">?</span> C<br/>\n\
    WiFi mode: <span id=\"");
  message += FPSTR(wifimodeArg);
  message += F("\">?</span><br/>\n\
    MQTT broker: <span id=\"");
  message += FPSTR(mqttconnectedArg);
  message += F("\">?</span><br/>\n\
    ");
  message += F("</label>\n\
    <p>\n\
    <input type=\"button\" value=\"WiFi Setup\" onclick=\"location.href='/wifi';\" />\n\
    <input type=\"button\" value=\"MQTT Setup\" onclick=\"location.href='/mqtt';\" />\n\
    <input type=\"button\" value=\"Reboot!\" onclick=\"if (confirm('Are you sure to reboot?')) location.href='/reboot';\" />\n\
  </form>\n\
</body>\n\
</html>");

  httpServer.send(200, F("text/html"), message);
}

// Отображение страницы настроек WiFi
void handleWiFiConfig()
{
  String message =
      F("<!DOCTYPE html>\n\
<html>\n\
<head>\n\
  <title>WiFi Setup</title>\n\
</head>\n\
<body>\n\
  <form name=\"wifi\" method=\"get\" action=\"/store\">\n\
    <h3>WiFi Setup</h3>\n\
    SSID:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(ssidArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(ssid);
  message += F("\" />\n\
    <br/>\n\
    Password:<br/>\n\
    <input type=\"password\" name=\"");
  message += FPSTR(passwordArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(password);
  message += F("\" />\n\
    <br/>\n\
    mDNS domain:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(domainArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(domain);
  message += F("\" />\n\
    \n\
    <p>\n\
    <input type=\"submit\" value=\"Save\" />\n\
    <input type=\"hidden\" name=\"");
  message += FPSTR(rebootArg);
  message += F("\" value=\"1\" />\n\
  </form>\n\
</body>\n\
</html>");

  httpServer.send(200, F("text/html"), message);
}

// Отображение страницы настроек MQTT
void handleMQTTConfig()
{
  String message =
      F("<!DOCTYPE html>\n\
<html>\n\
<head>\n\
  <title>MQTT Setup</title>\n\
</head>\n\
<body>\n\
  <form name=\"mqtt\" method=\"get\" action=\"/store\">\n\
    <h3>MQTT Setup</h3>\n\
    Server:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(serverArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(mqttServer);
  message += F("\" onchange=\"document.mqtt.reboot.value=1;\" />\n\
    \n\
    <br/>\n\
    Port:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(portArg);
  message += F("\" maxlength=5 value=\"");
  message += String(mqttPort);
  message += F("\" onchange=\"document.mqtt.reboot.value=1;\" />\n\
    <br/>\n\
    User:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(userArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(mqttUser);
  message += F("\" />\n\
    \n\
    <br/>\n\
    Password:<br/>\n\
    <input type=\"password\" name=\"");
  message += FPSTR(mqttpswdArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(mqttPassword);
  message += F("\" />\n\
    <br/>\n\
    Client:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(clientArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(mqttClient);
  message += F("\" />\n\
    <br/>\n\
    Topic:<br/>\n\
    <input type=\"text\" name=\"");
  message += FPSTR(topicArg);
  message += F("\" maxlength=");
  message += String(maxStrParamLength);
  message += F(" value=\"");
  message += quoteEscape(mqttTopic);
  message += F("\" />\n\
    <p>\n\
    <input type=\"submit\" value=\"Save\" />\n\
    <input type=\"hidden\" name=\"");
  message += FPSTR(rebootArg);
  message += F("\" value=\"0\" />\n\
  </form>\n\
</body>\n\
</html>");

  httpServer.send(200, F("text/html"), message);
}

// Функция перезаписи полученых данных из web интерфейса в EEPROM
void handleStoreConfig()
{
  String argName, argValue;

  Serial.print(F("/store("));
  for (byte i = 0; i < httpServer.args(); i++)
  {
    if (i)
      Serial.print(F(", "));
    argName = httpServer.argName(i);
    Serial.print(argName);
    Serial.print(F("=\""));
    argValue = httpServer.arg(i);
    Serial.print(argValue);
    Serial.print(F("\""));

    if (argName.equals(FPSTR(ssidArg)))
    {
      ssid = argValue;
    }
    else if (argName.equals(FPSTR(passwordArg)))
    {
      password = argValue;
    }
    else if (argName.equals(FPSTR(domainArg)))
    {
      domain = argValue;
    }
    else if (argName.equals(FPSTR(serverArg)))
    {
      mqttServer = argValue;
    }
    else if (argName.equals(FPSTR(portArg)))
    {
      mqttPort = argValue.toInt();
    }
    else if (argName.equals(FPSTR(userArg)))
    {
      mqttUser = argValue;
    }
    else if (argName.equals(FPSTR(mqttpswdArg)))
    {
      mqttPassword = argValue;
    }
    else if (argName.equals(FPSTR(clientArg)))
    {
      mqttClient = argValue;
    }
    else if (argName.equals(FPSTR(topicArg)))
    {
      mqttTopic = argValue;
    }
  }
  Serial.println(F(")"));

  writeConfig();

  String message =
      F("<!DOCTYPE html>\n\
<html>\n\
<head>\n\
  <title>Store Setup</title>\n\
  <meta http-equiv=\"refresh\" content=\"5; /index.html\">\n\
</head>\n\
<body>\n\
  Configuration stored successfully.\n");
  if (httpServer.arg(rebootArg) == "1")
    message += F("  <br/>\n\
  <i>You must reboot module to apply new configuration!</i>\n");
  message += F("  <p>\n\
  Wait for 5 sec. or click <a href=\"/index.html\">this</a> to return to main page.\n\
</body>\n\
</html>");

  httpServer.send(200, F("text/html"), message);
}

// Функция перезагрузки контроллера со страницы в браузере
void handleReboot()
{
  Serial.println(F("/reboot()"));

  String message =
      F("<!DOCTYPE html>\n\
<html>\n\
<head>\n\
  <title>Rebooting</title>\n\
  <meta http-equiv=\"refresh\" content=\"5; /index.html\">\n\
</head>\n\
<body>\n\
  Rebooting...\n\
</body>\n\
</html>");

  httpServer.send(200, F("text/html"), message);

  ESP.restart();
}

// Функция подготовки и отправки JSON
void handleData()
{
  String message = F("{\"");
  message += FPSTR(tempArg);
  message += F("\":");
  message += String(dht.readTemperature());
  message += F(",\"");
  message += FPSTR(wifimodeArg);
  message += F("\":\"");
  switch (WiFi.getMode())
  {
  case WIFI_OFF:
    message += F("OFF");
    break;
  case WIFI_STA:
    message += F("WiFi Client");
    break;
  case WIFI_AP:
    message += F("Access Point");
    break;
  case WIFI_AP_STA:
    message += F("Hybrid (AP+STA)");
    break;
  default:
    message += F("Unknown!");
  }
  message += F("\",\"");
  message += FPSTR(mqttconnectedArg);
  message += F("\":");
  if (pubsubClient.connected())
    message += F("true");
  else
    message += F("false");
  message += F("}");

  httpServer.send(200, F("text/html"), message);
}

// Переподключение к MQTT
bool mqttReconnect()
{
  const uint32_t timeout = 30000;
  static uint32_t lastTime;
  bool result = false;

  if (millis() > lastTime + timeout)
  {
    Serial.print(F("Attempting MQTT connection...\n"));
    Serial.printf("Client %s User %s Password %s\n", mqttClient.c_str(), mqttUser.c_str(), mqttPassword.c_str());
    digitalWrite(pinBuiltinLed, LOW);
    if (mqttUser.length())
      result = pubsubClient.connect(mqttClient.c_str(), mqttUser.c_str(), mqttPassword.c_str());
    else
      result = pubsubClient.connect(mqttClient.c_str());
    digitalWrite(pinBuiltinLed, HIGH);
    if (result)
    {
      Serial.println(F(" connected"));
      String topic('/');
      topic += mqttTopic;
      result = mqtt_subscribe(pubsubClient, topic);
    }
    else
    {
      Serial.print(F(" failed, rc="));
      Serial.println(pubsubClient.state());
    }
    lastTime = millis();
  }

  return result;
}

// Подписка на топик
bool mqtt_subscribe(PubSubClient &client, const String &topic)
{
  Serial.print(F("Subscribing to "));
  Serial.println(topic);
  return client.subscribe(topic.c_str());
}

// Отправка данных в топик
void readSendTemperature(const String &topic)
{

  float t = dht.readTemperature();

  Serial.print(F("Temp: "));
  Serial.print(t);
  Serial.println(F("C"));
  String str_temp(t);
  pubsubClient.publish(topic.c_str(), str_temp.c_str(), mqttSensorRetained);
}