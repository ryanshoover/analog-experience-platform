#include <Bridge.h>
#include <LiquidCrystal_I2C.h>
#include <Process.h>

/**
   Effects

   Laser switch → Lasers fly across the site from random directions
   Sharks switch → Shark fin back and forth across bottom of site
   Lasers + Sharks switches → Sharks with frickin lasers on their heads attack the site
   “Do not push” Missile Launch → Website cracks like glass. Message pops up saying “You pushed it, didn’t you”
   Flame sensor → Hack the site
   Fire alarm → Unhack the site
   Breathalyzer → Content changes to drunk-speak
   Pull chain → Change font size (12px, 18px, 24px, 36px)
   Color knobs → Change body (header?) background color
   Toilet flush → Visually flush all content on the page
*/

// set LCD address
LiquidCrystal_I2C lcd(0x3f, 20, 4);

// Analog pin setup
const int dial_red       = 0;
const int dial_green     = 1;
const int dial_blue      = 2;
const int sensor_alcohol = 3;

// Digital pin setup
const int light_success       = 12;
const int light_failure       = 11;
const int switch_lasers       = 10;
const int switch_sharks       = 9;
const int switch_missleLaunch = 8;
const int switch_flame        = 7;
const int switch_fireAlarm    = 6;
const int switch_toilet       = 5;
const int switch_pullChain1   = 4;
const int switch_pullChain2   = 3;
const int switch_pullChain3   = 2;
const int switch_triggerColor = 14;

// Static values
const int LED_DELAY     = 2000;
const int ALCOHOL_LIMIT = 75;
const String SSH_PREFIX = "dbclient -i ~/.ssh/id_dropbear demowpe@demowpe.ssh.wpengine.net";

// Initial variables
byte R, G, B;
int value_lasers,       prev_lasers       = 0;
int value_sharks,       prev_sharks       = 0;
int value_missleLaunch, prev_missleLaunch = 0;
int value_flame,        prev_flame        = 0;
int value_fireAlarm,    prev_fireAlarm    = 0;
int value_toilet,       prev_toilet       = 0;
int value_alcohol,      prev_alcohol      = 0;
int value_triggerColor, prev_triggerColor = 0;
int value_pullChain1,   prev_pullChain1,
    value_pullChain2,   prev_pullChain2,
    value_pullChain3,   prev_pullChain3,
    prev_pullChain0                       = 0;


// Values that are computed inside the code.
char value_hex[7] = {0};
char* command     = "";

////////////////////////////////////
// CONTROL SUCCESS / FAILURE LEDS //
////////////////////////////////////

// Clear LEDs
void noLeds() {
  digitalWrite( LED_BUILTIN, LOW );
  digitalWrite( light_success, LOW );
  digitalWrite( light_failure, LOW );
}

// Successful result
void successLed() {
  digitalWrite( LED_BUILTIN, HIGH );
  digitalWrite( light_success, HIGH );
  digitalWrite( light_failure, LOW );
  delay( LED_DELAY );
  noLeds();
}

// Unsuccessful result
void unsuccessLed() {
  digitalWrite( light_success, LOW );
  digitalWrite( light_failure, HIGH );
  delay( LED_DELAY );
  noLeds();
}

// Both LEDs on (Command successfully sent)
void bothLeds() {
  digitalWrite( LED_BUILTIN, HIGH );
  digitalWrite( light_success, HIGH );
  digitalWrite (light_failure, HIGH );
  delay( LED_DELAY );
  noLeds();
}

// Flash both 10 times (Request for command received - e.g. button pressed)
void flashLeds() {
  int count = 10;
  while ( count > 0 ) {
    digitalWrite( LED_BUILTIN, HIGH );
    digitalWrite( light_success, HIGH );
    digitalWrite( light_failure, LOW );
    delay( 100 );
    digitalWrite( LED_BUILTIN, LOW );
    digitalWrite( light_success, LOW );
    digitalWrite( light_failure, HIGH );
    delay( 100 );
    count -= 1;
  }
  noLeds();
}

// Play this sequence of LEDs when the bridge connection is successful
void bridgeSuccess() {
  bothLeds();
  flashLeds();
  successLed();
}

/**
   Initialize the text on the LCD screen
*/
void lcdInit() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor( 0, 0 );
  lcd.print( "Analogue Experience" );
  lcd.setCursor( 2, 1 );
  lcd.print( "--- Platform ---" );
  lcd.setCursor( 1, 2  );
  lcd.print( "RGB Value: #" );
  lcd.setCursor( 0, 3 );
  lcd.print( "Alcohol Sensor: " );
}

/**
   Read the values of the 3 RGB dials
*/
void readRGB() {
  R = map( analogRead( dial_red ), 100, 924, 0, 255 );
  G = map( analogRead( dial_green ), 100, 924, 0, 255 );
  B = map( analogRead( dial_blue ), 100, 924, 0, 255 );
  sprintf( value_hex, "%02X%02X%02X", R, G, B );
}

/**
   Updates the LCD display with the latest values of the
   RGB sensors and the alcohol level
*/
void updateDisplay() {
  lcd.setCursor( 13, 2 );
  lcd.print( value_hex );
  lcd.setCursor( 16, 3 );
  lcd.print( value_alcohol );
}

/**
   Send a command to the WP install.

   @param string success String to look for that means the command was successful.
*/
void sendCommand( char* success = "Success" ) {
  String output = "";

  flashLeds();
  bothLeds();

  Process p;

  p.runShellCommand( SSH_PREFIX + " 'wp axp " + command + "'" );
  // Let user know command has been received

  while ( p.available()  > 0 ) {
    char c = p.read();
    output += c;
  }


  if ( output.indexOf( success ) != -1 ) {
    successLed();
  } else {
    unsuccessLed();
  }

  command = "";
}

