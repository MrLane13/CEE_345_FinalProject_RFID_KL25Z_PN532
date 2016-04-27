/*********************************************
* Author: Lane Mills
*	Date  : 04/24/16
* 
*
*
*
*/

#include "delay.h"
#include "mbed.h"

// Empty constructor
delay::delay() {
  
}

// public function
void delay::delayTime(int inMilliSeconds) {
		wait(1.0 * inMilliSeconds / 1000);
}
