/*
 *Lightning detector - Matt Carless
*for use with smdkings lightning module and a OLED display module
**/
 
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/*#include <DHT.h>*/

//#define DHTPIN 2     // 
//#define DHTTYPE DHT11   // DHT 11
#define OLED_RESET 4
#define GRAPH_SAMPLES 15

Adafruit_SSD1306 display(OLED_RESET);

// Initialize DHT sensor for normal 16mhz Arduino
/*DHT dht(DHTPIN, DHTTYPE);*/
short sensorPin = A0;                  //Analoge voltage pin, pin we're connecting sig-A to on Arduino from SMDKINGS EMP module
unsigned short sensorValue = 0;

unsigned short page = 0;              //page 0 = test, page 1 chart
unsigned short int lightning_pin = 2; //Digital IO,  pin we're connecting sig-D to on Arduino from SMDKINGS EMP module
short strike_count = 0;               //count the total number of strikes
unsigned short strike_min = 0;        //number of strikes in a rolling min

unsigned short strike_min_count_window_time = 60000; //sliding window for strike count 60 secs
unsigned short switch_screen_time = 5000;            //Time to stay on one screen for before switching
unsigned long last_switch_time = 0;                  //Time the last page switch occured

unsigned short screen_update_time = 1000;            //period to update the screen being displayed
unsigned long last_screen_update_time = 0;


unsigned short strike_min_count_check_period = 1000; //how often to calculate strike count min for sliding window.
unsigned long start_strike_time = 0;
unsigned long strikeTime = 0;
unsigned long lastStrikeTime = 0;
unsigned short minStrikeWaitTime = 100; //time to wait in ms between strikes, to detect a new strike


unsigned short noMinsOnGraphs = GRAPH_SAMPLES;
unsigned short strikesArray[GRAPH_SAMPLES]; //= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 30 mins worth
unsigned short analogSensorArray[GRAPH_SAMPLES]; //= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // 30 mins worth

//unsigned long ringBufGetIdx = 1;
//unsigned long ringBufPutIdx = 0;

unsigned short strikeData[15];   //Experimetal array that holds data from sampling the wave form from sig-A 

unsigned short windowStart = 0;

short min_count = 0;
short drawCount = 0;

unsigned short min_sensor_val_a_min = 1023; //initialise to the maximum value
unsigned short sensor_val = 1023; //again init to the max value
unsigned short sensor_min_total = 0;
unsigned short sensor_min_ave = 0;

/*Iterrupt service routine that gets triggered on falling edge of sig-D*/
void strike(){
  strikeTime = millis();
  if ((strikeTime-lastStrikeTime) > minStrikeWaitTime){ //filters a out multiple strikes in a given time
    /*Sample data from sig-A and store in array - provides data for the pulse screen*/
    unsigned int sumSensVal = 0;
    for(unsigned short k = 0;k<15;k++){
        strikeData[k] = analogRead(sensorPin);
        sumSensVal = sumSensVal + strikeData[k];
        //Serial.println(strikeData[k]);
        delayMicroseconds(1000); //15 * 1ms = roughly 15ms sample coverage, there's extra delay in executing the code inside the loop.
    }
    min_sensor_val_a_min = sumSensVal/15; //get the avarage value of the pulse - trying to use this as indicator of signal strength
    sensor_min_total = sensor_min_total + min_sensor_val_a_min;
    strike_count++;                 //increment the total strike count
    strike_min++;                   //increment the variable strikes a minute
    sensor_min_ave = sensor_min_total/strike_min;
    lastStrikeTime = strikeTime;    //Store the srike time, ready for next call of this interrupt service routine
   }
   
        
}

    

