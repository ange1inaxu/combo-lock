#include <TFT_eSPI.h> // Graphics and font library for ST7735 driver chip
#include <SPI.h> //Used in support of TFT Display
#include <string.h>  //used for some string handling and processing.
#include <mpu6050_esp32.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
MPU6050 IMU; //imu object called, appropriately, imu

const uint8_t BUTTON1 = 45; //pin connected to button
const uint8_t BUTTON2 = 39; //pin connected to button

const uint8_t LOOP_PERIOD = 10; //milliseconds
uint32_t primary_timer = 0;
float angle = 0.0; //angle measured from gyroscope
int combo[3]; //3 digit lock combo
char output[30] = ""; //what to print to screen


void setup() {
  tft.init();  //init screen
  tft.setRotation(2); //adjust rotation
  tft.setTextSize(0);
  tft.fillScreen(TFT_BLUE); //fill blue background
  tft.setTextColor(TFT_BLUE, TFT_BLACK); 
  Serial.begin(115200); //begin serial comms
  delay(100); //wait a bit (100 ms)

  //Set up and calibrate iMU
  Wire.begin();
  IMU.initMPU6050();
  IMU.calibrateMPU6050(IMU.gyroBias, IMU.accelBias);
  delay(50); //pause to make sure comms get set up
  if (IMU.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }

  pinMode(BUTTON1, INPUT_PULLUP); //set input pin as an input!
  pinMode(BUTTON2, INPUT_PULLUP); //set input pin as an input!

}

void loop() {
  lock_combo_fsm();
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}

//Reads gyroscope measurement and returns (int) angular velocity
int get_velocity(){
  //get IMU information:
  IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z gyro values
  IMU.getGres(); // get Gyro scales saved to "gRes"
  IMU.gz = (float)IMU.gyroCount[2] * IMU.gRes;
  return IMU.gz;
}

//Given the angular veclocity, calculate the angular displacement and calculate the corresponding digit
int get_digit(){
  float velocity = get_velocity();

  if (velocity > 1 || velocity < -1){
    angle = angle + 0.001*LOOP_PERIOD*velocity;
  }

  int numbers[10] = {0, 9, 8, 7, 6, 5, 4, 3, 2, 1};
  if (angle < 0){angle += 360;}
  if (angle > 360){angle -= 360;}
  int number = floor(angle / 36.0);
  return numbers[number];
}

// State for button presses
int state = 0;
int REST_UNLOCKED = 0;
int LOCKED = 1;
int ENTRY_MODE1 = 2;
int CHECK1 = 3;
int ENTRY_MODE2 = 4;
int CHECK2 = 5;
int ENTRY_MODE3 = 6;
int CHECK3 = 7;

// State for program_mode
int program_mode = 0;
int button1_pressed = 0; // state for whether button1 is pressed (1) or not (0)
int button2_pressed = 0; // state for whether button2 is pressed (1) or not (0)
int digit;
int solved = 0; //Boolean variable to check if lock combo was solved (1) or not (0)

