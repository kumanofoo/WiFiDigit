/*
  Display numbers on LED matrix over HTTP for Arduino UNO R4 WiFi

  This sketch will print the IP address of your WiFi module (once connected)
  to the Serial Monitor. From there, you can send HTTP requests to that address
  to display and blink numbers.

  If the IP address of your board is yourAddress:
  $ curl -X PUT http://yourAddress/2digit/-6 display -6 on LED matrix
  $ curl -X PUT http://yourAddress/blink/1000.10 blink number
 */
#define VERSION "0.2.0"
#include "WiFiS3.h"
#include "Arduino_LED_Matrix.h"

#include "LEDMatrixDigit.h"

#if !defined(SECRET_SSID) || !defined(SECRET_PASS)
#include "arduino_secrets.h"
#endif

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;                 // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);

ArduinoLEDMatrix matrix;
LEDMatrixDigit font;

#define BLINK_DURATION 1000
bool blink = false;
bool blink_flipped = false;
uint16_t blink_on = BLINK_DURATION;
uint16_t blink_off = BLINK_DURATION;

String GET_VERSION = "/version";
String PUT_BITMAP = "/bitmap/";
String PUT_2DIGIT = "/2digit/";
String PUT_BLINK = "/blink/";
String PUT_BLINK_FLIPPED = "/blink-flipped/";
String PUT_BAR = "/bar/";
String PUT_BAR_FLIPPED = "/bar-flipped/";
String PUT_UPSIDE_DOWN = "/upside-down/";

