#include <WiFi.h>
#include <MQUnifiedsensor.h>
#include <NewPing.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <ArduinoJson.h>

#define API_KEY "AIzaSyAQ-M3EXP-cM0GKRWdcErW3NS2ng"
#define FIREBASE_PROJECT_ID "industrial-8ee"

#define USER_EMAIL "talal@gmail.com"
#define USER_PASSWORD "987654321ab"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

#define Board                   "ESP-32"
#define PinMQ2                  34
#define PinMQ135                32
#define TRIGGER_PIN 5
#define ECHO_PIN 18
#define DHTPIN 15      
#define DHTTYPE DHT22
#define PIR_PIN 27

#define MAX_DISTANCE 400

#define Voltage_Resolution      3.3
#define ADC_Bit_Resolution      12
#define RatioMQ2CleanAir        9.83
#define RatioMQ135CleanAir      3.6

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE);
DHT dht(DHTPIN, DHTTYPE);
MQUnifiedsensor MQ2(Board, Voltage_Resolution, ADC_Bit_Resolution, PinMQ2, "MQ-2");
MQUnifiedsensor MQ135(Board, Voltage_Resolution, ADC_Bit_Resolution, PinMQ135, "MQ-135");

const char* ssid     = "Huawei nova 7i";
const char* password = "03623201";

enum GasType {H2, LPG, CO, Alcohol, Propane, CO2, NH3, Toluene, Acetone};
GasType selectedGasMQ2 = H2;
GasType selectedGasMQ135 = CO2;

float getThresholdMQ2(GasType gasType) {
  switch (gasType) {
    case CO:
      return 100; 
    case LPG:
      return 2000; 
    case Alcohol:
      return 500; 
    default:
      return 0;
  }
}

// Function to get threshold value based on selected gas type for MQ-135
float getThresholdMQ135(GasType gasType) {
  switch (gasType) {
    case CO2:
      return 1000; 
    case CO:
      return 50;
    case NH3:
      return 100; 
    default:
      return 0; 
  }
}

int motionDetected = 0;
float temp = 0;
float hum = 0;
int mq2_ppm = 0;
int mq135_ppm = 0;
int distance;
String gass_mq2_name="none";
String gass_mq135_name="none";

