/********************************************************************************************************  
   Mirko Mancin - Tesi triennale a.a. 2011/12

   SCHEDA PER IL RICONOSCIMENTO AMBIENTALE ATTRAVERSO UN SENSORE DI MOVIMENTO, 
   UN RFID READER E UN SENSORE DI SUONO, MANDO I DATI CON UN MODULO XBEE
  
 - Sensore suono
 - Lettore RFID
 - PIR motion sensor
 - Modulo XBEE S1

*********************************************************************************************************/

//DEFINIZIONE LIBRERIE
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Ethernet.h>

//DEFINIZIONE MACRO PER IL RESET DELLA SCHEDA
#include <avr/io.h>
#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

//DOPO UN ORA SI RESETTERA' LA SCHEDA, ONDE EVITARE PROBLEMI CON LA COMUNICAZIONE WIFI
#define Reset_After_1hour 3600000 

//MI SERVONO DUE SERIALI PER COMUNICAZIONE CON LA RFID SHIELD E CON IL MODULO XBEE
SoftwareSerial rfid(7, 8);
SoftwareSerial xbee(10, 9);

//DEFINISCO I PIN DI LETTURA DEI SENSORI E GLI ID DEI SENSORI
#define Sound_PIN A0 
#define ledSound_PIN 4
#define id_Sound 1000
#define id_PIR 6000
#define id_RFID 7000

#define Periodo_Invio_Dati 30000     //30s = tempo minimo tra un'invio sul web e l'altro.(ms)
#define Periodo_Lettura_Sensore 2000 //2s = tempo minimo tra una lettura del sensore e l'altra (ms)

//PROTOTIPI DELLE PROCEDURE UTILI PER L'RFID READER
void halt(void);		//Procedura di arresto	
void parse(void);		//Procedura che legge la stringa dal modulo
void print_serial(void);	//Procedura che stamp ail numero seriale
void read_serial(void);	        //Procedura che legge dal modulo
void seek(void);		//commando per mettersi in attesa del tag
void set_flag(void);		//imposto flag se ho ricevuto un tag corretto

//VARIABILI GLOBALI
int flag = 0;			//FLAG per il tag
int Str1[11];			//Buffer di ricezione

//LED RGB PER IL RICONOSCIMENTO DEI TAG
int ledDigital[3] = {3, 5, 6}; 

//ESSENDO IL LED RGB AD ANODO COMUNE DEVO DEFINIRE PER COMODITA QUESTE DUE VARIABILI 
const boolean ON = LOW;   //Define on as LOW (this is because we use a common Anode RGB LED (common pin is connected to +5 volts)
const boolean OFF = HIGH; //Define off as HIGH

//COLORI PREDEFINITI
const boolean RED[] = {ON, OFF, OFF};    
const boolean GREEN[] = {OFF, ON, OFF}; 
const boolean BLUE[] = {OFF, OFF, ON}; 
const boolean YELLOW[] = {ON, ON, OFF}; 
const boolean CYAN[] = {OFF, ON, ON}; 
const boolean MAGENTA[] = {ON, OFF, ON}; 
const boolean WHITE[] = {ON, ON, ON}; 
const boolean BLACK[] = {OFF, OFF, OFF}; 

//ARRAY CHE CONTIENE I COLORI DEFINITI PRIMA
const boolean* COLORS[] = {RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA, WHITE, BLACK};

//VARIABILI PER I SENSORI
int sound_accum = 0;
long n_camp = 0;
float avg_sound = 0.0;

unsigned long time = 0;
unsigned long SendTime = 0;
unsigned long ReadTime = 0;
unsigned long readyPIR = 0;

//PIR SENSOR
int timerPIR = 100;
int alarmPin = A2;
int alarmValue = 0;
int ledPir = 4;
int count = 0;


