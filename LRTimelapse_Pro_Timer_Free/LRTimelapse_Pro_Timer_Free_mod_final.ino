//modified by Colin Moldenhauer, all credits to Gunther Wegner for the original project
//all changes marked with comments

/*
  Pro-Timer Free
  Gunther Wegner
  http://gwegner.de
  http://lrtimelapse.com
*/

#include <LiquidCrystal.h>
#include "LCD_Keypad_Reader.h"			// credits to: http://www.hellonull.com/?p=282

const String CAPTION = "Pro-Timer 0.85";

LCD_Keypad_Reader keypad;
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);	//Pin assignments for SainSmart LCD Keypad Shield

const int NONE = 0;						// Key constants
const int SELECT = 1;
const int LEFT = 2;
const int UP = 3;
const int DOWN = 4;
const int RIGHT = 5;

const int BACK_LIGHT = 10;

const int RELEASE_TIME = 100;			                                                                                                                  

unsigned long shutterSpeed = 0;                                                                                                                     // new variables added for actual release time
unsigned long releaseStart = 0;
unsigned long releaseEnd = 0;


int keyRepeatRate = 100;			// when held, key repeats 1000 / keyRepeatRate times per second                                                       //not a constant anymore
const int keySampleRate = 50;			// ms between checking keypad for key


int localKey = 0;						// The current pressed key
int lastKeyPressed = -1;				// The last pressed key

unsigned long lastKeyCheckTime = 0;
unsigned long lastKeyPressTime = 0;

int sameKeyCount = 0;
unsigned long previousMillis = 0;		// Timestamp of last shutter release
unsigned long runningTime = 0;

float interval = 4.0;					// the current interval
long maxNoOfShots = 0;
int isRunning = 0;						// flag indicates intervalometer is running

int imageCount = 0;                   	// Image count since start of intervalometer

unsigned long rampDuration = 10;		// ramping duration
float rampTo = 0.0;						// ramping interval
unsigned long rampingStartTime = 0;		// ramping start time
unsigned long rampingEndTime = 0;		// ramping end time
float intervalBeforeRamping = 0;		// interval before ramping

boolean backLight = HIGH;				// The current settings for the backlight

const int SCR_INTERVAL = 0;				// menu workflow constants
const int SCR_SHOOTS = 1;
const int SCR_RUNNING = 2;
const int SCR_CONFIRM_END = 3;
const int SCR_SETTINGS = 4;
const int SCR_PAUSE = 5;
const int SCR_RAMP_TIME = 6;
const int SCR_RAMP_TO = 7;
const int SCR_DONE = 8;
const int BULB = 9;                                                                                                                             //added menues for bulb mode
const int BulbRelease = 10;
const int Wait = 11;

int currentMenu = 0;					// the currently selected menu
int settingsSel = 1;					// the currently selected settings option

/**
   Initialize everything
*/
void setup() {

  pinMode(BACK_LIGHT, OUTPUT);
  digitalWrite(BACK_LIGHT, HIGH);		// Turn backlight on.

  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);

  // print welcome screen))
  lcd.print("LRTimelapse.com");
  lcd.setCursor(0, 1);
  lcd.print( CAPTION );

  pinMode(12, OUTPUT);					// initialize output pin for camera release

  delay(2000);							// wait a moment...

  Serial.begin(9600);
  Serial.println( CAPTION );

  lcd.clear();
}

/**
   The main loop
*/
void loop() {

  if (millis() > lastKeyCheckTime + keySampleRate) {
    lastKeyCheckTime = millis();
    localKey = keypad.getKey();
    //Serial.println( localKey );
    //Serial.println( lastKeyPressed );

    if (localKey != lastKeyPressed) {
      processKey();
    } else {
      // key value has not changed, key is being held down, has it been long enough?
      // (but don't process localKey = 0 = no key pressed)
      if (localKey != 0 && millis() > lastKeyPressTime + keyRepeatRate) {
        // yes, repeat this key
        if ( (localKey == UP ) || ( localKey == DOWN ) ) {
          processKey();
        }
      }
    }

    if ( currentMenu == SCR_RUNNING || currentMenu == BulbRelease) {                                                                                         //same for case BulbRelease
      printScreen();	// update running screen in any case
    }

    if ( currentMenu == BulbRelease) {                                                                                                                       //checks for end of release time
      CheckEndBulb();                                                                                                                                        
      printScreen();              
    }

  }
  if ( isRunning ) {	// release camera, do ramping if running
    running();
  }

}

