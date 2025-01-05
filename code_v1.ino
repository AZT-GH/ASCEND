#include <SPI.h>
#include <EEPROM.h>
#include <Mouse.h>
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// Registers
#define Product_ID  0x00
#define Revision_ID 0x01
#define Motion  0x02
#define Delta_X_L 0x03
#define Delta_X_H 0x04
#define Delta_Y_L 0x05
#define Delta_Y_H 0x06
#define SQUAL 0x07
#define Raw_Data_Sum  0x08
#define Maximum_Raw_data  0x09
#define Minimum_Raw_data  0x0A
#define Shutter_Lower 0x0B
#define Shutter_Upper 0x0C
#define Control 0x0D
#define Config1 0x0F
#define Config2 0x10
#define Angle_Tune  0x11
#define Frame_Capture 0x12
#define SROM_Enable 0x13
#define Run_Downshift 0x14
#define Rest1_Rate_Lower  0x15
#define Rest1_Rate_Upper  0x16
#define Rest1_Downshift 0x17
#define Rest2_Rate_Lower  0x18
#define Rest2_Rate_Upper  0x19
#define Rest2_Downshift 0x1A
#define Rest3_Rate_Lower  0x1B
#define Rest3_Rate_Upper  0x1C
#define Observation 0x24
#define Data_Out_Lower  0x25
#define Data_Out_Upper  0x26
#define Raw_Data_Dump 0x29
#define SROM_ID 0x2A
#define Min_SQ_Run  0x2B
#define Raw_Data_Threshold  0x2C
#define Config5 0x2F
#define Power_Up_Reset  0x3A
#define Shutdown  0x3B
#define Inverse_Product_ID  0x3F
#define LiftCutoff_Tune3  0x41
#define Angle_Snap  0x42
#define LiftCutoff_Tune1  0x4A
#define Motion_Burst  0x50
#define LiftCutoff_Tune_Timeout 0x58
#define LiftCutoff_Tune_Min_Length  0x5A
#define SROM_Load_Burst 0x62
#define Lift_Config 0x63
#define Raw_Data_Burst  0x64
#define LiftCutoff_Tune2  0x65



byte initComplete=0; //Stores if the sensor has been intialised
volatile int xydat[2]; //Array with two integers for delta x and delta y
long total[2]; //Array for total movement from home. CURRENTLY UNUSED
unsigned long curr_time; //Current time in loop
unsigned long poll_time = 0; //Poll time in loop

extern const unsigned short firmware_length; //SROM firmware length
extern const unsigned char firmware_data[]; //SROM firmware data

//Declare variables for debouncing system
volatile int button1_timer_enabled = 0;
int button1_timer_count = 0;

volatile int button2_timer_enabled = 0;
int button2_timer_count = 0;

volatile int button3_timer_enabled = 0;
int button3_timer_count = 0;

volatile int button4_timer_enabled = 0;
int button4_timer_count = 0;

volatile int button5_timer_enabled = 0;
int button5_timer_count = 0;

int debounce_length = 1.125; //in units of ms (must be a multiple of 1/8 or the debouncing WILL NOT WORK), should be stable for around 500k clicks or so
//Click polling rate = 1000/debounce_length (by default (1.125 debounce length) it is 889Hz) (The switches are not rated for below 1ms of bounce so try at your own risk)

//Declare dpi related variables and array
int dpi_set = EEPROM.read(0);

float dpi_array[4] = {400, 800, 1600, 3200};
float dpi_scaled[4] = {dpi_array[0]/16000, dpi_array[1]/16000, dpi_array[2]/16000, dpi_array[3]/16000};


