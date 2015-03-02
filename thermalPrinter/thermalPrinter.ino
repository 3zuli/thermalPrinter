/*
ThermalPrinter by 3zuli
Designed for CITIZEN LT-286 thermal printer module

CONNECTIONS
arduino --- l293d 
   2         1A 
   3         4A 
   4         2A 
   5         3A 

l293d --- stepper
 1Y         A
 4Y         B
 2Y         An
 3Y         Bn
*/

#include "font.c"

//pins that control the motor (bipolar mode, 4 wires using the ULN2003)
const int stepA  = 2;
const int stepAn = 4; 
const int stepB  = 3; 
const int stepBn = 5;


//pins for serial communication with the module
const int dataPin=6;  
const int clkPin=7;
const int latchPin=8;

//strobe pins
const int stbPins[6]={9,10,11,12,A0,A1};

const int timePerStep = 2000; //microseconds per motor step  
const int printTime=5500; //time of printing (microseconds)  //6420us @ 25°C, VH= ~4,2V

int currentStep=0;

//the stepper motor driving sequence                       
                        //  A    An     B    Bn                         
bool stepSequence[4][4]={{true ,false,false,true },  //step 1
                         {false,true ,false,true },  //step 2
                         {false,true ,true ,false},  //step 3
                         {true ,false,true ,false}}; //step 4    
                    
const int buffLen=48; //384bit / 8bit = 48byte
byte lineBuffer[buffLen]; //48 bytes to hold the current line
char serLineBuffer[24];
String serLine;
bool serReady=false;
int serLineLength=0;

byte yolo[7][48]={
  //u mad bro?
     //Y                       //O                       //L                               //O
     {255,255,0,0,0,255,255,0, 0,0,255,255,255,0,0,0,    255,255,0,0,0,0,0,0,              0,0,255,255,255,0,0,0,}, 
     {0,255,255,0,255,255,0,0, 255,255,0,0,0,255,255,0,  255,255,0,0,0,0,0,0,              255,255,0,0,0,255,255,0,},
     {0,0,255,255,255,0,0,0,   255,255,0,0,0,255,255,0,  255,255,0,0,0,0,0,0,              255,255,0,0,0,255,255,0,},
     {0,0,0,255,0,0,0,0,       255,255,0,0,0,255,255,0,  255,255,0,0,0,0,0,0,              255,255,0,0,0,255,255,0,},
     {0,0,0,255,0,0,0,0,       255,255,0,0,0,255,255,0,  255,255,0,0,0,0,0,0,              255,255,0,0,0,255,255,0,},
     {0,0,0,255,0,0,0,0,       255,255,0,0,0,255,255,0,  255,255,255,255,255,255,255,0,    255,255,0,0,0,255,255,0,},
     {0,0,0,255,0,0,0,0,       0,0,255,255,255,0,0,0,    255,255,255,255,255,255,255,0,    0,0,255,255,255,0,0,0,}
};

  char lineText[24]="Lorem ipsum dolor sit a";
//char lineText[24]="ABCDEFGHIJKLMNOPQRSTUVW";
//char lineText[24]="abcdefghijklmnopqrstuvw";

/*char lipsum[11][24]={
    "Lorem ipsum dolor sit a",
    "met, consectetur adipis",
    "icing elit, sed do eius",
    "mod tempor incididunt u",
    "t labore et dolore magn",
    "a aliqua. Ut enim ad mi",
    "nim veniam, quis nostru",
    "d exercitation ullamco ",
    "laboris nisi ut aliquip",
    "ex ea commodo consequat",
    ".                      "
};*/
  
void stepperFree(){ //set the stepper pins to LOW
    digitalWrite(stepA,LOW);
    digitalWrite(stepB,LOW);
    digitalWrite(stepAn,LOW);
    digitalWrite(stepBn,LOW);
}

void stepperInit(){  
    //set pins to OUTPUT mode and set them LOW 
    pinMode(stepA,OUTPUT);
    pinMode(stepB,OUTPUT);
    pinMode(stepAn,OUTPUT);
    pinMode(stepBn,OUTPUT);
    
    digitalWrite(stepA,LOW);
    digitalWrite(stepB,LOW);
    digitalWrite(stepAn,LOW);
    digitalWrite(stepBn,LOW);
}

