#include <Bridge.h>
#include <Process.h>
#include <LiquidCrystal_I2C.h>

/*****************************************************************************
 * # Effects
 * - Laser switch → Lasers fly across the site from random directions
 * - Sharks switch → Shark fin back and forth across bottom of site
 * - Lasers + Sharks switches → Sharks with frickin lasers on their heads
 * - “Do not push” Missile Launch → Website breaks out into disco mode
 * - Flame sensor → Hack the site
 * - Fire alarm → Unhack the site
 * - Breathalyzer → Content changes to drunk-speak
 * - Pull chain → Change font size (12px, 18px, 24px, 36px)
 * - Color knobs → Change body (header?) background color
 * - Toilet flush → Visually flush all content on the page
 *****************************************************************************/
// Settings
const bool   NETWORK_ENABLED       = true;
const bool   NO_DISABLE_WHEN_DRUNK = true;
const int    LED_DELAY             = 2000;
const int    LOOP_DELAY            = 200;
const int    ALCOHOL_LIMIT         = 50;
const String SSH_PREFIX            = "dbclient -i ~/.ssh/id_dropbear demowpe@demowpe.ssh.wpengine.net";

// Set the LCD I2C address
LiquidCrystal_I2C LCD(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Analog pin setup
const int dial_red       = 1;
const int dial_green     = 2;
const int dial_blue      = 3;
const int lcd_sda        = 4;
const int lcd_scl        = 5;
const int sensor_alcohol = 6;

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

// Initial variables
byte R, G, B;
int value_lasers,       prev_lasers       = LOW;
int value_sharks,       prev_sharks       = LOW;
int value_missleLaunch, prev_missleLaunch = LOW;
int value_flame,        prev_flame        = LOW;
int value_fireAlarm,    prev_fireAlarm    = LOW;
int value_toilet,       prev_toilet       = LOW;
int value_alcohol,      prev_alcohol      = LOW;
int value_triggerColor, prev_triggerColor = LOW;
int value_pullChain1,   prev_pullChain1,
    value_pullChain2,   prev_pullChain2,
    value_pullChain3,   prev_pullChain3   = LOW;
int prev_pullChain0                       = HIGH;

int state_led = LOW;
long time_led = millis();

// Values that are computed inside the code.
String value_hex  = "";
String command    = "";

// Our Process instance for connecting to the Yun shield
Process p;

////////////////////////////////////
// CONTROL SUCCESS / FAILURE LEDS //
////////////////////////////////////

// Clear LEDs
void maybeTurnOffLEDs() {
  bool turnOff = false;

  if ( state_led != HIGH && millis() - time_led < LED_DELAY ) {
    turnOff = true;
  }

  if ( millis() - time_led > LED_DELAY * 5 ) {
    turnOff = true;
  }

  if ( turnOff ) {
    digitalWrite( LED_BUILTIN, LOW );
    digitalWrite( light_success, LOW );
    digitalWrite( light_failure, LOW );
    state_led = LOW;
  }
}

// Successful result
void successLed() {
  digitalWrite( LED_BUILTIN, HIGH );
  digitalWrite( light_success, HIGH );
  digitalWrite( light_failure, LOW );

  state_led = HIGH;
  time_led  = millis();
}

// Unsuccessful result
void unsuccessLed() {
  digitalWrite( LED_BUILTIN, LOW );
  digitalWrite( light_success, LOW );
  digitalWrite( light_failure, HIGH );

  state_led = HIGH;
  time_led = millis();
}

// Both LEDs on (Command successfully sent)
void bothLeds() {
  digitalWrite( LED_BUILTIN, HIGH );
  digitalWrite( light_success, HIGH );
  digitalWrite (light_failure, HIGH );

  state_led = HIGH;
  time_led  = millis();
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

  state_led = HIGH;
  time_led  = millis();
}

// Play this sequence of LEDs when the bridge connection is successful
void bridgeSuccess() {
  flashLeds();
  successLed();
}

/**
   Initialize the text on the LCD screen
*/
void lcdInit() {
  LCD.begin(20, 4);  // 16 lines by 2 rows
  LCD.clear();
  LCD.backlight();
}

void bootMessage() {
  // Write a booting message to the screen
  LCD.setCursor( 0, 0 );
  LCD.print( "_ANALOG EXP PLATFRM_" );
  LCD.setCursor( 0, 2  );
  LCD.print( "Booting....." );

  // Turn on the red light.
  unsuccessLed();
}

/**
 * Prints the basic sensor labels
 */
void printSensorLabels() {
  LCD.setCursor( 0, 1  );
  LCD.print( "Color:   #          " );
  LCD.setCursor( 0, 2 );
  LCD.print( "Alcohol:            " );
  LCD.setCursor( 0, 3 );
  LCD.print( "Cmd:                " );
}

/**
 * Updates the LCD display with the latest values of the
 * RGB sensors and the alcohol level
 */
void updateDisplay() {
  LCD.setCursor( 10, 1 );
  LCD.print( value_hex );
  LCD.setCursor( 9, 2 );
  LCD.print( value_alcohol );
  LCD.setCursor( 4, 3 );
}

/**
 * Read the values of the 3 RGB dials
 */
void readRGB() {
  R = map( analogRead( dial_red ), 0, 1000, 0, 255 );
  G = map( analogRead( dial_green ), 0, 1000, 0, 255 );
  B = map( analogRead( dial_blue ), 0, 1000, 0, 255 );

  value_hex = "";
  value_hex.concat( String( R, HEX ) );
  value_hex.concat( String( G, HEX ) );
  value_hex.concat( String( B, HEX ) );
}

/**
 * Send a command to the WP install.
 *
 * @param string success String to look for that means the command was successful.
 */
void sendCommand( char* success = "Success" ) {
  String output = "";

  Serial.print("Command: ");
  Serial.println( command );

  LCD.setCursor( 4, 3 );
  LCD.print( command.substring( 0, 15 ) );

  flashLeds();
  bothLeds();

  // Send the command
  if ( NETWORK_ENABLED ) {
    p.runShellCommand( SSH_PREFIX + " 'wp axp " + command + "'" );
  
    // Read the output.
    while ( p.available()  > 0 ) {
      char c = p.read();
      output += c;
    }
  
    Serial.println( output );
  
    // See if we got a successful response
    if ( output.indexOf( success ) != -1 ) {
      successLed();
    } else {
      unsuccessLed();
    }
  }

  command = "";

  LCD.setCursor( 4, 3 );
  LCD.print( "                " );
  LCD.setCursor( 4, 3 );
  LCD.cursor();
  LCD.blink();
}

void setInitialValues() {
  prev_lasers = digitalRead( switch_lasers );
  prev_sharks = digitalRead( switch_sharks );
  prev_missleLaunch = digitalRead( switch_missleLaunch );
  prev_flame = digitalRead( switch_flame );
  prev_fireAlarm = digitalRead( switch_fireAlarm );
  prev_toilet = digitalRead( switch_toilet );
  prev_pullChain1 = digitalRead( switch_pullChain1 );
  prev_pullChain2 = digitalRead( switch_pullChain2 );
  prev_pullChain3 = digitalRead( switch_pullChain3 );
  prev_triggerColor = digitalRead( switch_triggerColor );
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
 * Toggles the missleLaunch effect on and off
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
 * Toggles the alcohol sensor value
 */
void toggle_alcohol() {
  value_alcohol = map( analogRead( sensor_alcohol ), 0, 1024, 0, 100 );

  if ( value_alcohol >= ALCOHOL_LIMIT && prev_alcohol == LOW ) {
    prev_alcohol = HIGH;

    flashLeds();
    unsuccessLed();
    unsuccessLed();
    unsuccessLed();

    command = "on alcohol";
    sendCommand();

    // @todo Display says fahgetabatit?

    return false;

  } else if ( value_alcohol < ALCOHOL_LIMIT && prev_alcohol == HIGH ) {
    prev_alcohol = LOW;

    command = "off alcohol";
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
    command = "set color ";
    command.concat( value_hex );
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

  // Open the serial port at 9600 bps:
  Serial.begin(9600);

  // Initialize LCD with static labels
  lcdInit();

  // Show our Booting message.
  bootMessage();

  // While booting, show booting message?

  // Start the bridge connection
  if ( NETWORK_ENABLED ) {
    Bridge.begin();
  }

  // Report success
  bridgeSuccess();

  // Print labels for our values
  printSensorLabels();

  // Set initial values
  setInitialValues();
}


//////////////////
// --- LOOP --- //
//////////////////

void loop() {
  // Turn off our LEDs if they're on and it's been long enough
  maybeTurnOffLEDs();

  // Read RGB values
  readRGB();

  // Display loop
  updateDisplay();

  // Only act if our alcohol level is acceptable.
  if ( NO_DISABLE_WHEN_DRUNK || prev_alcohol != HIGH ) {
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
  }

  // Delay so we don't overload the Arduino
  delay( LOOP_DELAY );
}
