#include "ISAMobile.h"

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
double avgR;
double avgL;
const int queueSize = 10;
double avgResults[queueSize];
int distFront, distBack, distRight, distLeft = 0;

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
  /* czujniki! */
  distFront = measure(US_FRONT_TRIGGER_PIN, US_FRONT_ECHO_PIN);
  distBack = measure(US_BACK_TRIGGER_PIN, US_BACK_ECHO_PIN);
  distRight = measure(US_RIGHT_TRIGGER_PIN, US_RIGHT_ECHO_PIN);
  distLeft = measure(US_LEFT_TRIGGER_PIN, US_LEFT_ECHO_PIN);
  
  //char buffer[64];
  //sprintf(buffer, "front: %d, back:", distFront);
  //Serial.println(buffer);
    
  boolean front = false;
  boolean back = false;
  boolean right = false;
  boolean left = false;
  if(distFront < 15 && distFront > 0 ){
    front = true;
  }
  if(distBack < 15 && distBack > 0 ){
    back = true;
  }
  if(distRight < 15 && distRight > 0 ){
    right = true;
  }
  if(distLeft < 15 && distLeft > 0 ){
    left = true;
  }

  if(front || back || left || right) {
    SetPowerLevel(PowerSideEnum::Left, 0.0);
    SetPowerLevel(PowerSideEnum::Right, 0.0);
  }
  
/*
  char rc = 0;
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
        leftVelocity = leftVelocity;
        rightVelocity = rightVelocity;
        /*sprintf(buffer, "\n Direct: %lf; leftVelocity: %lf, rightVelocity: %lf", direct, leftVelocity, rightVelocity);
        Serial.print(buffer);
        
        sprintf(buffer, "\n sumR: %lf", sumR);
        Serial.print(buffer);*/
        pushRight(rightVelocity);
        pushLeft(leftVelocity);

        if(countData < 10){
          countData = countData + 1;
          avgL = leftVelocity;//without it first avg will be from 0
          avgR = rightVelocity;
          /*sprintf(buffer, "\n CountData 10");
          Serial.print(buffer);*/
          
          
        }
        else{
          double popL = popLeft();
          double popR = popRight();
          /*sprintf(buffer, "\nleftBefAvg: %lf, leftPop: %lf, rightBefAvg: %lf, rightPop: %lf", avgL, popL, avgR, popR);
          Serial.print(buffer);*/
          avgL = (avgL + popL)/2;
          avgR = (avgR + popR)/2;
          sprintf(buffer, "\n->    direct: %lf leftAvg: %lf, rightAvg: %lf", direct, avgL, avgR);
          Serial.print(buffer);
        }
        SetPowerLevel(PowerSideEnum::Left, avgR);//swip direction to equals direction of moving
        SetPowerLevel(PowerSideEnum::Right, avgL);

        countNoData = 0;
    }
    else{
        countNoData = countNoData + 1;
        sprintf(buffer, "\n CountNoData: %d", countNoData);
        Serial.print(buffer);
        if(countNoData > 30){ //500000 - 2,3sek?
          popLeft(); //if nothing spotted clear queue
          popRight();
          //countData = 0;
          //sprintf(buffer, "\n CountNoData 50");
          //Serial.print(buffer);
          SetPowerLevel(PowerSideEnum::Left, 0.0);
          SetPowerLevel(PowerSideEnum::Right, 0.0);
          countNoData = 0;
        }
    }
  
  //delay(100);
}


/* do czujnikow ultradzwiekowych */
int measure(int trigger, int echo)
{
  digitalWrite(trigger, false);
  delayMicroseconds(2);

  digitalWrite(trigger, true);
  delayMicroseconds(10);
  digitalWrite(trigger, false);

  // zmierz czas przelotu fali dźwiękowej
  int duration = pulseIn(echo, true);

  // przelicz czas na odległość (1/2 Vsound(t=20st.C))
  int distance = (int)((float)duration * 0.03438f * 0.5f);
  return distance;
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
 FuzzySet* west = new FuzzySet(-270, -270, -70, -20); 
 angleF->addFuzzySet(west); 
 FuzzySet* north = new FuzzySet(-270, -20, 20, 270); 
 angleF->addFuzzySet(north); 
 FuzzySet* east = new FuzzySet(20, 70, 270, 270);
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


//*****************QUEUE******************//



struct kolejka
{
    double dane;
    kolejka *ref;
};
 
kolejka *pointerLeft = NULL; ;
kolejka *tmpLeft = NULL; ;
kolejka *firstLeft = NULL; ;
 
void pushLeft(double dane)
{
    tmpLeft = (struct kolejka*)malloc(sizeof(struct kolejka));
    (*tmpLeft).dane = dane;
    if(pointerLeft == NULL) firstLeft = tmpLeft; 
    else (*pointerLeft).ref = tmpLeft; //bądź pointer->ref
    tmpLeft -> ref = NULL; //przykładowy
    pointerLeft = tmpLeft;
}
 
double popLeft()
{
    double poped;
    if (firstLeft != NULL)
    {
            tmpLeft = (*firstLeft).ref;
            //printf("%d\n", (*first).dane);
            poped = (*firstLeft).dane;
            free(firstLeft);
            firstLeft = tmpLeft;
            if(firstLeft==NULL)
                    pointerLeft = NULL;
    }
    return poped;
}

kolejka *pointerRight = NULL; ;
kolejka *tmpRight = NULL; ;
kolejka *firstRight = NULL; ;
 
void pushRight(double dane)
{
    tmpRight = (struct kolejka*)malloc(sizeof(struct kolejka));
    (*tmpRight).dane = dane;
    if(pointerRight == NULL) firstRight = tmpRight; 
    else (*pointerRight).ref = tmpRight; //bądź pointer->ref
    tmpRight -> ref = NULL; //przykładowy
    pointerRight = tmpRight;
}
 
double popRight()
{
    double poped;
    if (firstRight != NULL)
    {
            tmpRight = (*firstRight).ref;
            //printf("%d\n", (*first).dane);
            poped = (*firstRight).dane;
            free(firstRight);
            firstRight = tmpRight;
            if(firstRight==NULL)
                    pointerRight = NULL;
    }
    return poped;
}