void setup() {
  
  debounce_length *= 8;
  
  delay(50); //Let all ics settle (namely level shifter)

  if (dpi_set < 0 || dpi_set >= 4) {
  dpi_set = 0; // Default to a safe value
  }

  
  pinMode(8, OUTPUT); //Slave select active low
  pinMode(12, OUTPUT); //Level shifter output enable
  pinMode(28, OUTPUT); //LED cluster 1
  pinMode(29, OUTPUT); //LED cluster 2
  pinMode(30, OUTPUT); //LED cluster 3
  pinMode(31, OUTPUT); //LED cluster 4

  pinMode(18, INPUT_PULLUP); //Left click
  pinMode(19, INPUT_PULLUP); //Middle click
  pinMode(20, INPUT_PULLUP); //Right click  
  pinMode(21, INPUT_PULLUP); //DPI L Switch
  pinMode(22, INPUT_PULLUP); //DPI R Switch
  pinMode(27, INPUT); //Motion o/p from optical sensor


  digitalWrite(12, HIGH); //Level shifter output enabled
  
  Serial.begin(115200);
  

  SPI.begin(); //Initialise SPI
  SPI.setDataMode(SPI_MODE3); //Clock idle high, Data sampled on rising edge of clock
  SPI.setBitOrder(MSBFIRST); //Most significant bit first
  SPI.setClockDivider(SPI_CLOCK_DIV16); //Clock divided by 16 to get 1MHz clock for spi (up to 2MHz supported)

  performStartup(); //Optical sensor startup

  delay(5000); //5s delay for optical sensor to stabilise

  dispRegisters(); //Registers printed over serial
  initComplete = 9; //Optical sensor intialisation complete
  total[0] = 0; //Total X movement from home reset
  total[1] = 0; //Total Y movement from home reset

  Serial.println("Current DPI : " + int(dpi_array[dpi_set]));
  
  Mouse.begin(); //Mouse library intialised
  timer0_init(); //Timer initialised

  Serial.println("Startup succesful");

}

void loop() {
  curr_time = micros(); //time at loop start /µs

  //if at least 1 ms has passed since the last poll a new poll can occur
  if(curr_time > poll_time){
    UpdatePointer();
    // Mouse.move(xydat[0]/dpi_scaled[dpi_set], xydat[1]/dpi_scaled[dpi_set], 0); this line needs to be rewritten
  }

  poll_time = curr_time + 1010; //1010µS ≈ 1000hz,  1250µS = 800hz, 2000µS = 500hz, 4000µS = 250hz  

  //if the left button is pressed a left click is sent to the computer
  //the same applies for all of the if statements here
  if (button_pressed(18) == 1){
    Mouse.press(MOUSE_LEFT);
  }
  else{
    Mouse.release(MOUSE_LEFT);
  }

    if (button_pressed(19) == 1){
    Mouse.press(MOUSE_MIDDLE);
  }
  else{
    Mouse.release(MOUSE_MIDDLE);
  }

    if (button_pressed(20) == 1){
    Mouse.press(MOUSE_RIGHT);
  }
  else{
    Mouse.release(MOUSE_RIGHT);
  }

  if (button_pressed(21) == 1){
    
    digitalWrite(28, HIGH);
    digitalWrite(29, HIGH);
    digitalWrite(30, HIGH);
    digitalWrite(31, HIGH);
    
    while (button_pressed(21) == 1){
      delay(10);
    }

    digitalWrite(28, LOW);
    digitalWrite(29, LOW);
    digitalWrite(30, LOW);
    digitalWrite(31, LOW);

    digitalWrite(dpi_set + 28, HIGH);

    while (button_pressed(21) == 0){
      if (button_pressed(22) == 1){
        dpi_set ++; 
        dpi_set %= 4; //Dpi_set operated on by modulus to reduce size
      }

      //Additional debounce to ensure switch has been released before cycling
      while(button_pressed(22) == 0){
        delay(10);
      } 
    }
    //Additional debounce to ensure switch has been released before saving
    while (button_pressed(21) == 1){
      delay(10);
    }
    
    EEPROM.write(0, dpi_set); //Dpi written to EEPROM after DPI_L is pressed

    if (EEPROM.read(0) == dpi_set){
    
      //All LEDs flashed to confirm EEPROM writing
      digitalWrite(28, HIGH);
      digitalWrite(29, HIGH);
      digitalWrite(30, HIGH);
      digitalWrite(31, HIGH);

      Serial.println("DPI successfully saved to EEPROM");
      Serial.println("Current DPI : " + int(dpi_array[dpi_set]));

      delay(250);
      
      digitalWrite(28, LOW);
      digitalWrite(29, LOW);
      digitalWrite(30, LOW);
      digitalWrite(31, LOW);
      }
    
    //First and third LED flash for 1 second if EEPROM read or write is unsuccseful
    else{
      digitalWrite(28, HIGH);
      digitalWrite(30, HIGH);

      Serial.println("EEPROM DPI read/write unsuccesful");

      delay(1000);

      digitalWrite(28, LOW);
      digitalWrite(30, LOW);
    }
  }
}