/**
   Process the key presses - do the Menu Navigation
*/
void processKey() {

  lastKeyPressed = localKey;
  lastKeyPressTime = millis();

  // select key will switch backlight at any time
  if ( localKey == SELECT ) {
    if ( backLight == HIGH ) {
      backLight = LOW;
    } else {
      backLight = HIGH;
    }
    digitalWrite(BACK_LIGHT, backLight); // Turn backlight on.
  }

  // do the menu navigation
  switch ( currentMenu ) {

    case BulbRelease:

      if ( localKey == RIGHT ) {
        lcd.clear();        
        digitalWrite(12, LOW);                                                                                              //if RIGHT key pressed, bulb exposure will be stopped 
        
        currentMenu = BULB;
      }
      break;

    case BULB: 

      keyRepeatRate = 200;
        
      if ( localKey == UP ) {
        
        if ( interval > 599) {                                                                                             //interval increase depends on length of interval
         interval = interval + 60;
        } else if ( interval > 179 ) {
          interval = interval + 30;
          } else if ( interval > 59 ) {
          interval =  interval + 10;
        } else if ( interval > 29 ) {
          interval =  interval + 5;
        } else {
          interval = interval + 1;
        }
        
        if ( interval > 1200 ) { 
          interval = 1200;
        }
      }

      if ( localKey == DOWN ) {
        
        if ( interval > 599) {                                                                                                                               
         interval = interval - 60;
        } else if ( interval > 180 ) {
          interval = interval - 30;
          } else if ( interval > 60 ) {
          interval = interval - 10;
        } else if ( interval > 30 ) {
          interval = interval - 5;
        } else {
          interval = interval - 1;
        }
        
        if ( interval < 0 ) { 
          interval = 0;
        }
      }

      if ( localKey == LEFT ) {
        lcd.clear();
        shutterSpeed = interval * 1000;
        currentMenu = BulbRelease;                                                                                                        
        releaseBulb( shutterSpeed );
      }

      if ( localKey == RIGHT ) {
        lcd.clear();
        currentMenu = SCR_INTERVAL;
        keyRepeatRate = 100;                                                                                                                          //resetting settings for normal intervalometer use
      }
      break;

      
    case SCR_INTERVAL:
      
      if ( localKey == UP ) {
        interval = (float)((int)(interval * 10) + 1) / 10; // round to 1 decimal place
        if ( interval > 99 ) { // no intervals longer as 99secs - those would scramble the display
          interval = 99;
        }
      }

      if ( localKey == DOWN ) {
        if ( interval > 0.2) {
          interval = (float)((int)(interval * 10) - 1) / 10; // round to 1 decimal place
        }
      }

      if ( localKey == RIGHT ) {
        lcd.clear();
        rampTo = interval;			// set rampTo default to the current interval
        currentMenu = SCR_SHOOTS;
      }

      if ( localKey == LEFT ) {
        lcd.clear();
        currentMenu = BULB;                                                                                           //added left key to change to Bulb mode
        interval = 10;                                                                                                //sets default interval to 10s
      }
      break;

    case SCR_SHOOTS:

      if ( localKey == UP ) {
        if ( maxNoOfShots >= 2500 ) {
          maxNoOfShots += 100;
        } else if ( maxNoOfShots >= 1000 ) {
          maxNoOfShots += 50;
        } else if ( maxNoOfShots >= 100 ) {
          maxNoOfShots += 25;
        } else if ( maxNoOfShots >= 10 ) {
          maxNoOfShots += 10;
        } else {
          maxNoOfShots ++;
        }
        if ( maxNoOfShots >= 9999 ) { // prevents screwing the ui
          maxNoOfShots = 9999;
        }
      }

      if ( localKey == DOWN ) {
        if ( maxNoOfShots > 2500 ) {
          maxNoOfShots -= 100;
        } else if ( maxNoOfShots > 1000 ) {
          maxNoOfShots -= 50;
        } else if ( maxNoOfShots > 100 ) {
          maxNoOfShots -= 25;
        } else if ( maxNoOfShots > 10 ) {
          maxNoOfShots -= 10;
        } else if ( maxNoOfShots > 0) {
          maxNoOfShots -= 1;
        } else {
          maxNoOfShots = 0;
        }
      }

      if ( localKey == LEFT ) {
        currentMenu = SCR_INTERVAL;
        isRunning = 0;
        lcd.clear();
      }

      if ( localKey == RIGHT ) { // Start shooting
        currentMenu = SCR_RUNNING;
        previousMillis = millis();
        runningTime = 0;
        isRunning = 1;

        lcd.clear();

        // do the first release instantly, the subsequent ones will happen in the loop
        releaseCamera();
        imageCount++;
      }
      break;

    case SCR_RUNNING:

      if ( localKey == LEFT ) { // LEFT from Running Screen aborts

        if ( rampingEndTime == 0 ) { 	// if ramping not active, stop the whole shot, other otherwise only the ramping
          if ( isRunning ) { 		// if is still runing, show confirm dialog
            currentMenu = SCR_CONFIRM_END;
          } else {				// if finished, go to start screen
            currentMenu = SCR_INTERVAL;
          }
          lcd.clear();
        } else { // stop ramping
          rampingStartTime = 0;
          rampingEndTime = 0;
        }

      }

      if ( localKey == RIGHT ) {
        currentMenu = SCR_SETTINGS;
        lcd.clear();
      }
      break;

    case SCR_CONFIRM_END:
      if ( localKey == LEFT ) { // Really abort
        currentMenu = SCR_INTERVAL;
        isRunning = 0;
        imageCount = 0;
        runningTime = 0;
        lcd.clear();
      }
      if ( localKey == RIGHT ) { // resume
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }
      break;

    case SCR_SETTINGS:

      if ( localKey == DOWN && settingsSel == 1 ) {
        settingsSel = 2;
      }

      if ( localKey == UP && settingsSel == 2 ) {
        settingsSel = 1;
      }

      if ( localKey == LEFT ) {
        settingsSel = 1;
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }

      if ( localKey == RIGHT && settingsSel == 1 ) {
        isRunning = 0;
        currentMenu = SCR_PAUSE;
        lcd.clear();
      }

      if ( localKey == RIGHT && settingsSel == 2 ) {
        currentMenu = SCR_RAMP_TIME;
        lcd.clear();
      }
      break;

    case SCR_PAUSE:

      if ( localKey == LEFT ) {
        currentMenu = SCR_RUNNING;
        isRunning = 1;
        previousMillis = millis() - (imageCount * 1000); // prevent counting the paused time as running time;
        lcd.clear();
      }
      break;

    case SCR_RAMP_TIME:

      if ( localKey == RIGHT ) {
        currentMenu = SCR_RAMP_TO;
        lcd.clear();
      }

      if ( localKey == LEFT ) {
        currentMenu = SCR_SETTINGS;
        settingsSel = 2;
        lcd.clear();
      }

      if ( localKey == UP ) {
        if ( rampDuration >= 10) {
          rampDuration += 10;
        } else {
          rampDuration += 1;
        }
      }

      if ( localKey == DOWN ) {
        if ( rampDuration > 10 ) {
          rampDuration -= 10;
        } else {
          rampDuration -= 1;
        }
        if ( rampDuration <= 1 ) {
          rampDuration = 1;
        }
      }
      break;

    case SCR_RAMP_TO:

      if ( localKey == LEFT ) {
        currentMenu = SCR_RAMP_TIME;
        lcd.clear();
      }

      if ( localKey == UP ) {
        rampTo = (float)((int)(rampTo * 10) + 1) / 10; // round to 1 decimal place
        if ( rampTo > 99 ) { // no intervals longer as 99secs - those would scramble the display
          rampTo = 99;
        }
      }

      if ( localKey == DOWN ) {
        if ( rampTo > 0.2) {
          rampTo = (float)((int)(rampTo * 10) - 1) / 10; // round to 1 decimal place
        }
      }

      if ( localKey == RIGHT ) { // start Interval ramping
        if ( rampTo != interval ) { // only if a different Ramping To interval has been set!
          intervalBeforeRamping = interval;
          rampingStartTime = millis();
          rampingEndTime = rampingStartTime + rampDuration * 60 * 1000;
        }

        // go back to main screen
        currentMenu = SCR_RUNNING;
        lcd.clear();
      }
      break;

    case SCR_DONE:

      if ( localKey == LEFT || localKey == RIGHT ) {
        currentMenu = SCR_INTERVAL;
        isRunning = 0;
        imageCount = 0;
        runningTime = 0;
        lcd.clear();
      }
      break;
  }
  printScreen();
}

