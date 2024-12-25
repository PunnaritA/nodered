#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <stdio.h>
#include <string.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

//WiFi credentials
const char* ssid = "hontouuuu";
const char* password = "123456789";
//const char* ssid = "AAPA-2.4G";
//const char* password = "Athpun4062";

//MQTT Broker
const char* mqtt_broker = "202.44.12.37";
//const char* mqtt_broker = "192.168.1.50"; // Local Test
const char* topic1 = "sensor_data";
const char* topic2 = "get_money";
const char* topic3 = "cup_data";
const char* topic4 = "Receive_Image"; // Topic to receive RGB565 image data
const char* topic5 = "call_image";
const char* mqtt_username = "student";
const char* mqtt_password = "idealab2024";
const int mqtt_port = 1883;

//Declare PINs setting
const int ButtonCallQRCode = 36;//Working
const int OutputPin = 12; //Working
const int InputPin = 14; //Working
const int SensorWater = 13; //Workinga
const int SensorPowderType1 = 32; //Working
const int SensorPowderType2 = 22; //Working
const int SensorPowderType3 = 33; //Working
const int MicroSwitchCup = 4; //Working
const int CountType1 = 39; //Working
const int CountType2 = 34; //Working
const int CountType3 = 35; //Working

// Pin configurations for TFT display
#define TFT_CS    5 //Working
#define TFT_RST   16 //Working
#define TFT_DC    17 //Working

// Create the display object
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

//Declare Status variables
int StateButtonCallQRCode = 1;
int StateCountType1 = 0;
int StateCountType2 = 0;
int StateCountType3 = 0;
int StateCoin = 1;
int StateMicroSwitchCup = 1;

//Declare Control loop variables
unsigned long CurrentTime ;
unsigned long ControlLoopButtonQR = 0;
unsigned long ControlLoopPaymentTime = 0;
unsigned long ControlLoopSensorTime = 0;
unsigned long ControlLoopSentWantQRTime = 0;
unsigned long ControlLoopTypeTime = 0;
unsigned long ControlLoopSentDataTime = 0;
unsigned long TimeBlockStartTimeButtonQR = 0;
unsigned long TimeBlockStartTimeType = 0;
unsigned long TimeBlockStartTimePayment = 0;
unsigned long DiffBlockTimeButtonQR = 0;
unsigned long DiffBlockTimeType = 0;
unsigned long DiffBlockTimePayment = 0;
const long PaymentInterval = 10 ; // Interval of Payment function for 100 Hz loop (T = 0.01 s)
const long ButtonQRInterval = 100 ; // Interval of ButtonQR function for 10 Hz loop (T = 0.1 s)
const long SensorInterval = 100 ; // Interval of Sensor function for 10 Hz loop (T = 0.1 s)
const long SentWantQRInterval = 1000 ; //Interval of SentWantQR function for 1 Hz loop (T = 1 s)
const long TypeInterval = 100 ; // Interval of TypeDetection function for 10 Hz loop (T = 0.1 s)
const long SentDataInterval = 3000 ; //Interval of SentData function for 0.33 Hz loop (T = 3 s)
bool TimeBlockButtonQR = false ;
bool TimeBlockType = false ;
bool TimeBlockPayment = false ;
bool PermissionToReplaceCups = true;
bool PermissionToCallImage = false;

//Declare Variables
int NumberType1 = 0;
int NumberType2 = 0;
int NumberType3 = 0;
int CupRemain = 50;
String x = "0.00";
int y ;
int previousNumber = 11;
int ResultSensorWater,ResultSensorPowderType1,ResultSensorPowderType2,ResultSensorPowderType3;
char payload1[8] = {0};
char payload2[18] = {0};

//Declare function
void setup_wifi();
void setup_broker();
void callback(char *topic, byte *payload, unsigned int length);
void displayImage(uint8_t* imageData, int width, int height);
void CallImage();
void payment();
void sensor();
void type_detection();
void sentdata();
void GenerateRandomNumber();
void SentWantQR();

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  // Initialize WiFi and MQTT
  setup_wifi();
  setup_broker();
  pinMode(SensorWater, INPUT);
  pinMode(SensorPowderType1, INPUT);
  pinMode(SensorPowderType2, INPUT);
  pinMode(SensorPowderType3, INPUT);
  pinMode(ButtonCallQRCode, INPUT);
  pinMode(MicroSwitchCup, INPUT_PULLUP);
  pinMode(InputPin, INPUT);
  pinMode(OutputPin, OUTPUT);
  pinMode(CountType1, INPUT);
  pinMode(CountType2, INPUT);
  pinMode(CountType3, INPUT);
  client.subscribe(topic1);
  client.subscribe(topic2);
  client.subscribe(topic3);
  client.subscribe(topic4);
  client.subscribe(topic5);
   // Display the defined MQTT_MAX_PACKET_SIZE
  Serial.printf("MQTT_MAX_PACKET_SIZE is set to: %d bytes\n", MQTT_MAX_PACKET_SIZE);
  // Initialize the TFT display
  tft.initR(INITR_BLACKTAB); // Initialize with a black tab
  tft.fillScreen(ST7735_BLACK); // Clear the screen
  //tft.setRotation(1); // Optional: set rotation
}

