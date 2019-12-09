//**************************************************************************************
/** \file task_user.cpp
 *    This file contains source code for a user interface task for a ME405/FreeRTOS
 *    test suite. 
 *
 *  Revisions:
 *    \li 09-30-2012 JRR Original file was a one-file demonstration with two tasks
 *    \li 10-05-2012 JRR Split into multiple files, one for each task
 *    \li 10-25-2012 JRR Changed to a more fully C++ version with class task_user
 *    \li 11-04-2012 JRR Modified from the data acquisition example to the test suite
 *    \li 11-04-2012 HVH Modified for the bowling bot 
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
#include "task_user.h"                      // Header for this file


/** This constant sets how many RTOS ticks the task delays if the user's not talking.
 *  The duration is calculated to be about 5 ms.
 */
const portTickType ticks_to_delay = ((configTICK_RATE_HZ / 1000) * 5);


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

task_user::task_user (const char* a_name, 
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
/** This task interacts with the user for force him/her to do what he/she is told. It
 *  is just following the modern government model of "This is the land of the free...
 *  free to do exactly what you're told." 
 */


// Create front_steer share
shared_data<uint8_t> steer_front;
// Create back_steer share
shared_data<uint8_t> steer_back;

void task_user::run (void)
{
	char char_in;                           // Character read from serial device
	time_stamp a_time;                      // Holds the time so it can be displayed

	// Tell the user how to get into motor control (state 2), where the user interface
	// drives front and back motors
	*p_serial << PMS ("Press E for command mode") << endl;

	// This is an infinite loop; it runs until the power is turned off. There is one 
	// such loop inside the code for each task
	for (;;)
	{
		//*p_serial << steer_front.get() << endl;
		//*p_serial << steer_back.get() << endl;
		// Run the finite state machine. The variable 'state' is kept by the parent class
		switch (state)
		{
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// In state 0, we transparently relay characters from the radio to the USB 
			// serial port and vice versa but watch for certain control characters
			case (0):
				if (p_serial->check_for_char ())        // If the user typed a
				{                                       // character, read
					char_in = p_serial->getchar ();     // the character

					// In this switch statement, we respond to different characters
					switch (char_in)
					{
						// Control-C means reset the AVR computer
						case (3):
							*p_serial << PMS ("Resetting AVR") << endl;
							wdt_enable (WDTO_120MS);
							for (;;);
							break;

						// Control-A puts this task in command mode
						case ('e'):
							*p_serial << PMS ("MOTOR CONTROL") << endl;
							transition_to (1);
							break;

						// Any other character will be ignored
						default:
							break;
					};
				}

				break; // End of state 0

			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// In state 1, we're in motor control mode, so when the user types characters, the
			// characters are interpreted as commands to do something
			case (1):
				steer_front.put(0);
				steer_back.put(0);
				if (p_serial->check_for_char ())				// If the user typed a
				{											// character, read
					char_in = p_serial->getchar ();			// the character

					// In this switch statement, we respond to different characters as
					// commands typed in by the user
					switch (char_in)
					{
						// The 's' command moves us to back motor
						case ('s'):
							*p_serial << "Moving back motor" << endl;
							transition_to(2);
							break;

						// The 'w' command moves us to front motor
						case ('w'):
							*p_serial << "Moving front motor" << endl;
							transition_to(3);
							break;

						// The 'q' key goes back to main
						case (27):
						case ('q'):
							*p_serial << PMS ("Exit command mode") << endl;
							transition_to (0);
							break;

						// If the character isn't recognized, ask: What's That Function?
						default:
							p_serial->putchar (char_in);
							*p_serial << PMS (":WTF?") << endl;
							break;
					}; // End switch for characters
				} // End if a character was received

				
				
				break; // End of state 1
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// In state 2, we're controlling back motor
			case (2):
				if (p_serial->check_for_char ())				// If the user typed a
					{											// character, read
						char_in = p_serial->getchar ();			// the character

						// In this switch statement, we respond to different characters as
						// commands typed in by the user
						switch (char_in)
						{
							// The 'q' command moves us back to motor control
							case ('q'):
								*p_serial << "Back to motor selector" << endl;
								transition_to(1);
								break;

							// The 'w' command moves us to front motor
							case ('w'):
								*p_serial << "Moving front motor" << endl;
								transition_to(3);
								break;

							// The 'a' key tells motor task to steer to port
							case ('a'):
								*p_serial << PMS ("Steering to port") << endl;
								steer_back.put(1);
								break;
								
							// The 'd' key tells motor task to steer to port
							case ('d'):
								*p_serial << PMS ("Steering to starboard") << endl;
								steer_back.put(2);
								break;
							
							default:
								steer_back.put(0);
								break;
						}; // End switch for characters
					} // End if a character was received
			break; // End of state 2
			
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// In state 3, we're controlling front motor
			case (3):
				if (p_serial->check_for_char ())				// If the user typed a
					{											// character, read
						char_in = p_serial->getchar ();			// the character

						// In this switch statement, we respond to different characters as
						// commands typed in by the user
						switch (char_in)
						{
							// The 'q' command moves us back to motor control
							case ('q'):
								*p_serial << "Back to motor selector" << endl;
								transition_to(1);
								break;

							// The 's' command moves us to back motor
							case ('s'):
								*p_serial << "Moving back motor" << endl;
								transition_to(2);
								break;

							// The 'a' key tells motor task to steer to port
							case ('a'):
								*p_serial << PMS ("Steering to port") << endl;
								steer_front.put(1);
								break;
		
							// The 'd' key tells motor task to steer to port
							case ('d'):
								*p_serial << PMS ("Steering to starboard") << endl;
								steer_front.put(2);
								break;
							
							default:
								steer_front.put(0);
								break;
		
						}; // End switch for characters
					} // End if a character was received
			
			break; // End of state 3	
			
			// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			// We should never get to the default state. If we do, complain and restart
			default:
				*p_serial << PMS ("Illegal state! Resetting AVR") << endl;
				wdt_enable (WDTO_120MS);
				for (;;);
				break;

		} // End switch state

		runs++;                             // Increment counter for debugging

		// No matter the state, wait for approximately a millisecond before we 
		// run the loop again. This gives lower priority tasks a chance to run
		vTaskDelay (configMS_TO_TICKS (1));
	}
}
