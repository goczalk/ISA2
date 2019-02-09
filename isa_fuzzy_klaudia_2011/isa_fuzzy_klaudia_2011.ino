#include <ISAMobile.h>

#include <FuzzyRule.h>
#include <FuzzyComposition.h>
#include <Fuzzy.h>
#include <FuzzyRuleConsequent.h>
#include <FuzzyOutput.h>
#include <FuzzyInput.h>
#include <FuzzyIO.h>
#include <FuzzySet.h>
#include <FuzzyRuleAntecedent.h>
#include "helper.h"

// Step 1 -  Instantiating an object library
Fuzzy* fuzzy = new Fuzzy();

QMC5883 qmc;
char buffer[64];
int BASE_VELOCITY = 100;

//PID
//Define Variables we'll be connecting to
double Setpoint, Input, Output;

//Specify the links and initial tuning parameters
//PID myPID(&Input, &Output, &Setpoint, 2, 0.15/*szybciej widac zmiany*/, 0, DIRECT);
//PID myPID(&Input, &Output, &Setpoint, 2, 5, 1, DIRECT);


// to get info from python opencv
const byte numChars = 32;
char receivedChars[numChars];
char tempChars[numChars];        // temporary array for use when parsing
dataPacket packet;
boolean newData = false;
volatile int countNoData = 0;
int countData = 0;
double sumR = 0.0;
double sumL = 0.0;



void setup(void)
{
  // Czujniki ultradĹşwiekowe
  for (int i = (int)UltraSoundSensor::__first; i <= (int)UltraSoundSensor::__last; i++)
  {
    pinMode(ultrasound_trigger_pin[i], OUTPUT);
    pinMode(ultrasound_echo_pin[i], INPUT);
    
    digitalWrite(ultrasound_trigger_pin[i], 0);
  }
  
  // Silniki
  pinMode(A_PHASE, OUTPUT);
  pinMode(A_ENABLE, OUTPUT);
  pinMode(B_PHASE, OUTPUT);
  pinMode(B_ENABLE, OUTPUT);
  pinMode(MODE, OUTPUT);

  digitalWrite(MODE, true);
  SetPowerLevel(PowerSideEnum::Left, 0);
  SetPowerLevel(PowerSideEnum::Right, 0);

  Serial.begin(9600);
  Serial1.begin(9600);
  Serial3.begin(9600);
  Serial.print("Test... ");
  
  Wire.begin();
  qmc.init();
  
  //Serial.begin(9600); // HC06
  qmc.reset();

  //initialize the variables we're linked to
  Input = analogRead(0);

  
  //to co chcemy miec

//  przy naszym
//  Setpoint = 90;
  Setpoint = 0;

  //turn the PID on
//  myPID.SetMode(AUTOMATIC);
  
  //do naszych wartosci
//  myPID.SetOutputLimits(-90, 90);
//  myPID.SetOutputLimits(-180, 180);

  fuzzylogic();
}



