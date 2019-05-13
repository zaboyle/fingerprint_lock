/*************************************************** 
 Adaptation of Adafruit fingerprint test from the 
 Adafruit Fingerprint Sensor Library

 This script uses a fingerprint sensor control a
 servo motor, acting as a fingerprint verification
 lock for any deadbolt door

 Users can add/delete fingerprints by entering valid enrolled
 fingerprint. Instructions will be given via an LCD

 By: Zachary Boyle
 4/27/19
 ****************************************************/

#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <LiquidCrystal.h>

//create the servo instance
Servo myservo;

// pin #2 is IN from sensor (GREEN wire)
const int FP_GREEN_WIRE_PIN = 7;
// pin #3 is OUT from arduino  (WHITE wire)
const int FP_WHITE_WIRE_PIN = 6;

//button pins
const int enroll_btn_pin = A5;
const int delete_btn_pin = A4;
const int cancel_btn_pin = A3;
const int inside_unlock_btn_pin = A2;

//lcd pins
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;

//servo pins
const int servo_pin = 9;

//button constants
bool enroll_btn_pressed = false;
bool delete_btn_pressed = false;
bool cancel_btn_pressed = false;
bool inside_unlock_btn_pressed = false;

//confidence threshold
const int conf_threshold = 80;
//wait time for keeping door unlocked (in milliseconds)
const int unlock_time = 5000;
//names by ID
const String people[15] = {
                    "Zach", 
                    "Mom", 
                    "Dad", 
                    "Brittany",
                    "Brady", 
                    "Yulia",
                    "Julia C.",
                    "Julia S.",
                    "Uncle Ted",
                    "Tante Eva",
                    "Guest1",
                    "Guest2",
                    "Guest3",
                    "Guest4",
                    "Guest5"
                    };

//add more people here as desired!
//NOTE: IF ADDING MORE PEOPLE, ADJUST THE SIZE OF THE ARRAY ACCORDINGLY
//      ALSO, THE INDICES MATTER!!! WRONG INDEX -> WRONG NAME


// For UNO and others without hardware serial, we must use software serial...
SoftwareSerial mySerial(FP_GREEN_WIRE_PIN, FP_WHITE_WIRE_PIN);

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


void setup()
{
  //set up the button pins
  pinMode(enroll_btn_pin, INPUT);
  pinMode(delete_btn_pin, INPUT);
  pinMode(cancel_btn_pin, INPUT);
  pinMode(inside_unlock_btn_pin, INPUT);

  //lcd
  lcd.begin(16,2);
  lcd.print("Welcome to ");
  lcd.setCursor(0,1);
  lcd.print("Zach's FP lock!");

  //servo
  myservo.attach(servo_pin);
  
  Serial.begin(9600);
  //while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  //Serial.println("\n\nZach's fingerprint lock script");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();
  Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  Serial.println("Waiting for valid finger...");
}

// run over and over again
void loop()
{
  //read the button inputs
  enroll_btn_pressed = digitalRead(enroll_btn_pin);
  delete_btn_pressed = digitalRead(delete_btn_pin);
  cancel_btn_pressed = digitalRead(cancel_btn_pin);
  inside_unlock_btn_pressed = digitalRead(inside_unlock_btn_pin);
  
  if(enroll_btn_pressed) {
    Serial.println("enroll branch taken");
    enroll_fp();
  } else if(delete_btn_pressed) {
    Serial.println("delete branch taken");
    delete_fp();
  } else if(inside_unlock_btn_pressed) {
    Serial.println("inside unlock branch taken");
    unlock_door();
    delay(unlock_time);
    lock_door();
  } else {
    lcd.print("Waiting for FP...");
    getFingerprintIDez(true);
  }

  lcd.clear();

  
  //delay for the fp sensor
  delay(50);
}


//this function is for debugging!
//if needed, replace the call in loop() with this function
//to get a more detailed error report
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence); 

  return finger.fingerID;
}


// returns -1 if failed, otherwise returns ID #
//print_name parameter is false when only obtaining ID, true when you want to unlock
int getFingerprintIDez(bool print_name) {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  //if we have a valid, confident finger
  if(finger.confidence >= conf_threshold && print_name) {
    
    lcd.clear();
    
    //welcome the resident/guest if a high enough confidence was achieved!
    if(finger.fingerID < 5) {
      lcd.print("Welcome Home, ");
      Serial.print("Welcome Home, ");
    } else if(print_name) {
      lcd.print("Welcome, ");
      Serial.print("Welcome, ");
    }

    //TODO:
    //FIX PROBLEM WHERE NEWLY ENROLLED PEOPLE'S NAMES ARE INCORRECTLY DISPLAYED
    lcd.setCursor(0,1);
    lcd.print(people[finger.fingerID]);
    Serial.print(people[finger.fingerID]);
    lcd.print("!");
    Serial.println("!");
    
    //unlock the door
    unlock_door();
    //wait for 30s to let people read the message and get inside
    delay(unlock_time);
    //re-lock the door after people had the chance to enter
    lock_door();
    
  }
  
  return finger.fingerID; 
}