///////////////////////////////////////////////////
// DEFINE ALL EXTERNALLY-REACHING FUNCTIONS HERE //
///////////////////////////////////////////////////

/**
   Toggles the lasers effect on and off
*/
void toggle_lasers() {
  value_lasers = digitalRead( switch_lasers );

  if ( value_lasers != prev_lasers ) {
    command = "toggle lasers";
    sendCommand();
  }

  prev_lasers = value_lasers;
}

/**
   Toggles the sharks effect on and off
*/
void toggle_sharks() {
  value_sharks = digitalRead( switch_sharks );

  if ( value_sharks != prev_sharks ) {
    command = "toggle sharks";
    sendCommand();
  }

  prev_sharks = value_sharks;
}

/**
   Toggles the missleLaunch effect on and off
*/
void toggle_missleLaunch() {
  value_missleLaunch = digitalRead( switch_missleLaunch );

  if ( value_missleLaunch != prev_missleLaunch ) {
    command = "toggle missle-launch";
    sendCommand();
  }

  prev_missleLaunch = value_missleLaunch;
}


/**
   Toggles the alcohol sensor value

   @return bool Whether the alcohol is higher than our limit.
*/
bool toggle_alcohol() {
  value_alcohol = map( analogRead( sensor_alcohol ), 0, 1024, 0, 100 );

  if ( value_alcohol >= ALCOHOL_LIMIT && prev_alcohol == LOW ) {
    prev_alcohol = HIGH;

    flashLeds();
    unsuccessLed();
    unsuccessLed();
    unsuccessLed();

    command = "wp axp on alcohol";
    sendCommand();

    // @todo Display says fahgetabatit?

    return false;

  } else if ( value_alcohol < ALCOHOL_LIMIT && prev_alcohol == HIGH ) {
    prev_alcohol = LOW;

    command = "wp axp off alcohol";
    sendCommand();

    return true;
  }
}

/**
   Turns flame effect on
*/
void detect_flame() {
  value_flame = digitalRead( switch_flame );

  if ( value_flame != prev_flame && value_flame == HIGH ) {
    command = "on flame";
    sendCommand();
  }

  prev_flame = value_flame;
}

/**
   Turns flame effect off
*/
void detect_fireAlarm() {
  value_fireAlarm = digitalRead( switch_fireAlarm );

  if ( value_fireAlarm != prev_fireAlarm && value_fireAlarm == HIGH ) {
    command = "off flame";
    sendCommand();
  }

  prev_fireAlarm = value_fireAlarm;
}

/**
   Turns toilet effect on
*/
void detect_toilet() {
  value_toilet = digitalRead( switch_toilet );

  if ( value_toilet != prev_toilet && value_toilet == HIGH ) {
    command = "on toilet";
    sendCommand();
  }

  prev_toilet = value_toilet;
}

/**
   Turns triggerColor effect on
*/
void detect_triggerColor() {
  value_triggerColor = digitalRead( switch_triggerColor );

  if ( value_triggerColor != prev_triggerColor && value_triggerColor == HIGH ) {
    command = strcat( "set color ", value_hex );
    sendCommand();
  }

  prev_triggerColor = value_triggerColor;
}

/**
   Handle cycling of the pull chain through its 4 stages.
*/
void cycle_pullChain() {
  value_pullChain1 = digitalRead( switch_pullChain1 );
  value_pullChain2 = digitalRead( switch_pullChain2 );
  value_pullChain3 = digitalRead( switch_pullChain3 );

  if ( value_pullChain1 != prev_pullChain1 && value_pullChain1 == HIGH ) {
    prev_pullChain0 = LOW;
    command = "set pullchain 1";
    sendCommand();

  } else if ( value_pullChain2 != prev_pullChain2 && value_pullChain2 == HIGH ) {
    prev_pullChain0 = LOW;
    command = "set pullchain 2";
    sendCommand();

  } else if ( value_pullChain3 != prev_pullChain3 && value_pullChain3 == HIGH ) {
    prev_pullChain0 = LOW;
    command = "set pullchain 3";
    sendCommand();

  } else if ( prev_pullChain0 != HIGH && value_pullChain1 == LOW && value_pullChain2 == LOW && value_pullChain3 == LOW  ) {
    prev_pullChain0 = HIGH;
    command = "set pullchain 0";
    sendCommand();
  }

  prev_pullChain1 = value_pullChain1;
  prev_pullChain2 = value_pullChain2;
  prev_pullChain3 = value_pullChain3;
}

///////////////////
// --- SETUP --- //
///////////////////

void setup() {
  // Set PIN types
  pinMode( LED_BUILTIN, OUTPUT );
  pinMode( light_success, OUTPUT );
  pinMode( light_failure, OUTPUT );

  // Initialize LCD with static labels
  lcdInit();

  // Start the bridge connection
  Bridge.begin();

  // Report success
  bridgeSuccess();
}


//////////////////
// --- LOOP --- //
//////////////////

void loop() {
  // While booting, show booting message?

  // Read RGB values
  readRGB();

  // Display loop
  updateDisplay();

  // Loop through our switches, looking for actions
  toggle_lasers();
  toggle_sharks();
  toggle_missleLaunch();
  toggle_alcohol();

  detect_flame();
  detect_fireAlarm();
  detect_toilet();
  detect_triggerColor();

  cycle_pullChain();

  // Delay so we don't overload the Arduino
  delay(200);
}
