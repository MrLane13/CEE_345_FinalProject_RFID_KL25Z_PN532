/*********************************************
* Author: Lane Mills
*	Date  : 04/26/16
* 
*
*
*
*/

#include "blinkLED.h"
#include "delay.h"
#include "Adafruit_PN532.h"

DigitalOut red(LED_RED);
DigitalOut green(LED_GREEN);
DigitalOut blue(LED_BLUE);
delay d;

// Empty constructor
blinkLED::blinkLED() {
	
}
	
// Single blink. Takes in r,g,b value of either 0 (on) or 1 (off) as well as a delay in milliseconds
void blinkLED::blink (int r, int g, int b, int delayInMs) {
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
}
// Double blink. Takes in r,g,b value of either 0 (on) or 1 (off) as well as a delay in milliseconds
void blinkLED::doubleBlink (int r, int g, int b, int delayInMs) {
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
	
}
// Triple blink. Takes in r,g,b value of either 0 (on) or 1 (off) as well as a delay in milliseconds
void blinkLED::tripleBlink (int r, int g, int b, int delayInMs) {
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
	red=r;green=g;blue=b;
	d.delayTime(delayInMs);
	stopBlink();
}
// Sets all LEDs to (off)
void blinkLED::stopBlink() {
	red=1;green=1;blue=1;
	d.delayTime(100);
}