void setup()
{
  pinMode(lightning_pin, INPUT);                  //Set the digital pin connected to sig-D to input
  attachInterrupt(digitalPinToInterrupt(lightning_pin), strike, FALLING);  //Attach our strike interrupt  service routine on falling edge of lighting_pin(Sig-D) 
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);// initialize with the I2C addr 0x3C (for the 128x32)(initializing the display)
  Serial.begin(9600);
    
  start_strike_time = millis();
    /*Initialise arrays that hold strike data(pulse stats from sig-D), analogue data(sig-A) waveform stats and the actual sig-A waveform */
    
    //init sig-D stats array 
    for (unsigned short i = 0;i<noMinsOnGraphs;i++){
        //strikesArray[i] = 0;
        short bufPos = abs(min_count-i);
        Serial.println(bufPos);
        setSrikeInBuffer(bufPos, (unsigned short)0);
              
    }

    //init sig-A stats array
    for (unsigned short i = 0;i<noMinsOnGraphs;i++){
        //strikesArray[i] = 0;
        short bufPos = abs(min_count-i);
        Serial.println(bufPos);
        setAnalogValInBuffer(bufPos,(short)1023);//1023 = no EMP signal 0=max signal i.e. init to no signal     
      
    }

    //init sig-A waveform data
    for (int j =0;j<15;j++){
      strikeData[j] = (short)1023; 
    }
    
     clear_display(); 
}


/*Function for getting data from our array using minutes.
Our array in this case is being used as a ring buffer
This gets our stats based on sig-D*/
unsigned short getStrikeFromBuffer(unsigned short timeMins){
    unsigned short bufferPos = timeMins % noMinsOnGraphs;
    return strikesArray[bufferPos];
}

/*Function for getting data from our array using minutes.
Our array in this case is being used as a ring buffer.
After noMinsOnGraphs dat will begin to get overwritten*/
void setSrikeInBuffer(unsigned short timeMins, unsigned short strikeCout){
     short bufferPos = timeMins % noMinsOnGraphs;
     if (bufferPos > noMinsOnGraphs - 1){
       Serial.println("Buffer Overflow StrikeBuffer"); //if we see these in the serial output we are outside of our array bounds
       Serial.println(bufferPos);
       Serial.println(timeMins);
     }
     if (bufferPos < 0){
       Serial.println("Buffer Underflow StrikeBuffer"); //if we see these in the serial output we are outside of our array bounds
       Serial.println(bufferPos);
       Serial.println(timeMins);
     }
     strikesArray[bufferPos] = strikeCout;
}

/*Same as above but for sig-A data*/
unsigned short getAnalogValFromBuffer(unsigned short timeMins){
    unsigned short bufferPos = timeMins % noMinsOnGraphs;
    return analogSensorArray[bufferPos];
}

/*Same as above but for sig-A data*/
void setAnalogValInBuffer(unsigned short timeMins, unsigned short sensorVal){
     short bufferPos = timeMins % noMinsOnGraphs;
     if (bufferPos > noMinsOnGraphs - 1){
       Serial.println("Buffer Overflow setAnalogValInBuffer");
     }
     if (bufferPos < 0){
       Serial.println("Buffer Underflow setAnalogValInBuffer");
     }
     analogSensorArray[bufferPos] = sensorVal;
}

/*Writes basic stats out to OLED screen*/
void update_display(){
  display.setRotation(0); 
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  
  display.print("Strike Count: ");
  display.print(strike_count);   //total strike count
    
  display.setCursor(0,10);
  display.print("Ave Intensity: ");  
  display.print(1023 - sensor_min_ave);  //This is still in development, currently doesn't provide whats required
  display.setCursor(0,20);
  display.print("Strikes Min: ");
  /*Displays strikes a min over 5 min window*/
   if (min_count >= 5){
       //If we have > 5 mins worth of data, use our buffere to calculate
       windowStart = min_count - 5;
       unsigned int strikesMinWindowTotal = 0;  
      for(unsigned short i = 1;i<=5;i++){
            strikesMinWindowTotal = strikesMinWindowTotal + getStrikeFromBuffer(windowStart+i);
      }
      display.print((float)strikesMinWindowTotal/(float)5);  //display the result
   }else{
      //Otherwise just take the average of where we are currently up to when runtime < 5 mins
      display.print((float)strike_count/(float)(min_count+1));
   }
      display.setCursor(0,30);
 display.print("Run Time(mins): ");
 display.print(min_count);          //Display the cirrent runtime in mins
 display.setCursor(0,40);
 display.print("Min Val: ");
 display.print(min_sensor_val_a_min);   //Display the minimum value captured from sig-A
  display.setCursor(0,50);
  display.print("This min: ");
  display.print(strike_min);         //Display stikes in the current minute
 // display.setCursor(0,58);
 // display.print("This min Tot: ");
 // display.print(sensor_min_total);
  //for(int i = 0;i<min_count;i++){
   // display.print(".");
  //}
  display.display();
  
    
}

