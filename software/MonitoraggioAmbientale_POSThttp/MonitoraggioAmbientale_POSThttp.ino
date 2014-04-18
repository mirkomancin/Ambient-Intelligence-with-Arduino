/********************************************************************************************************  
   Mirko Mancin - Tesi triennale a.a. 2011/12

   SCHEDA PER IL MONITORAGGIO AMBIENTALE CON 3 SENSORI E INVIO DEI DATI AD UN SERVER CON UN MODULO WIFI
  
 - Sensore Temperatura e Umidità DHT22 D2
 - Sensore Luce A1
 - Sensore Gas (CO2) A2 
 - Modulo RN-XV per comunicazione WiFi 

*********************************************************************************************************/

//DEFINIZIONE LIBRERIE
#include <WiFlyHQ.h>
#include <SoftwareSerial.h>
#include "DHT.h"

//DEFINIZIONE MACRO PER IL RESET DELLA SCHEDA
#include <avr/io.h>
#include <avr/wdt.h>
#define Reset_AVR() wdt_enable(WDTO_30MS); while(1) {}

//DOPO UN ORA SI RESETTERA' LA SCHEDA, ONDE EVITARE PROBLEMI CON LA COMUNICAZIONE WIFI
#define Reset_After_1hour 3600000    

#define Light_PIN A1          //imposto il pin a cui è connesso il sensore per la luce
#define Smoke_PIN A2          //imposto il pin a cui è connesso il sensore per il gas
#define DHT22_PIN 2           //imposto il pin a cui è connesso il sensore dht22

#define DHTTYPE DHT22         // DHT 22  (AM2302)
DHT dht(DHT22_PIN, DHTTYPE);

#define Periodo_Invio_Dati 3000     //tempo minimo tra un'invio sul web e l'altro.(ms)

//WIFI SETTINGS
WiFly wifly;

const char mySSID[] = "tsunami";
const char myPassword[] = "12345678912345678912345678";

char serverName[] = "monfortino.ce.unipr.it";//URL del server a cui connettersi
#define serverPort 80 //porta di connessione
char* macStr;

SoftwareSerial wifi(8,9);

//PROCEDURA DI SCRITTURA IN MEMORIA PROGRAMMA
void print_P(const prog_char *str);
void println_P(const prog_char *str);

//SENSOR SETTINGS
int light_accum = 0; 
float temp_accum = 0.0;
float smoke_accum = 0.0;
float hum_accum = 0.0;

unsigned long time = 0;
unsigned long SendTime = 0;;

char lightBuffer[8];    
char smokeBuffer[8];    
char tempBuffer[8];    
char humBuffer[8];
char timestamp[8];

//BUFFER DOVE SI ANDRANNO A SALVARE LE DUE STRINGHE JSON DA INVIARE AL SERVER
char jsonMsgHead[128];
char jsonMsgBody[128];

void setup(){
  Serial.begin(9600);
  Serial.println(" --- SCHEDA DI MONITORAGGIO AMBIENTALE --- ");
  delay(1000);
  Serial.println("Programma Avviato, Setup Terminato!");
  randomSeed(analogRead(A2)); 
}


void loop(){
    time = millis();
        
    configWIFI();
    SendTime = millis();
    light_accum = readLightSensor();
    smoke_accum = readSmokeSensor();
    readDHTSensor();              
             
    //CONVERTO I VALORI IN STRINGA
    dtostrf(light_accum, 5, 2, lightBuffer);
    dtostrf(smoke_accum, 5, 2, smokeBuffer);
    dtostrf(temp_accum, 5, 2, tempBuffer);
    dtostrf(hum_accum, 5, 2, humBuffer);
    
    //DEFINISCO UN TIMESTAMP FITTIZIO PERCHè NON RIESCO A PRELEVARE L'ORARIO DAL MICROCONTROLLORE
    sprintf(timestamp, "%d", random(1, 32000));    
       
    //CREO LA STRINGA JSON CHE MI SERVIRA PER INVIARE I DATI AL SERVER
    Serial.println("CREO JSON");
    sprintf(jsonMsgHead,"{\"timestamp\":%s,\"checksum\":\"%s\",\"mac\":\"%s\"\0", timestamp, "5EB63BBBE01EEED093CB22BB8F5ACDC3", "00:06:66:71:d2:68");
    sprintf(jsonMsgBody,",\"light\":%s,\"gas\":%s,\"temperature\":%s,\"humidity\":%s}\0", lightBuffer, smokeBuffer, tempBuffer, humBuffer);
    
    //CONTO QUANTO è LUNGA LA STRINGA JSON PERCHè MI SERVIRà PER LA RICHIESTA DA FARE AL SERVER
    int i; 
    for(i=0; jsonMsgHead[i]!=0; i++); i++;
    for(int j=0;jsonMsgBody[j]!=0; j++, i++);
    
    Serial.println("invio lettura sensori");             
    InvioWIFIHttp(jsonMsgHead,jsonMsgBody, i);            
    Serial.println("fine invio");            
    
    Reset_AVR();                             
}

