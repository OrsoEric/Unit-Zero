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
****************************************************************************/

/****************************************************************************
**	DESCRIPTION
*****************************************************************************
**	This is the control progam for the 5 servos lego 4 leg robots
*****************************************************************************
**	CALCULATION OF TIMER 1 OCR FOR PULSE WIDTH
**	Constant for the conversion from angle (S8) to OCR (U16)
**	x	= [I8] input angle, i use from -127 to +127, -128 is the disable special code
**	Tn 	= [s] (typ 1.5mS) neutral time of the servo, pulse width required for 0°
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
**	servo_global_time = 45 		//i'm in the 45° tick of the cycle
**	SERVO_POS( 0 , 3 ) = -50	//i start the new motion from current time and in position -50 (50/127*90°=-35.4°)
**	SERVO_TIME( 0 , 3 ) = 45
**	SERVO_POS( 1 , 3 ) = -30	//i end the new motion in 480mS and in position -30 (50/127*90°=-21.3°)
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
**	PB2			|	rear SX
**	PB3			|	rear DX
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
**	Control		|	1.5mS (center)	+/-900uS (90°)	~20mS (period)
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
volatile U8 servo_cnt;
//each servo has 1 bit, it's rised when the interpolator is done
volatile U8 servo_idle;

	//-----------------------------------------------------------------------
	//	SERVOS VARS
	//-----------------------------------------------------------------------

//servo direction correction mask, allow for the logical direction of a joint to be reversed in sign
//each bit is associated with a servo, if the bit is '1', the direction is reversed
U8 servo_dir			= SERVO_DIR;
//servo position offset for true 0 position
//offsets are accounted for in separately from the position, it does not eat into the dynamic
S8 servo_off[ N_SERVOS ] =
{
	+2,
	+10,
	-3,
	-5,
	+0,
	+10
};
//Servo motion planner time base (1 = 20.07mS)
U8 servo_global_time;
//positions for the interpolator (-127,+127) -128=servo idle
S8 servo_pos[ 2 * N_SERVOS ];
//first column is starting time, second column is ending time
U8 servo_time[ 2 * N_SERVOS ];
//Second layer of vars, store future motion planned
S8 servo_next_pos[N_SERVOS];
U8 servo_next_time[N_SERVOS];

	//-----------------------------------------------------------------------
	//	TRAJECTORY VARS
	//-----------------------------------------------------------------------

//current trajectory
U8 trajectory;
//optional trajectory parameter (ex: walking speed)
U8 trajectory_param;
//trajectory the user want the robot to follow
U8 wished_trajectory;
U8 wished_trajectory_param;

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

	//temp var
	U8 u8t, u8t1;

		///servos vars
	//timer 1 OCR calculation
	U16 delay;

	//signal that a future value on the buffer has been used and needs to be refreshed
	U8 servo_next_idle;

	///**********************************************************************
	///	 INITIALISATION
	///**********************************************************************

	//
	f.safe 				= 0;
	//
	servo_idle 			= 0x00;
	servo_next_idle		= 0x00;
	//Initialize status var to disabled
	servo_cnt 			= N_SERVOS;
	//Clear global servo time
	servo_global_time 	= 0;
	//Initialize servo position to zero (offset is accounted for during calculations, i must not add it here)
	for (u8t = 0;u8t < N_SERVOS;u8t++)
	{
		//Initialize start and end positions
		SERVO_POS( 0, u8t ) 	= 0;
		SERVO_POS( 1, u8t ) 	= 0;
		//Initialize start and end times (REMEMBER to set them, DT = 0 when DX != 0  is an error when ISR scan)
		SERVO_TIME( 0, u8t ) 	= 0;
		SERVO_TIME( 1, u8t ) 	= 0;
		//initialize next frame buffer
		servo_next_pos[ u8t ] 	= 0;
		servo_next_time[ u8t ] 	= 0;

	}

	//servo_next_pos[ 2 ] 	= +30;
	//servo_next_time[ 2 ] 	= 128;

	///**********************************************************************
	///	PHERIPERALS INITIALISATION
	///**********************************************************************

	//Initialize devices
	global_init();
	//Give time for the display to go offline (from previous down)
	_delay_ms( 200.0 );
	//Get the LCD display on line
	LCD_ON();
	//Give time for the display to load
	_delay_ms( 200.0 );
	//Initialize display
	display_initialisation();
	//Wait for good measure, i'm in no rush
	_delay_ms( 200.0 );
	//The proud name of this unit
	print_str( 0, 0, (U8 *)"Unit Zero Zero");

	///**********************************************************************
	///	MAIN LOOP
	///**********************************************************************

	for EVER
	{
		if (f.x == 1)
		{
			f.x = 0;
		}

		//-----------------------------------------------------------------------
		//	LCD DIPSPLAY UPDATE
		//-----------------------------------------------------------------------
		//	This routine update the display

		//Issued every 156.8uS
		if (f.lcd == 1)
		{
			f.lcd = 0;
			//activity: ON
			CLEAR_BIT( PORTC, 0 );
			//Update display
			display_update();
			//activity: OFF
			SET_BIT( PORTC, 0 );
		}

		//-----------------------------------------------------------------------
		//	START MOTOR SCAN (5.8uS max all servo functions)
		//-----------------------------------------------------------------------
		//	Flag rised by [Timer 0] (20mS)
		//	>activity pin (led signal uC use, oscilloscope allow to measure function times)
		//	>clear servo status var
		//	>calculate first delay, pull down first line
		//	>setup first delay, enable [Timer 1]
		//	>Timer 1 ISR will handle the update of the servos and disable it self when done

		//Issued every 20.07mS
		if (f.mot == 1)
		{
			//clear flag
			f.mot 			= 0;
			//activity: ON
			CLEAR_BIT( PORTC, 0 );
			//clear status var
			servo_cnt 		= 0;
			u8t 			= 0;
			//force safe flag to 0, it is unsafe now to update the servo vars
			f.safe 			= 0;
			//calculate delay
			delay 			= servo_calc_delay( 0 );
			//Store delay on T1
			OCR1A 			= delay;
			//start T1
			START_TIMER1();
			//pull up first line
			SET_BIT( PORTB, 0 );
			//activity: OFF
			SET_BIT( PORTC, 0 );
		}	//End If: motor scan flag

		//-----------------------------------------------------------------------
		//	MOTION BUFFER
		//-----------------------------------------------------------------------
		//	The interpolator fully executed a movement
		//	I load one set of futures values from the buffer and signal that new values have to be generated
		//	i use a mobile mask scan to handle the update

		//If: it's safe to write into the global servo vars AND at least one servo require instructions
		if ((f.safe == 1) && (servo_idle != 0x00))
		{
			//move the global var to a local one for speed
			u8t 				= servo_idle;
			//The trajector
			servo_next_idle 	= u8t;
			//For: all servos
			for (u8t1 = 0; u8t1 < N_SERVOS;u8t1++)
			{
				//If: if the servo end frame need to be updated
				if (IS_BIT_ONE( u8t, u8t1 ))
				{
					//fill the end position with the new one
					SERVO_POS( 1, u8t1 )	= servo_next_pos[ u8t1 ];
					SERVO_TIME( 1, u8t1 )	= servo_next_time[ u8t1 ];
				}
			}
			//I'm done
			servo_idle 		= 0x00;
		}

		//-----------------------------------------------------------------------
		//	TRAJECTORY GENERATION
		//-----------------------------------------------------------------------

		//If: some elements from the buffer have been used
		if (servo_next_idle != 0x00)
		{
			u8t = servo_next_idle;
			//For: all servos
			for (u8t1 = 0; u8t1 < N_SERVOS;u8t1++)
			{
				//If the servo need a new buffer
				if (IS_BIT_ONE( u8t, u8t1 ))
				{
					//request a new trajectory point
					if (servo_next_pos[u8t1]== 0)
					{
						servo_next_pos[u8t1]= 20;

					}
					else
					{
						servo_next_pos[u8t1]= 0;
					}

					servo_next_time[ u8t1 ] = 50;
				}
			}
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
**	The operation i must do is:
**	Position = Start + increment of position * fraction of time passed
**	X(t) = Xs + DX*(t/DT)
**	DX would require 9 bit, i unpack it
**	t/DT is <1 by definition, i multiply by 2^N and divide rounding later
**	There is little reason to round an S8 that i have to expand to a 16b delay anyway
**	D = K0 + K1 * Xoff + K1 * Xs * (2^8 - t*2^8/DT) +  + K1 * Xe * (t*2^8/DT)
**	i calculate t*2^8/DT stand alone
*****************************************************************************
**	DELTAT ordering
**	>>tested the DeltaT algorithm, much more to it than it first seems, it must understand where the temporal marker is gonna be
*****************************************************************************
**	T recalculation
**	t must be 0 < t < DeltaT to do the calculations
****************************************************************************/

