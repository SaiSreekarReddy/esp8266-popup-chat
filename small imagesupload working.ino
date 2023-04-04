#include <base64.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WebSocketsServer.h>
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
// Added a new struct to store image chunk information for each user
struct ImageChunks {
  String imgData;
  int chunksReceived;
  int totalChunks;
};

ImageChunks imgChunks[MAX_USERS];

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
  html += "<input type=\"file\" id=\"image-upload\" accept=\"image/*\" style=\"display: none;\" onchange=\"handleImageUpload()\">";
  html += "<label for=\"image-upload\" style=\"background-color: #075e54; color: #fff; border: none; padding: 10px; cursor: pointer;\">Upload Image</label>";

  html += "<script>";
  html += "var socket = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "socket.onmessage = function(event) {";
  html += "console.log('Received message:', event.data);";  // Add this line
  html += "  console.log('WebSocket connected!');";         // Add this line
  html += "  var messages = document.getElementById('messages');";
  html += "  var messageDisplay = document.getElementById('message-display');";
  html += "  var message = event.data;";
  html += "  if (message.startsWith('img:')) {";
  html += "    var imgData = message.substring(4);";
  html += "    messages.innerHTML += '<div style=\"margin-bottom: 10px;\"><img src=\"data:image/jpeg;base64,' + imgData + '\" style=\"max-width: 100%; border-radius: 5px;\"></div>';";
  html += "  } else {";
  html += "    messages.innerHTML += '<div style=\"margin-bottom: 10px;\"><span style=\"background-color: #075e54; color: #fff; padding: 5px 10px; border-radius: 20px; display: inline-block; margin-right: 10px;\">' + message + '</span></div>';";
  html += "  }";
  html += "  messages.scrollTop = messages.scrollHeight;";
  html += "};";
  html += "function sendWebSocketMessage() {";
  html += "  var messageDisplay = document.getElementById('message-display');";
  html += "  var message = messageDisplay.value;";
  html += "  if (message) {";
  html += "    socket.send('chat|' + message);";
  html += "    messageDisplay.value = '';";
  html += "  }";
  html += "}";
  html += "function handleImageUpload() {";
  html += "  var imageInput = document.getElementById('image-upload');";
  html += "  var file = imageInput.files[0];";
  html += "  var reader = new FileReader();";
  html += "  reader.onloadend = function() {";
  html += "    var base64 = reader.result.split(',')[1];";
  html += "console.log(' Base64 encoded image data : ', base64);";  // Add this line

  html += "    sendImageWebSocketMessage(base64);";
  html += "  };";
  html += "  if (file) {";
  html += "    reader.readAsDataURL(file);";
  html += "  }";
  html += "}";
  html += "function sendImageWebSocketMessage(base64) {";
  html += "  var chunkSize = 512;";  // Change this value if needed
  html += "  var chunksCount = Math.ceil(base64.length / chunkSize);";
  html += "  for (var i = 0; i < chunksCount; i++) {";
  html += "    var chunk = base64.substring(i * chunkSize, (i + 1) * chunkSize);";
  html += "    socket.send('img_chunk|' + i + '|' + chunksCount + '|' + chunk);";
  html += "  }";
  html += "}";
  html += "</script>";


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
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);

  // Set the ESP8266 in AP mode
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 10, 1);       // Change the IP address here
  IPAddress apNetMsk(255, 255, 255, 0);  // Change the subnet mask here if necessary
  WiFi.softAPConfig(apIP, apIP, apNetMsk);
  WiFi.softAP(ssid, password);
  // Set up the DNS server
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Start the web server
  server.on("/", handleRoot);
  server.on("/register", HTTP_POST, handleRegisterSubmit);
  server.on("/chat", handleChat);  // New handler for the chat page
  server.onNotFound(handleNotFound);
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

  int delimiterIndex1 = message.indexOf('|');
  String messageType = message.substring(0, delimiterIndex1);
  message = message.substring(delimiterIndex1 + 1);

  if (messageType == "img_chunk") {
    int delimiterIndex2 = message.indexOf('|');
    int chunkIndex = message.substring(0, delimiterIndex2).toInt();
    message = message.substring(delimiterIndex2 + 1);

    int delimiterIndex3 = message.indexOf('|');
    int chunksCount = message.substring(0, delimiterIndex3).toInt();
    String chunk = message.substring(delimiterIndex3 + 1);

    imgChunks[num].imgData += chunk;
    imgChunks[num].chunksReceived++;
    imgChunks[num].totalChunks = chunksCount;

    Serial.print("Received chunk ");
    Serial.print(chunkIndex + 1);
    Serial.print(" of ");
    Serial.print(chunksCount);
    Serial.print(" for user ");
    Serial.print(num);
    Serial.print(". Current data length: ");
    Serial.println(imgChunks[num].imgData.length());

    if (imgChunks[num].chunksReceived == imgChunks[num].totalChunks) {
      broadcastImage(imgChunks[num].imgData, num);
      imgChunks[num].imgData = "";
      imgChunks[num].chunksReceived = 0;
      imgChunks[num].totalChunks = 0;
    }
  } else if (messageType == "chat") {
    String chatMessage = message;
    Serial.printf("[%u] Message: %s\n", num, chatMessage.c_str());
    webSocket.broadcastTXT(String(usernames[num] + ": " + chatMessage).c_str());
  }
}


void broadcastImage(String imgData, int num) {
  webSocket.broadcastTXT(("img:" + imgData).c_str());
}





void loop() {
  server.handleClient();
  webSocket.loop();
  dnsServer.processNextRequest();
}