//function to see if a button has been pressed (pin low) with integrated debouncing
int button_pressed(int pin_number){
  //referenced array of variables
  int* timer_enabled_array []= {&button1_timer_enabled, &button2_timer_enabled, &button3_timer_enabled, &button4_timer_enabled, &button5_timer_enabled};

  //if the pin is low and the timer is disabled a 1 is returned and the timer is enabled
  if (digitalRead(pin_number) == 0 && *timer_enabled_array[pin_number - 18] == 0){
    *timer_enabled_array[pin_number - 18] = 1;
    
    if (pin_number != 21 && pin_number != 22){ //If buttons 1, 2 or 3 are pressed their coressponding LED lights up
      digitalWrite(pin_number + 10, HIGH);
    } 
    return 1;

  }
  //If the timer is already on a 1 is returned
  else if (*timer_enabled_array[pin_number - 18] == 1){
    return 1;
  }

  //If the input is high and the timer is off a 0 is returned and the relevant LED is turned off (for buttons 1, 2 or 3)
  else{
    if (pin_number != 21 && pin_number != 22){
      digitalWrite(pin_number + 10, LOW);
    } 
    return 0;   
  }

}

void timer0_init(){
  TCCR0A = 0x00; //Clears the TCCR0A register
  TCCR0B = (1 << WGM02);  //CTC mode selected for TTCR0B Register

  OCR0A = 249; //OCR0A register set to 249

  TIMSK0 = (1 << OCIE0A); //This will trigger an interrupt when the timer's value matches OCR0A

  TCCR0B = (1 << CS01); //Prescaler set to 8

  sei(); //Global interrupts enabled
}

void adns_com_begin(){
  digitalWrite(8, LOW); //Slave select low (active low)
}

void adns_com_end(){
  digitalWrite(8, HIGH); //Slave select high (active low)
}

byte adns_read_reg(byte reg_addr){
  adns_com_begin();
  SPI.transfer(reg_addr & 0x7f); //Register address sent with MSbit = 0 to indicate read
  delayMicroseconds(100); //Serial read access delay
  byte data = SPI.transfer(0); 

  delayMicroseconds(1); //Dont understand " tSCLK-NCS for read operation is 120ns "
  adns_com_end();
  delayMicroseconds(19);

  return data;
}

void adns_write_reg(byte reg_addr, byte reg_data){
  adns_com_begin();
  SPI.transfer(reg_addr | 0x80); //Register address sent with MSbit = 1 to indicate read
  SPI.transfer(reg_data);

  delayMicroseconds(20); //Dont understand " tSCLK-NCS for write operation "
  adns_com_end();
  delayMicroseconds(100);
}

void adns_upload_firmware(){

  Serial.println("Uploading firmware...");
  
  adns_write_reg(Config2, 0x20); //Disable rest mode
  adns_write_reg(SROM_Enable, 0x1d); //SROM written to for intialising

  delay(10); //Wait for one frame period (value is overkill but this function is not time critical)

  adns_write_reg(SROM_Enable, 0x18); //SROM download start

  //SROM data written in bursts
  adns_com_begin();
  SPI.transfer(SROM_Load_Burst | 0x80);
  delayMicroseconds(15);

  //All bytes of firmware sent
  unsigned char c;
  for(int i = 0; i < firmware_length; i++){ 
    c = (unsigned char)pgm_read_byte(firmware_data + i);
    SPI.transfer(c);
    delayMicroseconds(15);
  }

  //ID verified
  adns_read_reg(SROM_ID);

  //0x00 written as this is a wired mouse (0x20 for wireless mice)
  adns_write_reg(Config2, 0x00);

  //CPI resolution set (16000)
  adns_write_reg(Config1, 0x15);

  adns_com_end();

}