// Finite state machine for button presses
void lock_combo_fsm(){
  if (state == REST_UNLOCKED){

    // if lock is previously solved
    if (solved){
      tft.fillScreen(TFT_GREEN); //fill green background
      tft.setTextColor(TFT_GREEN, TFT_BLACK);
      tft.setCursor(0, 60, 1);
      tft.println("You did it! Fall seven times and stand up eight."); //print inspirational quote

      // reset variables
      solved = 0;
      program_mode = 0;
      button1_pressed = 0;
    }

    tft.setTextColor(TFT_BLUE, TFT_BLACK);
    tft.setCursor(0, 0, 2);
    tft.println("UNLOCKED");
    tft.println("Please press button 1 to enter a combo.");

    if (button1_pressed==0 && !digitalRead(BUTTON1)){ //B1 pressed
      button1_pressed = 1;
    }

    else if (button1_pressed==1 && digitalRead(BUTTON1)){ //B1 released
      button1_pressed = 0;
      program_mode += 1;
    }

    
    if (button1_pressed==0 && program_mode == 1){
      // Remind user to turn CW for 1st input
      tft.setCursor(0, 120, 2);
      tft.println("Please turn CW ");

      digit = get_digit();
      sprintf(output, "%dXX", digit);
      tft.setCursor(0, 60, 2);
      tft.println(output);
    }
    else if (button1_pressed==1 && program_mode == 1){
      combo[program_mode-1] = digit; //record 1st digit
      sprintf(output, "%dXX", combo[0]);

    } else if (button1_pressed==0 && program_mode == 2){
      // Remind user to turn CCW for 2nd input
      tft.setCursor(0, 120, 2);
      tft.println("Please turn CCW");

      digit = get_digit();
      sprintf(output, "%d%dX", combo[0], digit);
      tft.setCursor(0, 60, 2);
      tft.println(output);

    } else if (button1_pressed==1 && program_mode == 2){ // release button
      combo[program_mode-1] = digit; //record 2nd digit
      sprintf(output, "%d%dX", combo[0], combo[1]);
    } 
    else if (button1_pressed==0 && program_mode == 3){
      // Remind user to turn CW for 1st input
      tft.setCursor(0, 120, 2);
      tft.println("Please turn CW ");

      digit = get_digit();
      sprintf(output, "%d%d%d", combo[0], combo[1], digit);
      tft.setCursor(0, 60, 2);
      tft.println(output);

    } else if (button1_pressed==1 && program_mode == 3){ // release button
      combo[program_mode-1] = digit; //record 3rd digit
      sprintf(output, "%d%d%d", combo[0], combo[1], combo[2]);
    }
    
    if (button2_pressed==0 && !digitalRead(BUTTON2)){
      button2_pressed = 1;
    }
    else if (button2_pressed==1 && digitalRead(BUTTON2)){
      button2_pressed = 0;
      state = LOCKED; //switch to LOCKED stated

      tft.fillScreen(TFT_RED); //fill red background
    }

  }
  else if (state == LOCKED){

    tft.setTextColor(TFT_RED, TFT_BLACK); 
    tft.setCursor(0, 0, 2);
    tft.println("LOCKED");
    tft.println("Press button 1 to input the combo.");


    if (button1_pressed == 0 && !digitalRead(BUTTON1)){ //B1 pressed
      button1_pressed = 1;
    } else if (button1_pressed == 1 && digitalRead(BUTTON1)){//B1 released
      state = ENTRY_MODE1;
      button1_pressed = 0;
    }
  }
  else if (state == ENTRY_MODE1){
    //Remind user to turn CW for 1st digit
    tft.setCursor(0, 120, 2);
    tft.println("Please turn CW ");

    digit = get_digit();
    sprintf(output, "%dXX", digit);
    tft.setCursor(0, 60, 2);
    tft.println(output);

    if (button1_pressed == 0 && !digitalRead(BUTTON1)){ //B1 pressed
      button1_pressed = 1;
      state = CHECK1;
    }
  }
  else if (state == CHECK1){
    //return to locked state if incorrect input
    if (digit != combo[0]){
      state = LOCKED;
    }
    //else progress to 2nd digit entry
    else if (button1_pressed == 1 && digitalRead(BUTTON1)) {
      state = ENTRY_MODE2;
      button1_pressed = 0;
    }
  }
  else if (state == ENTRY_MODE2){
    //Remind user to turn CCW for 2nd digit
    tft.setCursor(0, 120, 2);
    tft.println("Please turn CCW");

    digit = get_digit();
    sprintf(output, "%d%dX", combo[0], digit);
    tft.setCursor(0, 60, 2);
    tft.println(output);

    if (button1_pressed == 0 && !digitalRead(BUTTON1)){ //B1 pressed
      button1_pressed = 1;
      state = CHECK2;
    }

  }
  else if (state == CHECK2){
    //return to locked state if incorrect input
    if (digit != combo[1]){
      state = LOCKED;
    }
    //else progress to 3rd digit entry
    else if (button1_pressed == 1 && digitalRead(BUTTON1)) {
      state = ENTRY_MODE3;
      button1_pressed = 0;
    }

  }
  else if (state == ENTRY_MODE3){
    //Remind user to turn CW for 1st digit
    tft.setCursor(0, 120, 2);
    tft.println("Please turn CW ");

    digit = get_digit();
    sprintf(output, "%d%d%d", combo[0], combo[1], digit);
    tft.setCursor(0, 60, 2);
    tft.println(output);

    if (button1_pressed == 0 && !digitalRead(BUTTON1)){ //B1 pressed
      button1_pressed = 1;
      state = CHECK3;
    }
  }
  else if (state == CHECK3){
    //return to locked state if incorrect input
    if (digit != combo[2]){
      state = LOCKED;
    }
    //else return to UNLOCKED STATE
    else if (button1_pressed == 1 && digitalRead(BUTTON1)) {
      state = REST_UNLOCKED;
      button1_pressed = 0;
      solved = 1;
    }
  }
}
