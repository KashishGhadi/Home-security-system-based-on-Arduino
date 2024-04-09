#include <Keypad.h>
#include <SoftwareSerial.h>

SoftwareSerial sim8001(2, 3); // Initialize software serial for communication with SIM800L GSM module

char password[5] = "1234"; // Define the password for system disarm
char enteredPassword[5]; // Array to store the password entered by the user
int passwordIndex = 0; // Index to keep track of the characters entered for the password

const byte rows = 4; // Number of rows in the keypad
const byte cols = 3; // Number of columns in the keypad

char keys[rows][cols] = { // Define the keypad layout
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[rows] = {9, 8, 7, 6}; // Pins connected to the rows of the keypad
byte colPins[cols] = {5, 4, 3};  // Pins connected to the columns of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, rows, cols); // Create Keypad object

#define Motion_Sensor A0 // Pin connected to the motion sensor
#define Sensor_Pin A0 // Pin connected to the motion sensor
bool Sensor_State; // Variable to store the state of the motion sensor

#define alarm 11 // Pin connected to the alarm buzzer
#define redLed A1 // Pin connected to the red LED
int contact = 10; // Pin connected to the alarm button
int val; // Variable to store the value of the alarm button
int ledBlink; // Variable to control LED blinking

int sensorData; // Variable to store the state of the motion sensor
unsigned long currentTime, lastActivationTime; // Variables to store current and last activation time

int activation_delay = 20; // Delay before system activation after arming
int delay_deactivation = 10; // Delay before system deactivation after activation
int before_delay = 10; // Delay before system activation after motion detection

int systemState = 0; // Variable to store the current state of the system

void setup() {
  pinMode(Motion_Sensor, INPUT); // Set motion sensor pin as input
  sim8001.begin(9600); // Initialize communication with SIM800L GSM module
  Serial.begin(9600); // Initialize serial communication
  delay(1000);

  pinMode(alarm, OUTPUT); // Set alarm pin as output
  pinMode(Sensor_Pin, INPUT); // Set motion sensor pin as input
  pinMode(contact, INPUT); // Set alarm button pin as input
  pinMode(redLed, OUTPUT); // Set red LED pin as output
  digitalWrite(contact, HIGH); // Activate internal pull-up resistor for alarm button
  Serial.println("System startup"); // Print startup message
  Serial.println("Alarm button status: "); // Print alarm button status
  Serial.println(digitalRead(contact)); // Print the status of the alarm button
}

void loop() {
  Sensor_State = digitalRead(Motion_Sensor); // Read the state of the motion sensor

  if (Sensor_State == HIGH) { // If motion is detected
    Serial.println("Sensor detected motion"); // Print motion detection message
    delay(200);
    SendSMS(); // Send SMS notification
    delay(4000);
  } else {
    Serial.println(".");
  }

  if (sim8001.available()) { // If data is available from SIM800L GSM module
    Serial.write(sim8001.read()); // Print the data
  }

  currentTime = millis(); // Get the current time
  val = digitalRead(contact); // Read the status of the alarm button

  char key = keypad.getKey(); // Get the pressed key from the keypad
  if (key) {
    handlePassword(key); // Handle keypad input
  }

  if (systemState == 0) { // If system is disarmed
    digitalWrite(redLed, LOW); // Turn off the red LED
    digitalWrite(alarm, LOW); // Turn off the alarm
  } else { // If system is armed
    if (currentTime - lastActivationTime >= activation_delay * 1000) { // If activation delay has passed
      systemState = 0; // Reset system state to disarmed
      digitalWrite(redLed, LOW); // Turn off the red LED
    }

    sensorData = digitalRead(Sensor_Pin); // Read the state of the motion sensor

    if (sensorData == HIGH) { // If motion is detected
      systemState = 1; // Arm the system
      lastActivationTime = currentTime; // Record the activation time
      digitalWrite(redLed, LOW); // Turn off the red LED
    }

    if (systemState == 1 && currentTime - lastActivationTime >= delay_deactivation * 1000) { // If delay before deactivation has passed
      digitalWrite(redLed, HIGH); // Turn on the red LED
      digitalWrite(alarm, HIGH); // Activate the alarm
      lastActivationTime = currentTime; // Record the activation time
    }
  }
}

void SendSMS() {
  Serial.println("Sending SMS..."); // Print sending SMS message
  sim8001.print("AT+CMGF=1\r\n"); // Set SMS mode to text mode
  delay(100);
  sim8001.print("AT+CMGS=\"+919321781545\"\r\n"); // Set recipient phone number
  delay(500);
  sim8001.print("INTRUDER ALERT!! Someone entered the house."); // Send SMS content
  delay(500);
  sim8001.print((char)26); // End SMS
  delay(500);
  Serial.println("Text sent."); // Print SMS sent message
}

void handlePassword(char key) {
  if (key == '#') { // If '#' key is pressed
    enteredPassword[passwordIndex] = '\0'; // Null-terminate the entered password
    if (strcmp(enteredPassword, password) == 0) { // If entered password is correct
      Serial.println("Entry Successful"); // Print success message
      systemState = 0; // Disarm the system
    } else { // If entered password is incorrect
      Serial.println("Wrong password"); // Print wrong password message
      for (int ledBlink = 0; ledBlink < 5; ledBlink++) { // Blink the red LED 5 times
        digitalWrite(redLed, HIGH); // Turn on the red LED
        delay(100);
        digitalWrite(redLed, LOW); // Turn off the red LED
        delay(100);
      }
    }
    passwordIndex = 0; // Reset the password index
  } else { // If any other key is pressed
    enteredPassword[passwordIndex++] = key; // Store the pressed key in the entered password array
    if (passwordIndex >= 4) { // If password length exceeds 4
      passwordIndex = 0; // Reset the password index
    }
  }
}