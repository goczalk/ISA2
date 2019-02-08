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

double getCompassAngle(){
    qmc.measure();
    int16_t x = qmc.getX();
    int16_t y = qmc.getY();

//https://pastebin.com/MTj68RFr
//    x = -(x + 90) / 5.4;
//    y = -(y + 200) / 5.8;

        double angle = atan2(x,y);
        angle = (angle > 0 ? angle : (2*PI + angle)) * 360 / (2*PI);
        angle -= 180;
//        y = angle;

        // 0 = N, 180/-180 = S, E = 90, W = -90
        // ale bladna -155 jest dodatni, a na 155 jest ujemny
        return angle;
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
    
/*      
        double direct = (double)packet.packet_int;
        Serial.println(packet.
        
        fuzzy->setInput(1, -direct);
      
        fuzzy->fuzzify();
      
        double leftVelocity = fuzzy->defuzzify(1);
        double rightVelocity= fuzzy->defuzzify(2);
      
      //  sprintf(buffer, "\n Direct: %lf; leftVelocity: %lf, rightVelocity: %lf", direct, leftVelocity, rightVelocity);
      //  Serial.print(buffer);
      
        SetPowerLevel(PowerSideEnum::Left, leftVelocity);
        SetPowerLevel(PowerSideEnum::Right, rightVelocity);*/
    }
    else{
        /*SetPowerLevel(PowerSideEnum::Left, 0.0);
        SetPowerLevel(PowerSideEnum::Right, 0.0);*/
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
                Serial.print("rc ");
                Serial.print(rc);
                Serial.println();
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                Serial.print("receivedChars " );
                Serial.print(receivedChars);
                Serial.println();
                recvInProgress = false;
                ndx = 0;
                newData = true;
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


void floop_N(void){
  double angle = getCompassAngle();
  fuzzy->setInput(1, angle);

  fuzzy->fuzzify();

  double leftVelocity = fuzzy->defuzzify(1);
  double rightVelocity= fuzzy->defuzzify(2);

  sprintf(buffer, "\n Angle: %lf; leftVelocity: %lf, rightVelocity: %lf", angle, leftVelocity, rightVelocity);
  Serial.print(buffer);

  SetPowerLevel(PowerSideEnum::Left, leftVelocity);
  SetPowerLevel(PowerSideEnum::Right, rightVelocity);

  delay(300);
}

int measureSoundSpeed(int trigger_pin, int echo_pin)
{
  digitalWrite(trigger_pin, false);
  delayMicroseconds(2);

  digitalWrite(trigger_pin, true);
  delayMicroseconds(10);
  digitalWrite(trigger_pin, false);

  // zmierz czas przelotu fali dĹşwiÄkowej
  int duration = pulseIn(echo_pin, true, 50 * 1000);
  

  // przelicz czas na odlegĹoĹÄ (1/2 Vsound(t=20st.C))
  int distance = (int)((float)duration * 0.03438f * 0.5f);
  return distance;
}

#if 0
void yloop(void)
{
  delay(1000);


  SetPowerLevel(Side_Left, 100);
  delay(2000);

  SetPowerLevel(Side_Left, 200);
  delay(2000);

  SetPowerLevel(Side_Left, 255);
  delay(2000);


  SetPowerLevel(Side_Left, 0);
  SetPowerLevel(Side_Right, 0);

  SetPowerLevel(Side_Left, -100);
  delay(2000);

  SetPowerLevel(Side_Left, -200);
  delay(2000);

  SetPowerLevel(Side_Left, -255);
  delay(2000);

  SetPowerLevel(Side_Left, 0);
  SetPowerLevel(Side_Right, 0);
  delay(4000);
  
  
    
  
  delay(1000);

  SetPowerLevel(Side_Right, 100);
  delay(2000);

  SetPowerLevel(Side_Right, 200);
  delay(2000);

  SetPowerLevel(Side_Right, 255);
  delay(2000);


  SetPowerLevel(Side_Right, 0);
  SetPowerLevel(Side_Right, 0);

  SetPowerLevel(Side_Right, -100);
  delay(2000);

  SetPowerLevel(Side_Right, -200);
  delay(2000);

  SetPowerLevel(Side_Right, -255);
  delay(2000);

  SetPowerLevel(Side_Right, 0);
  SetPowerLevel(Side_Right, 0);
  delay(4000);
}


void xloop() {
  
  qmc.measure();
  int16_t x = qmc.getX();
  int16_t y = qmc.getY();
  int16_t z = qmc.getZ();

  char buf[100];
  sprintf(buf, "\n %5d %5d %5d", x, y, z);
  Serial1.print(buf);
  delay(100);
}



#endif


void cmd_proximity(const char* msg, UltraSoundSensor sensor)
{
  char buffer[64];
  int d[5] = {};
  int sum = 0;
  int id = 0;
  
  while (Serial1.available() == 0)
  {
    int dist = measureSoundSpeed(
      ultrasound_trigger_pin[(int)sensor],
      ultrasound_echo_pin[(int)sensor]);

    // Ĺrednia kroczÄca
    sum -= d[id];
    sum += d[id] = dist;
    id = (id + 1) % 5;
    dist = sum / 5;

    sprintf(buffer, "\n%s: %0dcm", msg, dist);
    Serial1.print(buffer);
  }
  
  while (Serial1.available())
    Serial1.read(); 
}

void cmd_qmc(void)
{
  char buffer[64];

  int i = 0;
  qmc.reset();
  while (Serial1.available() == 0)
  {
    
    if(i > 50){
          qmc.measure();
        /*int16_t x = (qmc.getX() - 3600)/(-4);
        int16_t y = (qmc.getY() - 1550)/(-4 .5);*/

        int16_t x = qmc.getX();
        int16_t y = qmc.getY();
        int16_t z = qmc.getZ();
        x = -(x + 90) / 5.4;
        y = -(y + 200) / 5.8; 
    
        sprintf(buffer, "\n X=%5d Y=%5d Z=%5d", x, y, z);
        Serial1.print(buffer);
        i=0;
    }
    i++;

  }
  
  while (Serial1.available())
    Serial1.read(); 
}


void cmd_bluetooth(void)
{
  Serial1.println("### HC06: Tryb komunikacji z moduĹem HC06. Aby wyjĹÄ, wpisz \"++++++\"...");
  Serial1.println("### Uwaga! ModuĹ analizuje czas otrzymywania danych; polecenie musi");
  Serial1.println("###        koĹczyÄ siÄ krĂłtkÄ przerwÄ (ok. 500ms) BEZ znaku nowej linii");
  Serial1.print("\n> ");
  
  int plus_counter = 0;
  while (true) {
    int b = 0;
    if (Serial1.available()) {
      
      b = Serial1.read();
    
      if (b == '+') {
        plus_counter++;
        if (plus_counter >= 6)
          break; // wyjdĹş na 6 plusĂłw
      }
    
      Serial1.write(b); // wyĹlij do hc06
      //Serial1.write(b); // echo lokalne
    }
      
    if (Serial1.available()) {
      int b = Serial1.read();
      Serial1.write(b);
    }
    
  }
  
  Serial1.println("HC06: Koniec.");
}

void ploop(void){

//
  double y=0; //hehe
  sprintf(buffer, "\n Y=%5d", y);
  Serial1.print(buffer);

  Input = y;
//  myPID.Compute();
//  sprintf(buffer, "Y: %d\n", y);
//  Serial.print(buffer);
  
  //sprintf(buffer, "   Output: %lf   ", Output);
  //Serial1.print(buffer);

  // wartosc Y > 0 wiec output < 0 i na odwrot
  Output = -1 * Output;
  if(Output < -25/*> 25*/){
      //jest za bardzo na lewo
      sprintf(buffer, "\n Output < -25: %lf", Output);
      Serial1.print(buffer);
      Output = -1 * Output;
      SetPowerLevel(PowerSideEnum::Right, BASE_VELOCITY);
      SetPowerLevel(PowerSideEnum::Left, BASE_VELOCITY+Output);
  }
  else if (Output > 25 /*< -5*/) {
    //jest za bardzo na prawo
    SetPowerLevel(PowerSideEnum::Right, BASE_VELOCITY+Output);
    SetPowerLevel(PowerSideEnum::Left, BASE_VELOCITY);
      sprintf(buffer, "\n Output > 25: %lf", Output);
      Serial1.print(buffer);
  }
  else{
      SetPowerLevel(PowerSideEnum::Right, BASE_VELOCITY);
      SetPowerLevel(PowerSideEnum::Left, BASE_VELOCITY);
      printf(buffer, "\n Output else : %lf", Output);
      Serial1.print(buffer);
  }

  delay(200);
}

void zloop(void)
{
  delay(1000);
  for (int i = 0; i < 10; i++)
  {
    Serial1.print((char)(43+2*(i&1)));
    delay(200);
  }
  Serial1.println();
  Serial1.println("=======================================================");
  Serial1.println("# Programowanie Systemow Autonomicznych               #");
  Serial1.println("# Tester autek v1.0 Tomasz Jaworski, 2018             #");
  Serial1.println("=======================================================");
  Serial1.println("Polecenia powinny konczyc sie wylacznie znakiem '\\n'.");
  Serial1.println("ARDUINO IDE: ZmieĹ 'No line ending' na 'Newline' w dolnej czÄĹci okna konsoli...\n");
  
  while(1)
  {
    Serial1.print("> ");

    String s = "";
    while(true)
    {
      while(Serial1.available() == 0);
      int ch = Serial1.read();
      if (ch == '\n')
        break;
      s += (char)ch;
    }
    
    s.trim();
    s.toLowerCase();
    Serial1.println(s);
    
    //
    
    if (s == "help")
    {
      Serial1.println("Pomoc:");
      Serial1.println("   proxf - odczytuj czujnik odleglosc (PRZEDNI)");
      Serial1.println("   proxb - odczytuj czujnik odleglosc (TYLNY)");
      Serial1.println("   proxl - odczytuj czujnik odleglosc (LEWY)");
      Serial1.println("   proxr - odczytuj czujnik odleglosc (PRAWY)");
      
      Serial1.println("   mSD p - ustaw wysterowanie silnika napÄdowego");
      Serial1.println("       S (strona): 'L'-lewa, 'R'-prawa, 'B'-obie");
      Serial1.println("       D (kierunek): 'F'-do przodu, 'B'-do tyĹu, 'S'-stop");
      Serial1.println("       n (wysterowanie): poziom sterowania 0-255");
      Serial1.println("   reset - reset");
      Serial1.println("   qmc   - odczytuj pomiary pola magnetycznego w trzech osiach");
      Serial1.println("   bt    - komunikacja z moduĹem HC06 (Bluetooth)");
      continue;
    }
    
    if (s == "reset") {
      Serial1.println("Ok.");
      delay(1000);
      RSTC->RSTC_MR = 0xA5000F01;
      RSTC->RSTC_CR = 0xA500000D;
      while(1);
    }
    
    if (s == "bt") {
      cmd_bluetooth();
      continue;
    }
    
    if (s == "proxf") {
      cmd_proximity("PRZOD", UltraSoundSensor::Front);
      continue;
    }
    
    if (s == "proxb") {
      cmd_proximity("TYL", UltraSoundSensor::Back);
      continue;
    }
    
    if (s == "proxl") {
      cmd_proximity("LEWY", UltraSoundSensor::Left);
      continue;
    }
    
    if (s == "proxr") {
      cmd_proximity("PRAWY", UltraSoundSensor::Right);
      continue;
    }

    if (s == "qmc") {
      cmd_qmc();
      continue;
    }

    if (s.startsWith("m")) {
      if (s.length() < 3) {
        Serial1.println("Polecenie 'm': bĹad w poleceniu");
        continue;
      }
      
      int side = tolower(s[1]);
      int direction = tolower(s[2]);
      int power = -1;
      if (s.indexOf(" ") != -1) {
        s = s.substring(s.lastIndexOf(" ") + 1);
        char *endptr = NULL;
        power = strtol(s.c_str(), &endptr, 10);
        if (*endptr != '\0') {
          Serial1.println("Polecnie 'm': bĹad w zapisie wartoĹci wystarowania");
          continue;
        }
      }
      
      if (strchr("lrb", side) == NULL) {
        Serial1.println("Polecnie 'm': bĹad w formacie strony");
        continue;
      }
        
      if (strchr("fbs", direction) == NULL) {
        Serial1.println("Polecnie 'm': bĹad w formacie kierunku");
        continue;
      }
      
      if (direction != 's' && power == -1) {
        Serial1.println("Polecnie 'm': brak podanej wartoĹci wysterowania");
        continue;
      }
        
      // przeksztaĹcenia
      bool left = side == 'l' || side == 'b';
      bool right = side == 'r' || side == 'b';
      power = direction == 's' ? 0 : power;
      power = direction == 'b' ? -power : power;

      char msg[128];
      sprintf(msg, "Ustawienia: L=%d, R=%d, power=%d\n", left, right, power);
      Serial1.print(msg);
      if (left)
        SetPowerLevel(PowerSideEnum::Left, power);
      if (right)
        SetPowerLevel(PowerSideEnum::Right, power);
      
      continue;
    }

    Serial1.print(" Polecenie '");
    Serial1.print(s);
    Serial1.print("' jest nieznane; MoĹźe 'help'?\n");
  }
}

//============
// TO GET DATA FROM PYTHON OPEN CV



// ==========
