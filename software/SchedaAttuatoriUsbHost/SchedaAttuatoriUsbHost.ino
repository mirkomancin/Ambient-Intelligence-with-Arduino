/********************************************************************************************************  
 * Mirko Mancin - Tesi triennale a.a. 2011/12
 * 
 * SCHEDA PER IL RICONOSCIMENTO DI USB HOST, 
 * ESEMPI DI ATTUATORI
 * 
 * - MEGA ADK FOR ANDROID
 * - LED (6)
 * - VENTOLE 5V (8)
 * - SERVO MOTORE (3)
 * 
 *********************************************************************************************************/

//DEFINIZIONE LIBRERIE
#include <UsbHost.h>
#include <AndroidAccessory.h>
#include <SPI.h>
#include <Ethernet.h>
#include <Servo.h> 

//DEFINIZIONE MACRO PER IL RESET DELLA SCHEDA
#include <avr/io.h>
#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

//DOPO UN ORA SI RESETTERA' LA SCHEDA, ONDE EVITARE PROBLEMI CON LA COMUNICAZIONE WIFI
#define Reset_After_1hour 60000 

//DEFINIZIONE DELLE VARIABILI PER L'USB HOST
char accessoryName[] = "Mega_ADK"; // your Arduino board
char companyName[] = "Arduino";
bool conn = false;

MAX3421E Max;
UsbHost Usb;
AndroidAccessory usb_android(companyName, accessoryName);

//CONFIGURAZIONE PER L'ETHERNET
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x1D, 0x93 };
//byte ip[] = { 192,168,1,10 };

#define bufferMax 128
int bufferSize;
char buffer[bufferMax];
char bufferRisp[256];

char serverName[] = "monfortino.ce.unipr.it";//URL del server a cui connettersi
#define serverPort 80 //porta di connessione

EthernetClient client;//dichiaro una variabile globale per il client ethernet
EthernetServer server(8080); // Port 80 is http.

unsigned long time = 0;

char valBuffer[8];
char timestamp[8];

char jsonMsg[256];

int sizeRisp = 0;
int richiedorisposta = 0;
int i;

//ATTUATORI
Servo myservo;  // create servo object to control a servo 

int potpin = A10;  // analog pin used to connect the potentiometer
int valServo;    // variable to read the value from the analog pin 

const int buttonPinFan = 7;     // the number of the pushbutton pin
const int fanPin =  8;      // the number of the LED pin

int valFan=0; //val will be used to store state of pin 
int old_val_Fan=0;

int buttonFan = 0;

const int buttonPinLed = 5;     // the number of the pushbutton pin
const int LedPin =  6;      // the number of the LED pin

int valLed=0; //val will be used to store state of pin 
int old_val_Led=0;

int buttonLed = 0;



