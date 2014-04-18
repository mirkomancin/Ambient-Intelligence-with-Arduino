/********************************************************************************************************  
   Mirko Mancin - Tesi triennale a.a. 2011/12

   SCHEDA PER LA LETTURA DEI DATI DA XBEE E INVIO AL SERVER TRAMITE ETHERNET
  
 - XBEE MODULE
 - ETHERNET

*********************************************************************************************************/

//DEFINIZIONE LIBRERIE
#include <SPI.h>
#include <Ethernet.h>
#include <SoftwareSerial.h>
#include <String.h>

//DEFINIZIONE MACRO PER IL RESET DELLA SCHEDA
#include <avr/io.h>
#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

//DOPO UN ORA SI RESETTERA' LA SCHEDA, ONDE EVITARE PROBLEMI CON LA COMUNICAZIONE WIFI
#define Reset_After_1hour 3600000 

//DEFINISCO LA SERIALE DI COMUNICAZIONE PER XBEE
SoftwareSerial SerialXbee(2,3);

//CONFIGURAZIONE ETHERNET
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x1E, 0x52 };
//byte ip[] = { 192,168,1,10 };

char buffer[32];
char bufferRisp[256];
int valore=0;
int ID=0;
char* sensor;

char serverName[] = "monfortino.ce.unipr.it";//URL del server a cui connettersi
#define serverPort 80 //porta di connessione

EthernetClient client;//dichiaro una variabile globale per il client ethernet

unsigned long time = 0;
unsigned long SendTime = 0;

char valBuffer[8];
char timestamp[8];

char jsonMsg[256];

int sizeRisp = 0;
int richiedorisposta = 0;

void setup() {
  // start serial port:
  //SerialXbee.begin(9600);
  Serial.begin(9600);
  // give the ethernet module time to boot up:
  delay(1000);
  // start the Ethernet connection:
  Serial.println("...Configuro la connessione ethernet...");
  Ethernet.begin(mac);  //configuro lo shield ethernet
  Serial.println("Connessione Configurata.");
  delay(1000);//aspetto un secondo per far avviare lo shield ethernet
  Serial.println("Programma Avviato, Setup Terminato!");
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  
  int inputSize = 0;
  int i=0;  
  int moltip=1;
  
  //SE CI SONO DATI IN ARRIVO DA XBEE LI LEGGO
  if(Serial.available()>0) {
      delay(10);                //DEVO ASPETTARE UN PO PER RIEMPIRE IL BUFFER
      inputSize = Serial.available();
      
        for (i = 0; i < inputSize; i++){
           buffer[i] = Serial.read();           
           buffer[i+1] = '\0';      
        }
        
        Serial.print("SIZE: ");
        Serial.print(inputSize);
        Serial.print(" - BUFFER: ");
        Serial.println(buffer);
        if(inputSize<10){
            parsingString(buffer, inputSize);
        }else parsingBuffer(buffer, inputSize);
      
      //DEVO FARE QUESTO CONTROLLO PERCHE IL VALROE DEL SENSORE è FLOAT, MENTRE GLI ALTRI INT
      if(ID==1000) dtostrf(valore, 5, 2, valBuffer);
      else sprintf(valBuffer, "%d", valore);
      
      sprintf(timestamp, "%d", random(1, 32000));
      
      sprintf(jsonMsg,"{\"timestamp\":\"%s\",\"checksum\":\"%s\",\"mac\":\"%s\",\"%s\":%s}\0", timestamp, "5EB63BBBE01EEED093CB22BB8F5ACDC3", "90:A2:DA:0D:1E:52", sensor, valBuffer);
      for(i=0; jsonMsg[i]!=0; i++);
      i++;
      Serial.print(++i);
      Serial.print(" : ");
      Serial.println(jsonMsg);
             
            
      if (client.connect(serverName, serverPort)) //connessione al server per invio lettura sensore temperatura
          {
              Serial.println("CONNESSO...");
              delay(200);
              InvioJSONRequest(jsonMsg, i);
              delay(200);
              client.stop();
              Serial.println("...FINE INVIO!!!");
              ID=0;
              valore=0;
              if(richiedorisposta==1){
                parsingJSONRisp(bufferRisp, sizeof(bufferRisp));              
              }
          }
          
       ID=0;
       valore=0;
    }

    if(time>Reset_After_1hour){
        Serial.println("Goodbye!!");
        Reset_AVR();
    }    
}

