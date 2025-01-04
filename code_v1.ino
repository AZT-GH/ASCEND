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

#define motion_intterupt_pin 27


byte initComplete=0; //Stores if the sensor has been intialised
volatile int xydat[2]; //Array with two integers for delta x and delta y
long total[2]; //Array for total movement from home. CURRENTLY UNUSED
volatile byte movementflag=0; //Variable for ongoing movement. CURRENTLY UNUSED
unsigned long curr_time; //Current time in loop
unsigned long poll_time = 0; //Poll time in loop

extern const unsigned short firmware_length; //SROM firmware length
extern const unsigned char firmware_data[]; //SROM firmware data

//Declare variables for debouncing system
int button1_timer_enabled = 0;
int button1_timer_count = 0;

int button2_timer_enabled = 0;
int button2_timer_count = 0;

int button3_timer_enabled = 0;
int button3_timer_count = 0;

int button4_timer_enabled = 0;
int button4_timer_count = 0;

int global_timer_count = 0;

int debounce_length = 20; //in units of ms, lower time is possibly stable but unless 50 cpm is being exceeded 20ms is fine

//Declare dpi related variables and array
int dpi_set = EEPROM.read(0);
int dpi_current;
int dpi_array[4] = {400, 800, 1600, 3200};
int dpi_scaled[4] = {dpi_array[0]/16000, dpi_array[1]/16000, dpi_array[2]/16000, dpi_array[3]/16000};