//function for interfacing with enrolling FP
void enroll_fp() {
  //print a message telling to enter a valid fingerprint
  Serial.println("Please enter a valid fingerprint to enroll another...");
  lcd.print("Enter a valid FP");
  lcd.setCursor(0,1);
  lcd.print("...");
  
  int p = finger.getImage();
  while(p == FINGERPRINT_NOFINGER || p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(digitalRead(cancel_btn_pin)) {
      lcd.clear();
      lcd.print("Cancelling...");
      Serial.println("Cancelling new finger enroll...");
      return;
      }
  }

  lcd.clear();
  
  if(p != FINGERPRINT_OK) {
    Serial.println("Invalid ID. Cannot enroll a new fingerprint");
    lcd.print("Invalid ID");
    delay(1500);
    return;
  }

  Serial.println("Valid ID. Please select the name for the new fingerprint");
  lcd.print("Please pick name");
  delay(1500);
  lcd.clear();
  
  int i = 0;
  Serial.println("Enroll as " + people[i] + "?");
  lcd.print("Enroll as ");
  lcd.setCursor(0,1);
  lcd.print(people[i]);
  lcd.print("?");
  while(true) {

    //read the button inputs
    enroll_btn_pressed = digitalRead(enroll_btn_pin);
    delete_btn_pressed = digitalRead(delete_btn_pin);
    cancel_btn_pressed = digitalRead(cancel_btn_pin); 

    delay(100);

    //use the delete btn to "arrow up" and cancel btn to "arrow down"
    i += 1 * delete_btn_pressed;
    i -= 1 * cancel_btn_pressed;
    if(i > 14) { i = 0; }
    if(i < 0) { i = 14; }

    if(delete_btn_pressed || cancel_btn_pressed) {
      //ask if the user wants to enroll as this person
      Serial.println("Enroll as " + people[i] + "?");
      lcd.clear();
      lcd.print("Enroll as ");
      lcd.setCursor(0,1);
      lcd.print(people[i]);
      lcd.print("?");
    }

    if(enroll_btn_pressed) {
      //confirm that the person will be enrolled with that name
      Serial.println("Enrolling as " + people[i] + "...");
      lcd.clear();
      lcd.print("Enrolling as ");
      lcd.setCursor(0,1);
      lcd.print(people[i]);
      lcd.print("...");
      break;
    }
    
  }

  //enroll at index i
  enroll(i + 1);
  
}

//function for interfacing with deleting FP
void delete_fp() {
  //print a message telling to enter a valid fingerprint
  Serial.println("Please enter a valid fingerprint to delete another...");
  lcd.clear();
  lcd.print("Enter a valid FP");
  int p = finger.getImage();
  while(p == FINGERPRINT_NOFINGER || p != FINGERPRINT_OK) {
    p = finger.getImage();
    if(digitalRead(cancel_btn_pin)) {
      lcd.clear();
      lcd.print("Cancelling...");
      Serial.println("Cancelling fingerprint delete...");
      return;
      }
  }

  lcd.clear();

  //reject if invalid ID
  if(p != FINGERPRINT_OK) {
    Serial.println("Invalid ID. Cannot delete a fingerprint");
    lcd.print("Invalid ID");
    delay(1500);
    return;
  }
  

  Serial.println("Valid ID. Please select the fingerprint to delete");
  lcd.print("Please pick the FP");
  lcd.setCursor(0,1);
  lcd.print("to delete");
  delay(1500);
  lcd.clear();

  int i = 0;
  Serial.println("Delete " + people[i] + "?");
  lcd.print("Delete ");
  lcd.print(people[i]);
  lcd.print("?");
  while(true) {

    //read the button inputs
    enroll_btn_pressed = digitalRead(enroll_btn_pin);
    delete_btn_pressed = digitalRead(delete_btn_pin);
    cancel_btn_pressed = digitalRead(cancel_btn_pin); 

    delay(100);

    //use the delete btn to "arrow up" and cancel btn to "arrow down"
    i += 1 * delete_btn_pressed;
    i -= 1 * cancel_btn_pressed;
    if(i > 14) { i = 0; }
    if(i < 0) { i = 14; }

    if(delete_btn_pressed || cancel_btn_pressed) {
      //ask if the user wants to enroll as this person
      Serial.println("Delete " + people[i] + "?");
      lcd.clear();
      lcd.print("Delete ");
      lcd.print(people[i]);
      lcd.print("?");
    }

    if(enroll_btn_pressed) {
      //confirm that the person will be enrolled with that name
      Serial.println("Deleting " + people[i] + "...");
      lcd.clear();
      lcd.print("Deleting ");
      lcd.setCursor(0,1);
      lcd.print(people[i]);
      lcd.print("...");
      break;
    }
    
  }

  //delete at index i
  deleteFingerprint(i + 1);
  
}


//unlock to door using the servo motor
void unlock_door() {
  //move to 90 degrees
  for (int pos = 0; pos <= 90; pos += 1) {
    // in steps of 1 degree
    myservo.write(pos);
    delay(15);
  }
}


//lock the door using the servo
void lock_door() {
  //move to 0 degrees
  for (int pos = 90; pos >= 0; pos -= 1) {
    // in steps of 1 degree
    myservo.write(pos);
    delay(15);
  }
}

//actually enrolls a fingerprint
void enroll(int id) {
  lcd.clear();
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  lcd.print("enter valid FP");
  lcd.setCursor(0,1);
  lcd.print("for ");
  lcd.print(people[id-1]);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      //Serial.println("Unknown error");
      return p;
  }
  
  Serial.println("Remove finger");
  lcd.clear();
  lcd.print("Remove finger");
  delay(2000);
  lcd.clear();
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  lcd.print("Place ");
  lcd.setCursor(0,1);
  lcd.print("same finger");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }
  
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
  
  Serial.print("Fingerprint for "); Serial.println(people[id - 1]);
  lcd.print("FP for ");
  lcd.print(people[id-1]);
  lcd.setCursor(0,1);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
    lcd.print("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }   
}


//actually deletes a fingerprint
void deleteFingerprint(int id) {
  uint8_t p = -1;
  
  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted " + people[id - 1] + "'s fingerprint!");
    lcd.print("Deleted ");
    lcd.print(people[id-1]);
    lcd.print("'s");
    lcd.setCursor(0,1);
    lcd.print("fingerprint");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    return p;
  }   
}