//PROCEDURA DI LETTURA SENSORI
float readSmokeSensor(){
    int val = analogRead(Smoke_PIN);       // read the value from the analog sensor
    float Ro = 12000.0;                    // this has to be tuned 12K Ohm
    
    float Vrl = val * ( 5.00 / 1024.0  );  // V
    float Rs = 20000 * ( 5.00 - Vrl) / Vrl;// Ohm 
    float ratio =  Rs/Ro;    
    
    float ppm = 0.0;
    ppm = 37143 * pow (ratio, -3.178);
    return ppm;    
}

int readLightSensor(){
    int luce = analogRead(Light_PIN);     // Inserisco il valore della lettura dell'input analogico sull'intero ValoreSensore
    luce = (5.0* luce * 200.0) / 1024.0;   
    return luce;
}

void readDHTSensor(){
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    
    // check if returns are valid, if they are NaN (not a number) then something went wrong!
    if ((isnan(t))||(isnan(h))) {
        Serial.println("Failed to read from DHT");
    } else {
        temp_accum=t;
        hum_accum=h;
     }  
}

//PROCEDURA DI CONFIGURAZIONE E CONNESSIONE ALLA RETE WIFI
//SE CI SONO PROBLEMI CON IL WIFI RESETTO LA SCHEDA E RIPROVO AL PROSSIMO RIAVVIO
int configWIFI(){
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

//PROCEDURA D'INVIO CHE MANDA I DATI AL SERVER CON UNA STRINGA JSON, 
//OGNI VOLTA ATTENDO LA RISPOSTA DAL SERVER E LEGGO IL VALORE DI RITORNO PER 
//SAPERE QUANDO DEVO CAMPIONARE I PROSSIMI DATI
void InvioWIFIHttp(char* jsonStringHead,char* jsonStringBody, int lungh)
{
  Serial.println("CREO RICHIESTA");
  
  wifly.print("POST /dsgm2m/api/m2m.php HTTP/1.0\r\n");
  wifly.print("Content-Length: ");
  wifly.print(lungh);
  wifly.print("\r\n\r\n");
  wifly.print(jsonStringHead);
  wifly.print(jsonStringBody);
  
  Serial.println("RISPOSTA DEL SERVER");
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
           if(buf[0]=='{'){
             delay(50);
             long timeSend = parsingJSONString(buf, sizeof(buf));
             timeSend *= 1000;
             
             //ATTENDO QUESTO VALORE PER POI RESETTARE LA SCHEDA
             delay(timeSend);            
           }
       }      
    }   
}

//PROCEDURA CHE MI PERMETTE DI FARE UN PARSING "GREZZO" DI UNA STRINGA JSON
long parsingJSONString(char buffer[], int len){
    int k;
    unsigned long m;
    
    for(int i=0; i<len; i++){
      if((buffer[i]=='"')&&(buffer[i+1]=='c')&&(buffer[i+2]=='f')&&(buffer[i+3]=='g')&&(buffer[i+4]=='"')){
        for(k=i+23; buffer[k]!='"'; k++){
          Serial.print(buffer[k]);          
        }
        unsigned long value=0;
        Serial.println();
        
        for(int l=k-1, m=1; l>i+22; l--){
          value += m*((int)buffer[l]-48);
          m *= 10;
        }
        
        return value;
      }
    }
    
    return 0;
}
