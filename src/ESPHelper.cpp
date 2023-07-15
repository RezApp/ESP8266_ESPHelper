#include "ESPHelper.h"

#define DBG_OUTPUT (*dbg_out)

void ESPHelper::handleTelnet()
{
  if (TelnetServer.hasClient())
  {
    if (!Telnet || Telnet.connected())
    {
      if (Telnet)
        Telnet.stop();
      Telnet = TelnetServer.available();
    }
    else
    {
      TelnetServer.available().stop();
    }
  }
}

void ESPHelper::OTA_setup()
{
  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  if (con.deviceName.c_str()[0])
    ArduinoOTA.setHostname(con.deviceName.c_str());

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([&]()
                     { DBG_OUTPUT.println("OTA:\tStart"); });
  ArduinoOTA.onEnd([&]()
                   { DBG_OUTPUT.println("OTA:\tEnd"); });
  ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total)
                        { DBG_OUTPUT.printf("OTA:\tProgress: %u%%\r\n", (progress / (total / 100))); });
  ArduinoOTA.onError([&](ota_error_t error)
                     {
    DBG_OUTPUT.printf("OTA:\tError[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      DBG_OUTPUT.println("OTA:\tAuth Failed");
    else if (error == OTA_BEGIN_ERROR)
      DBG_OUTPUT.println("OTA:\tBegin Failed");
    else if (error == OTA_CONNECT_ERROR)
      DBG_OUTPUT.println("OTA:\tConnect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      DBG_OUTPUT.println("OTA:\tReceive Failed");
    else if (error == OTA_END_ERROR)
      DBG_OUTPUT.println("OTA:\tEnd Failed"); });
  ArduinoOTA.begin();
  DBG_OUTPUT.println("OTA:\tbegin");
}

bool ESPHelper::config(Config_t c)
{

  if (!c.stSSID || !c.stPASS || !c.ip)
    return false;

  con.stSSID = c.stSSID;
  con.stPASS = c.stPASS;
  con.ip = c.ip;
  con.gateway = c.gateway;
  con.subnet = c.subnet;
  con.dns = c.dns;

  con.deviceName = c.deviceName;
  con.www_username = c.www_username;
  con.www_password = c.www_password;

  con.apSSID = c.apSSID;
  con.apPASS = c.apPASS;

  return true;
}

void ESPHelper::setup(Config_t c, bool _usingTelnet, bool _usingOTA)
{
  usingTelnet = _usingTelnet;
  usingOTA = _usingOTA;

  SPIFFS.begin();
  // called when the url is not defined here
  // use it to load content from SPIFFS
  server.onNotFound([&]()
                    {
    if (!checkAuthentication())
      return;
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "File Not Found"); });

  if (config(c))
  {
    if (!setupST())
      setupAP();
  }

  if (usingOTA)
    OTA_setup();

  if (usingTelnet)
  {
    TelnetServer.begin();
    TelnetServer.setNoDelay(true);
  }
}

void ESPHelper::loop()
{
  if (usingOTA)
    ArduinoOTA.handle();

  if (usingTelnet)
    handleTelnet();

  server.handleClient();
}

void ESPHelper::printMyTime()
{
  long t = millis() / 1000;
  word h = t / 3600;
  byte m = (t / 60) % 60;
  byte s = t % 60;

  DBG_OUTPUT.print(h / 10);
  DBG_OUTPUT.print(h % 10);
  DBG_OUTPUT.print(":");
  DBG_OUTPUT.print(m / 10);
  DBG_OUTPUT.print(m % 10);
  DBG_OUTPUT.print(":");
  DBG_OUTPUT.print(s / 10);
  DBG_OUTPUT.print(s % 10);
  DBG_OUTPUT.println();
}

bool ESPHelper::testWifi(void)
{
  int c = 0;
  DBG_OUTPUT.println("Waiting for Wifi to connect...");
  while (c < 20)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      DBG_OUTPUT.println();
      return true;
    }
    delay(500);
    DBG_OUTPUT.print(WiFi.status());
    c++;
  }
  DBG_OUTPUT.println();
  DBG_OUTPUT.println("Connect timed out, opening AP");
  return false;
}

void ESPHelper::launchWeb(int webtype)
{
  DBG_OUTPUT.println();
  DBG_OUTPUT.println("WiFi connected");

  if (webtype == 0)
  {
    DBG_OUTPUT.print("Local IP: ");
    DBG_OUTPUT.println(WiFi.localIP());
  }
  else
  {
    DBG_OUTPUT.print("SoftAP IP: ");
    DBG_OUTPUT.println(WiFi.softAPIP());
  }

  createWebServer(webtype);
  // Start the server
  server.begin();
  DBG_OUTPUT.println("Server started");
}

bool ESPHelper::setupST()
{
  WiFi.mode(WIFI_STA);
  IPAddress ip, gateway, subnet, dns;
  bool isOk = ip.fromString(con.ip) && gateway.fromString(con.gateway) && subnet.fromString(con.subnet) && dns.fromString(con.dns);

  if (isOk && WiFi.config(ip, gateway, subnet, dns))
  {
    WiFi.begin(con.stSSID.c_str(), con.stPASS.c_str());
    if (testWifi())
    {
      launchWeb(0);
      return true;
    }
  }
  return false;
}

