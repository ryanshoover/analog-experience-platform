#include <LiquidCrystal_I2C.h>
#include <Bridge.h>
#include <Process.h>

// set LCD address
 LiquidCrystal_I2C lcd(0x3f,20,4);

// Analog pin setup
// CHANGE THESE FOR THE LEONARDO??
const int pot0Pin = 0;
const int pot1Pin = 1;
const int pot2Pin = 2;
const int alcoholPin = 3;

// Digital pin setup
const int goodPin = 2;
const int badPin = 3;
const int corePin = 4;
const int pluginPin = 5;
const int fan1Pin = 6;
const int fan2Pin = 7;
const int fan3Pin = 8;
const int plumbingPin = 9;
const int lightPin = 10;
const int flamePin = 11;
const int RGBConfPin = 12;
const int firePin = 13;
const int toiletPin = 20;

// Static values
const int LED_DELAY = 2000;
const int ALCOHOL_LIMIT = 75;
const String SSH_PREFIX = "dbclient -i ~/.ssh/id_dropbear -l 'ryan.hoover@wpengine.com+demowpe' ssh.gcp-us-central1-farm-01.wpengine.io";

// Initial variables
int val0, val1, val2 = 0;
byte R, G, B;
int core, coreLast = 0;
int plugin, pluginLast = 0;
int fan1, fan2, fan3, fan0Last, fan1Last, fan2Last, fan3Last = 0;
int plumbing, plumbingLast = 0;
int flame, flameLast = 0;
int RGB, RGBLast = 0;
int fire, fireLast = 0;
int toilet, toiletLast = 0;
int alcohol, alcoholLast = 0;
char hex[7] = {0};
String command;

// Clear LEDs
void noLeds() {
  digitalWrite( goodPin, LOW );
  digitalWrite( badPin, LOW );
}

// Successful result 
void successLed() {
  digitalWrite( goodPin, HIGH );
  digitalWrite( badPin, LOW );
  delay( LED_DELAY );
  noLeds();
}

// Unsuccessful result
void unsuccessLed() {
  digitalWrite( goodPin, LOW );
  digitalWrite( badPin, HIGH ); 
  delay( LED_DELAY );
  noLeds();
}

// Both LEDs on (Command successfully sent)
void bothLeds() {
  digitalWrite( goodPin, HIGH );
  digitalWrite (badPin, HIGH );
  delay( LED_DELAY );
  noLeds();
}