void parsingJSONRisp(char buffer[], int len){
    //{"status":"200","msg":"Log Correctly Added () !"}
    char tmp[256];
    int j=0;
    Serial.flush();
    
    for(int i=1; buffer[i-1]!='}'; i++){
        if((buffer[i]==',')||(buffer[i+2]==0)){          
          delay(100);
          tmp[j]=0;
          checkCmd(tmp);
          j=0;          
        }else{
          tmp[j]=buffer[i];        
          j++;
        }
    }
}

//CONTROLLO CHE COMANDO SIA ARRIVATO DAL SERVER
void checkCmd(char cmd[]){
    int k;
    char *intStr;
    //COMANDO CONFIGURAZIONE
    if((cmd[1]=='c')&&(cmd[2]=='f')&&(cmd[3]=='g')){
        //dalla 23 casella fino all doppio apice c'è il numero  
        Serial.print("TEMPO: ");
        for(k=23; cmd[k]!='"'; k++){
          Serial.print(cmd[k]);          
        }
        int value=0;
        for(int l=k-1, m=1; l>22; l--){
          value += m*((int)cmd[l]-48);
          m *= 10;
        }
        Serial.print(" - int: ");
        Serial.println(value);
        //SerialXbee.print(value);
    }
}

void parsingBuffer(char* string, int lenght){
    char tmp[10];
    int i=0;
    int j=0;
    while(i<lenght-1){
          if(string[i]!=';'){
              tmp[j]=string[i];
              j++;
          }else{
              tmp[j]=0;
              parsingString(tmp,j);              
              j=0;
              tmp[0]=0;              
          }
          i++;
    }
}

void parsingString(char* string, int lenght){
    //STRINGA = xxxx:valore;  dove xxxx è l'ID del sensore, valore viene mandato intero
    int moltipID = 1000;
    int moltipVALORE = 1;
        
    for(int i=0; i<4; i++){        
        ID += ((int)string[i]-48)*moltipID;
        moltipID /= 10;     
    }
    
    for(int i=lenght-2; i>=5; i--){
        valore += ((int)string[i]-48)*moltipVALORE;
        moltipVALORE *= 10;          
    }
    
    ID2Sensor(ID);
    
    Serial.print("Sensore: ");
    Serial.print(sensor);
    Serial.print(" - Valore: ");
    Serial.println(valore);   
}

void ID2Sensor(int _ID){
    if(_ID==1000){ 
      sensor="sound";
      richiedorisposta=1;
    }
    if(_ID==6000){
      sensor="PIR";
      richiedorisposta=0;
    }
    if(_ID==7000){ 
      sensor="RFID";  
      richiedorisposta=0;
    }
}

void InvioJSONRequest(char* jsonString, int lungh)
{
  client.print("POST /dsgm2m/api/m2m.php HTTP/1.0\r\n");
  client.print("Content-Length: ");
  client.print(lungh);
  client.print("\r\n\r\n");
  client.print(jsonString);
  
  Serial.println("...RISPOSTA...");
  
  boolean currentLineIsBlank = true;
  int exit=0;  
  int i=0;
  
  if(richiedorisposta==1){
  while(client.connected()){  
     if(client.available() > 0) {
       char c = client.read();
        Serial.write(c);
        if(exit==1){
            bufferRisp[i] = c;
            i++;
        }
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {                   
          if(exit==1){
             break;
          }
          else exit = 1;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
  } 
  }
}