void loop(void){
  char rc = 0;
/*
while(1)
{
  Serial.print("1");
  Serial3.print("1");

  delay(200);
}*/
  
 /* while (Serial3.available() > 0) {
        Serial.print("av");
        rc = Serial3.read();
        Serial.print("rcread");
        Serial.println(rc);
  }*/
  
  recvWithStartEndMarkers();
  if (newData == true) {
        strcpy(tempChars, receivedChars);
            // this temporary copy is necessary to protect the original data
            //   because strtok() used in parseData() replaces the commas with \0
        packet = parseData();
        
        newData = false;
    
      
        double direct = (double)packet.packet_int;
        //Serial.println(packet.)
        
        fuzzy->setInput(1, direct);
      
        fuzzy->fuzzify();
      
        double leftVelocity = fuzzy->defuzzify(1);
        double rightVelocity= fuzzy->defuzzify(2);
        leftVelocity = leftVelocity/2;
        rightVelocity = rightVelocity/2;
        sprintf(buffer, "\n Direct: %lf; leftVelocity: %lf, rightVelocity: %lf", direct, rightVelocity, leftVelocity);
        Serial.print(buffer);
        countData = countData + 1;
        sumR = sumR + leftVelocity;
        sumL = sumL + rightVelocity;
        sprintf(buffer, "\n sumR: %lf", sumR);
        Serial.print(buffer);
        if(countData >= 10){
          sprintf(buffer, "\n CountData 10");
          Serial.print(buffer);
          double avgR = sumR/10;
          double avgL = sumL/10;
          sprintf(buffer, "\nleftAvg: %lf, rightAvg: %lf", avgL, avgR);
          Serial.print(buffer);
          
          countData = 0;
          sumR = 0.0;
          sumL = 0.0;
        }
        SetPowerLevel(PowerSideEnum::Left, rightVelocity);
        SetPowerLevel(PowerSideEnum::Right, leftVelocity);
    }
    else{
        countNoData = countNoData + 1;
        //sprintf(buffer, "\n CountNoData: %d", countNoData);
        //Serial.print(buffer);
        if(countNoData > 500000){
          //sprintf(buffer, "\n CountNoData 50");
          //Serial.print(buffer);
          SetPowerLevel(PowerSideEnum::Left, 0.0);
          SetPowerLevel(PowerSideEnum::Right, 0.0);
          countNoData = 0;
        }
    }
  
  //delay(100);
}
void recvWithStartEndMarkers2()
{
    char rc;
    char startMarker = '<';
    char endMarker = '>';
    Serial.println();
    while (Serial3.available() > 0){
        rc = Serial3.read();
        if(rc == startMarker){
          Serial.print("start");
        }
        else if(rc == endMarker){
          Serial.print("end");
        }
        else{
          Serial.print(rc);
        }
        Serial.println();
    }
}
void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;

    while (Serial3.available() > 0 && newData == false) {
        rc = Serial3.read();
        
        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
                /*Serial.print("rc ");
                Serial.print(rc);
                Serial.println();*/
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                /*Serial.print("receivedChars " );
                Serial.print(receivedChars);
                Serial.println();*/
                recvInProgress = false;
                ndx = 0;
                newData = true;
                Serial3.flush();
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}
dataPacket parseData() {      // split the data into its parts

    dataPacket tmpPacket;

    char * strtokIndx; // this is used by strtok() as an index

    strtokIndx = strtok(tempChars,",");      // get the first part - the string
    strcpy(tmpPacket.message, strtokIndx); // copy it to messageFromPC
 
    strtokIndx = strtok(NULL, ","); // this continues where the previous call left off
    tmpPacket.packet_int = atoi(strtokIndx);     // convert this part to an integer

    strtokIndx = strtok(NULL, ",");
    tmpPacket.packet_float = atof(strtokIndx);     // convert this part to a float

    return tmpPacket;
}

void showParsedData(dataPacket packet) {
    Serial.print("Message ");
    Serial.println(packet.message);
    Serial.print("Integer ");
    Serial.println(packet.packet_int);
    Serial.print("Float ");
    Serial.println(packet.packet_float);
}