void printerInit(){
    pinMode(dataPin,OUTPUT);
    pinMode(clkPin,OUTPUT);
    pinMode(latchPin,OUTPUT);
    
    for(int i=0;i<6;i++)
      pinMode(stbPins[i],OUTPUT);
    digitalWrite(latchPin,HIGH);
    
}
                         
void movePaper(int lines=1,bool reset=false){  
    //move the paper
    //int lines - number of lines to move
    //bool reset - if true, start the step sequence from 0; if false, continue from last step
    if(reset)
        currentStep=0;
    if(lines>0){ //forward
        for(int i=0;i<lines;i++){
            currentStep++;
            if(currentStep>=4)
                currentStep=0;
            digitalWrite(stepA ,stepSequence[currentStep][0]);
            digitalWrite(stepAn,stepSequence[currentStep][1]);  
            digitalWrite(stepB ,stepSequence[currentStep][2]);
            digitalWrite(stepBn,stepSequence[currentStep][3]); 
            delayMicroseconds(timePerStep);
        }
    }
    else{      //backward
         for(int i=-1*lines-1;i>=0;i--){
            currentStep--;
            if(currentStep<0)
                currentStep=3;
            digitalWrite(stepA ,stepSequence[currentStep][0]);
            digitalWrite(stepAn,stepSequence[currentStep][1]);  
            digitalWrite(stepB ,stepSequence[currentStep][2]);
            digitalWrite(stepBn,stepSequence[currentStep][3]); 
            
            delayMicroseconds(timePerStep);
        }
    }
}

void compensateBackslash(){
     movePaper(-20);
     movePaper(20);
}

void generateTestLine(byte *buff, int len,byte val){
     for(int i=0;i<len;i++){
         //buff[i]=random(255);
         buff[i]=~val;
         /*if(i%2==0){
              buff[i]=val;
         }
         else{
              buff[i]=~val;
         } */
     }
}

void generateBitmapLine(byte *buff, int bufflen, byte *bitmap, int bitmapIndex){
     for(int i=0;i<bufflen;i++){ 
          buff[i]=bitmap[bitmapIndex];
     }
}

void pushLine(byte *buff, int len){ //send current line buffer to the printer head
     unsigned long time=micros();
     for(int i=0;i<len;i++){
          shiftOut(dataPin,clkPin,MSBFIRST,buff[i]);
          /*for(int a=0;a<8;a++){
               digitalWrite(dataPin,(buff[i]<<a)&128);
               digitalWrite(clkPin,HIGH);
               delayMicroseconds(1);
               digitalWrite(clkPin,LOW);
          }*/
     } 
     //delayMicroseconds(1);
     digitalWrite(latchPin,LOW);  //activate LATCH
     delayMicroseconds(1);
     digitalWrite(latchPin,HIGH);
     //Serial.println(micros()-time);
}

void printLine(byte *buff, int len,int printTime,bool push=true){
     // use pushLine() to send new line buffer to the printer, then activate STROBE1-6 to print it
     // buff - buffer that holds the line data
     // len - length of buffer
     // printTime - time of thermal head activation in microseconds
     // count - how many times should this line be printed
     // push - 
     if(push){
         pushLine(buff,len);
     }
     for(int i=0;i<6;i++){
          digitalWrite(stbPins[i],HIGH);
          delayMicroseconds(printTime);
          digitalWrite(stbPins[i],LOW);
     }
}

void nprintLine(byte *buff, int len,int printTime,int count=1,bool push=true){
    for(int a=0;a<count;a++){
        unsigned long time=micros();
        if(a==0 && push)
             printLine(buff,len,printTime,true);
        else 
             printLine(buff,len,printTime,false);
        movePaper(1);
        //Serial.println(micros()-time);
    }
}

