#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

static const char *WIFI_SSID = "126";
static const char *WIFI_PASS = "1234567890";

static const uint32_t UART_BAUD = 115200;
static const uint16_t HTTP_PORT = 80;
static const uint16_t WS_PORT = 81;
static const uint32_t LINK_REPORT_MS = 10000;
static const uint32_t WIFI_DIAG_MS = 3000;

ESP8266WebServer httpServer(HTTP_PORT);
WebSocketsServer webSocket(WS_PORT);

static String uartLine;
static String lastStateJson = "{}";
static uint32_t lastLinkReportMs = 0;
static uint32_t lastWifiDiagMs = 0;
static uint8_t wsClientCount = 0;
static String lastReportedIp;
static int lastReportedWs = -1;
static int lastWifiStatus = -1;

static String jsonEscape(const String &s) {
  String out;
  out.reserve(s.length() + 8);
  for (size_t i = 0; i < s.length(); i++) {
    const char c = s[i];
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (c == '\n') {
      out += "\\n";
    } else if (c == '\r') {
      out += "\\r";
    } else {
      out += c;
    }
  }
  return out;
}

static String getJsonString(const String &json, const char *key) {
  const String pattern = String("\"") + key + "\"";
  int p = json.indexOf(pattern);
  if (p < 0) return "";
  p = json.indexOf(':', p);
  if (p < 0) return "";
  p++;
  while (p < (int)json.length() && isspace(json[p])) p++;
  if (p >= (int)json.length()) return "";

  if (json[p] == '"') {
    p++;
    int e = json.indexOf('"', p);
    if (e < 0) return "";
    return json.substring(p, e);
  }

  int e = p;
  while (e < (int)json.length() && json[e] != ',' && json[e] != '}') e++;
  return json.substring(p, e);
}

static int getJsonInt(const String &json, const char *key, int fallback) {
  String value = getJsonString(json, key);
  if (value.length() == 0) return fallback;
  return value.toInt();
}

static void sendIpToMcu(bool force) {
  if (WiFi.status() != WL_CONNECTED) return;
  String ip = WiFi.localIP().toString();
  if (force || ip != lastReportedIp) {
    Serial.print("IP=");
    Serial.println(ip);
    lastReportedIp = ip;
  }
}

static void sendWsStateToMcu(bool force) {
  int wsState = wsClientCount > 0 ? 1 : 0;
  if (force || wsState != lastReportedWs) {
    Serial.print("WS=");
    Serial.println(wsState ? "1" : "0");
    lastReportedWs = wsState;
  }
}

static void sendDiagToMcu(const String &message) {
  Serial.print("DIAG ");
  Serial.println(message);
}

static void broadcastState() {
  String payload = String("{\"type\":\"state\",\"data\":") + lastStateJson + "}";
  webSocket.broadcastTXT(payload);
}

static void handleUartLine(const String &line) {
  if (line.startsWith("STAT ")) {
    lastStateJson = line.substring(5);
    broadcastState();
    return;
  }

  if (line.startsWith("ACK ")) {
    String payload = "{\"type\":\"ack\",\"raw\":\"" + jsonEscape(line) + "\"}";
    webSocket.broadcastTXT(payload);
    return;
  }

  if (line.length() > 0) {
    String payload = "{\"type\":\"uart\",\"raw\":\"" + jsonEscape(line) + "\"}";
    webSocket.broadcastTXT(payload);
  }
}

static void pollUart() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      handleUartLine(uartLine);
      uartLine = "";
      continue;
    }

    if (uartLine.length() < 512) {
      uartLine += c;
    } else {
      uartLine = "";
    }
  }
}

static void handleWsText(uint8_t clientId, const String &msg) {
  String type = getJsonString(msg, "type");
  String cmd = getJsonString(msg, "cmd");

  if (type == "ping") {
    String payload = "{\"type\":\"pong\"}";
    webSocket.sendTXT(clientId, payload);
    return;
  }

  if (type == "get_state") {
    String payload = String("{\"type\":\"state\",\"data\":") + lastStateJson + "}";
    webSocket.sendTXT(clientId, payload);
    return;
  }

  if (type == "cmd" || cmd.length() > 0) {
    String seq = getJsonString(msg, "seq");
    String target = getJsonString(msg, "target");
    int value = getJsonInt(msg, "value", 0);
    if (seq.length() == 0) seq = String(millis());

    if (target.length() > 0) {
      Serial.print("CMD seq=");
      Serial.print(seq);
      Serial.print(" target=");
      Serial.print(target);
      Serial.print(" value=");
      Serial.println(value);
      String payload = String("{\"type\":\"cmd_sent\",\"seq\":\"") + jsonEscape(seq) + "\"}";
      webSocket.sendTXT(clientId, payload);
    } else {
      String payload = String("{\"type\":\"ack\",\"raw\":\"ACK seq=") + jsonEscape(seq) + " ok=0 reason=BAD_TARGET\"}";
      webSocket.sendTXT(clientId, payload);
    }
  }
}

static void onWsEvent(uint8_t clientId, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_CONNECTED:
      if (wsClientCount < 255) wsClientCount++;
      sendWsStateToMcu(true);
      sendIpToMcu(false);
      {
        String payload = String("{\"type\":\"hello\",\"ip\":\"") + WiFi.localIP().toString() + "\"}";
        webSocket.sendTXT(clientId, payload);
      }
      {
        String payload = String("{\"type\":\"state\",\"data\":") + lastStateJson + "}";
        webSocket.sendTXT(clientId, payload);
      }
      break;

    case WStype_DISCONNECTED:
      if (wsClientCount > 0) wsClientCount--;
      sendWsStateToMcu(true);
      break;

    case WStype_TEXT:
      {
        String msg;
        msg.reserve(length);
        for (size_t i = 0; i < length; i++) {
          msg += (char)payload[i];
        }
        handleWsText(clientId, msg);
      }
      break;

    default:
      break;
  }
}

static void setupHttp() {
  httpServer.on("/", []() {
    httpServer.send(200, "text/plain", "Smart Solder ESP01S WebSocket bridge. Use ws://<ip>:81/");
  });

  httpServer.on("/status", []() {
    httpServer.send(200, "application/json", lastStateJson);
  });

  httpServer.begin();
}

void setup() {
  Serial.begin(UART_BAUD);
  Serial.setTimeout(0);
  uartLine.reserve(256);
  lastStateJson.reserve(256);

  delay(50);
  sendDiagToMcu(String("BOOT fw=ESP01S_WS_BRIDGE_V0.3 ssid=") + WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  sendDiagToMcu("WIFI_BEGIN");

  setupHttp();
  webSocket.begin();
  webSocket.onEvent(onWsEvent);
}

void loop() {
  const uint32_t now = millis();

  httpServer.handleClient();
  webSocket.loop();
  pollUart();

  int wifiStatus = (int)WiFi.status();
  if ((wifiStatus != lastWifiStatus) || (now - lastWifiDiagMs >= WIFI_DIAG_MS)) {
    lastWifiStatus = wifiStatus;
    lastWifiDiagMs = now;
    if (wifiStatus == WL_CONNECTED) {
      sendDiagToMcu(String("WIFI_OK ip=") + WiFi.localIP().toString());
    } else {
      sendDiagToMcu(String("WIFI_WAIT status=") + wifiStatus);
    }
  }

  if (WiFi.status() == WL_CONNECTED && (now - lastLinkReportMs >= LINK_REPORT_MS)) {
    lastLinkReportMs = now;
    sendIpToMcu(true);
    sendWsStateToMcu(false);
  }
}