void setup_wifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  int retry_count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry_count++;
    if (retry_count >= 60) { // 30 seconds timeout
      Serial.println("Failed to connect to WiFi");
      return;
    }
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_broker() {
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT broker with client ID: %s\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("MQTT broker connected");
    } else {
      Serial.print("MQTT connect failed, state: ");
      Serial.println(client.state());
      delay(1000);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {
  if (String(topic) == "get_money") {
    x = "";
    for (unsigned int i = 0; i < length; i++) {
      x += (char)payload[i];
    }
    x.trim();  // Remove any leading or trailing whitespace
    Serial.print("Received payment: ");
    Serial.println(x);
  }
  if (String(topic) == "Receive_Image"){
    Serial.println("Image received!");

    // Calculate the size of the incoming image
    int imageSize = length / 2; // Each pixel is 2 bytes in RGB565
    int incomingWidth = sqrt(imageSize); // Assume square images
    int incomingHeight = imageSize / incomingWidth;

    // Validate that the image dimensions are not larger than the display
    if (incomingWidth > 128 || incomingHeight > 160) {
        Serial.println("Error: Image dimensions exceed the display size of 128x160");
        return;
    }

    Serial.printf("Incoming Image Dimensions: %dx%d\n", incomingWidth, incomingHeight);

    // Clear the screen by filling it with black
    tft.fillScreen(ST7735_BLACK);

    // Define starting offsets to center the image
    int xOffset = (128 - incomingWidth) / 2; // Horizontal offset
    int yOffset = (160 - incomingHeight) / 2; // Vertical offset

    // If the image doesn't fully cover the display, pad the edges with black
    // Start drawing the image with the black-filled background
    int idx = 0;
    for (int y = 0; y < 160; y++) { // Full screen height
        for (int x = 0; x < 128; x++) { // Full screen width
            if (x >= xOffset && x < xOffset + incomingWidth &&
                y >= yOffset && y < yOffset + incomingHeight) {
                // If within the image area, use the pixel color
                uint16_t color = (payload[idx] << 8) | payload[idx + 1]; // Combine bytes into a 16-bit color
                tft.drawPixel(x, y, color);
                idx += 2; // Advance to the next pixel
            } else {
                // Fill missing pixels with black
                tft.drawPixel(x, y, ST7735_BLACK);
            }
        }
    }
    Serial.println("Image displayed with missing pixels filled as black.");
  }
}

void GenerateRandomNumber(){
  do {
    y = random(0, 10); // Generate a random number between 0 and 9
  } while (y == previousNumber);
  previousNumber = y; // Update the previous number
}

void ButtonQR(){
  StateButtonCallQRCode = digitalRead(ButtonCallQRCode);
   if (StateButtonCallQRCode ==LOW) {
    Serial.println("Call QR Code Image");
    GenerateRandomNumber();
    PermissionToCallImage = true;
    TimeBlockButtonQR = true;
   }
}

void displayImage(uint8_t* imageData, int width, int height) {
  // Assume imageData contains raw RGB565 data
  int idx = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint16_t color = (imageData[idx] << 8) | imageData[idx + 1]; // Combine bytes into a 16-bit color
      tft.drawPixel(x, y, color);
      idx += 2; // Advance to the next pixel
    }
  }
}

void payment() {
    StateCoin = digitalRead(InputPin);
    if (StateCoin == LOW) {
    digitalWrite(OutputPin, HIGH);
    Serial.println("Coin inserted");
    TimeBlockPayment = true;
  } else if (x == "10.00") {
    digitalWrite(OutputPin, HIGH);
    Serial.println("QR payment done");
    TimeBlockPayment = true;
    // Clear the TFT screen
    tft.fillScreen(ST7735_BLACK);

    // Set text properties
    tft.setTextColor(ST7735_WHITE);
    tft.setTextSize(2); // Adjust the text size as needed

    // Calculate text width and height
    int16_t x1, y1;
    uint16_t textWidth, textHeight;
    tft.getTextBounds("Payment Successful!", 0, 0, &x1, &y1, &textWidth, &textHeight);

    // Calculate the centered position
    int16_t centerX = (tft.width() - textWidth) / 2;
    int16_t centerY = (tft.height() - textHeight) / 2;

    // Display the text
    tft.setCursor(centerX, centerY);
    tft.println(" Payment  Successful");

    x = "0.00"; // Reset the payment status
  } else {
    digitalWrite(OutputPin, LOW);
  }
}

