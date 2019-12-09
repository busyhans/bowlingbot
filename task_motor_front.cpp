//**************************************************************************************
/** \file task_motor_front.cpp
 *    This file contains source code for a setting up and powering a motor in the
 *	  front of a bowling ramp robot.
 *
 *  Revisions:
 *    \li 12-3-2019 HVH Adapted from HVH task_PWM.cpp
 *
 *  License:
 *    This file is copyright 2019 by H Hershberger and released under the GNU
 *    Public License, version 2. It intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *	  (TLDR):  THIS CODE MIGHT SUCK AND YOU'RE ON YOUR OWN  */
//**************************************************************************************
#include <avr/io.h>                         // Port I/O for SFR's
#include <avr/wdt.h>                        // Watchdog timer header

#include "shared_data_sender.h"
#include "shared_data_receiver.h"
#include "task_motor_front.h"                      // Header for this file


//-------------------------------------------------------------------------------------
/** This constructor creates a new data acquisition task. Its main job is to call the
 *  parent class's constructor which does most of the work.
 *  @param a_name A character string which will be the name of this task
 *  @param a_priority The priority at which this task will initially run (default: 0)
 *  @param a_stack_size The size of this task's stack in bytes
 *                      (default: configMINIMAL_STACK_SIZE)
 *  @param p_ser_dev Pointer to a serial device (port, radio, SD card, etc.) which can
 *                   be used by this task to communicate (default: NULL)
 */

task_motor_front::task_motor_front (const char* a_name,
					  unsigned portBASE_TYPE a_priority,
					  size_t a_stack_size,
					  emstream* p_ser_dev
					 )
	: frt_task (a_name, a_priority, a_stack_size, p_ser_dev)
{
	// Nothing is done in the body of this constructor. All the work is done in the
	// call to the frt_task constructor on the line just above this one
}


//-------------------------------------------------------------------------------------
/** This task powers a motor at the front of a bowling robot
 */

void task_motor_front::run (void)
{
	// Make a variable which will hold times to use for precise task scheduling
	portTickType previousTicks = xTaskGetTickCount ();

	// Wait a little while for user interface task to finish up
	delay_ms(10);

	while(1)
	{
		switch (state)
		{
		case INIT:
			// Set up Front Motor Pins
			PORTD.OUTCLR = PIN0_bm | PIN1_bm;				// Make sure the pin is off before configuring it as output
			PORTD.DIRSET = PIN0_bm | PIN1_bm;				// Set the pin as an output
			PORTD.OUTSET = PIN0_bm | PIN1_bm;				// Turn the pin on again
			// Set up front motor timer
			TCD0_CTRLB = TC_WGMODE_SS_gc | TC0_CCAEN_bm | TC0_CCBEN_bm;	// single slope, compare to B and A
			TCD0_PER = 1600;					// Set period to 1600
			TCD0_CCABUF = 0;					// Set pwm 1 off
			TCD0_CCBBUF = 0;					// set pwm 2 off
			TCD0_CTRLD = 0;						// All event stuff off
			TCD0_CTRLC = 0;						// timer counter is always on
			TCD0_CTRLA |= TC_CLKSEL_DIV1_gc;		// Prescaler is just clock frequency
			// Enable motor
			PORTB.OUTCLR = PIN2_bm;				// THE LITTLE EXTRA PIN (SHOULD BE B2)
			PORTB.DIRSET = PIN2_bm;				// set pin high
			PORTB.OUTSET = PIN2_bm;				// turn pin on

			transition_to(MOTOR_STOPPED);				// Go to checking for pwm off state
			break;

		case MOTOR_STOPPED:
			TCD0_CCABUF = 0;						// Set port PWM OFF
			TCD0_CCBBUF = 0;						// Set starboard PWM (base 150)
			
			if(steer_front.get() == 1)
			{
				transition_to(MOTOR_PORT);
			}
			
			else if(steer_front.get() == 2)
			{
				transition_to(MOTOR_STARBOARD);
			}
			
			break;
			
		case MOTOR_PORT:
			TCD0_CCABUF = 300;						// Set motor duty cycle
			if(steer_front.get() == 0)
			{
				transition_to(MOTOR_STOPPED);								// Saturate duty cycle
			}
			
			break;
			
		case MOTOR_STARBOARD:
			TCD0_CCBBUF = 300;						// Set motor duty cycle
			if(steer_front.get() == 0)
			{
				transition_to(MOTOR_STOPPED);								// Saturate duty cycle
			}
			break;

		default:
			break;
		}
		runs++;
		delay_from_to_ms(previousTicks,10);
	}
}
