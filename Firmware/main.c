/****************************************************************************
**	ROBOTICA@POLITO PROJECT
*****************************************************************************
**	Unit Zero Zero
*****************************************************************************
**	Author: 			Orso Eric
**	Creation Date:
**	Last Edit Date:
**	Revision:			1
**	Version:			0.1 ALPHA
****************************************************************************/

/****************************************************************************
**	HYSTORY VERSION
*****************************************************************************
**	R1 V0.1ALPHA
**		>I wrote a cool function that allow to update servo in one function,
**		i think it's best to update them in sequence to avoid artifact
**		>i rewrote the servo scan function, now it scan the servo in order
**		i eliminated the artifact and greatly reduced var usage and time
**		>added the interpolator function, now servo position is extrapolated
**		from time and points in space and time, offset is accounted for during calculations
**		>global time is now same advancement as servo period
**		~20mS tick, 5.12S longest single movement length
**		>work like a charm
**		>I added the LCD and the trigonometric function library
**		>i moved timer 0 to 156.0uS to update the display and mot is now 156.8*128= 20.07mS
**		>I added progmem capability to the LUT of the trig library to save data memory
**		>tested at_sin8 and at_cos8 by showing them to the display, they seems to work fine
**		>added safe and servo_idle status flags
**		>now the interpolator copy Xe,Te in Xs, Ts when motion end, it setup for next motion and stop the servo
**	V0.2 ALPHA
**		>I vastly improved upon the servo driver. The version I did before
**		is not that user friendly, the user have to meddle with thing to do things.
**		What the user want is to set a position and a speed, and have a flag when 
**		pos is reached. My new driver does just that.
**		The user see three sets of var
**			CONTROL
**			>servo_pos
**			>servo_speed
**			FEEDBACK
**			>servo_lock
**			CONFIG
**			>servo_off
**			>SERVO_MAX_SPEED
**		>Porting to LCD library V3. Here the driver sync the display with a
**		memory. The user can make 1M printchar a second and the driver still
**		won't bug out. Muchbetter abstraction and lower overhead compared
**		to command fifo approach.
**		>Added DPRINTEF macros to at_utils. used in codeblock.
****************************************************************************/

/****************************************************************************
**	DESCRIPTION
*****************************************************************************
**	This is the control progam for the 5 servos lego 4 leg robots
*****************************************************************************
**	CALCULATION OF TIMER 1 OCR FOR PULSE WIDTH
**	Constant for the conversion from angle (S8) to OCR (U16)
**	x	= [I8] input angle, i use from -127 to +127, -128 is the disable special code
**	Tn 	= [s] (typ 1.5mS) neutral time of the servo, pulse width required for 0�
**	Td	= [s] (typ 0.9mS) maximum skew of the pulse width, from minimum angle (Tn-Td) to maximum angle (Tn+Td)
**	Tt	= [s] (typ 50nS) N/Fclk width of a single stepof the timer, required to calculate the OCR
**	N	= [1] timer prescaler, 1 maximum precision OCR bigger (might require 16bit), 2^n OCR smaller lower precision require less bits
**	K0	= [1] Tn / Tt neutral value of OCR
**	K1	= [1] Td / Tt / 127 angle constant, multiply angle to obtain OCR, 127 is the maximum value of the input
**	OCR	= K0 + K1 * x, calculation of OCR
**	Example
**	Fclk = 20e6, N = 1, Tn = 1.5mS, Td = 0.9mS, x = [-127,+127]
**	K0 = 30e3, K1 = 142 (141.7), >>>OCR = 30e3 + x*142<<<
**	SERVO OFFSET:
**	This is the correction for the 0 position, it is added during the caluclation
**	and does not eat into the servo position range
**	JOINTS:
**	Index		|	Joint		|	Increase position move toward what?
**	-------------------------------------------------------------------
**	0			|	front DX	|	front
**	1			|	front SX	|	back
**	2			|	rear SX		|	back
**	3			|	rear DX		|	front
**	4			|	front hip	|	rise 0, lower 1
**	5			|	rear hip	|	rise 3, lower 2
*****************************************************************************
**	INTERPOLATOR
**	Above the timing ISR for the servo there is the interpolator that generate the positions
**	The user write the wished positions and times in the following global vars
**		Xs	Ts	Xe	Te
**	0
**	...
**	5
**	The time is in tick unit (about 80mS), example:
**	servo_global_time = 45 		//i'm in the 45� tick of the cycle
**	SERVO_POS( 0 , 3 ) = -50	//i start the new motion from current time and in position -50 (50/127*90�=-35.4�)
**	SERVO_TIME( 0 , 3 ) = 45
**	SERVO_POS( 1 , 3 ) = -30	//i end the new motion in 480mS and in position -30 (50/127*90�=-21.3�)
**	SERVO_TIME( 1 , 3 ) = 45+6
**	The interpolator is a powerful function, allow for much easier high level motion generation
**	since it handle all the complexity of sub angle calculation and the timing routine
**	handle all the complexity of the time generations
****************************************************************************/