void printScreen() {

  switch ( currentMenu ) {

     case BulbRelease:                                                                                                  //added
      printBulbRelease();
      break;

    case BULB:                                                                    
      printBulb();
      break;
    
    case SCR_INTERVAL:
      printIntervalMenu();
      break;

    case SCR_SHOOTS:
      printNoOfShotsMenu();
      break;

    case SCR_RUNNING:
      printRunningScreen();
      break;

    case SCR_CONFIRM_END:
      printConfirmEndScreen();
      break;

    case SCR_SETTINGS:
      printSettingsMenu();
      break;

    case SCR_PAUSE:
      printPauseMenu();
      break;

    case SCR_RAMP_TIME:
      printRampDurationMenu();
      break;

    case SCR_RAMP_TO:
      printRampToMenu();
      break;

    case SCR_DONE:
      printDoneScreen();
      break;
  }
}


/**
   Running, releasing Camera
*/
void running() {

  // do this every interval only
  if ( ( millis() - previousMillis ) >=  ( ( interval * 1000 )) ) {

    if ( ( maxNoOfShots != 0 ) && ( imageCount >= maxNoOfShots ) ) { // sequence is finished
      // stop shooting
      isRunning = 0;
      currentMenu = SCR_DONE;
      lcd.clear();
      printDoneScreen(); // invoke manually
    } else { // is running
      runningTime += (millis() - previousMillis );
      previousMillis = millis();
      releaseCamera();
      imageCount++;
    }
  }

  // do this always (multiple times per interval)
  possiblyRampInterval();
}