void setup() {
  
  Serial.begin(115200);
  delay(10);
  dht.begin();
  pinMode(PIR_PIN, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }
  display.display();
  delay(2000);  
  display.clearDisplay();

  MQ2.setRegressionMethod(1); // _PPM = a*ratio^b
  switch (selectedGasMQ2) {
    case H2:
      MQ2.setA(987.99); MQ2.setB(-2.162);gass_mq2_name="H2";
      break;
    case LPG:
      MQ2.setA(574.25); MQ2.setB(-2.222);gass_mq2_name="LPG";
      break;
    case CO:
      MQ2.setA(36974); MQ2.setB(-3.109);gass_mq2_name="CO";
      break;
    case Alcohol:
      MQ2.setA(3616.1); MQ2.setB(-2.675);gass_mq2_name="Alcohol";
      break;
    case Propane:
      MQ2.setA(658.71); MQ2.setB(-2.168); gass_mq2_name="Propane";
      break;
  }

  // Set math model to calculate the PPM concentration and the value of constants for MQ-135
  MQ135.setRegressionMethod(1); // _PPM = a*ratio^b
  switch (selectedGasMQ135) {
    case CO2:
      MQ135.setA(110.47); MQ135.setB(-2.862); gass_mq135_name="CO2";
      break;
    case CO:
      MQ135.setA(605.18); MQ135.setB(-3.937); gass_mq135_name="CO2";
      break;
    case NH3:
      MQ135.setA(102.2); MQ135.setB(-2.769); gass_mq135_name="NH3";
      break;
    case Alcohol:
      MQ135.setA(44.947); MQ135.setB(-3.445); gass_mq135_name="Alcohol";
      break;
    case Toluene:
      MQ135.setA(34.668); MQ135.setB(-3.369); gass_mq135_name="Toluene";
      break;
    case Acetone:
      MQ135.setA(33.119); MQ135.setB(-3.221); gass_mq135_name="Acetone";
      break;
  }

  
  MQ2.init();
  MQ135.init();

  
  Serial.print("Calibrating MQ-2 please wait.");
  float calcR0MQ2 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ2.update(); 
    calcR0MQ2 += MQ2.calibrate(RatioMQ2CleanAir);
    Serial.print(".");
  }
  MQ2.setR0(calcR0MQ2 / 10);
  Serial.println("  done!");

  
  Serial.print("Calibrating MQ-135 please wait.");
  float calcR0MQ135 = 0;
  for (int i = 1; i <= 10; i++) {
    MQ135.update(); 
    calcR0MQ135 += MQ135.calibrate(RatioMQ135CleanAir);
    Serial.print(".");
  }
  MQ135.setR0(calcR0MQ135 / 10);
  Serial.println("  done!");

 
  if (isinf(calcR0MQ2)) {
    Serial.println("Warning: MQ-2 connection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while (1);
  }
  if (calcR0MQ2 == 0) {
    Serial.println("Warning: MQ-2 connection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while (1);
  }

  
  if (isinf(calcR0MQ135)) {
    Serial.println("Warning: MQ-135 connection issue, R0 is infinite (Open circuit detected) please check your wiring and supply");
    while (1);
  }
  if (calcR0MQ135 == 0) {
    Serial.println("Warning: MQ-135 connection issue found, R0 is zero (Analog pin shorts to ground) please check your wiring and supply");
    while (1);
  }

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;

  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  config.token_status_callback = tokenStatusCallback;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {

  String documentPath = "nodemcu/XW6dN0IdK8GkEXTP3SyO";  

  FirebaseJson content;                

 
  MQ2.update();
  mq2_ppm=MQ2.readSensor();
  mq135_ppm=MQ135.readSensor();
  hum=dht.readHumidity();
  temp=dht.readTemperature();

  Serial.print("MQ-2 PPM: ");
  Serial.println(mq2_ppm);
  if (digitalRead(PIR_PIN) == HIGH) {
    motionDetected = 1;
  } else {
    motionDetected = 0;
  }

  // Update and read from MQ-135 sensor
  MQ135.update();
  Serial.print("MQ-135 PPM: ");
  Serial.println(mq135_ppm);

  distance = sonar.ping_cm();

  // Print the distance to the serial monitor
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  Serial.print("Humidity: ");
  Serial.print(dht.readHumidity());
  Serial.print(" %, Temp: ");
  Serial.print(dht.readTemperature());
  Serial.println(" Celsius");
  
  Serial.print(", Motion: ");
  Serial.print(motionDetected);
  Serial.println(" ");


  // Update OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("MQ-2 PPM: ");
  display.println(mq2_ppm);
  if(mq2_ppm > getThresholdMQ2(selectedGasMQ2) ) {
    display.println("Danger: ");
  }

  display.setCursor(0, 16);
  display.print("MQ-135 PPM: ");
  display.println(mq135_ppm);
  if(mq135_ppm > getThresholdMQ135(selectedGasMQ135)) {
    display.println("Danger: ");
  }
  
  display.setCursor(0, 32);
  display.print("Temp: ");
  display.print(temp);
  display.println(" C");
  if(temp > 35) {
    display.println("Danger: High Temp!");
  }

  display.print("Hum: ");
  display.print(hum);
  display.println(" %");
  if(hum > 70) {
    display.println("Danger: High Humidity!");
  }

  display.setCursor(0, 48);
  display.print("Motion: ");
  display.println(motionDetected);
  if(motionDetected) {
    display.print("Dist: ");
    display.print(distance);
    display.println(" cm");
  }

  display.display();

  if (!isnan(mq2_ppm) && !isnan(mq135_ppm) && !isnan(hum) && !isnan(temp) && !isnan(distance)) {
    
    content.set("fields/mq2/stringValue", String(mq2_ppm));
    content.set("fields/mq135/stringValue", String(mq135_ppm));
    content.set("fields/humidity/stringValue", String(hum));
    content.set("fields/temperature/stringValue", String(temp));
    content.set("fields/motion/stringValue", String(motionDetected));
    content.set("fields/distance/stringValue", String(distance));

    Serial.print("Updating Data... ");

        

    if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "mq2") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "mq135") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "distance") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "motion") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "temperature") && Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "humidity")) {
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      } else {
            Serial.println(fbdo.errorReason());
          }
        } else {
          Serial.println("Failed to read DHT data.");
        }

  delay(500); 

}
