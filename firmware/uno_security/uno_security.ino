/*
 * Smart Home — Arduino Uno security controller
 * Bachelor's thesis, Faculty of Robotics, Politehnica University of Timisoara, 2022.
 *
 * Board:   Arduino Uno
 * Inputs:  4x4 keypad, PIR motion sensor, magnetic reed contact
 * Outputs: 16x2 I2C LCD, piezo buzzer, alarm LED
 *
 * The system has three states. DISARMED is idle. Pressing * asks to arm:
 * if the door is already shut the system goes straight to ARMED, otherwise it
 * waits in IN_PROGRESS and shows "Close the door." until the contact closes.
 * From ARMED, motion or an opened door raises the alarm; typing the code
 * followed by # stands the system down.
 *
 * The access code lives in config.h, which is NOT committed.
 * Copy config.example.h to config.h and set your own.
 */

#include "config.h"

#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
bool doInitilizeLCD = true;

// --- keypad -----------------------------------------------------------
const byte ROWS = 4;
const byte COLS = 4;

char hexaKeys[ROWS][COLS] = {
  { '1', '2', '3', 'A' },
  { '4', '5', '6', 'B' },
  { '7', '8', '9', 'C' },
  { '*', '0', '#', 'D' }
};

byte rowPins[ROWS] = { 9, 8, 7, 6 };
byte colPins[COLS] = { 5, 4, 3, 2 };

Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

char customKey;
char accessCode[] = ACCESS_CODE;
char keyPadInput[] = "    ";
unsigned char inputCounter = 0;

// --- other I/O --------------------------------------------------------
#define Sensor Tx            // reed contact input

int ledPin = 11;             // alarm LED
int pirPin = 10;             // PIR motion sensor
int pirValue;
int buzzer = 12;             // piezo buzzer

boolean shouldBeAlerting = false;

// --- state ------------------------------------------------------------
enum ArmedStates
{
  ARMED,
  DISARMED,
  IN_PROGRESS
};

ArmedStates armedState = DISARMED;
int relayNC = false;         // true while the door reads as shut
bool debug = false;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
    ;
  Serial.println("ready");

  pinMode(buzzer, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(pirPin, INPUT);
  pinMode(Sensor, INPUT);

  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
}

void loop()
{
  initilizeLCD();
  handleKeyPadInput();
  handleInProgress();
  handlePIR();
  handleRelay();
  alert();
}

// Leave the waiting state as soon as the door actually closes.
void handleInProgress()
{
  if (armedState != IN_PROGRESS)
  {
    return;
  }
  if (relayNC)
  {
    armedState = ARMED;
    handleLCD(true);
  }
}

const char *getStateString(enum ArmedStates armedState)
{
  switch (armedState)
  {
    case ARMED:
      return "ARMED";
    case DISARMED:
      return "DISARMED";
  }
}

void initilizeLCD()
{
  if (doInitilizeLCD)
  {
    doInitilizeLCD = false;
    lcd.clear();
    String a = getStateString(armedState);

    if (debug)
    {
      Serial.println("armedState: " + a);
    }

    lcd.setCursor(0, 0);
    lcd.print(a);
    lcd.setCursor(0, 1);
    lcd.print("PRESS * to ARM.");
  }
}

void printErrorMessage()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Error ");
  lcd.setCursor(0, 1);
  lcd.print("Close the door.");
}

void handleLCD(bool shouldClearLCD)
{
  if (shouldClearLCD)
  {
    lcd.clear();
  }

  String a = getStateString(armedState);

  if (debug)
  {
    Serial.println("armedState: " + a);
  }

  lcd.setCursor(0, 0);
  lcd.print(a);
  lcd.setCursor(0, 1);
  lcd.print("DISARM KEY:");

  uint8_t cursorStart = 11;
  lcd.setCursor(cursorStart, 1);
  lcd.cursor();

  // overwrite the input positions with blank space or the keys typed so far
  for (int i = 0; i < 4; i++)
  {
    if (keyPadInput[i] == 'z')
    {
      lcd.setCursor(cursorStart + i, 1);
      lcd.print(" ");
    }
    else
    {
      lcd.setCursor(cursorStart + i, 1);
      lcd.print(keyPadInput[i]);
    }
  }
}

void handleKeyPadInput()
{
  customKey = customKeypad.getKey();
  if (!customKey)
  {
    return;
  }

  switch (armedState)
  {
    case DISARMED:
      if (customKey == '*')
      {
        if (!relayNC)
        {
          printErrorMessage();
          armedState = IN_PROGRESS;
        }
        else
        {
          armedState = ARMED;
          handleLCD(true);
        }
      }
      break;

    default:
      if (customKey == '*')
      {
        resetCodeInput();
      }
      else if (customKey == '#')
      {
        if (strcmp(keyPadInput, accessCode) == 0)
        {
          Serial.println("Keypad input matches access code");
          resetCodeInput();
          armedState = DISARMED;
          doInitilizeLCD = true;
          initilizeLCD();
        }
        else
        {
          resetCodeInput();
        }
      }
      else
      {
        if (inputCounter <= 3)
        {
          keyPadInput[inputCounter] = customKey;
          inputCounter++;
          handleLCD(true);
        }
      }
      break;
  }
}

void resetCodeInput()
{
  Serial.println("reseting keyPadInput");
  for (int i = 0; i < 4; i++)
  {
    keyPadInput[i] = 'z';
  }
  inputCounter = 0;
  handleLCD(true);
}

void handlePIR()
{
  pirValue = digitalRead(pirPin);
}

void handleRelay()
{
  relayNC = digitalRead(Sensor);
}

void alert()
{
  if (armedState == DISARMED)
  {
    shouldBeAlerting = false;
  }

  // armed and the door has been opened
  if (armedState == ARMED && !relayNC)
  {
    shouldBeAlerting = true;
  }

  // armed and something moved inside
  if (pirValue > 0 && armedState == ARMED)
  {
    shouldBeAlerting = true;
  }

  if (shouldBeAlerting)
  {
    Serial.println("shouldBeAlerting");
  }

  handleLed();
  handleBuzz();
}

void handleLed()
{
  if (shouldBeAlerting)
  {
    Serial.println("setting LED HIGH");
    digitalWrite(ledPin, HIGH);
  }
  else
  {
    digitalWrite(ledPin, LOW);
  }
}

void handleBuzz()
{
  if (!shouldBeAlerting)
  {
    return;
  }

  // two tones, alternating, so the alarm is not a flat beep
  for (unsigned char i = 0; i < 20; i++)
  {
    digitalWrite(buzzer, HIGH);
    delay(1);
    digitalWrite(buzzer, LOW);
    delay(1);
  }

  for (unsigned char i = 0; i < 20; i++)
  {
    digitalWrite(buzzer, HIGH);
    delay(2);
    digitalWrite(buzzer, LOW);
    delay(2);
  }
}
