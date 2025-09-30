#include <Servo.h>
#include <Stepper.h>

//ultrasonic constants
const int EMPTY_BOX_ULTRASONIC_DISTANCE_SENSITIVITY = 7.1;
const int EMPTY_BOX_ULTRASONIC_DISTANCE_VARIANCE = 30;
//servo constants
const int stepsPerRevolution = 200;
const int servoGateClosed = 140;
const int servoGateOpen = 33;
const int servoBoxTipUp = 110;
const int servoBoxTipDown = 12;
//stepper constants
const int stepdown = 145;
const int finaldrop = 270;


//pinout
int StepperMotor_en1 = 23;
int StepperMotor_en2 = 25;
int StepperMotor_en3 = 27;
int StepperMotor_en4 = 29;

int DCMotorKeychain_en1 = 28;
int DCMotorKeychain_en2 = 26;

int DCMotorBox_en3 = 24;
int DCMotorBox_en4 = 22;

int LinearActuator_en1 = 32;
int LinearActuator_en2 = 34;

int ServoGatePin = 43;
int ServoBoxTipPin = 45;

int whisker_sensor_pin = 13;    // Remember to use a 10K resistor before ground

int trigPin_fullBox = 8;    // Full Box Ultrasonic TRIG pin
int echoPin_fullBox = 9;    // Full Box Ultrasonic ECHO pin
int trigPin_emptyBox = 10;    // Full Box Ultrasonic TRIG pin
int echoPin_emptyBox = 11;    // Full Box Ultrasonic ECHO pin

int KeychainCount = 0;


// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution,StepperMotor_en1,StepperMotor_en2,StepperMotor_en3,StepperMotor_en4);

//Define other variables
double ultrasonic_time, ultrasonic_dist_cm;

bool keychainFound = false;
bool BoxMoveAfterLoad = false;
bool GateOpen = false;
bool exptyBoxAvailable = false;
bool checkForFullBox = false;
bool boxTippedDown = true;
bool sevenKeychainsFound = false;

bool newKeychain = false;
int boxCount = 0;
int keychainCount = 0;
int miscCount = 0;

Servo ServoTipper;          // Declares a Servo object of type Servo
Servo ServoGate;            // Declares a Servo object of type Servo
int servoPosition = 0;


// Setup & assign Arduino logic
void setup() {
  Serial.begin (9600);

  pinMode(DCMotorKeychain_en1, OUTPUT);
  pinMode(DCMotorKeychain_en2, OUTPUT);

  pinMode(DCMotorBox_en3, OUTPUT);
  pinMode(DCMotorBox_en4, OUTPUT);

  pinMode(LinearActuator_en1, OUTPUT);
  pinMode(LinearActuator_en2, OUTPUT);

  pinMode(ServoGatePin, OUTPUT);
  pinMode(ServoBoxTipPin, OUTPUT);

  pinMode(trigPin_emptyBox, OUTPUT);
  pinMode(echoPin_emptyBox, INPUT);
  pinMode(trigPin_fullBox, OUTPUT);
  pinMode(echoPin_fullBox, INPUT);

  pinMode(whisker_sensor_pin, INPUT);    // sets the digital pin as output

  ServoTipper.attach(ServoBoxTipPin);
  ServoGate.attach(ServoGatePin);
  
  myStepper.setSpeed(200);

  /*
	tipBoxUp();
  delay(1000);
	tipBoxDown();
  delay(1000);
  */
	tipBoxUp();
  //ServoTipper.write(servoBoxTipUp);
  changeMotorDirection(LinearActuator_en1,LinearActuator_en2);
  delay(2000);
  ServoGate.write(servoGateClosed);
	turnonMotor(DCMotorKeychain_en1, DCMotorKeychain_en2);
  turnonMotor(DCMotorBox_en3, DCMotorBox_en4);
  myStepper.step(750);
  
}