void fuzzylogic(void){
  // Step 2 - Creating a FuzzyInput distance
  FuzzyInput* angleF = new FuzzyInput(1);// With its ID in param

  // Creating the FuzzySet to compond FuzzyInput distance
 FuzzySet* west = new FuzzySet(0, 0, -45, -10); 
 angleF->addFuzzySet(west); 
 FuzzySet* north = new FuzzySet(-45, -10, 10, 45); 
 angleF->addFuzzySet(north); 
 FuzzySet* east = new FuzzySet(10, 45, 90, 90);
 angleF->addFuzzySet(east); 

 fuzzy->addFuzzyInput(angleF); // Add FuzzyInput to Fuzzy object


// Passo 3a - Creating FuzzyOutput velocity for left wheels
 FuzzyOutput* leftWheelsVelocity = new FuzzyOutput(1);// With its ID in param

 // Creating FuzzySet to compond FuzzyOutput 
 FuzzySet* straightL = new FuzzySet(50, 200, 255, 255); // 
 leftWheelsVelocity->addFuzzySet(straightL); // Add FuzzySet slow to turnnessF
 FuzzySet* backL = new FuzzySet(-255, -255, -200, -50); // 
 leftWheelsVelocity->addFuzzySet(backL); // Add FuzzySet fast to turnnessF

 fuzzy->addFuzzyOutput(leftWheelsVelocity); // Add FuzzyOutput to Fuzzy object


// Passo 3b - Creating FuzzyOutput velocity for right wheels
 FuzzyOutput* rightWheelsVelocity = new FuzzyOutput(2);// With its ID in param

FuzzySet* straightR = new FuzzySet(50, 200, 255, 255); // 
 FuzzySet* backR = new FuzzySet(-255, -255, -200, -50); // 

 // Creating FuzzySet to compond FuzzyOutput 
 rightWheelsVelocity->addFuzzySet(straightR); // Add FuzzySet slow to turnnessF
 rightWheelsVelocity->addFuzzySet(backR); // Add FuzzySet fast to turnnessF
 fuzzy->addFuzzyOutput(rightWheelsVelocity); // Add FuzzyOutput to Fuzzy object


 //Passo 4 - Assembly the Fuzzy rules
 // FuzzyRule "IF ifDirectionWest THEN staright left, back right"
 FuzzyRuleAntecedent* ifDirectionWest = new FuzzyRuleAntecedent(); // Instantiating an Antecedent to expression
 ifDirectionWest->joinSingle(west); // Adding corresponding FuzzySet to Antecedent object
 FuzzyRuleConsequent* thenTurnRight = new FuzzyRuleConsequent(); // Instantiating a Consequent to expression
 thenTurnRight->addOutput(straightL);// Adding corresponding FuzzySet to Consequent object
 thenTurnRight->addOutput(backR);// Adding corresponding FuzzySet to Consequent object
 // Instantiating a FuzzyRule object
 FuzzyRule* fuzzyRule01 = new FuzzyRule(1, ifDirectionWest, thenTurnRight); // Passing the Antecedent and the Consequent of expression
 
 fuzzy->addFuzzyRule(fuzzyRule01); // Adding FuzzyRule to Fuzzy object
 
 // FuzzyRule "IF ifDirectionEast THEN staright back, back left"
 FuzzyRuleAntecedent* ifDirectionEast = new FuzzyRuleAntecedent(); // Instantiating an Antecedent to expression
 ifDirectionEast->joinSingle(east); // Adding corresponding FuzzySet to Antecedent object
 FuzzyRuleConsequent* thenTurnLeft = new FuzzyRuleConsequent(); // Instantiating a Consequent to expression
 thenTurnLeft->addOutput(backL);// Adding corresponding FuzzySet to Consequent object
  thenTurnLeft->addOutput(straightR);// Adding corresponding FuzzySet to Consequent object
 // Instantiating a FuzzyRule object
 FuzzyRule* fuzzyRule02 = new FuzzyRule(2, ifDirectionEast, thenTurnLeft); // Passing the Antecedent and the Consequent of expression
 
 fuzzy->addFuzzyRule(fuzzyRule02); // Adding FuzzyRule to Fuzzy object
 
 // FuzzyRule "IF distance = big THEN velocity = fast"
 FuzzyRuleAntecedent* ifDirectionNorth = new FuzzyRuleAntecedent(); // Instantiating an Antecedent to expression
 ifDirectionNorth->joinSingle(north); // Adding corresponding FuzzySet to Antecedent object
 FuzzyRuleConsequent* thenStraight = new FuzzyRuleConsequent(); // Instantiating a Consequent to expression
 thenStraight->addOutput(straightL);// Adding corresponding FuzzySet to Consequent object
 thenStraight->addOutput(straightR);
 // Instantiating a FuzzyRule object
 FuzzyRule* fuzzyRule03 = new FuzzyRule(3, ifDirectionNorth, thenStraight); // Passing the Antecedent and the Consequent of expression
 
 fuzzy->addFuzzyRule(fuzzyRule03); // Adding FuzzyRule to Fuzzy object

};

void SetPowerLevel(PowerSideEnum side, int level)
{
  level = constrain(level, -255, 255);
  
  if (side == PowerSideEnum::Right) {
    if (level > 0) {
      // do przodu
      digitalWrite(A_PHASE, 1);
      analogWrite(A_ENABLE, level);
    } else if (level < 0) {
      // do tyĹu
      digitalWrite(A_PHASE, 0);
      analogWrite(A_ENABLE, -level);
    } else {
      // stop
      digitalWrite(A_PHASE, 0);
      analogWrite(A_ENABLE, 0);
    }
  }
  
  if (side == PowerSideEnum::Left) {
    if (level > 0) {
      // do przodu
      digitalWrite(B_PHASE, 1);
      analogWrite(B_ENABLE, level);
    } else if (level < 0) {
      // do tyĹu
      digitalWrite(B_PHASE, 0);
      analogWrite(B_ENABLE, -level);
    } else {
      // stop
      digitalWrite(B_PHASE, 0);
      analogWrite(B_ENABLE, 0);
    }
  } 
}