/*Clears the OLED display*/
void clear_display(){
    display.clearDisplay();
}

/*Draw the chart that holds stats based on sig-D
Number of strikes per min over the last 15(noMinsOnGraphs)mins*/
void drawChart (){
    unsigned short xaismargin = 8;
    unsigned short yaxismargin = 10;
    
    unsigned short yIndLength = 2;
    unsigned short yIndCount = 5;
    
    unsigned short xIndHeight = 3;
    unsigned short xIndCount = 10;
   
    
   display.setRotation(0);
    //Draw the x and y axis   
   display.drawLine(xaismargin,0,xaismargin,display.height()-yaxismargin,WHITE);
   display.drawLine(xaismargin,display.height()-yaxismargin,display.width()-xaismargin,display.height()-yaxismargin,WHITE);
    
    //Draw the indicators on the x y axis
    //calculate the spacing for the y axis
    unsigned short ySpacing = (display.height()  - yaxismargin)/yIndCount;
    //Serial.println(ySpacing);
    //Serial.println(display.height());
    //unsigned short yPos = ySpacing;
    for(short i = 0;i < yIndCount;i++){
           
          display.drawLine(xaismargin-yIndLength,display.height()-yaxismargin-(ySpacing * i),xaismargin,display.height()-yaxismargin-(ySpacing * i),WHITE);
          //yPos=yPos+ySpacing;
    }
    //calculate the spacing for the x axis
    unsigned short xSpacing = (display.width() - xaismargin)/xIndCount;
   // unsigned short xPos = xSpacing;
    //Serial.println(xSpacing);
    //Serial.println(display.width());
    for(short i = 0;i < xIndCount;i++){
           
          display.drawLine((xSpacing*i)+xaismargin,display.height()-yaxismargin+xIndHeight,(xSpacing*i)+xaismargin,display.height()-yaxismargin,WHITE);
          //xPos=xPos+xSpacing;
    }
    //Draw axis labels
    display.setTextSize(0);
    display.setTextColor(WHITE);
    display.setCursor((display.width()/2)-30,display.height()-7);
    display.println("Time(mins)");
    display.setCursor((display.width()/2)-58,display.height()-7);
    display.println("15");
    display.setCursor((display.width()/2)+54,display.height()-7);
    display.println("0");
    
    //draw data on chart
    unsigned short startPos = noMinsOnGraphs-1;
    unsigned short xPos = 0;
    unsigned short yScale = 2;
    unsigned short xDataSpacing = (display.width() - xaismargin)/noMinsOnGraphs;
    unsigned short maxYval =0;
    float scaleFactor = 1.0;
    
    for(unsigned short i = 0;i<noMinsOnGraphs;i++){
      if (strikesArray[i]*yScale > maxYval){
        maxYval = strikesArray[i]*yScale;
      }
    }
    
    //Scale the data if over full height of graph
    if ((short)(display.height()-yaxismargin-maxYval) < 0){
        scaleFactor = (float)((float)(display.height()-yaxismargin) / (float)maxYval);
    }
    
    for(short i = startPos;i>=0;i--){
       //unsigned int  dataXPos = ((yaxismargin+i)*2)-9;
       unsigned short  dataXPos = ((xaismargin)+(xPos*xDataSpacing));
       
      
       unsigned short  dataYPos;
       if (min_count > noMinsOnGraphs){
          unsigned short bufPos = abs(min_count-i); 
          //Serial.println(bufPos);
          dataYPos = getStrikeFromBuffer(bufPos)*yScale*scaleFactor;
       }else{
          dataYPos = strikesArray[xPos]*yScale*scaleFactor;
       }
      
       
       // Serial.println("dataYpos:" + String((int)xPos));
       //unsigned int  dataYPos = strikesArray[i]*2;
       display.drawLine(dataXPos,display.height()-yaxismargin,dataXPos,display.height()-yaxismargin-dataYPos,WHITE);
       xPos++;
       
    }
    display.display();
    //y label
    
    
    display.setRotation(3);
    display.setTextSize(0);
    display.setCursor(13,0);
    display.print("Strikes");   
    display.display();    
}

