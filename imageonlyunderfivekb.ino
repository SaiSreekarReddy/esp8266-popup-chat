#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <DNSServer.h>


#define MAX_MSG_LEN 1024

const char* ssid = "Connecting_to_the_help_you_need";  // Set your desired AP name here
const char* password = "";                             // Set your desired AP password here
const int MAX_USERS = 10;                              // Maximum number of users allowed
String usernames[MAX_USERS];                           // Array to store usernames
const byte DNS_PORT = 53;
DNSServer dnsServer;
ESP8266WebServer server(80);
WebSocketsServer webSocket(81);


void handleChat() {
  String html = "<html><head><title>My Group Chat</title></head><body style=\"margin: 0; font-family: 'Helvetica Neue', sans-serif;\">";
  html += "<div style=\"background-color: #f4f4f4; padding: 10px;\">";
  html += "<img src=\"https://i.imgur.com/yFqvDFO.png\" style=\"height: 35px; margin-right: 10px;\">";  // replace with your group icon
  html += "<span style=\"font-size: 18px; font-weight: bold;\">My Group Chat</span>";
  html += "</div>";
  html += "<div id=\"messages\" style=\"height: 300px; overflow-y: scroll; padding: 10px;\"></div>";
  html += "<div style=\"background-color: #f4f4f4; padding: 10px;\">";
  html += "<form onsubmit=\"return false;\">";
  html += "<div style=\"display: flex;\">";
  html += "<textarea id=\"message-display\" style=\"width: 100%; height: 50px; margin-right: 10px; border: none; resize: none; padding: 10px;\" placeholder=\"Type a message...\"></textarea>";
  html += "<button onclick=\"sendWebSocketMessage()\" style=\"background-color: #075e54; color: #fff; border: none; padding: 10px;\">Send</button>";
  html += "</div>";
  html += "<input type=\"text\" id=\"message-input\" style=\"display: none;\">";  // hide this input field
  html += "</form>";
  html += "</div>";
  html += "<input type=\"file\" id=\"image-input\" onchange=\"handleImageUpload()\" style=\"display: none;\">";                                                                                          // Add this line
  html += "<button onclick=\"document.getElementById('image-input').click()\" style=\"background-color: #075e54; color: #fff; border: none; padding: 10px; margin-left: 10px;\">Upload Image</button>";  // Add this line


  html += "<script>";
  html += "var socket = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "socket.onmessage = function(event) {";
  html += "console.log('Received message:', event.data);";
  html += "  console.log('WebSocket connected!');";
  html += "  var messages = document.getElementById('messages');";
  html += "  var messageDisplay = document.getElementById('message-display');";
  html += "  var message = event.data;";
  html += "  messages.innerHTML += '<div style=\"margin-bottom: 10px;\"><span style=\"background-color: #075e54; color: #fff; padding: 5px 10px; border-radius: 20px; display: inline-block; margin-right: 10px;\">' + message + '</span></div>';";
  html += "  messages.scrollTop = messages.scrollHeight;";

  html += "};";
  html += "function sendWebSocketMessage() {";
  html += "  var messageDisplay = document.getElementById('message-display');";
  html += "  var message = messageDisplay.value;";
  html += "  if (message.startsWith('data:image')) {";
  html += "    messages.innerHTML += '<div style=\"margin-bottom: 10px;\"><img src=\"' + message + '\" style=\"max-width: 200px; max-height: 200px; border-radius: 10px;\"></div>';";
  html += "  } else {";
  html += "    messages.innerHTML += '<div style=\"margin-bottom: 10px;\"><span style=\"background-color: #075e54; color: #fff; padding: 5px 10px; border-radius: 20px; display: inline-block; margin-right: 10px;\">' + message + '</span></div>';";
  html += "  }";
  html += "}";
  html += "function handleImageUpload() {";
  html += "  var fileInput = document.getElementById('image-input');";
  html += "  var file = fileInput.files[0];";
  html += "  var reader = new FileReader();";
  html += "  reader.onloadend = function() {";
  html += "    var base64data = reader.result;";
  html += "    var jsonData = JSON.stringify({type: 'image', data: base64data});";
  html += "    socket.send(jsonData);";
  html += "  };";
  html += "  reader.readAsDataURL(file);";
  html += "}";


  html += "</script>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleRoot() {
  String html = "<html><head><title>My Captive Portal</title></head><body>";
  html += "<div style='text-align:center;'><img src='https://example.com/help.jpg' alt='Help is on the way' width='300' height='200'></div>";
  html += "<h1 style='text-align:center;'>Register Your Name to Enter the Chat</h1>";
  html += "<p style='text-align:center;'>Please enter your name below:</p>";
  html += "<form method=\"POST\" action=\"/register\" style='text-align:center;'>";
  html += "<label>Username:</label><br>";
  html += "<input type=\"text\" name=\"username\"><br><br>";
  html += "<input type=\"submit\" value=\"Register\">";
  html += "</form>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleRegisterSubmit() {
  String cusername = server.arg("username");

  // Add the new user to your user database
  // Here, we just print the value to the serial monitor
  Serial.print("Registration request: ");
  Serial.println(cusername);

  // Add the new user to the usernames array
  for (int i = 0; i < MAX_USERS; i++) {
    if (usernames[i] == "") {
      usernames[i] = cusername;
      break;
    }
  }

  // Broadcast message that a new user has joined the chat
  webSocket.broadcastTXT(String(cusername + " has joined the chat!").c_str());

  // Redirect to the chat page
  server.sendHeader("Location", "/chat");
  server.send(302);
}

void handleWebSocketClose(uint8_t num) {
  Serial.printf("[%u] Disconnected!\n", num);

  // Notify other users that this user has left the chat
  if (usernames[num] != "") {
    webSocket.broadcastTXT(String(usernames[num] + " has left the chat!").c_str());
  }

  // Remove the user from the usernames array
  usernames[num] = "";
}

void handleNotFound() {
  server.sendHeader("Location", "/");  // Redirect to root
  server.send(302, "text/plain", "");  // Send a temporary redirect response
}



void setup() {
  Serial.begin(115200);

  // Set the ESP8266 in AP mode
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 10, 1);       // Change the IP address here
  IPAddress apNetMsk(255, 255, 255, 0);  // Change the subnet mask here if necessary
  WiFi.softAPConfig(apIP, apIP, apNetMsk);
  WiFi.softAP(ssid, password);

  // Configure the DNS server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Start the web server
  server.on("/", handleRoot);
  server.on("/register", HTTP_POST, handleRegisterSubmit);
  server.on("/chat", handleChat);  // New handler for the chat page
  server.onNotFound(handleNotFound);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  server.begin();
  SPIFFS.begin();

  // Start the WebSocket server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("Server started");
}


void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      handleWebSocketClose(num);
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


void handleWebSocketMessage(uint8_t num, uint8_t* payload, size_t length) {
  String message = "";
  for (size_t i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.printf("Received message: %s\n", message.c_str());
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, message);
  const char* messageType = doc["type"];

  if (strcmp(messageType, "chat") == 0) {
    // ... (existing chat message handling code)
  } else if (strcmp(messageType, "image") == 0) { // Add this line
    String imageData = doc["data"].as<String>();
    Serial.printf("[%u] Image data: %s\n", num, imageData.substring(0, 50).c_str()); // Print only the first 50 characters of the base64 data for readability
    webSocket.broadcastTXT(String(usernames[num] + ": " + imageData).c_str());
  }
}



void loop() {
  server.handleClient();
  webSocket.loop();
  dnsServer.processNextRequest();  // Process DNS requests
}