/****************************************************************************
**	USED PIN
**	TIPS: compile this field before writing any code: it really helps!
*****************************************************************************
**	PB0			|	front DX
**	PB1			|	front SX
**	PB2			|	rear DX
**	PB3			|	rear SX
**	PB4			|	front hip
**	PB5			|	rear hip
**	PB6			| 	core joint
**	PB7			|	head
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	USED PHERIPERALS
**	TIPS: compile this field before writing any code: it really helps!
*****************************************************************************
**	HS-311 Servo
**	Voltage		|	5.0
**	Current		|	7m (idle)		160m (free)		700m (stall)
**	Torque		|	3 [Kg*cm]		294 [mNm]
**	Angle		|
**	Control		|	1.5mS (center)	+/-900uS (90�)	~20mS (period)
**				|	HZ -> no force
**				|	positive DT -> clockwise direction
*****************************************************************************

**
****************************************************************************/

/****************************************************************************
**	KNOWN BUG
*****************************************************************************
**	>
****************************************************************************/

/****************************************************************************
**	TODO
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	ENVROIMENT VARIABILE
****************************************************************************/

/****************************************************************************
**	INCLUDE
**	TIPS: ".h" should not include other ".h", it lower the leggibility of the code
**	TIPS: ".h" must not contain anything that generate code, write only declaration or prototype
**	this help the leggibility and the debug phase
**	TIPS: type from the stdint.h have a well defined width and signedness, use them
**	( uint8_t = unsigned 8 bit, int8_t = signed 8 bit, uint32_t = unsiged 32 bit, ecc... )
****************************************************************************/

#include "global.h"

/****************************************************************************
**	DEFINE
****************************************************************************/

//define for the mail loop
#define EVER (;;)

/****************************************************************************
**	MACRO
****************************************************************************/

/****************************************************************************
**	STRUCTURE
****************************************************************************/

/****************************************************************************
**	PROTOTYPE: FUNCTION
**	TIPS: use "extern" in function prototype, it's not necessary, but any other
**	prototype need it, it help the leggibility of the code
****************************************************************************/

/****************************************************************************
**	PROTOTYPE: GLOBAL VARIABILE
****************************************************************************/

/****************************************************************************
**	GLOBAL VARIABILE:
**	TIPS: "const" variabile will be loaded in the flash memory, saving the ram,
**	use it for string that will not be modified
**	TIPS: if you want a ISR to manipulate a global variabile, than that variabile
**	**must** be declared "volatile" so that the c compiler will not wipe out the
**	variabile by optimising the code, use that variabile as less as possibile
**	because it will not be optimised
**	TIPS: "volatile int" variabile may give problem, don't use it (uP is 8 bits, while
**	int is 16 bits, it's implemented by concatenating 2 byte, the volatile statement
**	disable the optimisation on that variabile, and mess up that code)
****************************************************************************/

	//-----------------------------------------------------------------------
	//	STATUS VARS
	//-----------------------------------------------------------------------

//Those are the flags updated by ISRs
volatile Isr_flags f;
//Status variabile for the servos, keep track of which servo to do next
volatile U8 servo_cnt	= N_SERVOS;

	//-----------------------------------------------------------------------
	//	SERVOS VARS
	//-----------------------------------------------------------------------

//One flag per servo. '1' if the servo is keeping up with target_pos
U8 servo_lock			= 0x00;
//Current position of the servos. Used by the driver, user should not write here
U16 servo_delay[ N_SERVOS ];
//Target position. The user write here, the driver will do it's best to reach it
S8 servo_target_pos[ N_SERVOS ];
//The servo will rotate at this speed [unit/second]
U8 servo_target_speed[ N_SERVOS ];
//servo position offset for true 0 position
//offsets are accounted for in separately from the position, it does not eat into the dynamic
S8 servo_off[ N_SERVOS ];

	//-----------------------------------------------------------------------
	//	TRAJECTORY VARS
	//-----------------------------------------------------------------------

//Timebase for trajectory generation. 1 time unit = 1/50Hz.
U16 servo_global_time 	= 0;
//Current motion plan
Trajectories trajectory = MOVE_IDLE;



/****************************************************************************
**	MAIN
****************************************************************************/