void setup() {
  delay(50); //Let all ics settle (namely level shifter)
  
  pinMode(8, OUTPUT); //Slave select
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

  //Turn on applicable pullups
  digitalWrite(18, HIGH);
  digitalWrite(19, HIGH);
  digitalWrite(20, HIGH);
  digitalWrite(21, HIGH);
  digitalWrite(22, HIGH);

  digitalWrite(12, HIGH); //Level shifter output enabled
  
  Serial.begin(115200);
  
  SPI.begin(); //Initialise SPI
  SPI.setDataMode(SPI_MODE3); //Clock idle high, Data sampled on rising edge of clock
  SPI.setBitOrder(MSBFIRST); //Most significant bit first
  SPI.setClockDivider(SPI_CLOCK_DIV16); //Clock divided by 16 to get 1MHz clock for spi (up to 2MHz supported)

  attachInterrupt(27, UpdatePointer, FALLING); //Motion pin interrupt

  performStartup(); //Optical sensor startup

  delay(5000); //5s delay for optical sensor to stabilise

  dispRegisters(); //Registers printed over serial
  initComplete = 9; //Optical sensor intialisation complete
  total[0] = 0; //Total X movement from home reset
  total[1] = 0; //Total Y movement from home reset


  //If dpi has not been set the program enters dpi setting mode automatically
  if (dpi_set == 255){
    dpi_set = 0; //First dpi setting
    dpi_current = dpi_array[dpi_set]; //DPI set accordingly to array
    
    //Flash all leds High for 250ms
    digitalWrite(28, HIGH);
    digitalWrite(29, HIGH);
    digitalWrite(30, HIGH);
    digitalWrite(31, HIGH);
   
    delay(250);
    
    //Keep only first dpi led on
    digitalWrite(29, LOW);
    digitalWrite(30, LOW);
    digitalWrite(31, LOW);

    //Dpi settings cycled through by DPI_R until DPI_L is pressed
    while (button_pressed(21) == 0){
      if (digitalRead(22) == 0){
        delay(50); //Crude 50 msdebounce
        if (digitalRead(22) == 0){
          dpi_set ++; 
          dpi_set %= 4; //Dpi_set operated on by modulus to reduce size
          dpi_current = dpi_array[dpi_set]; //Dpi chosen
        }

        //Additional debounce to ensure switch has been released before cycling
        while(digitalRead(22) == 1){
          delay(10);
        } 
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

      delay(250);
      
      digitalWrite(28, LOW);
      digitalWrite(29, LOW);
      digitalWrite(30, LOW);
      digitalWrite(31, LOW);
      }
    
    else{

      //First and third LED flash for 1 second if EEPROM read or write is unsuccseful
      digitalWrite(28, HIGH);
      digitalWrite(30, HIGH);

      Serial.println("EEPROM read/write unsuccesful");

      delay(1000);

      digitalWrite(28, LOW);
      digitalWrite(30, LOW);
    }
  }

  Serial.println("Current DPI : " + dpi_array[dpi_set]);
  
  timer0_init(); //Timer initialised
  Mouse.begin(); //Mouse library intialised

  Serial.println("Startup succesful");

}

void loop() {
  curr_time = micros(); //time at loop start /µs

  //if at least 1 ms has passed since the last poll a new poll can occur
  if(curr_time > poll_time){
    UpdatePointer();
    Mouse.move(xydat[0]/dpi_scaled[dpi_set], xydat[1]/dpi_scaled[dpi_set], 0);
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

}

//function to see if a button has been pressed (pin low) with integrated debouncing
int button_pressed(int pin_number){
  //referenced array of variables
  int* timer_enabled_array []= {&button1_timer_enabled, &button2_timer_enabled, &button3_timer_enabled, &button4_timer_enabled};

  //if the pin is low and the timer is disabled a 1 is returned and the timer is enabled
  if (digitalRead(pin_number) == 0 && *timer_enabled_array[pin_number - 18] == 0){
    *timer_enabled_array[pin_number - 18] = 1;
    
    if (pin_number != 21){ //If buttons 1, 2 or 3 are pressed their coressponding LED lights up
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
    if (pin_number != 21){
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

  TCCR0B |= (1 << CS01) | (1 << CS00); //Prescaler set to 64

  sei(); //Global interrupts enabled
}

void adns_com_begin(){
  digitalWrite(8, LOW);
}

void adns_com_end(){
  digitalWrite(8, HIGH);
}

byte adns_read_reg(byte reg_addr){
  adns_com_begin();
  SPI.transfer(reg_addr & 0x7f);
  delayMicroseconds(100);
  byte data = SPI.transfer(0);

  delayMicroseconds(1);
  adns_com_end();
  delayMicroseconds(19);

  return data;
}

void adns_write_reg(byte reg_addr, byte reg_data){
  adns_com_begin();
  SPI.transfer(reg_addr | 0x80);
  SPI.transfer(reg_data);

  delayMicroseconds(20);
  adns_com_end();
  delayMicroseconds(100);
}

void adns_upload_firmware(){

  Serial.println("Uploading firmware...");
  
  adns_write_reg(Config2, 0x20);
  adns_write_reg(SROM_Enable, 0x1d);

  delay(10);

  adns_write_reg(SROM_Enable, 0x18);
  
  adns_com_begin();
  SPI.transfer(SROM_Load_Burst | 0x80);
  delayMicroseconds(15);

  unsigned char c;

  for(int i = 0; i < firmware_length; i++){ 
    c = (unsigned char)pgm_read_byte(firmware_data + i);
    SPI.transfer(c);
    delayMicroseconds(15);
  }

  adns_read_reg(SROM_ID);

  adns_write_reg(Config2, 0x00);
  adns_write_reg(Config1, 0x15);

  adns_com_end();

}

void performStartup(void){
  
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

void UpdatePointer(void){
  if(initComplete==9){

    //write 0x01 to Motion register and read from it to freeze the motion values and make them available
    adns_write_reg(Motion, 0x01);
    adns_read_reg(Motion);

    // Use 16 bit deltas
    xydat[0] = (adns_read_reg(Delta_X_H) << 8) | adns_read_reg(Delta_X_L);
    xydat[1] = (adns_read_reg(Delta_Y_H) << 8) | adns_read_reg(Delta_Y_L);
    
    movementflag=1;
    if (xydat[0] != 0 && xydat[0] != 0){
      digitalWrite(31, HIGH);
    }
    else{
      digitalWrite(31, LOW);
    }
    }
}

void dispRegisters(void){
  int oreg[7] = {
    0x00,0x3F,0x2A,0x02  };
  char* oregname[] = {
    "Product_ID","Inverse_Product_ID","SROM_Version","Motion"  };
  byte regres;

  digitalWrite(8,LOW);

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
  // Code to execute every 1 ms

  if (button1_timer_enabled == 1){
    button1_timer_count ++;
  }

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

  if(global_timer_count == 5){
    
    global_timer_count = 0;
    
    if (button4_timer_enabled == 1){
      button4_timer_count ++;
    }
    if (button4_timer_count == debounce_length/5){
      button4_timer_enabled = 0;
      button4_timer_count =0;
    }
  }
}