void sensor() {
  StateMicroSwitchCup = digitalRead(MicroSwitchCup);
  ResultSensorWater = digitalRead(SensorWater);
  ResultSensorPowderType1 = digitalRead(SensorPowderType1);
  ResultSensorPowderType2 = digitalRead(SensorPowderType2);
  ResultSensorPowderType3 = digitalRead(SensorPowderType3);
  if (StateMicroSwitchCup == LOW) {
    CupRemain = 0;
    PermissionToReplaceCups  = true ;
    Serial.println("No more cups available");
  } else if (StateMicroSwitchCup == HIGH) {
    if (PermissionToReplaceCups) {
    CupRemain = 50 ;
    Serial.println("Cups are ready!");
    PermissionToReplaceCups  = false ;
    }   
  } 
}

void type_detection(){
  StateCountType1 = digitalRead(CountType1);
  StateCountType2 = digitalRead(CountType2);
  StateCountType3 = digitalRead(CountType3);
  if (StateCountType1  == HIGH) {
    TimeBlockType = true;
    CupRemain -= 1;
    NumberType1 += 1;
  } else if (StateCountType2 == HIGH) {
     TimeBlockType = true;
     CupRemain -= 1;
     NumberType2 += 1;
  } else if (StateCountType3 == HIGH) {
     TimeBlockType = true;
     CupRemain -= 1;
     NumberType3 += 1;
  }
}

void SentWantQR(){
  if (PermissionToCallImage  == true) {
    PermissionToCallImage = false;
    char buffer[2];
    snprintf(buffer, sizeof(buffer), "%d", y);
    client.publish(topic5, buffer);
  }
}

void sentdata(){
  snprintf(payload1, sizeof(payload1),"%d,%d,%d,%d", ResultSensorWater, ResultSensorPowderType1, ResultSensorPowderType2, ResultSensorPowderType3);
  snprintf(payload2, sizeof(payload2),"%d,%d,%d,%d", NumberType1, NumberType2, NumberType3, CupRemain);
  //Serial.println(payload1);
  //Serial.println(payload2);
  client.publish(topic1, payload1);
  client.publish(topic3, payload2);
}

void loop() {
  CurrentTime = millis();  // Get current time in milliseconds
  
  //check connection of MQTT
  if (!client.connected()) {
    setup_broker();
  }
  client.loop();
  
  //Control loop of Payment 100 Hz (work every 0.01 s) if detect delay 0.3 s
  if (CurrentTime - ControlLoopPaymentTime >= PaymentInterval) { 
     ControlLoopPaymentTime = CurrentTime ;
    if (not TimeBlockPayment){      //Work when TimeBlockPayment = false , Always work this at first time or after reset
     TimeBlockStartTimePayment = CurrentTime ;
     payment();
    }else{ //Work when TimeBlock = true , which only happen after detect coin
     DiffBlockTimePayment = CurrentTime - TimeBlockStartTimePayment ;
     if (DiffBlockTimePayment >= 300){      //Reset all once time pass 0.3 s
      TimeBlockPayment = false;
     }
    }
  }

   //Control loop of ButtonQR for 10 Hz (work every 0.1 s) if detect delay 0.5 s
  if (CurrentTime - ControlLoopButtonQR >= ButtonQRInterval) {
     ControlLoopButtonQR = CurrentTime ;
    if (not TimeBlockButtonQR){        //Work when TimeBlock = false , Always work this at first time or after reset
     TimeBlockStartTimeButtonQR = CurrentTime ;
     ButtonQR();
    }else{                         //Work when TimeBlock = true , which only happen after detect type
     DiffBlockTimeButtonQR  = CurrentTime - TimeBlockStartTimeButtonQR ;
     if (DiffBlockTimeButtonQR >= 500){      //Reset all once time pass 0.5 s
      TimeBlockButtonQR = false;
     }
    }
  }
  
  //Control loop of Sensor 10 Hz (work every 0.1 s) 
  if (CurrentTime - ControlLoopSensorTime >= SensorInterval) {
    ControlLoopSensorTime  = CurrentTime ;
    sensor();
  }
  
  //Control loop of TypeSensor for 10 Hz (work every 0.01 s)  but delay after detect type for 8 second
  if (CurrentTime - ControlLoopTypeTime >= TypeInterval) {
    ControlLoopTypeTime = CurrentTime ;
    if (not TimeBlockType){        //Work when TimeBlock = false , Always work this at first time or after reset
     TimeBlockStartTimeType = CurrentTime ;
     type_detection();
    }else{                         //Work when TimeBlock = true , which only happen after detect type
     DiffBlockTimeType = CurrentTime - TimeBlockStartTimeType ;
     if (DiffBlockTimeType >= 8000){      //Reset all once time pass 8 s
      TimeBlockType = false;
     }
    }
  }

   //Control loop of SentWantQR 1 Hz (work every 1 s) 
  if (CurrentTime - ControlLoopSentWantQRTime >=  SentWantQRInterval) {
    ControlLoopSentWantQRTime = CurrentTime ;
    SentWantQR();
  }
  
  //Control loop of Sendata 0.33 Hz (work every 3 s) 
  if (CurrentTime - ControlLoopSentDataTime >=  SentDataInterval) {
    ControlLoopSentDataTime = CurrentTime ;
    sentdata();
  }
}