int main( void )
{
	///**********************************************************************
	///	LOCAL VARIABILE
	///	TIPS: main local variabile last the entiere program, but unlike the global variabile,
	///	they are not visible from other function, the c compiler can better optimize them
	///**********************************************************************

	//temp vars
	U8 u8t;
	U16 u16t;
	S8 s8t;

	U8 pre_traj = 0;

	U8 status_traj = 0;

	///**********************************************************************
	///	VARIABILES INITIALISATION
	///**********************************************************************
	
	//Correct mechanical offsets of the servos
	servo_off[ SERVO_FDX ] 		= -3;
	servo_off[ SERVO_FSX ] 		= +2;
	servo_off[ SERVO_RDX ] 		= -3;
	servo_off[ SERVO_RSX ] 		= +2;
	servo_off[ SERVO_FHIP ] 	= +0;
	servo_off[ SERVO_RHIP ] 	= +12;
	servo_off[ SERVO_TORSO ] 	= +0;

	//Clear global servo time
	//Initialize servo position to zero (offset is accounted for during calculations, i must not add it here)
	for (u8t = 0;u8t < N_SERVOS;u8t++)
	{
		servo_delay[u8t] 		= K0;	//Servo true position
		servo_target_pos[u8t] 	= +0;	//Servo targt position (user)
		servo_target_speed[u8t]	= 80;	//Servo target speed (default)
	}

	///**********************************************************************
	///	PHERIPERALS INITIALISATION
	///**********************************************************************

	//Initialize devices
	global_init();
	//The proud name of this unit
	lcd_print_str( LCD_POS(0,0), (U8 *)"Unit Zero");
	lcd_print_str( LCD_POS(1,0), (U8 *)"Time:");

	///**********************************************************************
	///	MAIN LOOP
	///**********************************************************************

	for EVER
	{
		//-----------------------------------------------------------------------
		//	LCD DIPSPLAY UPDATE
		//-----------------------------------------------------------------------
		//	call the lcd display driver

		//If: update the LCD display (10KHz)
		if (f.lcd_update == 1)
		{
			f.lcd_update = 0;
			//Update display
			lcd_update();
		}

		//-----------------------------------------------------------------------
		//	START MOTOR SCAN (5.8uS max all servo functions)
		//-----------------------------------------------------------------------
		//	Flag rised by [Timer 0]
		//	>activity pin (led signal uC use, oscilloscope allow to measure function times)
		//	>clear servo status var
		//	>calculate first delay, pull down first line
		//	>setup first delay, enable [Timer 1]
		//	>Timer 1 ISR will handle the update of the servos and disable it self when done

		//If: Start Servo Scan (50Hz)
		if (f.servo_scan == 1)
		{
			//clear flag
			f.servo_scan = 0;
				//Startup the servo scan
			//clear status var
			servo_cnt 	= 0;
			//calculate delay
			u16t		= servo_calc_delay( 0 );
			//Store delay on T1
			OCR1A 		= u16t;
			//start T1
			START_TIMER1();
			//pull up first line
			SET_BIT( SERVO_PORT, 0 +SERVO_PIN_OFFSET );
				//Advance global time by one tick
			servo_global_time++;
			lcd_print_u16( LCD_POS(1,6), servo_global_time );

			pre_traj = AT_TOP_INC( pre_traj, 25 );
			if (pre_traj == 0)
			{
				f.servo_traj = 1;
			}
		}	//End If: motor scan flag

		//-----------------------------------------------------------------------
		//	TRAJECTORY GENERATION
		//-----------------------------------------------------------------------

		if (f.servo_traj == 1)
		{
			f.servo_traj = 0;
			s8t = 50;
			//Upright
			if (status_traj == 0)
			{
				//Initialize servo position to zero (offset is accounted for during calculations, i must not add it here)
				for (u8t = 0;u8t < N_SERVOS;u8t++)
				{
					servo_target_pos[u8t] 	= 0;	//Servo targt position (user)	
				}	
			}
			//Hip
			else if (status_traj == 1)
			{
				servo_target_pos[ SERVO_FDX ]	= +0;
				servo_target_pos[ SERVO_FSX ]	= +0;
				servo_target_pos[ SERVO_RDX ]	= +0;
				servo_target_pos[ SERVO_RSX ]	= +0;
				servo_target_pos[ SERVO_FHIP ]	= +0;
				servo_target_pos[ SERVO_RHIP ]	= +0;
			}
			//Leg
			else if (status_traj == 2)
			{
				servo_target_pos[ SERVO_FDX ]	= +s8t;
				servo_target_pos[ SERVO_FSX ]	= +s8t;
				servo_target_pos[ SERVO_RDX ]	= +s8t;
				servo_target_pos[ SERVO_RSX ]	= +s8t;
				servo_target_pos[ SERVO_FHIP ]	= +0;
				servo_target_pos[ SERVO_RHIP ]	= +0;
			}
			//Reverse Hip
			else if (status_traj == 3)
			{
				servo_target_pos[ SERVO_FDX ]	= +0;
				servo_target_pos[ SERVO_FSX ]	= +0;
				servo_target_pos[ SERVO_RDX ]	= +0;
				servo_target_pos[ SERVO_RSX ]	= +0;
				servo_target_pos[ SERVO_FHIP ]	= +0;
				servo_target_pos[ SERVO_RHIP ]	= +0;
			}
			//reset leg
			else if (status_traj == 4)
			{
				servo_target_pos[ SERVO_FDX ]	= -s8t;
				servo_target_pos[ SERVO_FSX ]	= -s8t;
				servo_target_pos[ SERVO_RDX ]	= -s8t;
				servo_target_pos[ SERVO_RSX ]	= -s8t;
				servo_target_pos[ SERVO_FHIP ]	= +0;
				servo_target_pos[ SERVO_RHIP ]	= +0;
				status_traj = 0;
			}
			//
			else 
			{
				status_traj = 255;
			}
			status_traj++;


			/*
			//fetch global time
			u16t = servo_global_time;
			if ((u16t & 0x00ff) == 0x0000)
			{
				//Initialize servo position to zero (offset is accounted for during calculations, i must not add it here)
				for (u8t = 0;u8t < N_SERVOS;u8t++)
				{
					servo_target_pos[u8t] 	= +20;	//Servo targt position (user)	
				}
			}
			else if ((u16t & 0x00ff) == 0x0080)
			{
				//Initialize servo position to zero (offset is accounted for during calculations, i must not add it here)
				for (u8t = 0;u8t < N_SERVOS;u8t++)
				{
					servo_target_pos[u8t] 	= -20;	//Servo targt position (user)	
				}
			}
			*/
		}
	}	//end for: for EVER

	return 0;
}	//end main