/*Draw chart based on data gained from sig-A
Intensity over the last 15(noMinsOnGraphs)mins
Note- this doesn't currently provide what is actually required and is still wip*/
void drawIntensityChart (){
    unsigned short xaismargin = 8;
    unsigned short yaxismargin = 10;
    
    unsigned short yIndLength = 2;
    unsigned short yIndCount = 5;
    
    unsigned short xIndHeight = 3;
    unsigned short xIndCount = 10;
    display.setRotation(0);
    //Draw the x and y axis   
   display.drawLine(xaismargin,0,xaismargin,display.height()-yaxismargin,WHITE);
   display.drawLine(xaismargin,display.height()-yaxismargin,display.width()-xaismargin,display.height()-yaxismargin,WHITE);
    
    //Draw the indicators on the x y axis
    //calculate the spacing for the y axis
    unsigned short ySpacing = (display.height() - 1 - yaxismargin)/yIndCount;
    //Serial.println(ySpacing);
    //Serial.println(display.height());
    //unsigned short yPos = ySpacing;
    for(short i = 0;i < yIndCount;i++){
          display.drawLine(xaismargin-yIndLength,display.height()-yaxismargin-(ySpacing * i),xaismargin,display.height()-yaxismargin-(ySpacing * i),WHITE); 
    }
    //calculate the spacing for the x axis
    unsigned short xSpacing = (display.width() - xaismargin)/xIndCount;
//    unsigned short xPos = xSpacing;
//    Serial.println(xSpacing);
//    Serial.println(display.width());
    for(short i = 0;i < xIndCount;i++){
           display.drawLine((xSpacing*i)+xaismargin,display.height()-yaxismargin+xIndHeight,(xSpacing*i)+xaismargin,display.height()-yaxismargin,WHITE);
    }
    //Draw axis labels
    display.setTextSize(0);
    display.setTextColor(WHITE);
    display.setCursor((display.width()/2)-30,display.height()-7);
    display.println("Time(mins)");
    display.setCursor((display.width()/2)-58,display.height()-7);
    display.println("15");
    display.setCursor((display.width()/2)+54,display.height()-7);
    display.println("0");
    
    
    //draw data on chart
    unsigned short startPos = noMinsOnGraphs-1;
    unsigned short posCounter = 0;
    unsigned short xPos = 0;
    unsigned short xDataSpacing = (display.width() - xaismargin)/noMinsOnGraphs;
    for(short i = startPos;i>=0;i--){
       //unsigned int  dataXPos = ((yaxismargin+i)*2)-9;
       unsigned int  dataXPos = ((xaismargin)+(xPos*xDataSpacing));
       
       float yposFactor = (float)((float)(display.height()-yaxismargin)/(float)1024); //1024 because that is the data range
       unsigned short  dataYPos;
       if (min_count > noMinsOnGraphs){
          unsigned short bufPos = abs(min_count-i);
          dataYPos = getAnalogValFromBuffer(bufPos);
       }else{
          dataYPos = analogSensorArray[xPos];
       }
       //Serial.println(dataYPos);
       //unsigned int dataYPos = analogSensorArray[i];
       display.drawPixel(dataXPos,display.height()-yaxismargin- (int)(1023*yposFactor) + (int)(dataYPos*yposFactor),WHITE);

       xPos++;
   
    }
    display.display();
    //y label
    
    
    display.setRotation(3);
    display.setTextSize(0);
    display.setCursor(13,0);
    display.print("Strength");   
    display.display();    
}