/**
   If ramping was enabled do the ramping
*/
void possiblyRampInterval() {

  if ( ( millis() < rampingEndTime ) && ( millis() >= rampingStartTime ) ) {
    interval = intervalBeforeRamping + ( (float)( millis() - rampingStartTime ) / (float)( rampingEndTime - rampingStartTime ) * ( rampTo - intervalBeforeRamping ) );
  } else {
    rampingStartTime = 0;
    rampingEndTime = 0;
  }
}

/**
   Actually release the camera
*/
void releaseCamera() {

  lcd.setCursor(15, 0);
  lcd.print((char)255);

  digitalWrite(12, HIGH);
  delay(RELEASE_TIME);
  digitalWrite(12, LOW);

  lcd.setCursor(15, 0);
  lcd.print(" ");
}


void releaseBulb(unsigned long shutterSpeed ) {                                                                                                         //copied releaseCamera() and changed to delay for a variable amount of time
  releaseStart = millis();
  releaseEnd = releaseStart + shutterSpeed;
  
  lcd.setCursor(15, 0);
  lcd.print((char)255);
  
  digitalWrite(12, HIGH);
}

void CheckEndBulb() {
   if ( millis() >= releaseEnd ) {
    digitalWrite(12, LOW);
    lcd.clear();
    currentMenu = BULB;
  }
  
  lcd.setCursor(15, 0);
  lcd.print(" ");
}

/**
   Pause Mode
*/
void printPauseMenu() {

  lcd.setCursor(0, 0);
  lcd.print("PAUSE...        ");
  lcd.setCursor(0, 1);
  lcd.print("< Continue");
}

// ---------------------- SCREENS AND MENUS -------------------------------

void printBulbRelease() {                                                                                       //added print functions
  lcd.setCursor(0, 0);                                                                                          //prints interval time, remaining exposure, time already exposed
  lcd.print("Exposure: ");
  lcd.setCursor(10, 0);
  lcd.print( printMin ( interval ) );
  printTimer();
  printExposed();                                                                                               
}

void printBulb() {                                                                                              
  printMin( interval );
  lcd.setCursor(0, 0);
  lcd.print("Bulb");
  lcd.setCursor(0, 1);
  lcd.print( printMin ( interval ) );
}

void printExposed() {                                                                                           //counts time since release
  
  if ( (millis() - lastKeyPressTime) / 1000 < interval ) {
    lcd.setCursor(11, 1);
    lcd.print( printMin ( (millis() - lastKeyPressTime) / 1000) );
  } 
}


/**
   Configure Interval setting (main screen)
*/
void printIntervalMenu() {

  lcd.setCursor(0, 0);
  lcd.print("Interval");
  lcd.setCursor(0, 1);
  lcd.print( interval );
  lcd.print( "     " );
}

/**
   Configure no of shots - 0 means infinity
*/
void printNoOfShotsMenu() {

  lcd.setCursor(0, 0);
  lcd.print("No of shots");
  lcd.setCursor(0, 1);
  if ( maxNoOfShots > 0 ) {
    lcd.print( printInt( maxNoOfShots, 4 ) );
  } else {
    lcd.print( "----" );
  }
}