void setup(){
  for(int i = 0; i < 3; i++){   
    pinMode(ledDigital[i], OUTPUT);   //Set the three LED pins as outputs   
  }
  pinMode(ledSound_PIN, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Start");
  
  pinMode(ledPir, OUTPUT);  
  pinMode(alarmPin, INPUT);
  delay(2000);
  Serial.println("PIR Sensor READY!");
  
  // SETTO IL BAUDRATE PER LE SERIALI DI COMUNICAZIONE
  xbee.begin(9600);
  rfid.begin(19200);
  
  //ATTENDO QUALCHE MILLISECONDO PRIMA DI DARE IL COMANDO D'INIZIALIZZAZIONE AL RFID SHIELD
  delay(10);
  halt(); 
}


void loop()                 
{
  read_serial();
  
  if(xbee.available()>0) {
      delay(10);                //DEVO ASPETTARE UN PO PER RIEMPIRE IL BUFFER
      int inputSize = xbee.available();
      char buffer[256];
      
      for (int i = 0; i < inputSize; i++){
        buffer[i] = xbee.read();           
        buffer[i+1] = '\0';      
       }
       Serial.println(buffer);
  }
        
  time = millis();  

  //INVIO LETTURA DEL SENSORE DEL SUONO
  if(time > SendTime + Periodo_Invio_Dati){
     SendTime = millis();
     avg_sound = float(sound_accum / double(n_camp));//calcolo la media delle lettura     
     if(n_camp > 0){
           digitalWrite(ledSound_PIN, HIGH);
           Serial.println("connesso");
           Serial.println("invio sound sensor");
           Serial.println(avg_sound); 
           sendXbee((int)avg_sound,id_Sound);          
           Serial.println("Client Disconnesso");
           Serial.println("fine invio");
           digitalWrite(ledSound_PIN, LOW);       
     }
     else
           Serial.println("Nessuna Campionatura, controllare sensore");
      n_camp = 0; //azzero le variabili per iniziare nuovamente il calcolo della media
      sound_accum = 0;       
   }
   
   //LETTURA DEL SENSORE DEL SUONO
   if(time > ReadTime + Periodo_Lettura_Sensore){
       ReadTime = millis();
       sound_accum += readSoundSensor();               
        
       n_camp++;        
       Serial.print("Campione : ");
       Serial.print(n_camp);
       Serial.print(" - ");
       Serial.println(time);  
   }
   
   //IL PIR HA BISOGNO DI UN PERIODO DI SETTAGGIO TRA UNA LETTURA E L'ALTRA
   if(time>readyPIR+timerPIR){
       alarmValue = analogRead(alarmPin);
       
       if (alarmValue < 100) count++;
       else count=0; 
       
       if (count>2){          
         Serial.println("MOVIMENTO!!");
         sendXbee(101,id_PIR);
         blinky(ledPir);
         count=0;         
       }       
       readyPIR=millis();
     }  

    if(time>Reset_After_1hour){
        Serial.println("Goodbye!!");
        Reset_AVR();
    }     
}

//MANDO I DATI ALL'ALTRO XBEE
void sendXbee(int value, int id){
    xbee.print(id);
    xbee.print(":");    
    xbee.print(value);
    xbee.print(";"); 
    xbee.flush();
    delay(200);   
}

//MANDO I TAG ALL'ALTRO XBEE
void sendTAGtoXbee(String A, String B, String C, String D, int id){
    xbee.print(id);
    xbee.print(":");    
    xbee.print(A);
    xbee.print(B);
    xbee.print(C);
    xbee.print(D);
    xbee.print(";"); 
    xbee.flush();
    delay(200);   
}

//FACCIO LAMPEGGIARE UN LED PER 3 VOLTE
void blinky(int pin) {
  for(int i=0; i<3; i++) {
    digitalWrite(pin,HIGH);
    delay(200);
    digitalWrite(pin,LOW);
    delay(200);
  }
}

//LEGGO IL SENSORE DEL SUONO
int readSoundSensor(){
   int value = analogRead(Sound_PIN); // Inserisco il valore della lettura dell'input analogico sull'intero ValoreSensore
   int suono = 16.801 * log(value) + 9.872;   //calcolo per il corretto valore in dB
   return suono;
}

//LEGGO IL TAG RFID E LO SALVO IN UN BUFFER
void parse()
{
  while(rfid.available()){
    if(rfid.read() == 255){
      for(int i=1;i<11;i++){
        Str1[i]= rfid.read();
      }
    }
  }
}

//STAMPO SULLA SERIALE IL TAG LETTO DALL'RFID E CON LA PROCEDURA
//riconosci() INVIO A XBEE IL TAG CORRETTO
void print_serial()
{
  if(flag == 1){
    riconosci();
    //print to serial port
    Serial.print(Str1[8], HEX);
    Serial.print(Str1[7], HEX);
    Serial.print(Str1[6], HEX);
    Serial.print(Str1[5], HEX);
    Serial.println();
    sendTAGtoXbee(String(Str1[8],HEX),String(Str1[7],HEX),String(Str1[6],HEX),String(Str1[5],HEX),id_RFID);
    delay(100);    
  }
}

//INVIO IL TAG A XBEE
void riconosci(){
  if((Str1[8])==176){              //TAG RJ
      setColor(ledDigital, RED);
      delay(50);    
      setColor(ledDigital, BLACK);
      delay(50);    
      setColor(ledDigital, RED);
      delay(50);       
  }
  
  if((Str1[8])==165){               //TAG H
      setColor(ledDigital, BLUE);
      delay(50);    
      setColor(ledDigital, BLACK);
      delay(50);    
      setColor(ledDigital, BLUE);
      delay(50);          
  }
  
}

void read_serial()
{
  seek();
  delay(10);
  parse();
  set_flag();
  print_serial();
  delay(100);
  setColor(ledDigital, BLACK);
}

//-----COMANDI PER RFID-----
void seek()
{
  //search for RFID tag
  rfid.write((byte)0xFF);
  rfid.write((byte)0x00);
  rfid.write((byte)0x01);
  rfid.write((byte)0x82);
  rfid.write((byte)0x83); 
  delay(10);
}

void halt()
{
 //Halt tag
  rfid.write((byte)0xFF);
  rfid.write((byte)0x00);
  rfid.write((byte)0x01);
  rfid.write((byte)0x93);
  rfid.write((byte)0x94);
}

void set_flag()
{
  if(Str1[2] == 6){
    flag++;
  }
  if(Str1[2] == 2){
    flag = 0;
  }
}

//PROCEDURE PER SETTARE IL COLORE DEL LED
void setColor(int* led, boolean* color){ 
  for(int i = 0; i < 3; i++){   
    digitalWrite(led[i], color[i]); 
  }
}

void setColor(int* led, const boolean* color){  
  boolean tempColor[] = {color[0], color[1], color[2]};  
  setColor(led, tempColor);
}