void loop() {
  
  miscCount++;
  Serial.println(miscCount);
  
                                                      Serial.println("    A");
  
  //if keychain is found, run program to load and lower stepper
  if(!sevenKeychainsFound && checkKeychain()){
	  keychainCount++;
  	//delay for belt to load
	  turnoffMotor(DCMotorBox_en3,DCMotorBox_en4);
	  delay(700);
  	
    //turn belt off
	  turnoffMotor(DCMotorKeychain_en1,DCMotorKeychain_en2);
  	
  	//lower the motor x amount
	  KeychainStep();                         //Stepper not Working Yet
    Serial.print("Kc #");
    Serial.print(keychainCount);
    Serial.println(" found! Lowering platform by 1");
    delay(400);
  	
  	//turn the belt on
	  turnonMotor(DCMotorKeychain_en1, DCMotorKeychain_en2);
    if (!exptyBoxAvailable) {
	    turnonMotor(DCMotorBox_en3, DCMotorBox_en4);
    }
  	
  }
  
                                                                    Serial.println("    B");
  
  //if box is found, turn off the belt motor
  if(checkBox(trigPin_emptyBox, echoPin_emptyBox) && (!GateOpen) && (!exptyBoxAvailable)){
	  //turn off the belt for the box
	  turnoffMotor(DCMotorKeychain_en1,DCMotorKeychain_en2);
    delay(1000);
	  turnoffMotor(DCMotorBox_en3,DCMotorBox_en4);

    Serial.print("Empty box found at ");
    Serial.print(readUltrasonic(trigPin_emptyBox, echoPin_emptyBox));
    Serial.println(" cm away!");

	  turnonMotor(DCMotorKeychain_en1, DCMotorKeychain_en2);

	  exptyBoxAvailable = true;
  }
  
                                                                  Serial.println("    C");
  
  if(keychainCount == 7) {
  	//turn belt off
  
	  turnoffMotor(DCMotorKeychain_en1,DCMotorKeychain_en2);
    sevenKeychainsFound = true;

  	if (exptyBoxAvailable){
      
  		//lower the stepper 
	    KeychainFullLower();    //FIXME
      Serial.println("Lowering Platform");
  	
  		//insert the delay for the stepper to be lowered
  		//delay(700);

      //bump top keychain
      turnonMotor(DCMotorKeychain_en1,DCMotorKeychain_en2);
  		delay(1000);
      turnoffMotor(DCMotorKeychain_en1,DCMotorKeychain_en2);
	
  		//turn on linear actuator to load keychains
		  turnonMotor(LinearActuator_en1,LinearActuator_en2);
	
		  //tip the platform to be ready for incoming box
		  tipBoxUp();
  		boxTippedDown = false;
  	
  		//delay to allow for LA to push in keychains
		  delay(4000);
  	 
  		//retract linear actuator
		  changeMotorDirection(LinearActuator_en1,LinearActuator_en2);
  	
  		//delay to allow LA to retract
		  delay(2000);

		  //open the gate
    	ServoGate.write(servoGateOpen);
	  
  		//turn on the box motor
		  turnonMotor(DCMotorBox_en3,DCMotorBox_en4);
	
		  //delay for the box to pass the gate
		  delay(2000);
  	
		  //close the gate
    	ServoGate.write(servoGateClosed);

      //raise platform
      PlatformFullRaise();    //FIXME
      Serial.println("Raising Platform");

      //deliver the box
  	  tipBoxDown();
      boxCount++;
      Serial.print("Box #");
      Serial.print(boxCount);
      Serial.println(" delivered!");

  		//turn the belts on
		  turnonMotor(DCMotorKeychain_en1, DCMotorKeychain_en2);
      turnonMotor(DCMotorBox_en3,DCMotorBox_en4);
  	
      //reset toggles
  		exptyBoxAvailable = false;
      keychainCount = 0;
      sevenKeychainsFound = false;
      checkForFullBox = true;
  	}
  }
  
}


/*  readUltrasonic is largely based off of this source code
 * Created by ArduinoGetStarted, https://arduinogetstarted.com
 * Tutorial is available here: https://arduinogetstarted.com/tutorials/arduino-ultrasonic-sensor
 */
double readUltrasonic(int trigPin, int echoPin) {
  // generate 10-microsecond pulse to TRIG pin
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // measure duration of pulse from ECHO pin
  ultrasonic_time = pulseIn(echoPin, HIGH);

  // calculate the distance
  ultrasonic_dist_cm = 0.017 * ultrasonic_time;

  // print the value to Serial Monitor
  
  //Serial.print("distance: ");
  //Serial.print(ultrasonic_dist_cm);
  //Serial.println(" cm");

  //delay(500);
  return ultrasonic_dist_cm;
}


bool checkBox(int trigPin, int echoPin) {
  bool boxPresent = false;
  double distRead = readUltrasonic(trigPin, echoPin);
  //Serial.print("distRead = ");
  //Serial.println(distRead);

	if ((distRead > EMPTY_BOX_ULTRASONIC_DISTANCE_SENSITIVITY + EMPTY_BOX_ULTRASONIC_DISTANCE_VARIANCE) || (distRead < EMPTY_BOX_ULTRASONIC_DISTANCE_SENSITIVITY - EMPTY_BOX_ULTRASONIC_DISTANCE_VARIANCE)) {
		boxPresent = true;
	}
	
	return boxPresent;
}

bool checkKeychain() {		  // Remember to use a 10K resistor before ground
	bool keychainFound = false;
	const int errorCheckDelay = 5;
	
	if (digitalRead(whisker_sensor_pin) == HIGH) {
    //Serial.println("Keychain passed Check 1!");
	  delay(errorCheckDelay);
	  if (digitalRead(whisker_sensor_pin) == HIGH) {
			keychainFound = true;
		}
	}
	
	return keychainFound;
}

void tipBoxDown() {
	for (int angle = servoBoxTipUp; angle > servoBoxTipDown; angle -= 1) {
		ServoTipper.write(angle);
  	delay(7);
	}
}

void tipBoxUp() {
	ServoTipper.write(servoBoxTipUp);
}

void KeychainStep(){   // lower for 1 keychain
  myStepper.step(stepdown);
  delay(5);
}

void KeychainFullLower(){  //lower all the way to the bottom
  myStepper.step(finaldrop);
  delay(5);
}

void PlatformFullRaise(){ // raise to initial position
  myStepper.step(-((stepdown*7)+finaldrop));
  delay(5);
}

void turnonMotor(int en1, int en2){ //turns on motor
	digitalWrite(en1, HIGH);
	digitalWrite(en2, LOW);
}

void turnoffMotor(int en1, int en2){ //turns off motor
	digitalWrite(en1, LOW);
	digitalWrite(en2, LOW);
}

void changeMotorDirection(int en1, int en2){ //switches motor direction
	digitalWrite(en1, LOW);
	digitalWrite(en2, HIGH);
}

