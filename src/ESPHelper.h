#ifndef ESPHelper_h
#define ESPHelper_h

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include <ESP8266WebServer.h>
#include <FS.h>
#include <ArduinoOTA.h>





class ESPHelper
{


  public:

    typedef struct
    {
      String deviceName = "MyIoT";
      String stSSID;
      String stPASS;
      String ip;
      String gateway = "192.168.1.1";
      String subnet = "255.255.255.0";
      String dns = "8.8.8.8";
      String www_username = "admin";
      String www_password = "admin";
      String apSSID = "MyIoT";
      String apPASS = "a1234567";
    } Config_t;


    String content;
    int statusCode;
    bool authMode;

    bool usingTelnet = true;
    bool usingOTA = true;

    // Create an instance of the server
    // specify the port to listen on as an argument
    ESP8266WebServer server;

    WiFiServer TelnetServer;
    WiFiClient Telnet;

    Stream *dbg_out;


    Config_t con;

    //holds the current upload
    File fsUploadFile;

    void (*stHandler)(void);
    void (*apHandler)(void);

    bool (*configHandler)(String *, String *, String *, String *, String *, String *, String *);

    ESPHelper() : server(80), TelnetServer(23)
    {
      dbg_out = &Telnet;
    }
    ESPHelper(Stream *s) : server(80), TelnetServer(23)
    {
      dbg_out = s;
    }

    void setup(Config_t c, bool _usingTelnet = true, bool _usingOTA = true);
    bool config(Config_t);
    void loop();
    void handleTelnet();
    void OTA_setup();
    bool setupST();
    void setupAP();

    void set_ap_ssid_and_pass(String ssid, String pass);

    bool testWifi(void);
    void launchWeb(int webtype);

    String listNetworks(void);

    void active_auth_mode();
    void deactive_auth_mode();

    String getContentType(String filename);
    bool handleFileRead(String path);
    void handleFileUpload();
    void handleFileDelete();

    void printMyTime();

    void createWebServer(int webtype);
    void setHandlers(void (*st_h)(void), void (*ap_h)(void));

    bool checkAuthentication();

    void on(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn, ESP8266WebServer::THandlerFunction ufn);
    void on(const String &uri, HTTPMethod method, ESP8266WebServer::THandlerFunction fn);
    void on(const String &uri, ESP8266WebServer::THandlerFunction fn);
};

class MyRequestHandler : public RequestHandler
{
  public:
    //typedef bool (*MyHandlerFunction)(void);
    typedef std::function<bool(void)> AuthHandlerFunction;

    MyRequestHandler(AuthHandlerFunction auth, ESP8266WebServer::THandlerFunction fn, ESP8266WebServer::THandlerFunction ufn, const String &uri, HTTPMethod method)
      : _auth(auth), _fn(fn), _ufn(ufn), _uri(uri), _method(method)
    {
    }

    bool canHandle(HTTPMethod requestMethod, const String& requestUri) override
    {
      if (_method != HTTP_ANY && _method != requestMethod)
        return false;

      if (requestUri != _uri)
        return false;

      return true;
    }

    bool canUpload(const String& requestUri) override
    {
      if (!_ufn || !canHandle(HTTP_POST, requestUri))
        return false;

      return true;
    }

    bool handle(ESP8266WebServer &server, HTTPMethod requestMethod, const String& requestUri) override
    {
      (void)server;
      if (!canHandle(requestMethod, requestUri))
        return false;
      if (_auth())
        _fn();
      return true;
    }

    void upload(ESP8266WebServer &server, const String& requestUri, HTTPUpload &upload) override
    {
      (void)server;
      (void)upload;
      if (canUpload(requestUri))
      {
        if (_auth())
          _ufn();
      }
    }

  protected:
    AuthHandlerFunction _auth;
    ESP8266WebServer::THandlerFunction _fn;
    ESP8266WebServer::THandlerFunction _ufn;
    String _uri;
    HTTPMethod _method;
};

#endif