/*Draws chart that displays waveform sampled from by sig-A
after sig-D gets fired over roughly 15ms period*/
void drawLightningLevelChart (short range){
    unsigned short xaismargin = 8;
    unsigned short yaxismargin = 10;
    
    unsigned short yIndLength = 2;
    unsigned short yIndCount = 5;
    
    unsigned short xIndHeight = 3;
    unsigned short xIndCount = 15;
    display.setRotation(0);
    //Draw the x and y axis   
   display.drawLine(xaismargin,0,xaismargin,display.height()-yaxismargin,WHITE);
   display.drawLine(xaismargin,display.height()-yaxismargin,display.width()-xaismargin,display.height()-yaxismargin,WHITE);
    
    //Draw the indicators on the x y axis
    //calculate the spacing for the y axis
    unsigned short ySpacing = (display.height() - 1 - yaxismargin)/yIndCount;
    //Serial.println(ySpacing);
    //Serial.println(display.height());
    //unsigned short yPos = ySpacing;
    for(short i = 0;i < yIndCount;i++){
          display.drawLine(xaismargin-yIndLength,display.height()-yaxismargin-(ySpacing * i),xaismargin,display.height()-yaxismargin-(ySpacing * i),WHITE); 
    }
    //calculate the spacing for the x axis
    unsigned short xSpacing = (display.width() - xaismargin)/xIndCount;
//    unsigned short xPos = xSpacing;
//    Serial.println(xSpacing);
//    Serial.println(display.width());
    for(short i = 0;i < xIndCount;i++){
           display.drawLine((xSpacing*i)+xaismargin,display.height()-yaxismargin+xIndHeight,(xSpacing*i)+xaismargin,display.height()-yaxismargin,WHITE);
    }
    //Draw axis labels
    display.setTextSize(0);
    display.setTextColor(WHITE);
    display.setCursor((display.width()/2)-30,display.height()-7);
    display.println("Time(ms)");
    display.setCursor((display.width()/2)-58,display.height()-7);
    display.println("0");
    display.setCursor((display.width()/2)+50,display.height()-7);
    display.println("15");
    
    
    //draw data on chart
    short dataPoints = 15;
    short startPos = 1;
    unsigned short posCounter = 0;
    float area = 0.0;
    unsigned short xDataSpacing = (display.width() - xaismargin)/dataPoints;
    
    for(short i = startPos;i<dataPoints;i++){
       //unsigned int  dataXPos = ((yaxismargin+i)*2)-9;
       unsigned int  dataXPos = ((xaismargin)+(i*xDataSpacing));
       unsigned int  preDataXPos = ((xaismargin)+((i-1)*xDataSpacing));
       
       float yposFactor = (float)((float)(display.height()-yaxismargin)/(float)range); //1024 because that is the data range
       //Serial.println(i);
       //Serial.println(strikeData[i]);
       short  dataYPos = strikeData[i];
       short  preDataYPos = strikeData[i-1];
       
       
       //unsigned int dataYPos = analogSensorArray[i];
       //display.drawPixel(dataXPos,display.height()-yaxismargin - (int)(1023*yposFactor) + (short)(dataYPos*yposFactor),WHITE);
       
       display.drawLine(preDataXPos,display.height()-yaxismargin - (int)(1023*yposFactor) + (short)(preDataYPos*yposFactor),dataXPos,display.height()-yaxismargin - (int)(1023*yposFactor) + (short)(dataYPos*yposFactor),WHITE);

       //Roughly calculate area under chart - needs work
       //find area of top triangle
       float triangleArea = 0.5 * (float)abs((1023-dataYPos)-(1023-preDataYPos)) * (float)(dataXPos - preDataXPos);
       float rectangle =  (float)(dataXPos - preDataXPos) * (float)(1023-dataYPos);
       //xPos++;
       area = area + triangleArea + rectangle;
   
    }
    display.setCursor(((display.width()/2)+10),(display.height()/2)-15);
    display.println(area);
    display.display();
    //y label
    
    
    display.setRotation(3);
    display.setTextSize(0);
    display.setCursor(13,0);
    display.print("Strength");   
    display.display();    
}


