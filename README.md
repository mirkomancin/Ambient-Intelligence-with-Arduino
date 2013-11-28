Ambient Intelligence with Arduino
=================================

Ambient intelligence system created with Arduino. In this git I published my thesis work for bachelor degree in "Università degli studi di Parma".

I started working with Arduino for an examination and during the thesis work, I studied this platform and the main types of connection are possible with it.

The system illustrated in Figure 1 is formed by four types of cards made ​​through the Arduino platform (three Arduino UNO board and a Mega ADK). The Arduino boards have been used to create the following devices:

* An Arduino UNO environmental monitoring , by which we can collect data from a temperature sensor and humidity, a smoke sensor
and a light sensor with high precision . The data collected in this form are then
sent to the server , thanks to a WiFi module , in JSON format via HTTP ;

* An Arduino UNO recognition with an RFID reader to read tags
RFID and NFC , a sound sensor high sensitivity is a and a PIR motion sensor . The
Collected data is sent through an Xbee module 1 Series in the form of string
alphanumeric ;

* An Arduino UNO for ZigBee communication ( always through a form
Xbee ), which collects data on the card described above , one gets a JSON string and send it with an HTTP request to the server through an Ethernet Shield.

* Arduino Mega ADK has been used to fit the data to the actuators
and for the USB Host . Also communicates with the server with an Ethernet module .
In the following paragraphs are explained in detail all the cards presented above .
