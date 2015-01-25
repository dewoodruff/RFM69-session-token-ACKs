// Sample RFM69 sender/node sketch, with ACK, encryption and replay prevention key
// Sends periodic messages of increasing length to gateway (id=1)
// Library and original code by Felix Rusu - felix@lowpowerlab.com
// Get the RFM69 and SPIFlash library at: https://github.com/LowPowerLab/
// Session key POC by Dan Woodruff - daniel.woodruff@gmail.com

#include <RFM69.h>		//get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPI.h>

#define NODEID				2		//unique for each node on same network
#define NETWORKID		 105	//the same on all nodes that talk to each other
#define GATEWAYID		 1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY		 RF69_915MHZ
#define ENCRYPTKEY		"sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
//#define IS_RFM69HW		//uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME			30 // max # of ms to wait for an ack
#define LED					 9 // Moteinos have LEDs on D9

#define SERIAL_BAUD	 115200

int TRANSMITPERIOD = 200; //transmit a packet to gateway so often (in ms)
char payload[] = "123 ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char buff[20];
byte sendSize=0;
RFM69 radio;

int connectionTimeout = 500;

void setup() {
	Serial.begin(SERIAL_BAUD);
	radio.initialize(FREQUENCY,NODEID,NETWORKID);
	#ifdef IS_RFM69HW
		radio.setHighPower(); //uncomment only for RFM69HW!
	#endif
	radio.encrypt(ENCRYPTKEY);
	char buff[50];
	sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
	Serial.println(buff);
}

long lastPeriod = 0;
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
	int currPeriod = millis()/TRANSMITPERIOD;
	if (currPeriod != lastPeriod)
	{
		lastPeriod=currPeriod;
		// connect. process only if connection successful
		Serial.print("connecting...");
		if (radio.sendWithRetry(GATEWAYID, "CNX", 3)){
			Blink(LED,3);
			// wait for a response until the defined timeout
			long start = millis();
			while (millis() - start < connectionTimeout) 
			{
				if (radio.receiveDone())
				{
					// store key before sending ACK
					byte key = radio.DATA[0];
					if (radio.ACKRequested()) radio.sendACK();
					Serial.print("key:");
					Serial.print(key);
					Serial.print(" Sending[");
					Serial.print(sendSize);
					Serial.print("]: ");
					
					char buffer[50];
					buffer[0] = key;
					for(byte i = 0; i < sendSize; i++){
						Serial.print((char)payload[i]);
						// i+1 to account for the key already in buffer
						buffer[i+1] = payload[i];
					}
					if (radio.sendWithRetry(GATEWAYID, buffer, sendSize+1))
						Serial.print(" ok!");
					else Serial.print(" nothing...");
					Blink(LED,3);
					sendSize = (sendSize + 1) % 31;
					// break out of the timeout loop if we've actually gotten this far (success!)
					break;
				}
			}
		}
		Serial.println();
	}
}

void Blink(byte PIN, int DELAY_MS)
{
	pinMode(PIN, OUTPUT);
	digitalWrite(PIN,HIGH);
	delay(DELAY_MS);
	digitalWrite(PIN,LOW);
}