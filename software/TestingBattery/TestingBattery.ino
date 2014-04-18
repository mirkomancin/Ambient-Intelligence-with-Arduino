// **** INCLUDES *****
#include <WiFlyHQ.h>
#include "LowPower.h"
#include <SoftwareSerial.h>
#define XBEE_sleepPin 6

//DEFINIZIONE MACRO PER IL RESET DELLA SCHEDA
#include <avr/io.h>
#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

#define Periodo_Invio_Dati 3000     //tempo minimo tra un'invio sul web e l'altro.(ms)

//WIFI SETTINGS
WiFly wifly;

const char mySSID[] = "Alice-83216855";
const char myPassword[] = "pippoeplutovannoincitta123";

char serverName[] = "192.168.0.7";//URL del server a cui connettersi
#define serverPort 8080 //porta di connessione
char* macStr;

SoftwareSerial wifi(8,9);

//PROCEDURA DI SCRITTURA IN MEMORIA PROGRAMMA
void print_P(const prog_char *str);
void println_P(const prog_char *str);

int ledBattery=7;

long time;
long readBattery;

void setup(){
    Serial.begin(9600);
    pinMode(ledBattery,OUTPUT);
    pinMode(XBEE_sleepPin,OUTPUT);
    digitalWrite(XBEE_sleepPin,HIGH);
    delay(100);
}

int counter = 1; // timer for counting watchdog cicles

void loop() {
    time = millis();
    Serial.print(counter);
    Serial.print(" ");
    Serial.println(time);
    delay(50);
    if(counter == 1){

      char battery[9];
      dtostrf(readBatteryStatus(), 5, 2, battery);
      Serial.print("Battery: ");
      Serial.print(battery);
      Serial.println("%");
      delay(50);
      //sendWifi(battery);
      sendXbee(battery);
      counter++;
      
    }else if(counter == 8){
      
      counter = 1;
      Reset_AVR();
      
    }else{ 
      counter ++;
      // Enter power down state for 8 s with ADC and BOD module disabled
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
      //delay(8000);   
    }
    // Do something here
    // Example: Read sensor, data logging, data transmission.
}

float readBatteryStatus(){
    int batMonPin = A1;          // input pin for the divider
    int val = 0;                // variable for the A/D value
    float pinVoltage = 0;       // variable to hold the calculated voltage
    float batteryVoltage = 0;
    float batteryVin = 11.98;
    float ratio = 2.4448979591836734693877551020408;        //Vin/Vout iniziali Change this to match the MEASURED ration of the circuit
    
    val = analogRead(batMonPin);    // read the voltage on the divider      
    pinVoltage = val * 0.00488;       //  Calculate the voltage on the A/D pin
                                      //  A reading of 1 for the A/D = 0.0048mV
                                      //  if we multiply the A/D reading by 0.00488 then 
                                      //  we get the voltage on the pin.                                
    batteryVoltage = pinVoltage * ratio;    //  Use the ratio calculated for the voltage divider
                                            //  to calculate the battery voltage
    float percent = (batteryVoltage/batteryVin)*100;
    
    if(percent < 20.0) digitalWrite(ledBattery, HIGH);
    
    return percent;
}

void sendWifi(char* value){  
  println_P(PSTR("Starting"));
    print_P(PSTR("Free memory: "));
    Serial.println(wifly.getFreeMemory(),DEC);

    wifi.begin(9600);    //DEFINISCO IL BAUDRATE DI COMUNICAZIONE WIFI
    if (!wifly.begin(&wifi, &Serial)) {
        println_P(PSTR("Failed to start wifly"));
	Reset_AVR();
    }

    char buf[32];
    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
	println_P(PSTR("Joining network"));
	wifly.setSSID(mySSID);
	//wifly.setPassphrase(myPassword);
        wifly.setKey(myPassword);
	wifly.enableDHCP();

	if (wifly.join()) {
	    println_P(PSTR("Joined wifi network"));
	} else {
	    println_P(PSTR("Failed to join wifi network"));
	    Reset_AVR();
	}
    } else {
        println_P(PSTR("Already joined network"));
    }

    print_P(PSTR("MAC: "));
    macStr = (char *)(wifly.getMAC(buf, sizeof(buf)));
    Serial.println(macStr);
    print_P(PSTR("IP: "));
    Serial.println(wifly.getIP(buf, sizeof(buf)));
    print_P(PSTR("Netmask: "));
    Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    print_P(PSTR("Gateway: "));
    Serial.println(wifly.getGateway(buf, sizeof(buf)));
    print_P(PSTR("SSID: "));
    Serial.println(wifly.getSSID(buf, sizeof(buf)));

    wifly.setDeviceID("Wifly-WebClient");
    print_P(PSTR("DeviceID: "));
    Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    if (wifly.isConnected()) {
        println_P(PSTR("Old connection active. Closing"));
	wifly.close();
    }
    
    if (wifly.open(serverName, serverPort)) {
        print_P(PSTR("Connected to "));
	Serial.println(serverName);

	Serial.println("WIFI ALREADY");
    } else {
        println_P(PSTR("Failed to connect"));
        Reset_AVR();
    }
    
    char buffer[512];
    Serial.println("MANDO RICHIESTA ");
    Serial.println(value);
    wifly.print("GET /arduino/data_upload.php?battery="); 
    wifly.print(value);
    wifly.print(" HTTP/1.0\r\n\r\n");
    
    delay(500);
    /*Serial.println("RISPOSTA DEL SERVER");
  //ATTENDO LA RISPOSTA DAL SERVER
  while(wifly.available()==0){}
  
     if(wifly.available() > 0) {
       char buf[200] = "buffer";
       int exit = 0;
       
       while(exit<2){
           wifly.gets(buf, sizeof(buf));
           Serial.println(buf);
           if(buf[0]==0){ 
             exit++; 
           }           
       }      
    }
    //Reset_AVR(); */
}

void sendXbee(char value[]){
  digitalWrite(XBEE_sleepPin,LOW);
  Serial.println("TEST XBEE");
  delay(100);
  SoftwareSerial SerialXbee(2,3);
  SerialXbee.begin(9600);
  
  SerialXbee.print("POST /dsgm2m/api/m2m.php HTTP/1.0\r\n");
  SerialXbee.print("Content-Length: ");
  SerialXbee.print(10);
  SerialXbee.print("\r\n\r\n");
  SerialXbee.println(value);
  //SerialXbee.print("1234567890zxcvbnmasdfghjklqwertyuiop");
  delay(200);
  digitalWrite(XBEE_sleepPin,HIGH);
}

//STAMPO UNA STRINGA DA MEMORIA PROGRAMMA
void print_P(const prog_char *str)
{
    char ch;
    while ((ch=pgm_read_byte(str++)) != 0) {
	Serial.write(ch);
    }
}

void println_P(const prog_char *str)
{
    print_P(str);
    Serial.println();
}
