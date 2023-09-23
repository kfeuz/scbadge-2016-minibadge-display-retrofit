#include "LedControl.h"
#include "Wire.h"


/*********************************************************
 * DEFINES
 *********************************************************/
#define DISPLAY_SIZE 8 //num of characters in display
#define BUFFER_SIZE 256
#define ADDR_LIST_SIZE 127

/*********************************************************
 * Globals variables
 *********************************************************/

char message[BUFFER_SIZE];
uint8_t addr_list[ADDR_LIST_SIZE];
int num_addresses = 0;
int cur_addr = 0;
int message_size = 0;

/*
 pin D7 is connected to the DataIn
 pin D5 is connected to the CLK
 pin D8 is connected to LOAD
 We have only a single MAX72XX.
 */
LedControl lc=LedControl(D7,D5,D8,1);

int intensity = 4; //Not to bright to start

/* we always wait a bit between updates of the display */
unsigned long delaytime=500;
/* # of times through the loop, with modulo applied */
int count = 0;
bool pulse = true;

/*********************************************************
 * Code
 ********************************************************/
void build_info(){
  Serial.println("Build Info:");
  Serial.println(__FILE__);
  Serial.println(__TIME__);
  Serial.println(__DATE__);
}
  
void display_text(){
  for(int i=0; i<8; i++){
    if(i+count < 0 || i+count >= message_size){
      lc.setChar(0, i, ' ', false);
    }
    else{
      lc.setChar(0, i, message[i+count], false);
    }
  }
}

void set_count_offset(){
    count = -7;
}

void scan_addresses(){
  int error = 0;
  num_addresses = 0;
  for(uint8_t address = 1; address < 127; address++ )
  {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      Serial.print("I2C device found at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.print(address,HEX);
      Serial.println("  !");
      addr_list[num_addresses++] = address;
    }
    else if (error==4)
    {
      Serial.print("Unknown error at address 0x");
      if (address<16)
        Serial.print("0");
      Serial.println(address,HEX);
    }    
  }
  Serial.print(num_addresses);
  Serial.println(" I2C addresses found");
}

// This reads from the provided device if it is connected
// getWriteState will output if the minibadge supports writing as well as make read less verbose
void get_message(){
  if (num_addresses == 0 || cur_addr >= num_addresses){
    strncpy(message, "kfeuz badges", 12);
    message[12] = '\0';
    message_size = 12;
    return;
  }
  uint8_t addr = addr_list[cur_addr];
  uint8_t res = Wire.read();
  Wire.beginTransmission(addr);
  if(Wire.endTransmission(addr) == 0){ // Check if device is on bus before beginning read.
    Serial.print(F("Reading from 0x"));
    Serial.print( (addr< 16)?"0":"");
    Serial.println(addr, HEX);
    Serial.println(F("\tSending 0x00 0x01"));

    Wire.beginTransmission(addr);
    Wire.write(0x00);
    Wire.write(0x01);
    Wire.endTransmission();

    Wire.requestFrom(addr, 1);
    res = Wire.read();
    Serial.print(F("\tRecieved 0x"));
    Serial.print( (res< 16)?"0":"");
    Serial.println(res, HEX);

    if(res == 0){
      Serial.println(F("Noting to do.\n\rEnding read."));
    }else if(res == 1){
      Serial.println(F("Read button press.\n\rEnding read."));
    }else if(res == 2){
      Wire.requestFrom(addr, 1);
      res = Wire.read();
      Serial.print(F("Read available message. Length: "));
      Serial.print(res);
      Serial.print(" Text: \"");
      
      uint8_t counter = 0;
      Wire.requestFrom(addr, res);
      while(Wire.available() && counter < BUFFER_SIZE){
        message[counter++] = (char)Wire.read();
      }
      message[res] = '\0';
      message_size = res;
      Serial.print(message);
      Serial.println("\"\n\rEnding read.");
    }

  }else{
    Serial.print(F("Unable to find device at 0x"));
    Serial.print( (addr< 16)?"0":"");
    Serial.println(addr, HEX);
  }
}

void sketch_init(){
  pinMode(BUILTIN_LED, OUTPUT);  // initialize onboard LED as output
  pinMode(D3, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  Wire.begin();
  /*
   The MAX72XX is in power-saving mode on startup,
   we have to do a wakeup call
   */
  lc.shutdown(0,false);
  /* Set the brightness to a medium values */
  lc.setIntensity(0,intensity);
  /* and clear the display */
  lc.clearDisplay(0);

  set_count_offset();
  scan_addresses();
  get_message();
  
  delay(delaytime);
  digitalWrite(BUILTIN_LED, HIGH);
}

void sketch_loop(){
  if(pulse){
    digitalWrite(BUILTIN_LED, LOW);
    digitalWrite(D3, LOW);
  }
  else{
    digitalWrite(BUILTIN_LED, HIGH);
    digitalWrite(D3, HIGH);
  }
  display_text();
  count++;
  if(count > message_size) {
    count = -7;
    cur_addr++;
    if (cur_addr > num_addresses){ //Let it go one extra to display the default message.
      cur_addr = 0;
      scan_addresses();
    }
    get_message();
  }
  delay(delaytime);
}

 
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  build_info();
  sketch_init();
  Serial.println("Ready");
}

void loop() {
  sketch_loop();
}
