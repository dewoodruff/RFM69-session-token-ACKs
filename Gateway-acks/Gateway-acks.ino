// Sample RFM69 sender/node sketch, with ACK, encryption and replay prevention key
// Sends periodic messages of increasing length to gateway (id=1)
// Library and original code by Felix Rusu - felix@lowpowerlab.com
// Get the RFM69 and SPIFlash library at: https://github.com/LowPowerLab/
// Session key POC by Dan Woodruff - daniel.woodruff@gmail.com

#include <RFM69.h>		//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>

#define NODEID				1		//unique for each node on same network
#define NETWORKID		 105	//the same on all nodes that talk to each other
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY		 RF69_915MHZ
#define ENCRYPTKEY		"sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
//#define IS_RFM69HW		//uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME			30 // max # of ms to wait for an ack
#define SERIAL_BAUD	 115200

#define LED					 9 // Moteinos have LEDs on D9

RFM69 radio;
bool promiscuousMode = false; //set to 'true' to sniff all packets on the same network

int connectionTimeout = 500;

void setup() {
	// if analog input pin 0 is unconnected, random analog
	// noise will cause the call to randomSeed() to generate
	// different seed numbers each time the sketch runs.
	// randomSeed() will then shuffle the random function.
	randomSeed(analogRead(0));
	
	Serial.begin(SERIAL_BAUD);
	delay(10);
	radio.initialize(FREQUENCY,NODEID,NETWORKID);
	#ifdef IS_RFM69HW
		radio.setHighPower(); //only for RFM69HW!
	#endif
	radio.encrypt(ENCRYPTKEY);
	radio.promiscuous(promiscuousMode);
	char buff[50];
	sprintf(buff, "\nListening at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
	Serial.println(buff);
}
void loop() {
	//process any serial input
	if (Serial.available() > 0)
	{
		char input = Serial.read();
		if (input == 't')
		{
			byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
			byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
			Serial.print( "Radio Temp is ");
			Serial.print(temperature);
			Serial.print("C, ");
			Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
			Serial.println('F');
		}
	}
	if (radio.receiveDone())
	{
		// store the stuff needed to process before sending the ACK
		byte sender = radio.SENDERID;
		byte data[radio.DATALEN];
		for (byte i = 0; i < radio.DATALEN; i++) 
		{
			data[i] = radio.DATA[i];
		}
		if (radio.ACKRequested()) radio.sendACK();
		// check to see if we received a connection request
		if (strcmp((const char*)data,"CNX") == 0) {
			// if yes, save the sender and generate random connection key
			byte connectionNode = sender;
			byte connectionKey = random(256);
			// send it to the node
			if (!radio.sendWithRetry(connectionNode, &connectionKey, 8)) { return; }
			Blink(LED, 3);
			long start = millis();
			// wait for response to connection until timeout
			while ( millis() - start < connectionTimeout )
			{
				// if we've received a response
				if (radio.receiveDone()) 
				{
					// store the stuff needed to process before sending the ACK
					sender = radio.SENDERID;
					byte data[radio.DATALEN];
					byte datalen = radio.DATALEN;
					for (byte i = 0; i < datalen; i++) 
					{
						data[i] = radio.DATA[i];
					}
					// send ACK and continue only if key checks out
					if (radio.ACKRequested() && sender == connectionNode && data[0] == connectionKey)
					{
						radio.sendACK();
						Serial.print("connectionNode:");
						Serial.print(connectionNode);
						Serial.print(" sender:");
						Serial.print(sender, DEC);
						Serial.print(" connectionKey:");
						Serial.print(connectionKey);
						Serial.print(" datalen:");
						Serial.print(datalen);
						Serial.print(" data[0]:");
						Serial.print(data[0]);
						Serial.print(" data:");
						// print data, starting at 1 to not show the key
						for (byte i = 1; i < datalen; i++) 
						{
							Serial.print((char)data[i]);
						}
						Serial.println();
						// break out of the timeout loop if we've actually gotten this far (success!)
						break;
					}
					else Serial.println("key doesn't match");
				}
			}
		}
	} 
}

void Blink(byte PIN, int DELAY_MS)
{
	pinMode(PIN, OUTPUT);
	digitalWrite(PIN,HIGH);
	delay(DELAY_MS);
	digitalWrite(PIN,LOW);
}