void performStartup(){
  
  // ensure that the serial port is reset
  adns_com_end();
  adns_com_begin();
  adns_com_end();


  adns_write_reg(Power_Up_Reset, 0x5a); // force reset
  
  delay(50); // wait for reboot
  
  // read registers 0x02 to 0x06 (and discard the data)
  adns_read_reg(Motion);
  adns_read_reg(Delta_X_L);
  adns_read_reg(Delta_X_H);
  adns_read_reg(Delta_Y_L);
  adns_read_reg(Delta_Y_H);
  
  // upload the firmware
  adns_upload_firmware();
  delay(10);
    
  Serial.println("Optical Chip Initialized");
  
}

void UpdatePointer(){
  if(initComplete==9){

    //write 0x01 to Motion register and read from it to freeze the motion values and make them available
    adns_write_reg(Motion, 0x01);
    adns_read_reg(Motion);

    // Use 16 bit deltas
    xydat[0] = (adns_read_reg(Delta_X_H) << 8) | adns_read_reg(Delta_X_L);
    xydat[1] = (adns_read_reg(Delta_Y_H) << 8) | adns_read_reg(Delta_Y_L);
    
    //Motion LED on if any motion detected
    if (xydat[0] != 0 || xydat[0] != 0){
      digitalWrite(31, HIGH);
    }
    else{
      digitalWrite(31, LOW);
    }
  }
}

//Register addresses and names initalised
void dispRegisters(){
  int oreg[7] = {           //Dont know why this is set to 7 (potentially mistyped)
    0x00,0x3F,0x2A,0x02  };
  char* oregname[] = {
    "Product_ID","Inverse_Product_ID","SROM_Version","Motion"  };
  byte regres;

  digitalWrite(8,LOW);

  //Registers printed over serial in human readable format
  int rctr=0;
  for(rctr=0; rctr<4; rctr++){
    SPI.transfer(oreg[rctr]);
    delay(1);
    Serial.println("---");
    Serial.println(oregname[rctr]);
    Serial.println(oreg[rctr],HEX);
    regres = SPI.transfer(0);
    Serial.println(regres,BIN);  
    Serial.println(regres,HEX);  
    delay(1);
  }
  digitalWrite(8,HIGH);
} 

ISR(TIMER0_COMPA_vect) {
  // Code to execute every 1/8 ms

  //If the timer is enabled the count increases
  if (button1_timer_enabled == 1){
    button1_timer_count ++;
  }

  //If the timer is at the debounce length the timer count is reset and the timer is disabled
  if (button1_timer_count == debounce_length){
    button1_timer_enabled = 0;
    button1_timer_count = 0;
  }

  if (button2_timer_enabled == 1){
    button2_timer_count ++;
  }

  if (button2_timer_count == debounce_length){
    button2_timer_enabled = 0;
    button2_timer_count = 0;
  }

  if (button3_timer_enabled == 1){
    button3_timer_count ++;
  }

  if (button3_timer_count == debounce_length){
    button3_timer_enabled = 0;
    button3_timer_count = 0;
  }

  if (button4_timer_enabled == 1){
    button4_timer_count ++;
  }
  if (button4_timer_count == debounce_length){
    button4_timer_enabled = 0;
    button4_timer_count =0;
  }

    if (button5_timer_enabled == 1){
    button5_timer_count ++;
  }
  if (button5_timer_count == debounce_length){
    button5_timer_enabled = 0;
    button5_timer_count =0;
  }
}
