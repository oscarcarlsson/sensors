#include <JeeLib.h>
#include <avr/wdt.h>
#include "ocLib.h"

enum { TASK_PREP_TX, TASK_TX, TASK_RFM12B, TASK_LIMIT };
Scheduler scheduler (TASK_LIMIT);

PulseMeter pm[4] = {PulseMeter(4), PulseMeter(5), PulseMeter(6), PulseMeter(7)};

// Max 66 bytes (33 ints)
typedef struct {
	unsigned int power;
	unsigned int pulses;
} PayloadItem;
PayloadItem payload[4];

void setup() {
	Serial.begin(57600);

	wdt_enable(WDTO_8S);
	
	rf12_initialize(5, RF12_868MHZ, 210);
	
	lowerDataRate();

	//Start TX after 10s
	scheduler.timer(TASK_PREP_TX, 100);
	scheduler.timer(TASK_RFM12B, 1);
}

void loop() {
	//Reset watchdog
	wdt_reset();

	pm[0].check();
	pm[1].check();
	pm[2].check();
	pm[3].check();

	switch (scheduler.poll()) {

		case TASK_PREP_TX:
			for (int i = 0; i < 4; i++) {
				payload[i].pulses = pm[i].pulses;
				payload[i].power = 0;
				if (pm[i].power.getStatus() == pm[i].power.OK)
					pm[i].power.getMedian(payload[i].power);
			}
			scheduler.timer(TASK_TX, 0);
			break;
	
		case TASK_TX:
			if (rf12_canSend()) {
				rf12_sendStart(0, &payload, sizeof(payload), 2);
				scheduler.timer(TASK_PREP_TX, 37); //Primes  31, 37, 41, 43, 47
			} else {
				//Retry in 100ms
				scheduler.timer(TASK_TX, 1);
			}
			break;

		case TASK_RFM12B:
			rf12_recvDone();
			scheduler.timer(TASK_RFM12B, 1);
			break;
	}	
	
}

void lowerDataRate() {
	rf12_control(0xC623); //Data Rate 9.579kbps
	rf12_control(0x94C2); //RX Bandwidth 67kHz
	rf12_control(0x9820); //Deviation 45kHz 
}