/**
   Print running screen
*/
void printRunningScreen() {

  lcd.setCursor(0, 0);
  lcd.print( printInt( imageCount, 4 ) );

  if ( maxNoOfShots > 0 ) {
    lcd.print( " R:" );
    lcd.print( printInt( maxNoOfShots - imageCount, 4 ) );
    lcd.print( " " );

    lcd.setCursor(0, 1);
    // print remaining time
    unsigned long remainingSecs = (maxNoOfShots - imageCount) * interval;
    lcd.print( "T-");
    lcd.print( fillZero( remainingSecs / 60 / 60 ) );
    lcd.print( ":" );
    lcd.print( fillZero( ( remainingSecs / 60 ) % 60 ) );
  }

  updateTime();

  lcd.setCursor(11, 0);
  if ( millis() < rampingEndTime ) {
    lcd.print( "*" );
  } else {
    lcd.print( " " );
  }
  lcd.print( printFloat( interval, 4, 1 ) );
}

void printDoneScreen() {

  // print elapsed image count))
  lcd.setCursor(0, 0);
  lcd.print("Done ");
  lcd.print( imageCount );
  lcd.print( " shots.");

  // print elapsed time when done
  lcd.setCursor(0, 1);
  lcd.print( "t=");
  lcd.print( fillZero( runningTime / 1000 / 60 / 60 ) );
  lcd.print( ":" );
  lcd.print( fillZero( ( runningTime / 1000 / 60 ) % 60 ) );
  lcd.print("    ;-)");
}

void printConfirmEndScreen() {
  lcd.setCursor(0, 0);
  lcd.print( "Stop shooting?");
  lcd.setCursor(0, 1);
  lcd.print( "< Stop    Cont.>");
}

/**
   Update the time display in the main screen
*/
void updateTime() {

  unsigned long finerRunningTime = runningTime + (millis() - previousMillis);

  if ( isRunning ) {

    int hours = finerRunningTime / 1000 / 60 / 60;
    int minutes = (finerRunningTime / 1000 / 60) % 60;
    int secs = (finerRunningTime / 1000 ) % 60;

    String sHours = fillZero( hours );
    String sMinutes = fillZero( minutes );
    String sSecs = fillZero( secs );

    lcd.setCursor(8, 1);
    lcd.print( sHours );
    lcd.setCursor(10, 1);
    lcd.print(":");
    lcd.setCursor(11, 1);
    lcd.print( sMinutes );
    lcd.setCursor(13, 1);
    lcd.print(":");
    lcd.setCursor(14, 1);
    lcd.print( sSecs );
  } else {
    lcd.setCursor(8, 1);
    lcd.print("   Done!");
  }
}


void printTimer() {                                                                                                                               //timer method for countdown 
  int timesinceStart = (millis() - lastKeyPressTime) / 1000;
  int restTime = interval - timesinceStart;

  lcd.setCursor(0, 1);
  lcd.print( printMin ( restTime ) );
}


/**
   Print Settings Menu
*/
void printSettingsMenu() {

  lcd.setCursor(1, 0);
  lcd.print("Pause          ");
  lcd.setCursor(1, 1);
  lcd.print("Ramp Interval  ");
  lcd.setCursor(0, settingsSel - 1);
  lcd.print(">");
  lcd.setCursor(0, 1 - (settingsSel - 1));
  lcd.print(" ");
}

/**
   Print Ramping Duration Menu
*/
void printRampDurationMenu() {

  lcd.setCursor(0, 0);
  lcd.print("Ramp Time (min) ");

  lcd.setCursor(0, 1);
  lcd.print( rampDuration );
  lcd.print( "     " );
}

/**
   Print Ramping To Menu
*/
void printRampToMenu() {

  lcd.setCursor(0, 0);
  lcd.print("Ramp to (Intvl)");

  lcd.setCursor(0, 1);
  lcd.print( rampTo );
  lcd.print( "     " );
}




// ----------- HELPER METHODS -------------------------------------

/**
   Fill in leading zero to numbers in order to always have 2 digits
*/
String fillZero( int input ) {

  String sInput = String( input );
  if ( sInput.length() < 2 ) {
    sInput = "0";
    sInput.concat( String( input ));
  }
  return sInput;
}

String printFloat(float f, int total, int dec) {

  static char dtostrfbuffer[8];
  String s = dtostrf(f, total, dec, dtostrfbuffer);
  return s;
}

String printInt( int i, int total) {
  float f = i;
  static char dtostrfbuffer[8];
  String s = dtostrf(f, total, 0, dtostrfbuffer);
  return s;
}

String printMin( int interval ) {                                                                                            //added helper method to convert seconds into MM:SS
  String minutes =  String(interval / 60);
  String seconds = String(interval % 60);
  
    if (interval < 600) {
        minutes = "0" + minutes;
    }
    if ( interval % 60 < 10 ) {
        seconds = "0" + seconds;
    }
    if ( interval % 60 == 0 ) {
      seconds = "00";
    }


  String timerMin = minutes + ":" +  seconds;
  return timerMin;
}