/****************************************************************************
** FUNCTION:
****************************************************************************/

/****************************************************************************
**	SERVO CALC POS
*****************************************************************************
**	This function will calculate the delay that move servo[index] closer to the target position
**		SLEW RATE LIMITER: The function will only move at the speed set by the user
**		OFFSET CORRECTION:	In this function i account for mechanical misalignement
**	Formula:
**	xpos = xtarget if ABS(xtarget -xpos) < xspeed/50
**	delay[OCR] = K0 + K1*xoff + K1*xpos
****************************************************************************/

U16 servo_calc_delay( U8 index )
{
	///--------------------------------------------------------------------------
	///	STATIC VARIABILE
	///--------------------------------------------------------------------------

	///--------------------------------------------------------------------------
	///	LOCAL VARIABILE
	///--------------------------------------------------------------------------

	U16 delay;

	S16 slew_rate;

	S16 s16t;


	U16 ret;

	///--------------------------------------------------------------------------
	///	CHECK AND INITIALIZATIONS
	///--------------------------------------------------------------------------

	///--------------------------------------------------------------------------
	///	BODY
	///--------------------------------------------------------------------------
	//	>Calculate target delay
	//	>Calculate maximum delay change target_delay-old delay (slew rate)
	//		>based on user defined target_speed 
	//		>limited by servo max slew rate (max rotation speed normalized to my units)
	//	>calculate new delay (apply delay change)
	//	>save delay on servo_delay
	//	>return new delay
		///Calculate target OCR of servo [index]
	//Calculate position dependent coefficient
	s16t 	= K1 *servo_off[ index ] +K1 *servo_target_pos[ index ];
	//apply sign correction
	if (IS_BIT_ONE( SERVO_DIR, index ))
	{
		s16t = -s16t;
	}
	delay	= K0 + s16t;
		///Speed Limiter
	//calculate maximum allowed motion. I need to convert from [unit/second] -> [unit/20mS]
	slew_rate = K1*servo_target_speed[ index ] /50;
	//If: The user wants to exceed the servo limits
	if (slew_rate > SERVO_MAX_SLEW_RATE)
	{
		//Clip the Slew rate
		slew_rate = SERVO_MAX_SLEW_RATE;
	}
	//calculate slew rate required to meet user input
	s16t = delay - servo_delay[ index ];
	if (s16t > +slew_rate)
	{
		//I'm NOT locked: the servo is moving at max speed
		CLEAR_BIT( servo_lock, index );
		ret = servo_delay[ index ] +slew_rate;
	}
	else if (s16t < -slew_rate)
	{
		//I'm NOT locked: the servo is moving at max speed
		CLEAR_BIT( servo_lock, index );
		ret = servo_delay[ index ] -slew_rate;
	}
	else
	{
		//I'm locked: The motor is not moving at max speed
		SET_BIT( servo_lock, index );
		ret = delay;
	}
	//Write back result and return the delay
	servo_delay[ index ] = ret;

	///--------------------------------------------------------------------------
	///	RETURN
	///--------------------------------------------------------------------------	

	return ret;
}	//end function: servo_calc_delay