U16 servo_calc_delay( U8 index )
{
	//***********************************************************************
	//	STATIC VARIABILE
	//***********************************************************************

	//***********************************************************************
	//	LOCAL VARIABILE
	//***********************************************************************

	//global time
	U8 gt;
	//Positions
	S8 x[2];
	//Times
	U8 t[2];
	//Delay
	U16 d;
	//temp var
	U8 u8t;

	U16 u16t;

	S16 s16t;
	//	local flow control flags
	//	0	: enable temporal interpolation, it's meant to skip calculation during start, end and constant position
	//	1	: select between x[0] and x[1], it's meant to handle swaps, start and ending
	//	2	: copy ending in starting once ending is computed, it's meant to stop when ending position is reached and setup for next motion
	//	3	: idle detection, it's meant to detect when the interpolator is done
	U8 flags;

	//***********************************************************************
	//	INITIALIZATIONS
	//***********************************************************************
	//	>initialize vars
	//	>fetch global vars
	//	>Order vars with Te > Ts
	//	>Handle special cases
	//		>X fixed, no interpolation needed (happen when Xs == Xe or when gt == Ts or gt == Te)
	//		>ERROR (happen when Ts == Te but Xs != Xe, i have 2 points in the same temporal slot)

	//clear flags
	flags 	= 0x00;
	//global time
	gt 		= servo_global_time;
	//get servo reference time
	t[0] 	= SERVO_TIME( 0, index );
	t[1]	= SERVO_TIME( 1, index );
	//I put them if plus order to halve possible cases (i have to swap positions as well for this to work)
	//If: Tstart > Tend
	if (t[0] > t[1])
	{
		//swap times
		u8t = t[0];
		t[0] = t[1];
		t[1] = u8t;
		//Signal that the positions needs to be swapped
		SET_BIT( flags, 1 );
	}
	//fetch servo vars, i handle swaps with a flag
	x[0] 	= SERVO_POS( 0, index );
	x[1] 	= SERVO_POS( 1, index );
		///Handle special cases
	//handle DELTAX == 0
	if (x[0] == x[1])
	{
		//If starting and end position are the same:
		//I have no interpolation to do irregarding to all others time vars
		//I use x[0]
		//TOGGLE_BIT( flags, 1);
		//If: DT == 0 it means that the interpolator is idle
		if (t[0] == t[1])
		{
			//this servo is idle
			SET_BIT( flags, 3 );
		}
	}
	//handle DELTAT == 0
	else if (t[0] == t[1])
	{
		//here mean that DX is NOT zero, but DT is, it's an ERROR
		//as recovery i set the two position to the same value
		SET_BIT( flags, 2 );
		//this servo is idle
		SET_BIT( flags, 3 );
	}
	//exactly on start
	else if (t[0] == gt)
	{
		//I use x[0]
		//SET_BIT( flags, 1);
	}
	//exactly on end
	else if (t[1] == gt)
	{
		//I use ending x as position (no interpolation needed)
		TOGGLE_BIT( flags, 1);
		//I ended the interpolation, i will write ending time and position in the starting one
		SET_BIT( flags, 2 );
		//this servo is idle
		SET_BIT( flags, 3 );
	}
	//i'm done with special cases, all others are time dependent
	else
	{
		//Enable temporal calculations
		SET_BIT( flags, 0);
	}

	//***********************************************************************
	//	BODY
	//***********************************************************************
	//	>Temporal Interpolation enabled?
	//		Y
	//		>Calculate DeltaT and t
	//			>DeltaT (temporal width of the interpolation)
	//			>t	(time elapsed since start of interpolation)
	//		>Calculate Kt = 256*t/DT
	//		>Calculate time dependent delay K1*Xs*(256-KT)/256 + K1*Xe*KT/256
	//		N
	//		>Calculate time dependent delay K1*X
	//	>Calculate delay
	//		>D = K0 + K1*Xoff+D(t,x)

	//If: temporal calculations are enabled
	if (IS_BIT_ONE( flags, 0 ))
	{
		//-----------------------------------------------------------------------
		//	DeltaT and t calculation
		//-----------------------------------------------------------------------
		//	[Ts t Te]	gt is inside servo time
		//		>t = gt -Ts
		//		>DT = Te -Ts
		//	[t Ts Te]	gt is before servo time
		//		>t = 2^8 -Te +t
		//		>DT = 2^8 -Te +Ts
		//	[Ts Te t]	gt is after servo time
		//		>t = gt -Te
		//		>DT = 2^8 -Te +Ts

		//if: global time is inside servo time [Ts t Te]
		if ((gt > t[0]) && (gt < t[1]))
		{
			//t = t - Ts
			gt = gt - t[0];
			//DeltaT = Te - Ts
			t[0] = t[1] - t[0];

		}
		//if: global time is before servo time [t Ts Te]
		else if (gt < t[0])
		{
			//t = 2^8 - Te + t (i unpack 2^8 into 255+1)
			gt = 255 - t[1] + 1 + gt;
			//DeltaT = 2^8 - Te + Ts
			t[0] = 255 - t[1] +1 + t[0];
			//I need to swap the positions
			TOGGLE_BIT( flags, 1 );
		}
		//if: global time is after servo time [Ts Te t]
		else
		{
			//t = t - Te
			gt = gt - t[1];
			//DeltaT = 2^8 - Te + Ts
			t[0] = 255 - t[1] +1 + t[0];
			//I need to swap the positions
			TOGGLE_BIT( flags, 1 );
		}

		//-----------------------------------------------------------------------
		//	CALCULATE KT
		//-----------------------------------------------------------------------
		//	KT is the operation t/DT, it is the fraction of DT elapsed
		//	it is bounded [0,1] i multiply by 256 to keep it within an U8
		//	this var is used by the interpolator
		//	i round the result toward even to increase resolution

		//calculate NUM
		u16t = (U16)256 * gt;
		//divide and obtain unrounded result
		u8t = (U8)0 + u16t/t[0];
		//i have to do: 2*num -2*res*gt, if this quantity is bigger than gt -> i round up, if the quantity is equal and the result is odd -> i round up
		u16t = 2*(u16t-u8t*t[0]);
		//if the 2*residual is bigger then divisor (DT)
		if (u16t > t[0])
		{
			//round up
			u8t++;
		}
		//if the 2*residual is equal to the divisor (DT) AND the unrounded result is ODD
		else if ((u16t == t[0]) && (u8t & 0x01))
		{
			//round up (round toward even ex: 217.5 -> 218, 216.5 -> 216)
			u8t++;
		}
		//if the 2*residual is lower than the divisor
		else
		{
			//round down (do nothing, the bits are masked out)
		}
		//t[1] now is KT
		t[1] = u8t;

		//-----------------------------------------------------------------------
		//	CALCULATE POSITION DEPENDENT DELAY
		//-----------------------------------------------------------------------
		//	Calculate the part of the delay that is function of Xs, Xe, Ts, Te, gt
		//	the operation to do is:
		//	Dx = K1 * [ Xs * (256 - KT) / 256 + Xe * (KT) / 256 ]
		//	vars:
		//	Xs: x[0], Xe: x[0], DT: t[0], KT: t[1]

		//if: i need to swap positions
		if (IS_BIT_ONE( flags, 1))
		{
			//i am better off complementing kt, i get the same result without an additional S8 temp var
			t[1] = 255 - t[1] +1;
		}
		//u16t = 127 * K1;

		//calc Xs * (2^8 - Kt) + Xe * (Kt)
		s16t = x[0] * (255 - t[1] +1) + x[1] * t[1];
		//divide by 2^8 and round the result, corrected for negative numerators
		s16t = AT_DIVIDE_NEG_RTO( s16t, 8 );
		//multiply by K1
	}	//End If: temporal calculations are enabled
	//If: i already know the position to be used and do not need interpolation
	else
	{
		//-----------------------------------------------------------------------
		//	CALCULATE POSITION DEPENDENT DELAY
		//-----------------------------------------------------------------------

		//If: i use x[1]
		if (IS_BIT_ONE( flags, 1))
		{
			s16t = x[1];
		}
		//If: i use x[0]
		else
		{
			s16t = x[0];
		}
	}	//End If: i already know the position to be used and do not need interpolation

	//now i only need to multiply by the OCR scaling constant K1
	s16t = s16t * K1;

	//-----------------------------------------------------------------------
	//	CALCULATE POSITION INDEPENDENT DELAY
	//-----------------------------------------------------------------------

	//K0 is the neutral delay, the offset is meant to compensate for mechanical misalignement
	d = K0 + K1 * SERVO_OFFSET( index ) + s16t;

	//-----------------------------------------------------------------------
	//	CALCULATE POSITION INDEPENDENT DELAY
	//-----------------------------------------------------------------------

	//If: end frame was executed
	if (IS_BIT_ONE( flags, 2 ))
	{
		//I copy the ending position in the servo global vars
		//If: ending position was x[1]
		if (IS_BIT_ONE( flags, 1 ))
		{
			SERVO_POS( 0, index ) = x[1];
			SERVO_TIME( 0, index ) = SERVO_TIME( 1, index );
		}
		//If: ending position was x[0], i end up here if Ts was bigger than Te
		else
		{
			//should not happen during normal operation, but in case of creative use of the interpolator i handle this special case
			SERVO_POS( 1, index ) = x[0];
			SERVO_TIME( 1, index ) = SERVO_TIME( 0, index );
		}
	}

	//If: the servo is idle after this operation
	if (IS_BIT_ONE( flags, 3 ))
	{
		//signal that the servo is now idle
		SET_BIT( servo_idle, index );
	}

	//***********************************************************************
	//	RETURN
	//***********************************************************************

	return d;
}	//end function: servo_calc_delay

