#ifndef GLOBAL_H
	//header envroiment variabile, is used to detect multiple inclusion
	//of the same header, and can be used in the c file to detect the
	//included library
	#define GLOBAL_H

	/****************************************************************************
	**	ENVROIMENT VARIABILE
	****************************************************************************/

	/****************************************************************************
	**	GLOBAL INCLUDE
	**	TIPS: you can put here the library common to all source file
	****************************************************************************/

	//type definition using the bit width and signedness
	#include <stdint.h>
	//define the ISR routune, ISR vector, and the sei() cli() function
	#include <avr/interrupt.h>
	//name all the register and bit
	#include <avr/io.h>
	//hard delay
	#include <util/delay.h>
	//
	//#include <avr/pgmspace.h>
	//General porpouse macros
	#include "at_utils.h"
	//Port configuration macros specific to AtMega Microcontrollers
	#include "at_mega_port.h"
	//LCD device functions and init
	#include "at_lcd.h"
	//trigonometric functions
	#include "at_trig8.h"

	/****************************************************************************
	**	DEFINE
	****************************************************************************/

		//-----------------------------------------------------------------------
		//	SERVO CONSTANTS
		//-----------------------------------------------------------------------
	
	//maximum number of servos
	//#define MAX_SERVOS	8
	//
	#define SERVO_PLANNER_PRE	2
	//Total number of servos
	#define N_SERVOS			6
	//Special code to disable a servo
	#define SERVO_OFF			-128
	//Servo maximum angular velocity [Units/Ticks], at 5V i have 190mS per 60°, 180° are 256 Units, 1 tick is 20mS. Every Tick i can move ~ Units
	#define SERVO_MAX_SPEED		9
	//For math reason i need the inverse multiplied by the scale of Kalfa, which is 252/9. It means that with Kalfa = 252 i use 100% of the maximum alngulare velocity during the walk
	#define SERVO_MAX_FREQ		28
	//Neutral OCR
	#define	K0					30000
	//Linear OCR (40°-400uS-63) (90°-900uS-141.7)
	#define K1					142
	//Half of minimum call time
	//#define OCR_MIN_2	150

		//-----------------------------------------------------------------------
		//	MOTOR ALIASES
		//-----------------------------------------------------------------------
	
	//enum servos names (positions and functions in the robot)
	enum
	{
		SERVO_FSX		= 0,	//Hip Front SX 			(+ => Leg rear -> front)
		SERVO_FDX		= 1,	//Hip Front DX 			(+ => Leg front -> Rear)
		SERVO_RSX		= 3,	//Hip Rear SX			(+ => Leg rear -> front)
		SERVO_RDX		= 2,	//Hip Rear DX			(+ => Leg front -> Rear)
		SERVO_FC		= 4,	//Hip Front Rotation	(+ => rise leg dx, lower leg sx)
		SERVO_RC		= 5		//Hip Rear Rotation		(+ => lower leg dx, rise leg sx)
	};

		//-----------------------------------------------------------------------
		//	MOTOR CORRECTIONS
		//-----------------------------------------------------------------------

	//servo direction correction mask, allow for the logical direction of a joint to be reversed in sign
	//each bit is associated with a servo, if the bit is '1', the direction is reversed
	#define SERVO_DIR	(MASK(1) | MASK(2) | MASK(5))

		//-----------------------------------------------------------------------
		//	PHYSICAL CONSTANTS OF THE ROBOT
		//-----------------------------------------------------------------------
		//	Those physical dimension in mm are used to calculate the trajectories
		//	DEFINITIONS:
		//	X:		Rear to Front dimension, positive toward head
		//	Y:		Side to Side dimension, positive toward Dx
		//	Z: 		Bottom to Top dimension, positive toward Top 
		//	Hip: 	Leg Joing

	//Length of the Spine in X [mm] from Rear Hip to Front Hip, length of the robot
	#define SIZE_SX				170
	//Distance from Spine to Hip in Y, half of the shoulder to shoulder length
	#define SIZE_HY				50
	//Distance from Hip to base of the Boot in Z, length of a Leg
	#define SIZE_LZ				130

	/****************************************************************************
	**	MACRO
	****************************************************************************/

		//-----------------------------------------------------------------------
		//	LCD MACROS
		//-----------------------------------------------------------------------

	#define LCD_ON()	\
		CLEAR_BIT( PORTC, PC1 )

	#define LCD_OFF()	\
		SET_BIT( PORTC, PC1 )

		//-----------------------------------------------------------------------
		//	TIMER 1 MACROS
		//-----------------------------------------------------------------------
		//	Timer 1 is used to wait precises delay to pull down the servo lines
		//	I have to start it up, stop it, clear the content and setup the delay

	//connect the clock at N=1
	#define START_TIMER1()	\
		SET_BIT( TCCR1B, CS10 )

	//disconnect the clock (if N==1)
	#define STOP_TIMER1()	\
		CLEAR_BIT( TCCR1B, CS10 )

		//-----------------------------------------------------------------------
		//	SERVOS MACROS
		//-----------------------------------------------------------------------
		//	Those macros are wrappers to access servos variabiles

		//access servo pos as matrix
	#define SERVO_POS( col, index )	\
		servo_pos[ N_SERVOS*(col) + (index) ]

	//access servo pos as matrix
	#define SERVO_TIME( col, index )	\
		servo_time[ N_SERVOS*(col) + (index) ]

	//offset
	#define SERVO_OFFSET( index )	\
		servo_off[ (index) ]

	/****************************************************************************
	**	TYPEDEF
	****************************************************************************/
	
	//Global flags raised by ISR functions
	typedef struct _Isr_flags Isr_flags;

	/****************************************************************************
	**	STRUCTURE
	****************************************************************************/

	//Global flags raised by ISR functions
	struct _Isr_flags
	{
		//First byte
		U8 mot			: 1;	//start the motor scan
		U8 pos			: 1;	//motor pos update
		U8 lcd			: 1;	//lcd update flag
		U8 x			: 1;	//test flag
		U8 safe			: 1;	//it is safe to write in the global servo vars
		U8 				: 3;	//unused bits
	};

	/****************************************************************************
	**	PROTOTYPE: INITIALISATION
	****************************************************************************/

	//port configuration and call the pheriperals initialisation
	extern void global_init( void );
	//System tick
	extern void timer0_init( void );
	//servo lines handler
	extern void timer1_init( void );

	/****************************************************************************
	**	PROTOTYPE: FUNCTION
	****************************************************************************/

		//-----------------------------------------------------------------------
		//
		//-----------------------------------------------------------------------

	//very complex function, it is called by the ISR to calculate the delay
	//delay is calculated from global time, initial and ending time and position of the servo
	extern U16 servo_calc_delay( U8 index );

	//The meat of the trajectory generation, it's called when the buffer of a servo is emptied
	extern U8 servo_motion_planner( U8 index );

	/****************************************************************************
	**	PROTOTYPE: GLOBAL VARIABILE
	****************************************************************************/

		//-----------------------------------------------------------------------
		//	STATUS VARS
		//-----------------------------------------------------------------------

	//Those are the flags updated by ISRs
	extern volatile Isr_flags f;	
	//Status variabile for the servos, keep track of which servo to do next
	volatile extern U8 servo_cnt;
	//each servo has 1 bit, it's rised when the interpolator is done
	extern volatile U8 servo_idle;

		//-----------------------------------------------------------------------
		//	SERVO VARS
		//-----------------------------------------------------------------------

	//servo direction correction mask, allow for the logical direction of a joint to be reversed in sign
	//each bit is associated with a servo, if the bit is '1', the direction is reversed
	extern U8 servo_dir;
	//Servo offset for true 0 position
	extern S8 servo_off[ N_SERVOS ];
	//Servo motion planner time base (1 = 79.87mS)
	extern U8 servo_global_time;
	//positions for the interpolator (-127,+127) -128=servo idle
	extern S8 servo_pos[ 2 * N_SERVOS ];
	//first column is starting time, second column is ending time
	extern U8 servo_time[ 2 * N_SERVOS ];
	//Second layer of vars, store next motion planned
	extern S8 servo_next_pos[N_SERVOS];
	extern U8 servo_next_time[N_SERVOS];


#else
	#warning "multiple inclusion of the header file global.h"
#endif
