/*********************************************
* Author: Lane Mills
*	Date  : 04/26/16
* 
*
*
*
*/

#include <stdint.h>

class blinkLED {
	public:
		blinkLED();
		void blink(int r, int g, int b, int delayInMs);
		void doubleBlink(int r, int g, int b, int delayInMs);
		void tripleBlink(int r, int g, int b, int delayInMs);
	
	private:
		void stopBlink();
		void blinkRed(int delayInMs);
		void blinkBlue(int delayInMs);
		void blinkGreen(int delayInMs);
	
};