void loop()
{
    if (false){ //test code to sample sig-A and display in chart - enable set to true but it will not display anyother screens
      unsigned int sumSensVal = 0;
      for(unsigned short k = 0;k<15;k++){
          strikeData[k] = analogRead(sensorPin);
          sumSensVal = sumSensVal + strikeData[k];
          //Serial.println(strikeData[k]);
          delayMicroseconds(100); //50 * 0.1ms = 2ms sample coverage
          //delay(100);
      }
      float area = 0.0;
      for(short i = 1;i<15;i++){
         float triangleArea = 0.5 * (float)abs((1023-strikeData[i])-(1023-strikeData[i-1])) * (float)(1.0);
         float rectangle =  (float)(1.0) * (float)(1023-strikeData[i]);
         area = area + triangleArea + rectangle;
      }
      
      if (area > 70000){ //looking for signal, if found draw the chart.
        Serial.println("Area:" + String(area));
        //find the max value for scale
        unsigned short minVal =1025;
        for(short i = 1;i<15;i++){
          if (strikeData[i] < minVal){
            minVal = strikeData[i];
          }
        }
        
        clear_display();
        drawLightningLevelChart(1025-minVal);
        //drawCount++;
      }
      //if (drawCount > 5){
      //  clear_display();
      //   drawCount = 0;
      //}
      
    }else{
        //Normal running mode here
        unsigned long time_now = millis();
        if ((time_now - start_strike_time) > strike_min_count_window_time ){
            //stats period expired reset some data 
            start_strike_time = millis();//reset the delay counter
            //strikesArray[min_count] = strike_min;
            setSrikeInBuffer(min_count,strike_min);
            setAnalogValInBuffer(min_count,(sensor_min_ave));
            strike_min = 0;//reset the strike min count
            sensor_min_total = 0; //reset running 1 min total for sensor value
            sensor_min_ave = 1023; //reset average 1 min average for sensor value
            min_sensor_val_a_min = 1023; //reset min analog value
            min_count++;
                 
        }
        /*Logic to switch between given screens after a given period - switch_screen_time*/
        if ((time_now - last_switch_time) > switch_screen_time){
            //for(int j = 0;j<10;j++){
            //   Serial.println(strikeData[j]);
            //}
            last_switch_time = millis();
            if (page == 3){   //sig-A stats chart
                //strikesArray[min_count] = strike_min;
                setAnalogValInBuffer(min_count,sensor_min_ave);
                clear_display();
                drawIntensityChart();
                page = 0;
            } else if (page == 2){ //sig-A sampled data chart
                //strikesArray[min_count] = strike_min;
                //setAnalogValInBuffer(min_count,sensor_min_ave);
                clear_display();
                drawLightningLevelChart(1200);
                page = 3;
            } else if (page == 1){ //sig-D stats data chart
                //strikesArray[min_count] = strike_min;
                setSrikeInBuffer(min_count,strike_min);
                clear_display();
                drawChart();
                page = 2;
            } else if (page == 0){ //Stats text
                clear_display();
                update_display();
                page = 1;
            }    
        }
      
        //Updates the current screen
        if ((time_now - last_screen_update_time) > screen_update_time){
            //Serial.println(analogRead(sensorPin));
            Serial.println("Update Page:" + page);
            last_screen_update_time = millis();
            if (page == 1){
               Serial.println("Page 1");
               clear_display();
               //drawChart();
               update_display();    
            }
            if (page == 2){
               Serial.println("Page 2");
               //clear_display();
               drawChart();
               //update_display();    
            }
            if (page == 0){
               Serial.println("Page 0");
               //clear_display();
               drawIntensityChart();
               //update_display();    
            }
        }
    }
    
 

}