// Flash both 10 times (Request for command received - e.g. button pressed)
void flashLeds() {
  int count = 10;
  while ( count > 0 ) {
    digitalWrite( goodPin, HIGH );
    digitalWrite( badPin, LOW );
    delay( 100 );
    digitalWrite( goodPin, LOW );
    digitalWrite( badPin, HIGH ); 
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

void lcdInit() {
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor( 0,0 );
  lcd.print( "Analogue Experience" );
  lcd.setCursor( 2,1 );
  lcd.print( "--- Platform ---" );
  lcd.setCursor( 1,2  );
  lcd.print( "RGB Value: #" );
  lcd.setCursor( 0,3 );
  lcd.print( "Alcohol Sensor: " );
}

void readRGB() {
  R = map( analogRead( pot0Pin ), 100, 924, 0, 255 );
  G = map( analogRead( pot1Pin ), 100, 924, 0, 255 );
  B = map( analogRead( pot2Pin ), 100, 924, 0, 255 );
  sprintf( hex, "%02X%02X%02X", R, G, B );
}

void updateDisplay() {
  lcd.setCursor( 13,2 );
  lcd.print( hex ); 
  lcd.setCursor( 16,3 );
  lcd.print( alcohol );
}

void sendCommand( String success = "Success" ) {
  String output = "";

  flashLeds();
  bothLeds();
  
  Process p;
  p.runShellCommand( SSH_PREFIX + " 'cd /home/wpe-user/sites/demowpe; " + command + "'" );
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

  command = String("");
}

///////////////////////////////////////////////////
// DEFINE ALL EXTERNALLY-REACHING FUNCTIONS HERE //
///////////////////////////////////////////////////

bool lightSwitch() {
  if ( digitalRead( lightPin ) == HIGH) {
    return true;    
  } else {
    unsuccessLed();
    delay(500);
    return false;
  }
}

// Main function to update the core when corePin is high
void coreJack() {
  core = digitalRead( corePin );
  if ( core != coreLast && core == HIGH ) {
    command = String("wp core update");
    sendCommand();
  }
  coreLast = core;
}

void pluginJack() {
  plugin = digitalRead( pluginPin );
  if ( plugin != pluginLast && plugin == HIGH ) {
    //    command = String("wp plugin update --all --skip-plugins");
    command = String("wp plugin install woocommerce");
    sendCommand();
  }
  pluginLast = plugin;
}

void fanSwitch() {
  fan1 = digitalRead( fan1Pin );
  fan2 = digitalRead( fan2Pin );
  fan3 = digitalRead( fan3Pin );
  if ( fan1 != fan1Last && fan1 == HIGH ) {
    fan0Last = LOW;
    command = String("wp theme mod set fontsize 12");
    sendCommand();
    
  } else if ( fan2 != fan2Last && fan2 == HIGH ) {
    fan0Last = LOW;
    command = String("wp theme mod set fontsize 16");
    sendCommand();
    
  } else if ( fan3 != fan3Last && fan3 == HIGH ) {
    fan0Last = LOW;
    command = String("wp theme mod set fontsize 21");
    sendCommand();
    
  } else if ( fan0Last != HIGH && fan1 == LOW && fan2 == LOW && fan3 == LOW ) {
    fan0Last = HIGH;
    command = String("wp theme mod set fontsize 36");
    sendCommand();
  
  }
  fan1Last = fan1;
  fan2Last = fan2;
  fan3Last = fan3;
}

void plumbingSwitch() {
  plumbing = digitalRead( plumbingPin );
  if ( plumbing != plumbingLast && plumbing == HIGH ) {
    command = String("wp post update --post_status=publish $(wp post list --post_status=draft,pending,future --field=ID)");
    sendCommand();
  }
  plumbingLast = plumbing;
}

void toiletSensor() {
  toilet = digitalRead( toiletPin );
  if ( toilet != toiletLast && toilet == HIGH ) {
    //sendCommand( command );
  }
  toiletLast = toilet;
}

void colorSensor() {
  RGB = digitalRead( RGBConfPin );
  if ( RGB != RGBLast && RGB == HIGH ) {
    flashLeds();
    command = String("wp theme mod set header_textcolor ");
    command += hex;
    sendCommand();
  }
  RGBLast = RGB;
}

void flameSensor() {
  flame = digitalRead( flamePin );
  if ( flame != flameLast && flame == HIGH ) {
    command = String("wp plugin activate axp-hacked");
    sendCommand();
  }
  flameLast = flame;
}

void fireAlarm() {
  fire = digitalRead( firePin );
  if ( fire != fireLast && fire == HIGH ) {
    command = String("wp plugin deactivate axp-hacked");
    sendCommand();
  }
  fireLast = fire;
}

// Be able to use this as a conditional for other functions by returning true/false
bool alcoholSensor() {
  alcohol = map( analogRead(alcoholPin), 0, 1024, 0, 100 );
  if ( alcohol >= ALCOHOL_LIMIT && alcoholLast == LOW ) {
    flashLeds();
    unsuccessLed();
    unsuccessLed();
    unsuccessLed();

    command = String("wp user update axp --user_pass=''");
    sendCommand();

    // Display says fahgetabatit?
    return false;
    
  } else if ( alcohol < ALCOHOL_LIMIT && alcoholLast == HIGH ) {
    return true;
  }
  alcoholLast = alcohol;
}


///////////////////
// --- SETUP --- //
///////////////////

void setup() {
  // Set PIN types
  pinMode( goodPin, OUTPUT );
  pinMode( badPin, OUTPUT );

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
  if ( ! lightSwitch() ) {
    return;
  }
  
  // Read RGB values
  readRGB();

  // Display loop
  updateDisplay();

  // List all functions we're receiving inputs for here
  coreJack();
  pluginJack();
  fanSwitch();
  plumbingSwitch();
  alcoholSensor();
  colorSensor();
  flameSensor();
  fireAlarm();

  // Delay so we don't overload the Arduino
  delay(200);
}


