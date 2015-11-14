/*
Sainsmart LCD Shield for Arduino
Key Grab v0.2
Written by jacky

www.sainsmart.com

Displays the currently pressed key on the LCD screen.

Key Codes (in left-to-right order):

None   - 0
Select - 1
Left   - 2
Up     - 3
Down   - 4
Right  - 5

*/

/* #include <LiquidCrystal.h> */
/* #include <DFR_Key.h> */
/*  */
/* //Pin assignments for SainSmart LCD Keypad Shield */
/* LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  */
/* //--------------------------------------------- */
/*  */
/* DFR_Key keypad; */
/*  */
/* int localKey = 0; */
/* String keyString = ""; */
/*                   */
/* void setup()  */
/* {  */
/*   lcd.begin(16, 2); */
/*   lcd.clear(); */
/*   lcd.setCursor(0, 0); */
/*   lcd.print("Key Grab v0.2"); */
/*   delay(2500); */
/*    */
/*   */
/*   OPTIONAL */
/*   keypad.setRate(x); */
/*   Sets the sample rate at once every x milliseconds. */
/*   Default: 10ms */
/*   */
/*   keypad.setRate(10); */
/*  */
/* } */
/*  */
/* void loop()  */
/* {  */
/*    */
/*   keypad.getKey(); */
/*   Grabs the current key. */
/*   Returns a non-zero integer corresponding to the pressed key, */
/*   OR */
/*   Returns 0 for no keys pressed, */
/*   OR */
/*   Returns -1 (sample wait) when no key is available to be sampled. */
/*   */
/*   localKey = keypad.getKey(); */
/*    */
/*   if (localKey != SAMPLE_WAIT) */
/*   { */
/*     lcd.clear(); */
/*     lcd.setCursor(0, 0); */
/*     lcd.print("Current Key:"); */
/*     lcd.setCursor(0, 1); */
/*     lcd.print(localKey); */
/*   } */
/* } */
//
// LLAP - Lightweight Local Automation Protocol
//
// Arduino pinata code for the XinoRF
//

#define deviceID1 'X'
#define deviceID2 'X'

String msg;	  // storage for incoming message
String reply;  // storage for reply

namespace PM {
  typedef struct {
    byte pin : 6;
    byte mode : 1;
    byte state : 1;
  } DigitalPin;

  void set(PM::DigitalPin& pin, boolean mode=INPUT, boolean state=false)
  {
   pin.mode = mode;
   pin.state = state;
   pinMode( pin.pin, pin.mode);
   digitalWrite( pin.pin, pin.state);
  }

  DigitalPin& read(PM::DigitalPin& pin)
  {
    pin.state = digitalRead( pin.pin );
    return pin;
  }

  String power(PM::DigitalPin& pin)
  {
    pin = PM::read(pin);
    if(pin.state) {
      return "1";
    } else {
      return "0";
    }
  }

  boolean changed(PM::DigitalPin& pin)
  {
    return (pin.state != digitalRead( pin.pin ) ? true : false);
  }

  void write(PM::DigitalPin& pin,boolean state)
  {
    pin.state = state;
    digitalWrite( pin.pin, pin.state);
  }

  void toggle(PM::DigitalPin& pin){
    PM::write(pin, !pin.state);
  }
}

PM::DigitalPin outlets[] = {
  { 4, OUTPUT, false },
  { 5, OUTPUT, false },
  { 6, OUTPUT, false },
  { 7, OUTPUT, false },
  { 9, OUTPUT, false },
  { 10, OUTPUT, false },
  { 11, OUTPUT, false },
  { 12, OUTPUT, false },
  { 13, OUTPUT, false }
};

boolean readMsg() {
  if (Serial.read() == 'a') // start of message?
  {
    msg = "a";
    // 11 characters in the message body
    for (byte i=0; i<11; i++) {
      msg += (char)Serial.read();
    }
    return true;
  } else {
    return false;
  }
}

boolean msgForMe() {
  if (msg.charAt(1) == deviceID1 && msg.charAt(2) == deviceID2) {
    return true;
  } else {
    return false;
  }
}

boolean validateOutletNumber(int num) {
  // return isDigit(num) && num < sizeof(outlets) - 1;
  return true;
}

void setReplyToError() {
  reply = reply.substring(0,3) + "ERROR----";
}

void startSequence() {
  for(int i = 0; i < 6; i++) {
    delay(500);
    PM::toggle(outlets[7]);
  }
}

void setup()
{
  int i;
  for(i = 0; i < sizeof(outlets); i++) {
    PM::set(outlets[i], OUTPUT);
  }
  startSequence();

  pinMode(8, OUTPUT);     // initialize pin 8 to control the radio
  digitalWrite(8, HIGH);  // select the radio
  Serial.begin(115200);     // start the serial port at 115200 baud (correct for XinoRF and RFu, if using XRF + Arduino you might need 9600)
  String hdr = hdr + "a" + deviceID1 + deviceID2;         // message header
  Serial.print(hdr + "STARTED");// transmit started packet
}

void loop() // repeatedly called
{
  if (Serial.available() >= 12 && readMsg() && msgForMe()) // have we got enough characters for a message?
  {
    reply = msg;
    msg = msg.substring(3); // removes aXX from the message
    if (msg.compareTo("HELLO----") == 0)
    {
      ;  // just echo the message back
    }
    else if(msg.compareTo("STATUS---") == 0) {
      ;  // return status
    }
    else {
      String command;
      int outletNumber;
      command = msg.substring(0, 6);
      outletNumber = msg.charAt(6) - '0';
      reply = reply.substring(0, 3) + command + outletNumber;

      if(command.compareTo("STATUS") == 0) {
        if(validateOutletNumber(outletNumber)) {
          PM::read(outlets[outletNumber]);
          reply += "=" + PM::power(outlets[outletNumber]);
        } else {
          setReplyToError();
        }
      } else if(command.compareTo("TOGGLE") == 0) {
        if(validateOutletNumber(outletNumber)) {
          PM::toggle(outlets[outletNumber]);
          reply += "=" + PM::power(outlets[outletNumber]);
        } else {
          setReplyToError();
        }
      } else {
        setReplyToError();
      }
    }

    if (reply.length() < 12)
    {
      byte i = 12-reply.length();
      while (i--) reply += '-';
    }
    Serial.print(reply);
  }
}

