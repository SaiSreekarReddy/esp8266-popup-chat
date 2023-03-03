#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>

const char* ssid = "MyESP8266AP"; // Set your desired AP name here
const char* password = "MyESP8266Password"; // Set your desired AP password here

ESP8266WebServer server(80);
WebSocketsServer webSocket(81);
String cusername; // global variable
void handleRoot() {
  String html = "<html><head><title>My Captive Portal</title></head><body>";
  html += "<h1>Welcome YOU ARE SAVE JUST ENTER YOUR NAME TO ADDRESS YOU</h1>";
  html += "<p>Please register to access the chat.</p>";
  html += "<form method=\"POST\" action=\"/register\">";
  html += "<label>Username:</label><br>";
  html += "<input type=\"text\" name=\"username\"><br>";
  html += "<br>";
  html += "<input type=\"submit\" value=\"Register\">";
  html += "</form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}
void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Starting file upload: %s\n", upload.filename.c_str());
    fs::File file = SPIFFS.open(upload.filename, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Serial.printf("Writing file: %d bytes\n", upload.currentSize);
    fs::File file = SPIFFS.open(upload.filename, "a");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }
    file.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.printf("File upload complete: %s, %d bytes\n", upload.filename.c_str(), upload.totalSize);
    fs::File file = SPIFFS.open(upload.filename, "r");
    if (!file) {
      Serial.println("Failed to open file for reading");
      return;
    }
    server.sendHeader("Content-Type", "application/octet-stream");
    server.sendHeader("Content-Disposition", "attachment; filename=\"" + upload.filename + "\"");
    server.sendHeader("Content-Length", String(file.size()));
    server.sendContent(file);
    file.close();
  }
}

void handleFileDownload() {
  String filename = server.arg("filename");
  if (!SPIFFS.exists(filename)) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  fs::File file = SPIFFS.open(filename, "r");
  if (!file) {
    server.send(500, "text/plain", "Failed to open file for reading");
    return;
  }
  server.sendHeader("Content-Type", "application/octet-stream");
  server.sendHeader("Content-Disposition", "attachment; filename=\"" + filename + "\"");
  server.sendHeader("Content-Length", String(file.size()));
  server.sendContent(file);
  file.close();
}
void handleRegisterSubmit() {
  cusername = server.arg("username");

  // Add the new user to your user database
  // Here, we just print the value to the serial monitor
  Serial.print("Registration request: ");
  Serial.println(cusername);

  // Broadcast message that a new user has joined the chat
  webSocket.broadcastTXT(String(cusername + " has joined the chat!").c_str());

  // Redirect to the chat page
  server.sendHeader("Location", "/chat");
  server.send(302);
}


void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);

  // Set the ESP8266 in AP mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Start the web server
  server.on("/", handleRoot);
  server.on("/register", HTTP_POST, handleRegisterSubmit);
  server.on("/chat", handleChat); // New handler for the chat page
  server.onNotFound(handleNotFound);
  server.begin();

   // Mount SPIFFS file system
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }

  // Start the WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Server started");
}
void handleChat() {
  String html = "<html><head><title>THE GROUP CHAT FOR SURVIORS</title></head><body>";
  html += "<h1>Welcome to my group chat</h1>";
  html += "<div style=\"border: 1px solid black; padding: 10px; height: 400px; overflow-y: scroll;\" id=\"messages\"></div>";
  html += "<br>";
  html += "<form onsubmit=\"return false;\">";
  html += "<div style=\"display: flex;\">";
  html += "<input style=\"flex: 1;\" type=\"text\" id=\"message\">";
  html += "<input type=\"file\" id=\"file\" name=\"file\" onchange=\"uploadFile()\">";
  html += "<button style=\"margin-left: 10px;\" onclick=\"sendWebSocketMessage()\">Send</button>";
  html += "</div>";
  html += "</form>";
  html += "<script>";
  html += "var socket = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "socket.onmessage = function(event) {";
  html += "  var messages = document.getElementById('messages');";
  html += "  var message = event.data;";
  html += "  var messageParts = message.split(':');";
  html += "  var username = messageParts[0];";
  html += "  var messageText = messageParts[1];";
  html += "  if (username === '" + cusername + "') {";
  html += "    messages.innerHTML += '<div style=\"color: blue;\">' + messageText + '</div>';";
  html += "  } else {";
  html += "    messages.innerHTML += '<div style=\"color: black;\">' + message + '</div>';";
  html += "  }";
  html += "};";
  
  html += "function sendWebSocketMessage() {";
  html += "  var messageInput = document.getElementById('message');";
  html += "  var message = messageInput.value;";
  html += "  if (message) {";
  html += "    socket.send('" + cusername + ": ' + message);";
  html += "    messageInput.value = '';";
  html += "  }";
  html += "}";
  
  html += "function uploadFile() {";
  html += "  var fileInput = document.getElementById('file');";
  html += "  var file = fileInput.files[0];";
  html += "  var reader = new FileReader();";
  html += "  reader.onload = function(event) {";
  html += "    var message = {'type': 'file', 'filename': file.name, 'data': event.target.result};";
  html += "    socket.send(JSON.stringify(message));";
  html += "  };";
  html += "  reader.readAsDataURL(file);";
  html += "}";
  
  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}




void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected!\n", num);
      break;
    case WStype_TEXT:
      handleWebSocketMessage(num, payload, length);
      break;
    default:
      break;
  }
}
void handleWebSocketMessage(uint8_t num, uint8_t *payload, size_t length) {
  String message = "";
  for (size_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message.startsWith("join:")) {
    String cusername = message.substring(5);
    Serial.printf("[%u] User %s joined!\n", num, cusername.c_str());
    webSocket.broadcastTXT(String(cusername + " joined the chat!").c_str());
  } else {
    Serial.printf("[%u] Message: %s\n", num, message.c_str());
    webSocket.broadcastTXT(message.c_str());
  }
}


void loop() {
  server.handleClient();
  webSocket.loop();
}