#define cbi(sfr,bit)(_SFR_BYTE(sfr)&= ~_BV(bit));
#define sbi(sfr,bit)(_SFR_BYTE(sfr)|=_BV(bit));

const unsigned short numReadings = 900;
unsigned short analogVals [numReadings];
boolean fndidx = false;
short val = 0;

// Define the ADC prescaler's 
const unsigned char PS_16 = (1 << ADPS2);  
const unsigned char PS_32 = (1 << ADPS2) | (1 << ADPS0); 
const unsigned char PS_64 = (1 << ADPS2) | (1 << ADPS1); 
const unsigned char PS_128 = (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); 


void setup() {
  
  // set up the ADC 
  ADCSRA &= ~PS_128; // remove bits set by Arduino library

   //Choose a prescaler from above. // PS_16, PS_32, PS_64 or PS_128 
  ADCSRA |= PS_16;// set prescaler to 16, the fastest.
  
  Serial.begin(9600);

}

void loop() {
    
    val = analogRead(A0); //Sample the vlotage from sig-A
    if (val < 800){      //See if we have a voltage drop in the signal below sample value of 800   
      
      for (int j=1; j<=numReadings;j++){   //yes we have, collect  numReadings and
        analogVals[j] = analogRead(A0);    //store each value in memory
      }
      
      fndidx = true;                       //mark the fact we have found a signal that has been sampled
      analogVals[0] = val;                 //finally store the first value we found in slot 0; 
      
  }
  /*Print out the values to the serial port*/
  if (fndidx){
    for (int i=0; i<=numReadings; i++){
      Serial.println(analogVals[i]);
    }
    fndidx = false;
  }
  

}