void ESPHelper::setupAP()
{
  DBG_OUTPUT.print("SETUP AP: ");
  WiFi.mode(WIFI_AP);

  if (con.apSSID.length() == 0 || con.apPASS.length() == 0)
  {
    set_ap_ssid_and_pass("MyIoT", "a1234567");
  }

  if (WiFi.softAP(con.apSSID.c_str(), con.apPASS.c_str()))
  {
    DBG_OUTPUT.println("OK");
  }
  else
  {
    DBG_OUTPUT.println("Failed");
    delay(1000);
    ESP.restart();
  }
  launchWeb(1);
}

String ESPHelper::listNetworks(void)
{
  int n = WiFi.scanNetworks();
  DBG_OUTPUT.println("scan done");
  if (n == 0)
    DBG_OUTPUT.println("no networks found");
  else
  {
    DBG_OUTPUT.print(n);
    DBG_OUTPUT.println(" networks found");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      DBG_OUTPUT.print(i + 1);
      DBG_OUTPUT.print(": ");
      DBG_OUTPUT.print(WiFi.SSID(i));
      DBG_OUTPUT.print(" (");
      DBG_OUTPUT.print(WiFi.RSSI(i));
      DBG_OUTPUT.print(")");
      DBG_OUTPUT.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
      delay(10);
    }
  }
  DBG_OUTPUT.println("");
  String st = "[";
  for (int i = 0; i < n; ++i)
  {
    // Print SSID and RSSI for each network found
    st += "{\"ssid\":\"";
    st += WiFi.SSID(i);
    st += "\",\"rssi\":\"";
    st += WiFi.RSSI(i);
    st += "\",\"encyrption\":";
    st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "false" : "true";
    st += "}";

    if (i + 1 < n)
      st += ",";
  }
  st += "]";

  return st;
}

String ESPHelper::getContentType(String filename)
{
  if (server.hasArg("download"))
    return "application/octet-stream";
  else if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  else if (filename.endsWith(".json"))
    return "application/json";

  return "text/plain";
}

bool ESPHelper::handleFileRead(String path)
{
  DBG_OUTPUT.println("Read:\t" + path);
  if (path.endsWith("/"))
    path += "index.html";
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(path))
  {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void ESPHelper::handleFileUpload()
{
  if (server.uri() != "/upload")
    return;
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    DBG_OUTPUT.println("Uploading...\tName: " + filename);

    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    // DBG_OUTPUT.print("handleFileUpload Data: "); DBG_OUTPUT.println(upload.currentSize);
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize);
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
      fsUploadFile.close();
    DBG_OUTPUT.print("Uploaded\tName: " + upload.filename);
    DBG_OUTPUT.print("\tSize: ");
    DBG_OUTPUT.println(upload.totalSize);
  }
}

void ESPHelper::handleFileDelete()
{
  if (server.args() == 0)
    return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  DBG_OUTPUT.println("Delete:\t" + path);
  if (path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if (!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}

void ESPHelper::createWebServer(int webtype)
{

  on("/networks", HTTP_GET, [&]()
     { server.send(200, "application/json", listNetworks()); });

  // delete file
  on("/upload", HTTP_DELETE, [&]()
     {
    handleFileDelete();
    server.send(200, "text/plain", "Deleted\r\n"); });

  // first callback is called after the request has ended with all parsed arguments
  // second callback handles file uploads at that location
  on(
      "/upload", HTTP_POST, [&]()
      { server.send(200, "text/plain", "Uploaded\r\n"); },
      [&]()
      {
        handleFileUpload();
      });

  on("/act", HTTP_POST, [&]()
     {

    String v_todo = server.arg("todo");

    content = "{\"result\":";
    content += "\"error\"";
    content += "}\r\n";

    if (v_todo.length() > 0){
      if (v_todo.equals("reboot"))
      {
        server.send(200, "application/json", "{\"result\":\"ok\"}");
        delay(3000);
        ESP.restart();
      }
    }

    server.send(200, "application/json", content); });

  if (webtype == 1)
  {
    apHandler();
  }
  else if (webtype == 0)
  {
    stHandler();
  }
}

void ESPHelper::setHandlers(ESPHelperHandler_t st_h, ESPHelperHandler_t ap_h)
{
  stHandler = st_h;
  apHandler = ap_h;
}

bool ESPHelper::checkAuthentication()
{
  if (!authMode)
    return true;

  if (!server.authenticate(con.www_username.c_str(), con.www_password.c_str()))
  {
    server.requestAuthentication();
    return false;
  }

  return true;
}

void ESPHelper::set_ap_ssid_and_pass(String ssid, String pass)
{
  con.apSSID = ssid;
  con.apPASS = pass;
}

void ESPHelper::on(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn, ESP8266WebServer::THandlerFunction ufn)
{
  server.addHandler(new MyRequestHandler([&]()
                                         { return checkAuthentication(); },
                                         fn, ufn, uri, method));
}

void ESPHelper::on(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn)
{
  on(uri, method, fn, [&]() {});
}

void ESPHelper::on(const String &uri, ESP8266WebServer::THandlerFunction handler)
{
  on(uri, HTTP_ANY, handler);
}