void setup(){
  //CONFIGURAZIONE USB HOST
  usb_android.begin(); 
  Serial.begin( 9600 );
  Serial.println("Start");
  Max.powerOn();
  Serial.println("USB HOST SHIELD"); 

  //CONFIGURAZIONE ETHERNET
  Serial.println("...Configuro la connessione ethernet...");
  Ethernet.begin(mac);  //configuro lo shield ethernet
  Serial.println("Connessione Configurata.");
  delay(1000);//aspetto un secondo per far avviare lo shield ethernet
  Serial.println("Programma Avviato, Setup Terminato!");
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  server.begin();
  client.flush();
  // initialize the LED pin as an output:
  pinMode(fanPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(buttonPinFan, INPUT); 
  
  // initialize the LED pin as an output:
  pinMode(LedPin, OUTPUT);      
  // initialize the pushbutton pin as an input:
  pinMode(buttonPinLed, INPUT);

  myservo.attach(3);  // attaches the servo on pin 33 to the servo object 
  delay(2000);
}

void loop(){
  time = millis();

  //setServo();
  setFan();
  setLed();
  
  client = server.available();
  if (client)
  {
    Serial.println("Request incoming..");
    WaitForRequest(client);
    ParseReceivedRequest();
    PerformRequestedCommands();
    Serial.println("Richiesta servita");
    byte a;
    while (client.available()) {a=client.read();}
    client.stop();
    EmptyBuffer();
    delay(1000);
  }

  if(usb_android.isConnected() && !conn){
    conn = true;
    if (!usbcheck()) Serial.println("USB connection test failed."); 
  }    

  if(!usb_android.isConnected()){ 
    conn = false;     
  }

  delay(100);
  EmptyBuffer();
  delay(100);
  /*if(time>Reset_After_1hour){
    Serial.println("Goodbye!");
    Reset_AVR();
  }*/
}

//CONTROLLO IL POTENZIOMETRO E SETTO IL SERVOMOTORE
void setServo(){
  valServo = analogRead(potpin);            // reads the value of the potentiometer (value between 0 and 1023) 
  valServo = map(valServo, 0, 1023, 0, 179);     // scale it to use it with the servo (value between 0 and 180) 
  myservo.write(valServo);                  // sets the servo position according to the scaled value 
  delay(15);
}

//CON IL BOTTONE VADO AD AZIONE LE VENTOLE
void setFan(){
  valFan = digitalRead(buttonPinFan);
  //check if input is HIGH
  
  if ((valFan == HIGH) && (old_val_Fan == LOW)) {
    buttonFan = 1 - buttonFan;
    delay(50);
  }

  old_val_Fan = valFan;

  if (buttonFan == 1) {     
    // turn LED on:    
    digitalWrite(fanPin, HIGH);  
  } 
  else {
    // turn LED off:
    digitalWrite(fanPin, LOW); 
  }
}

//CON IL BOTTONE VADO AD AZIONARE IL LED
void setLed(){
  valLed = digitalRead(buttonPinLed);
  //check if input is HIGH
 
  if ((valLed == HIGH) && (old_val_Led == LOW)) {
    buttonLed = 1 - buttonLed;    
    delay(50);
  }

  old_val_Led = valLed;

  if (buttonLed == 1) {     
    // turn LED on:    
    digitalWrite(LedPin, HIGH);  
  } 
  else {
    // turn LED off:
    digitalWrite(LedPin, LOW); 
  }
}

/* Test USB connectivity */
bool usbcheck(){
  bool exit=false;
  byte rcode;
  byte usbstate;
  Max.powerOn();
  delay( 200 );
  Serial.println("\r\nUSB Connectivity test. Waiting for device connection... ");
  while(!exit){      
    delay( 200 );
    Max.Task();
    Usb.Task();
    usbstate = Usb.getUsbTaskState();
    //--DEBUG Serial.println(usbstate);
    switch( usbstate ) {
      case( USB_ATTACHED_SUBSTATE_RESET_DEVICE ):
      Serial.println("\r\nDevice connected. Resetting");          
      break;
      case( USB_ATTACHED_SUBSTATE_WAIT_SOF ): 
      Serial.println("\r\nReset complete. Waiting for the first SOF...");
      break;  
      case( USB_ATTACHED_SUBSTATE_GET_DEVICE_DESCRIPTOR_SIZE ):
      Serial.println("\r\nSOF generation started. Enumerating device.");
      break;
      case( USB_STATE_ADDRESSING ):
      Serial.println("\r\nSetting device address");
      break;
      case( USB_STATE_RUNNING ):
      Serial.println("\r\nGetting device descriptor");
      rcode = getdevdescr( 1 );
      exit = true;
      if( rcode ) {
        Serial.print("\r\nError reading device descriptor. Error code ");
        //print_hex( rcode, 8 );
        Serial.print( rcode, OCT );
      }
      else {
        Serial.println("\r\n\nAll tests passed.");
        Usb.setUsbTaskState(0xF0);               
      }          
      break;
      case( USB_STATE_ERROR ):
      Serial.println("\r\nUSB state machine reached error state");
      break;
    default:
      {          
        break;
      }
    }//switch
  }//while(1)
}

/* Get device descriptor */
byte getdevdescr( byte addr )
{
  USB_DEVICE_DESCRIPTOR buf;
  byte rcode;
  rcode = Usb.getDevDescr( addr, 0, 0x12, ( char *)&buf );
  if( rcode ) {
    return( rcode );
  }
  Serial.print("\r\nDevice descriptor: ");
  Serial.print("\r\nDescriptor Length:\t");
  //print_hex( buf.bLength, 8 );
  Serial.print( buf.bLength, OCT );
  Serial.print("\r\nUSB version:\t");
  //print_hex( buf.bcdUSB, 16 );
  Serial.print( buf.bcdUSB, HEX );
  Serial.print("\r\nVendor ID:\t");
  Serial.print(buf.idVendor, HEX);
  //print_hex( buf.idVendor, 16 );
  Serial.print("\r\nProduct ID:\t");
  Serial.print(buf.idProduct, HEX);
  //print_hex( buf.idProduct, 16 );
  Serial.print("\r\nRevision ID:\t");
  Serial.println(buf.bcdDevice, HEX);
  //print_hex( buf.bcdDevice, 16 );
  delay(100);
  sendDataToEthernet(buf.idVendor, buf.idProduct, buf.bcdDevice);
  delay(100);
  return( 0 );
}

//MANDO I DATI DELL'USB HOST AL SERVER, LI TRATTO COME SE FOSSERO UN TAG
void sendDataToEthernet(int idVendor, int idProduct, int bcdDevice){
  sprintf(timestamp, "%d", random(1, 32000));

  sprintf(jsonMsg,"{\"timestamp\":\"%s\",\"checksum\":\"%s\",\"mac\":\"%s\",\"usbhost\":\"%d.%d.%d\"}\0", timestamp, "5EB63BBBE01EEED093CB22BB8F5ACDC3", "90:A2:DA:0D:1D:93", idVendor, idProduct, bcdDevice);
  for(i=0; jsonMsg[i]!=0; i++);
  i++;

  Serial.println("");
  Serial.println(jsonMsg);
  
  //while(!client.connect(serverName, serverPort))
  delay(3000);
  if(client.connect(serverName, serverPort)) //connessione al server per invio lettura sensore temperatura
  {
    Serial.println("CONNESSO...");
    delay(200);
    InvioJSONRequest(jsonMsg, i);
    delay(200);
    client.stop();
    Serial.println("...FINE INVIO!");              
  }
  client.flush();
  client.stop();

}

//INVIO LA STRINGA JSON AL SERVER
void InvioJSONRequest(char* jsonString, int lungh)
{
  client.print("POST /dsgm2m/api/m2m.php HTTP/1.0\r\n");
  client.print("Content-Length: ");
  client.print(lungh);
  client.print("\r\n\r\n");
  client.print(jsonString); 
}

//-- Commands and parameters (sent by browser) --
char cmd[15];    // Nothing magical about 15, increase these if you need longer commands/parameters
char param1[15];
char param2[15];


void WaitForRequest(EthernetClient client) // Sets buffer[] and bufferSize
{
  bufferSize = 0;

  while (client.connected()) {
    if (client.available()) {
	char c = client.read();
	if (c == '\n')
	  break;
	else
	  if (bufferSize < bufferMax)
	    buffer[bufferSize++] = c;
	  else
	    break;
    }
  }

  PrintNumber("bufferSize", bufferSize);
}

void ParseReceivedRequest()
{
  Serial.println("in ParseReceivedRequest");
  Serial.println(buffer);

  //Received buffer contains "GET /cmd/param1/param2 HTTP/1.1".  Break it up.
  char* slash1;
  char* slash2;
  char* slash3;
  char* space2;

  slash1 = strstr(buffer, "/") + 1; // Look for first slash
  slash2 = strstr(slash1, "/") + 1; // second slash
  slash3 = strstr(slash2, "/") + 1; // third slash
  space2 = strstr(slash2, " ") + 1; // space after second slash (in case there is no third slash)
  if (slash3 > space2) slash3=slash2;

  PrintString("slash1",slash1);
  PrintString("slash2",slash2);
  PrintString("slash3",slash3);
  PrintString("space2",space2);

  // strncpy does not automatically add terminating zero, but strncat does! So start with blank string and concatenate.
  cmd[0] = 0;
  param1[0] = 0;
  param2[0] = 0;
  strncat(cmd, slash1, slash2-slash1-1);
  strncat(param1, slash2, slash3-slash2-1);
  strncat(param2, slash3, space2-slash3-1);

  PrintString("cmd",cmd);
  PrintString("param1",param1);
  PrintString("param2",param2);
}

void PerformRequestedCommands()
{
  if ( strcmp(cmd,"digitalWrite") == 0 ) RemoteDigitalWrite();
  if ( strcmp(cmd,"digitalRead") == 0 ) RemoteDigitalRead();
  if ( strcmp(cmd,"analogRead") == 0 ) RemoteAnalogRead();
  if ( strcmp(cmd,"analogWrite") == 0 ) RemoteAnalogWrite();
}

void RemoteDigitalWrite()
{
  int ledPin = param1[0] - '0'; // Param1 should be one digit port
  int ledState = param2[0] - '0'; // Param2 should be either 1 or 0
  digitalWrite(ledPin, ledState);
  
  if(ledPin==fanPin) buttonFan = ledState;
  if(ledPin==LedPin) buttonLed = ledState;
  
  //-- Send response back to browser --
  server.print("D");
  server.print(ledPin, DEC);
  server.print(" is ");
  server.print( (ledState==1) ? "ON" : "off" );
  delay(200);
  //-- Send debug message to serial port --
  Serial.println("RemoteDigitalWrite");
  PrintNumber("ledPin", ledPin);
  PrintNumber("ledState", ledState);
}
void RemoteDigitalRead()
{
  int ledPin = param1[0] - '0'; // Param1 should be one digit port
  int ledState; // Param2 should be either 1 or 0
  ledState = digitalRead(ledPin);

  //-- Send response back to browser --
  server.print("D");
  server.print(ledPin, DEC);
  server.print(" is ");
  server.print( (ledState==1) ? "ON" : "off" );
  delay(200);
  //-- Send debug message to serial port --
  Serial.println("RemoteDigitalRead");
  PrintNumber("ledPin", ledPin);
  PrintNumber("ledState", ledState);
}

void RemoteAnalogRead()
{
  // If desired, use more server.print() to send http header instead of just sending the analog value.
  int analogPin = param1[0] - '0'; // Param1 should be one digit analog port
  int analogValue = analogRead(analogPin);

  //-- Send response back to browser --
  server.print("A");
  server.print(analogPin, DEC);
  server.print(" is ");
  server.print(analogValue,DEC);
  delay(200);
  //-- Send debug message to serial port --
  Serial.println("RemoteAnalogRead");
  PrintNumber("analogPin", analogPin);
  PrintNumber("analogValue", analogValue);
}

void RemoteAnalogWrite()
{
  // If desired, use more server.print() to send http header instead of just sending the analog value.
  int analogPin = param1[0] - '0'; // Param1 should be one digit analog port
  int analogValue = param2[0] - '0';
  
  if(analogPin!=3) analogWrite(analogPin, analogValue);
  else{     
    myservo.write((analogValue*20)-1);
    delay(15);
  }
  
  //-- Send response back to browser --
  server.print("A");
  server.print(analogPin, DEC);
  server.print(" is ");
  server.print(analogValue,DEC);
  delay(200);
  //-- Send debug message to serial port --
  Serial.println("RemoteAnalogWrite");
  PrintNumber("analogPin", analogPin);
  PrintNumber("analogValue", analogValue);
}

void PrintString(char* label, char* str)
{
  Serial.print(label);
  Serial.print("=");
  Serial.println(str);
  EmptyBuffer();
}

void PrintNumber(char* label, int number)
{
  Serial.print(label);
  Serial.print("=");
  Serial.println(number, DEC);
  EmptyBuffer();
}

void EmptyBuffer(){
  byte a;
  while (Serial.available()) {a=Serial.read();}
}