void nprintBitmapLine(byte *buff, int len, byte *bitmap, int bitmapLen, int printTime,int count=1){
    for(int i=0;i<count;i++){
        for(int a=0;a<bitmapLen;a++){
     //void generateBitmapLine(byte *buff, int bufflen, byte *bitmap, int bitmapIndex){
            generateBitmapLine(buff,len,bitmap,a);
            printLine(buff,len,printTime,true);
            movePaper(1);
            nprintLine(buff,len,printTime,1,false);
        }  
    }
}

void printTextLine(char *text, int textLen, byte *buff, int buffLen, int printTime,int height=2){
     if(textLen<24){    //create whitespace for the entire line 
          for(int i=textLen-1;i<24;i++)
                text[i]=' ';
     }
     for(int lineIndex=0;lineIndex<(fontHeight-1)*2;lineIndex+=2){
          int c=0;
          for(int i=0;i<buffLen-2;i+=2,c++){
                buff[i]=pgm_read_byte(fontBitmaps + (text[c]-32)*28+lineIndex);   //O(1)!!! //left part of char bitmap
                buff[i+1]=pgm_read_byte(fontBitmaps + (text[c]-32)*28+lineIndex+1);   //O(1)!!! //left part of char bitmap
          }
          nprintLine(buff,buffLen,printTime,height,true);
     } 
}

void serialEvent(){ //hmm
     //static int i=0;
     while (Serial.available()) {
         char inChar=(char)Serial.read();
         serLine+=inChar;
         serLineLength++;
         if(inChar=='\n') {
             serReady=true;
         } 
     }
}


void printMandel(int IterationMax=50){
        int iX,iY;
        const int iXmax = 384;  //height
        const int iYmax = 384;  //length
        /* world ( double) coordinate = parameter plane*/
        double Cx,Cy;
        const double CxMin=-2.0;  //-2.5
        const double CxMax=1.0;   //1.5
        const double CyMin=-1.0;  //-2.0
        const double CyMax=1.0;   //2.0
        /* */
        double PixelWidth=(CxMax-CxMin)/iXmax;
        double PixelHeight=(CyMax-CyMin)/iYmax; 
        
        /* Z=Zx+Zy*i  ;   Z0 = 0 */
        double Zx, Zy;
        double Zx2, Zy2; /* Zx2=Zx*Zx;  Zy2=Zy*Zy  */
        double ER2 = 4.0;//2*2; //escape radius
        int Iteration;
        for(iY=0;iY<iYmax;iY++)
        {
             Cy=CyMin + iY*PixelHeight;
             if (fabs(Cy)< PixelHeight/2) Cy=0.0; /* Main antenna */
             unsigned long lineTime=millis();
             for(iX=0;iX<iXmax;iX++)
             {          
                        Cx=CxMin + iX*PixelWidth;
                        /* initial value of orbit = critical point Z= 0 */
                        Zx=0.0;
                        Zy=0.0;
                        Zx2=Zx*Zx;
                        Zy2=Zy*Zy;
                        /* */
                        for (Iteration=0;Iteration<IterationMax && ((Zx2+Zy2)<ER2);Iteration++)
                        {
                            Zy=2*Zx*Zy + Cy;
                            Zx=Zx2-Zy2 +Cx;
                            Zx2=Zx*Zx;
                            Zy2=Zy*Zy;
                        };

                        if (Iteration==IterationMax)
                        { /*  interior of Mandelbrot set = black */
                              lineBuffer[iX/8] |= 128>>(iX%8); //(int)floor(iX/8)
                        }
                     else 
                        { /* exterior of Mandelbrot set = white */
                              lineBuffer[iX/8] &= !(128>>(iX%8));
                        };
                }
                Serial.print("Line time = ");
                Serial.println(millis()-lineTime);
                nprintLine(lineBuffer,buffLen,printTime,1,true);
                stepperFree();
        }
}

