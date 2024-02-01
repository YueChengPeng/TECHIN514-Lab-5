#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

const char *ssid = "UW MPSK";
const char *password = "GV%sq{Nx5%";                                               // Replace with your network password
#define DATABASE_URL "https://esp32-firebase-demo-pp-default-rtdb.firebaseio.com/" // Replace with your database URL
#define API_KEY "AIzaSyDKYloRbouid0aoSlyYybzm08rm4SVlkFo"                          // Replace with your API key
#define UPLOAD_INTERVAL 1000                                                       // 1 seconds each upload
#define STAGE_INTERVAL 15000                                                       // 15 seconds each stage
#define SLEEP_INTERVAL 45000                                                       // 45 seconds sleep stage

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// HC-SR04 Pins
const int trigPin = D2;
const int echoPin = D3;

// Define sound speed in cm/usec
const float soundSpeed = 0.034;

// Function prototypes
float measureDistance();
void connectToWiFi();
void initFirebase();
void sendDataToFirebase(float distance);

bool isWiFiConnected = false;

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // we start with the ultrasonic sensor only
  Serial.println("Measuring distance for 15 seconds...");
  unsigned long startTime = millis();
  while (millis() - startTime < STAGE_INTERVAL)
  {
    float distance = measureDistance();

    // If an object is detected, turn on WiFi and send data to Firebase
    if (distance < 10)
    {
      Serial.println("Object detected");
      if (isWiFiConnected == false)
      {
        connectToWiFi();
        initFirebase();
        isWiFiConnected = true;
        sendDataToFirebase(distance);
      }
      else
      {
        sendDataToFirebase(distance);
      }
    }
    delay(100); // Delay between measurements
  }

  // Go to deep sleep for 45 seconds
  Serial.println("Going to deep sleep for 45 seconds...");
  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL * 1000); // in microseconds
  esp_deep_sleep_start();
}

void loop()
{
}

float measureDistance()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi()
{
  // Print the device's MAC address.
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase()
{
  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance)
{
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > UPLOAD_INTERVAL || sendDataPrevMillis == 0))
  {
    sendDataPrevMillis = millis();
    // Write an Float number on the database path test/float
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance))
    {
      Serial.println("PASSED");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: ");
      Serial.println(fbdo.dataType());
    }
    else
    {
      Serial.println("FAILED");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
    count++;
  }
}