void setup() {
  Serial.begin(9600);      // initialize serial communication
  font.begin(matrix);
  matrix.begin();
  
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(ssid);                   // print the network name (SSID);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();                           // start the web server on port 80
  printWifiStatus();                        // you're connected now, so print out the status

  //
  // Test Pattern
  //
#ifdef _TEST_PATTERN_
  font.testPattern(); for (;;);
#endif /* _TEST_PATTERN_ */
  
  uint16_t bitmap[8];
  // bitmap
  // 001100011000
  // 010010100100
  // 010001000100
  // 001000001000
  // 000100010000
  // 000010100000
  // 000001000000
  // 000000000000
  //
  // frame
  // 0011 0001 1000 0100 1010 0100 0100 0100
  //    3    1    8    4    a    4    4    4
  // 0100 0010 0000 1000 0001 0001 0000 0000
  //    4    2    0    8    1    1    0    0
  // 1010 0000 0000 0100 0000 0000 0000 0000
  //    a    0    0    4    0    0    0    0
  //
  String frame = "3184a44442081100a0040000";
  if (frame2bitmap(frame, bitmap)) {
    font.loadBitmap(bitmap);
    blink = true;
    blink_on = 1000;
    blink_off = 50;
  }
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("new client");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String method = "";
    String target = "";
    String version = "";
    int startLineIndex = 0;
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out to the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          bool completed = false;
          if (currentLine.length() == 0) {
            String response = "";
            if (method == "GET") {
              response = get_method(target);
            }
            if (method == "PUT") {
              response = put_method(target);
            }
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            if (response == "") {
              client.println("HTTP/1.1 404 Not Found");
              response = target.substring(0);
            }
            else {
              client.println("HTTP/1.1 200 OK");
            }
            client.print("Content-Length: "); client.println(response.length());
            if (response.length() != 0) {
              client.println("Content-type: text/plain");
            }
            client.println();

            // the content of the HTTP response follows the header:
            client.print(response);
            
            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            if (startLineIndex < 2) {
              client.println("HTTP/1.1 400 Bad Request");
              client.println("Content-Length: 0");
              client.println();
              break;
            }
            if (startLineIndex == 2) {
              startLineIndex++; // stop analyzing start-line in HTTP Requests
            }
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
          if (startLineIndex < 3) {
            if (c == ' ') {
              startLineIndex++;
            }
            else {
              if (startLineIndex == 0) {
                method += c;
              }
              if (startLineIndex == 1) {
                target += c;
              }
              if (startLineIndex == 2) {
                version += c;
              }
            }
          }
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
  // blink numbers if blink is true:
  if (blink) {
    if (font.blink() == 0) {
      delay(blink_on);
    }
    else {
      delay(blink_off);
    }
  }
  // blink flipped numbers if blink=flipped is true:
  if (blink_flipped) {
    if (font.blinkInvert() == 0) {
      delay(blink_on);
    }
    else {
      delay(blink_off);
    }
  }
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
}

bool isInteger(String str) {
  if (str.length() == 0) return false;

  bool is_integer = true;
  int start_index = 0;
  if (str.charAt(0) == '-' || str.charAt(0) == '+') {
    if (str.length() == 1) return false;
    start_index = 1;
  }
  for (int i = start_index; i < str.length(); i++) {
    if (!isDigit(str.charAt(i))) {
      is_integer = false;
      break;
    }
  }
  return is_integer;
}

void setBlinkDuration(String param) {
  blink_on = BLINK_DURATION;
  blink_off = BLINK_DURATION;
  int period = param.length();
  for (int i = 0; i < param.length(); i++) {
    if (param.charAt(i) == '.') {
      period = i;
      break;
    }
  }
  String on_duration = "";
  String off_duration = "";
  on_duration = param.substring(0, period);
  if (period > 0 && period < param.length() - 1) {
    off_duration =param.substring(period+1);
  }
  if (isInteger(on_duration)) {
    blink_on = on_duration.toInt();
    blink_off = blink_on;
  }
  if (isInteger(off_duration)) {
    blink_off = off_duration.toInt();
  }
}

int32_t hexAtoi(String hex) {
  if (hex.length() > 4) return -1;
  int32_t ret = 0;
  for (int i = 0; i < hex.length(); i++) {
    ret <<= 4;
    char c = hex.charAt(i);
    if (c >= 'A' && c <= 'F') {
      ret += c - 'A' + 10;
    }
    else if (c >= 'a' && c <= 'f') {
      ret += c - 'a' + 10;
    }
    else if (c >= '0' && c <= '9') {
      ret += c - '0';
    }
    else {
      return -1;
    }
  }
  return ret;
}

bool frame2bitmap(String frame, uint16_t bitmap[MATRIX_HEIGHT]) {
  if (frame.length() != 24) return false;

  int bitmap_index = 0;
  int w = 12;
  int h = 8;
  for (int i = 0; i < 24; i += 3) {
    bitmap[bitmap_index++] = hexAtoi(frame.substring(i, i+3));
  }
  return true;
}

String get_method(String target) {

  // Get the version number
  //
  // GET /version/
  if ((target + '/').startsWith(GET_VERSION)) {
    if (target.length() <= GET_VERSION.length() + 1) {
      return VERSION;
    }
  }
  return "";
}

String put_method(String target) {

  // Display the screen from the bitmap data.
  //
  // PUT /bitmap/[96-bit data in hexadecimal format]
  if (target.startsWith(PUT_BITMAP)) {
    String frame = target.substring(PUT_BITMAP.length());
    uint16_t bitmap[3];
    if (frame2bitmap(frame, bitmap)) {
      font.loadBitmap(bitmap);
      blink = false;
      blink_flipped = false;
      return target;
    }
  }

  // BLINK the matrix LED.
  //
  // PUT /blink/
  if (target.startsWith(PUT_BLINK)) {
    String param = target.substring((PUT_BLINK.length()));
    setBlinkDuration(param);
    blink_flipped = false;
    blink = true;
    return target;
  }

  // BLINK the matrix LED with inverted.
  //
  // PUT /blink-flipped/
  if (target.startsWith(PUT_BLINK_FLIPPED)) {
    String param = target.substring((PUT_BLINK_FLIPPED.length()));
    setBlinkDuration(param);
    blink = false;
    blink_flipped = true;
    return target;
  }

  // Display two digit.
  //
  // PUT /2digit/[NUMBER]
  if (target.startsWith(PUT_2DIGIT)) {
    String signed_int = target.substring(PUT_2DIGIT.length());
    if (isInteger(signed_int)) {
      int n = signed_int.toInt();
      if (n < 100 && n > -100) {
        blink = false;
        blink_flipped = false;
        font.animatedClear();
        font.print(signed_int.toInt());
        return target;
      }
    }
  }

  // Display the bar.
  //
  // PUT /bar/[LENGTH OF BAR]
  if (target.startsWith(PUT_BAR)) {
    String signed_int = target.substring(PUT_BAR.length());
    if (isInteger(signed_int)) {
      int length = signed_int.toInt();
      int row = 7;
      if (length >= 0 && length <= 12) {
        for (int i = 0; i < length; i++) {
          font.setDot(i, row, true);
        }
        for (int i = length; i < MATRIX_WIDTH; i++) {
          font.setDot(i, row, false);
        }
        return target;
      }
    }
  }

  // Display the bar in LED off.
  //
  // PUT /bar-flipped/[LENGTH OF BAR]
  if (target.startsWith(PUT_BAR_FLIPPED)) {
    String signed_int = target.substring(PUT_BAR_FLIPPED.length());
    if (isInteger(signed_int)) {
      int length = signed_int.toInt();
      int row = 0;
      if (length >= 0 && length <= 12) {
        for (int i = 0; i < length; i++) {
          font.setDot(i, row, false);
        }
        for (int i = length; i < MATRIX_WIDTH; i++) {
          font.setDot(i, row, true);
        }
        return target;
      }
    }
  }

  // Display the screen upside down.
  //
  // PUT /upside-down/true|false
  if (target.startsWith(PUT_UPSIDE_DOWN)) {
    String upside_down = target.substring(PUT_UPSIDE_DOWN.length());
    if (upside_down == "true") {
      font.upsideDown(true);
      return target;
    }
    if (upside_down == "false") {
      font.upsideDown(false);
      return target;
    }
  }

  return "";
}