void printSin(byte *buff, int len, int steps,int printTime, int n=3){
     double dx=6.28/(double)steps;
     double x=0;
     int val;
     for(int i=0;i<len;i++)
          buff[i]=0;
     for(int i=0;i<=steps;i++){
          //val=map(sin(x),(double)-1,(double)1,(double)0,(double)384);
          val=(sin(x) - (double)(-1)) * ((double)384 - (double)0) / ((double)1 - (double)(-1)) + (double)0;
          Serial.print(x);
          Serial.print(" ");
          Serial.print(sin(x));
          Serial.print(" ");
          Serial.println(val);
          for(int i=0;i<len;i++)
              buff[i]=0;
          buff[val/8]=128>>(val%8);
          nprintLine(buff,len,printTime,n,true);
          x+=dx;
     } 
}

void setup(){
    Serial.begin(115200);
    serLine.reserve(200);
    stepperInit();
    printerInit();
    compensateBackslash();
    movePaper(10);
    delay(10);
    //printMandel(32);
    //movePaper(50);
    //printSin(lineBuffer,48,300,printTime,3);
    for(int i=0;i<48;i++)
        lineBuffer[i]=i%2==0?255:0;
    nprintLine(lineBuffer,48,printTime,3,true);
    for(int i=0;i<48;i++)
        lineBuffer[i]=0;
    movePaper(5);
    
    /*for(int i=0;i<6;i++)
        printTextLine(lipsum[i],24,lineBuffer,48,printTime,3);*/
    
    /*for(int i=1;i<11;i++)
        printTextLine(lineText,24,lineBuffer,48,printTime,i);*/
    //printTextLine(lineText,24,lineBuffer,48,printTime);
    
    /*for(int a=0;a<3;a++){
        for(int i=0;i<8;i++){
              generateTestLine(lineBuffer,48,3<<i%8);
              nprintLine(lineBuffer,48,printTime,1,true);
              movePaper(1,false); 
        }
        for(int i=0;i<8;i++){
              generateTestLine(lineBuffer,48,192>>i%8);
              nprintLine(lineBuffer,48,printTime,1,true);
              movePaper(1,false); 
        }
        generateTestLine(lineBuffer,48,0);
        nprintLine(lineBuffer,48,printTime,1,true);
        movePaper(1,false);  
    }*/
    //nprintBitmapLine(lineBuffer,48,bitmap,8,printTime,2);
    /*nprintBitmapLine(lineBuffer,48,Y,8,printTime,1);
    nprintBitmapLine(lineBuffer,48,O,8,printTime,1);
    nprintBitmapLine(lineBuffer,48,L,8,printTime,1);
    nprintBitmapLine(lineBuffer,48,O,8,printTime,1);*/
    /*for(int i=0;i<7;i++){
         nprintLine(yolo[i],48,printTime,20,true); 
    }
    movePaper(10);*/
    /*generateTestLine(lineBuffer,48,240);
    nprintLine(lineBuffer,48,printTime,2,true);
    movePaper(1,false);*/
    
    stepperFree();
    
}

//qwertyuiopasdfghjklzxcvbnm0123456789
void loop(){
    if(serReady){
         //printTextLine 
         Serial.print(serLine);
         Serial.println(serLineLength);
         int part=0;
         while(part<serLineLength){
             printTextLine((char*)serLine.c_str()+part, 
                           serLineLength-part<24 ? (serLineLength-part)%24 : 24,
                           lineBuffer,
                           48,
                           printTime,
                           3);
             part+=23;
         }
         stepperFree();
         //movePaper(10);
         
         serLine="";
         serLineLength=0;
         serReady=false;
    }
    /*for(int i=0;i<7;i++){
             if(i==4)
                 nprintLine(yolo[i],48,printTime,100,true); 
             else
                 nprintLine(yolo[i],48,printTime,20,true); 
        }
        movePaper(30);*/
        //delay(100);
   /* for(int x=1;x<=21;x+=5){
        for(int i=0;i<7;i++){
             nprintLine(yolo[i],48,printTime,x,true); 
        }
        movePaper(10);
        delay(100);
    }
    for(int x=16;x>=5;x-=5){
        for(int i=0;i<7;i++){
             nprintLine(yolo[i],48,printTime,x,true); 
        }
        movePaper(10);
        //delay(100);
    }*/
    /*movePaper(10);
    delay(200);
    movePaper(-10);
    delay(200);*/
  
}


