#include <Stepper.h> //stpper library//
#include <Adafruit_SSD1306.h>// OLED display library//
#include <Adafruit_GFX.h>
#include <Keypad.h>//Keypad library//
#include <Wire.h>//communication library//

//Configure OLED//
#define DISP_WIDTH 128
#define DISP_HEIGHT 64
#define DISP_RST -1
#define DISP_ADDRS 0x3C
Adafruit_SSD1306 display(DISP_WIDTH, DISP_HEIGHT, &Wire, DISP_RST);

//Configure Keypads//
const byte ROWS=4;
const byte COLS=4;
char hexakeys[ROWS][COLS]=
{
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS]={13, 12, 11, 10}; 
byte colPins[COLS]={5, 4, 3, 2};
Keypad Keys=Keypad(makeKeymap(hexakeys), rowPins, colPins, ROWS, COLS);

//Configure stepper//
const int stepPR=500;
Stepper myStepper(stepPR, 6, 7, 8, 9);

String masterPIN = "1538";
String inputPIN = "";
unsigned long SET_T = 0;       // Holds time duration in milliseconds
unsigned long runStartTime = 0; // Tracks when the milling cycle began

// Strict Workflow States
enum SystemState { 
  INIT_WELCOME, 
  ASK_PIN, 
  ACCESS_GRANTED, 
  SELECT_DURATION, 
  RUNNING_MOTOR, 
  WORK_COMPLETE 
};
SystemState currentState = INIT_WELCOME;


void setup() 
{
  Serial.begin(9600);
  if(!display.begin(SSD1306_SWITCHCAPVCC, DISP_ADDRS))
  {
    Serial.println(F("display not found"));
    for(;;);
  }
}

void loop() 
{
 char key = Keys.getKey();

  switch (currentState) {
    
    case INIT_WELCOME:
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 20);
      display.println(F("WELCOME TO MILL"));
      display.display();
      
      // Hardware text right scroll animation
      display.startscrollright(0x00, 0x0F);
      delay(3000);
      display.stopscroll();
      
      // Move directly to step 3
      currentState = ASK_PIN;
      showPinScreen();
      break;

    case ASK_PIN: //Here the system ask user for PIN:
      if (key) {
        if (key == '#' || key == 'D') {
          verifyEnteredPIN();
        } else if (key == '*') {
          inputPIN = "";
          showPinScreen();
        } else if (inputPIN.length() < 4) {
          inputPIN += key;
          renderScreenText(getMaskedString(inputPIN.length()), 25, 2); 
        }
      }
      break;

    case ACCESS_GRANTED:
      // Display GRANTED for 3 seconds
      renderScreenText("GRANTED", 25, 2);
      
      // Spin access indicator mechanism (90 steps out, 90 steps back)
      myStepper.setSpeed(50);
      myStepper.step(90); 
      delay(200);
      myStepper.setSpeed(50);
      myStepper.step(-90);
      delay(2800); // Balance remaining time
      
      // Display MILL IS READY for 2 seconds
      renderScreenText("MILL IS READY", 20, 1);
      delay(2000);
      
      // Move to step 5
      currentState = SELECT_DURATION;
      showDurationMenu();
      break;

    case SELECT_DURATION:
      if (key) {
        // Map user input keys to exact testing durations (Minutes * Seconds * Milliseconds)
        if (key == '1') { SET_T = 1.0 * 60 * 1000UL; startMillingCycle("1 MIN"); }
        else if (key == '2') { SET_T = 2.0 * 60 * 1000UL; startMillingCycle("2 MIN"); }
        else if (key == '3') { SET_T = 3.0 * 60 * 1000UL; startMillingCycle("3 MIN"); }
        else if (key == '4') { SET_T = 4.0 * 60 * 1000UL; startMillingCycle("4 MIN"); }
      }
      break;

    case RUNNING_MOTOR:
      // Keep moving the motor while the active countdown timer has time left
      if (millis() - runStartTime < SET_T) {
        myStepper.setSpeed(30);
        myStepper.step(-10); // Continuous step increments
      } else {
        // Time is up! Stop the motor and transition
        currentState = WORK_COMPLETE;
      }
      break;

    case WORK_COMPLETE:
      // Final message clean display
      renderScreenText("READY TO\n WORK", 15, 2);
      delay(5000); // Leave it up on screen before returning to main welcome screen
      
      // Reset variables for next run
      inputPIN = "";
      SET_T = 0;
      currentState = INIT_WELCOME;
      break;
  }
}

// Visual helpers
void showPinScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 20);
  display.println(F("ENTER PIN"));
  display.display();
}

void showDurationMenu() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("SELECT TIME OPTION:"));
  display.println(F("-------------------"));
  display.println(F("1: 1.0 Mins"));
  display.println(F("2: 2.0 Mins"));
  display.println(F("3: 3.0 Mins"));
  display.println(F("4: 4.0 Mins"));
  display.display();
}

void renderScreenText(String message, int cursorX, int textSize) {
  display.clearDisplay();
  display.setTextSize(textSize);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(cursorX, 20);
  display.println(message);
  display.display();
}

String getMaskedString(int length) {
  String mask = "";
  for(int i=0; i<length; i++) mask += "*";
  return mask;
}

void verifyEnteredPIN() {
  if (inputPIN == masterPIN) {
    currentState = ACCESS_GRANTED;
  } else {
    renderScreenText("DENIED!", 25, 2);
    delay(2000);
    inputPIN = "";
    showPinScreen();
  }
}

void startMillingCycle(String selectedLabel) {
  renderScreenText("RUNNING...\n" + selectedLabel, 10, 2);
  runStartTime = millis(); // Save current execution time checkpoint
  currentState = RUNNING_MOTOR;
}
