/****************************************************************************
**	ROBOTICA@POLITO PROJECT
*****************************************************************************
**	Library to make use of lcd display in 4bit or 8bit mode
**	The library is interrupt based to reduce the MPU consumption
*****************************************************************************
**	Author: 			Orso Eric
**	Creation Date:
**	Last Edit Date:
**	Revision:
**	Version:			2.0
****************************************************************************/

/****************************************************************************
**	HYSTORY VERSION
*****************************************************************************
**	>
****************************************************************************/

/****************************************************************************
**	USED PIN
**	TIPS: compile this field before writing any code: it really helps!
*****************************************************************************
**
****************************************************************************/

/****************************************************************************
**	DESCRIPTION
*****************************************************************************
**	>The library solve the problem of extrime time loss due to delay.
**	Now the function display_update can be called when the lcd_busy flag is rised
**	every 100uS or so, replacing the delay
**	>32 byte command buffer, allow for 16 charter to be loaded in the buffer at once
****************************************************************************/

/****************************************************************************
**	KNOWN BUG
*****************************************************************************
**	>
****************************************************************************/

/****************************************************************************
**	INCLUDE
****************************************************************************/

#include "global.h"

/****************************************************************************
**	GLOBAL VARIABILE
****************************************************************************/

//Command buffer
uint8_t lcd_buffer[ LCD_BUFFER_SIZE ];
uint8_t lcd_buffer_bot 		= 0;
uint8_t lcd_buffer_top 		= 0;
//Low when there is nothing to do
uint8_t lcd_busy_flag		= 0x00;

/****************************************************************************
**	FUNCTION
****************************************************************************/

void display_initialisation( void )
{
	//***********************************************************************
	//	STATIC VARIABILE
	//***********************************************************************

	//***********************************************************************
	//	LOCAL VARIABILE
	//***********************************************************************

	//***********************************************************************
	//	MAIN
	//***********************************************************************
	
	
	//4 bit mode is used
	#ifdef MODE_4BIT

		//init the display in 4bit mode, i have to send the high part as low part
		//to first configure the 4bit mode
		send_cmd( INIT_4B2L58F >> 4 );

		//re-init the display in 4 bit mode ( pervious bit have been ignored )
		send_cmd( INIT_4B2L58F );
	
		//clear display
		send_cmd( DISPLAY_CLEAR );
	
		//engage display and cursor
		send_cmd( DISPLAY_ON_CURSOR );

	//8 bit mode is used
	#else

		//init the display in 8bit mode
		send_cmd( INIT_8B2L58F );
	
		//clear display
		send_cmd( DISPLAY_CLEAR );
	
		//engage display and cursor
		send_cmd( DISPLAY_ON_CURSOR );
	
	#endif


	//***********************************************************************
	//	RETURN
	//***********************************************************************

	return;
}

void send_cmd(uint8_t cmd)
{
	//***********************************************************************
	//	STATIC VARIABILE
	//***********************************************************************

	//***********************************************************************
	//	LOCAL VARIABILE
	//***********************************************************************

	#ifdef MODE_4BIT
		//temp variabile for the nibble
		uint8_t nh, nl;
		uint8_t temp;

	#endif

	//***********************************************************************
	//	MAIN
	//***********************************************************************
	//	This routine is used to send a command, the routine execute different
	//	operation if compiled for 8bit operation or 4bit operation

	//Rise the LCD busy flag
	lcd_busy_flag = 0x01;

	#ifdef MODE_4BIT

		nh = ((cmd & 0xf0) >> 4);
		nl = (cmd & 0x0f);

		#ifdef MODE_LOW_BIT
			//send the high nibble
			temp = LCD_PORT;
			temp = temp & 0xf0;
			temp = temp | (nh);
			LCD_PORT = temp;
		#else
			//send the high nibble
			temp = LCD_PORT;
			temp = temp & 0x0f;
			temp = temp | (nh << 4);
			LCD_PORT = temp;
		#endif

		//set the rs signal: i'm using the istruction register
		CLEAR_RS();

		//Strobe the enable pin, time specify the delay after each edge
		STROBE_EN( 200.0 );

		#ifdef MODE_LOW_BIT
			//send the high nibble
			temp = LCD_PORT;
			temp = temp & 0xf0;
			temp = temp | (nl);
			LCD_PORT = temp;
		#else
			//send the high nibble
			temp = LCD_PORT;
			temp = temp & 0x0f;
			temp = temp | (nl << 4);
			LCD_PORT = temp;
		#endif

		//Strobe the enable pin, time specify the delay after each edge
		STROBE_EN( 200.0 );

		//the slowest command take 1.5ms
		_delay_ms( 2.0 );

	#else

		//output the command on the output pin
		PORTD = cmd;

		//set the rs signal: i'm using the istruction register
		CLEAR_RS();

		//Strobe the enable pin, time specify the delay after each edge
		STROBE_EN( 200.0 );

		//the slowest command take 1.5ms
		_delay_ms( 2.0 );
	
	#endif



	//***********************************************************************
	//	RETURN
	//***********************************************************************

	return;
}

/****************************************************************************
**	DISPLAY_UPDATE
*****************************************************************************
**	This function has to be called every Tlcd mS, allow the update of the LCD
**	
****************************************************************************/

void display_update( void )
{
	///**************************************************************************
	///	STATIC VARIABILE
	///**************************************************************************

	//Store the status of the Lcd
	static Lcd_status status = 
	{ 
		0, 	//en
		0, 	//rs
		0, 	//ist
		0,	//low
		0 	//exe
	};

	//Allow to skip the address send instruction
	static uint8_t last_address = 255;

	///**************************************************************************
	///	LOCAL VARIABILE
	///**************************************************************************

	uint8_t tmp8;

	///**************************************************************************
	///	PARAMETER CHECK
	///**************************************************************************


	///**************************************************************************
	///	UPDATE ALGORITHM
	///**************************************************************************
	//	>if buffer >2 byte: start


	//idle
	if (status.exe == 0)
	{
		//If i have at least one complete command inside the buffer
		if ( LCD_BUFFER_NUMELEM() >= 2 )
		{
			//Read the address
			tmp8 = LCD_BUFFER_PEEK();
			//If the address is equal to the pervious one +1, i can avoid sending the address again
			if (last_address == (tmp8 +1 ) )
			{
				//Jump to the data send
				status.exe = 5;
			}
			else
			{
				//start the send operation from the address send
				status.exe = 1;
			}
			
			//save last address
			last_address = tmp8;
		}
	}

	//Send AH
	//Rise EN
	else if (status.exe == 1)
	{
		CLEAR_BIT( RS_PORT, RS_PIN );
		//fetch the low part of the port
		tmp8 = LCD_PORT & 0x0f;
		//load the hi nibble to the hi part of the port
		tmp8 = tmp8 | (LCD_BUFFER_PEEK() & 0xf0);
		//write back
		LCD_PORT = tmp8;
		//Rise the EN
		SET_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;		
	}
	//Clear EN (strobe)
	else if (status.exe == 2)
	{
		CLEAR_BIT( RS_PORT, RS_PIN );
		//clear EN
		CLEAR_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;
	}
	//Send AL
	//Rise EN
	else if (status.exe == 3)
	{
		CLEAR_BIT( RS_PORT, RS_PIN );
		//fetch the low part of the port
		tmp8 = LCD_PORT & 0x0f;
		//load the hi nibble to the hi part of the port
		tmp8 = tmp8 | ((LCD_BUFFER_PEEK() & 0x0f) << 4);
		//write back
		LCD_PORT = tmp8;
		//Rise the EN
		SET_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;		
	}
	//Clear EN (strobe)
	else if (status.exe == 4)
	{
		CLEAR_BIT( RS_PORT, RS_PIN );
		//clear EN
		CLEAR_BIT( EN_PORT, EN_PIN );

		LCD_BUFFER_KICK();
		//jump
		status.exe++;
	}
	//Send DH
	//Rise EN
	else if (status.exe == 5)
	{
		SET_BIT( RS_PORT, RS_PIN );
		//fetch the low part of the port
		tmp8 = LCD_PORT & 0x0f;
		//load the hi nibble to the hi part of the port
		tmp8 = tmp8 | (LCD_BUFFER_PEEK() & 0xf0);
		//write back
		LCD_PORT = tmp8;
		//Rise the EN
		SET_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;		
	}
	//Clear EN (strobe)
	else if (status.exe == 6)
	{
		SET_BIT( RS_PORT, RS_PIN );
		//clear EN
		CLEAR_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;
	}
	//Send DL
	//Rise EN
	else if (status.exe == 7)
	{
		SET_BIT( RS_PORT, RS_PIN );
		//fetch the low part of the port
		tmp8 = LCD_PORT & 0x0f;
		//load the hi nibble to the hi part of the port
		tmp8 = tmp8 | ((LCD_BUFFER_PEEK() & 0x0f) << 4);
		//write back
		LCD_PORT = tmp8;
		//Rise the EN
		SET_BIT( EN_PORT, EN_PIN );
		//jump
		status.exe++;		
	}
	//Clear EN (strobe)
	else if (status.exe == 8)
	{
		SET_BIT( RS_PORT, RS_PIN );
		//clear EN
		CLEAR_BIT( EN_PORT, EN_PIN );

		LCD_BUFFER_KICK();
		//jump
		status.exe = 0;
		//If all command are executed
		if (LCD_BUFFER_NUMELEM() == 0)
		{
			//Clear the LCD busy flag
			lcd_busy_flag = 0x00;
		}
	}
	
	///**************************************************************************
	///	RETURN
	///**************************************************************************

	return;
}	//end function: display_update

/****************************************************************************
**	PRINT CHAR
*****************************************************************************
**	Put a charter in the execution buffer of the display
****************************************************************************/

//Print a char on screen
void print_char( uint8_t row, uint8_t col, uint8_t data )
{
	///**************************************************************************
	///	STATIC VARIABILE
	///**************************************************************************

	///**************************************************************************
	///	LOCAL VARIABILE
	///**************************************************************************

	uint8_t address;

	///**************************************************************************
	///	PARAMETER CHECK
	///**************************************************************************

	if ( (row > 1) || (col > 31) )
	{
		return;
	}

	///**************************************************************************
	///	ALGORITHM
	///**************************************************************************

	//Rise the LCD busy flag
	lcd_busy_flag = 0x01;

	address = MASK(7) | (row << 6) | col;

	LCD_BUFFER_PUSH( address );
	LCD_BUFFER_PUSH( data );

	///**************************************************************************
	///	RETURN
	///**************************************************************************

	return;
}

/****************************************************************************
**	PRINT STR
*****************************************************************************
**	Print a string starting from row, col
****************************************************************************/

void print_str( uint8_t row, uint8_t col, uint8_t *str )
{
	//***********************************************************************
	//	STATIC VARIABILE
	//***********************************************************************

	//***********************************************************************
	//	LOCAL VARIABILE
	//***********************************************************************

	uint8_t t;

	//***********************************************************************
	//	PARAMETER CHECK
	//***********************************************************************

	if ( (str == NULL)|| (row > 1) || (col > 15) )
	{
		return;
	}

	//***********************************************************************
	//	MAIN
	//***********************************************************************

	//Initialise
	t = 0;
	while ( ((col +t) <= 16) && (str[ t ] != '\0') )
	{
		
		print_char( row, (col +t), str[ t ] );

		t++;
	}
	
	//***********************************************************************
	//	RETURN
	//***********************************************************************

	return;
}

/****************************************************************************
**	PRINT_INT8
*****************************************************************************
**	Print an input 8 bit signed integer in the specified row and column
**	the function return the first free column after the number, 0 in case of error
**	The user has to clean up charter if necessary (example if one write
**	-123 and then 0, the display will show 0123).
**	row, col: position
**	point: position of the point
**	size: size of the number, will blank unused char
****************************************************************************/

uint8_t print_int8( uint8_t row, uint8_t col, uint8_t point, uint8_t size, int8_t num )
{
	//***************************************************************************
	//	STATIC VARIABILE
	//***************************************************************************

	static int8_t base[ 3 ] =
	{
		-1,
		-10,
		-100
	};

	//***************************************************************************
	//	LOCAL VARIABILE
	//***************************************************************************

	//base index
	int8_t index;
	//
	uint8_t pos			= 0;

	uint8_t temp;
	//flag that authorize the zero blanking
	uint8_t blank_flag 	= 0x01;

	//***************************************************************************
	//	PARAMETER CHECK
	//***************************************************************************

	if ( (row > 1) || (col > 15) )
	{
		return pos;
	}	

	//***************************************************************************
	//	ALGORITHM
	//***************************************************************************
	//	>allocate the string
	//	>fill the string with terminator
	//	>

	//if the number is negative, put a negative sign
	if (num < 0)
	{
		print_char( row, col, '-');
		pos++;
	}
	else
	{
		//i make all number negative because the negative range is bigger
		num = -num;
	}
	//for any base
	for (index = 3 -1;index >= 0;index--)
	{
		//preset the position with an ASCII 0
		temp = '0';
		//until the number is higher than the base
		while ( num <= base[ index ] )
		{
			//decrease the number by the base
			num = num - base[ index ];
			//increase the char
			temp++;
		}	//end while the number is higher than the base
		//Stop blanking when:
		//	>chipher is not '0'
		//	>i have not reached the least significant digit
		//	>i have not pass the point
		if ( (blank_flag == 0x01) && (temp == '0') && ( (index+1) > point)  )
		{
			//do nothing
		}
		//I have reached the point position
		else if ( (index+1) == point)
		{
			//Print point
			print_char( row, col +pos, '.' );
			//Next pos
			pos++;						
			//Print chipher
			print_char( row, col +pos, temp );
			//Next pos
			pos++;
			//Stop blanking
			blank_flag = 0x00;
		}
		//write back the char and advance to the next position
		else
		{
			//Print chipher
			print_char( row, col +pos, temp );
			//Next pos
			pos++;
			//Stop blanking
			blank_flag = 0x00;
		}
	}	//end for any base
	//Add padding space at the bottom of the number to cover older entry
	//while: i have not reached the given padding size
	while (pos < size)
	{
		//print padding
		print_char( row, col +pos, ' ' );
		//next position
		pos++;
	}	//end while: i have not reached the given padding size

	//***************************************************************************
	//	RETURN
	//***************************************************************************

	return pos;
}	//end function: print int8

/****************************************************************************
**	PRINT_INT16
*****************************************************************************
**	Print an input 16 bit signed integer in the specified row and column
**	the function return the first free column after thje number, 0 in case of error
**	The user has to clean up charter if necessary (example if one write
**	-123 and then 0, the display will show 0123, the user must clean the 123 part).
****************************************************************************/

uint8_t print_int16( uint8_t row, uint8_t col, uint8_t point, uint8_t size, int16_t num )
{
	//***************************************************************************
	//	STATIC VARIABILE
	//***************************************************************************

	static int16_t base[ 5 ] =
	{
		-1,
		-10,
		-100,
		-1000,
		-10000
	};

	//***************************************************************************
	//	LOCAL VARIABILE
	//***************************************************************************

	//base index
	int8_t index;
	//
	uint8_t pos			= 0;

	uint8_t temp;
	//flag that authorize the zero blanking
	uint8_t blank_flag = 0x01;

	//***************************************************************************
	//	PARAMETER CHECK
	//***************************************************************************

	if ( (row > 1) || (col > 15) )
	{
		return col;
	}	

	//***************************************************************************
	//	ALGORITHM
	//***************************************************************************
	//	>allocate the string
	//	>fill the string with terminator
	//	>

	//if the number is negative, put a negative sign
	if (num < 0)
	{
		print_char( row, col, '-');
		col++;
	}
	else
	{
		//i make all number negative because the negative range is bigger
		num = -num;
	}

	//for any base
	for (index = 5 -1;index >= 0;index--)
	{
	//preset the position with an ASCII 0
		temp = '0';
		//until the number is higher than the base
		while ( num <= base[ index ] )
		{
			//decrease the number by the base
			num = num - base[ index ];
			//increase the char
			temp++;
		}	//end while the number is higher than the base
		//Stop blanking when:
		//	>chipher is not '0'
		//	>i have not reached the least significant digit
		//	>i have not pass the point
		if ( (blank_flag == 0x01) && (temp == '0') && ( (index+1) > point)  )
		{
			//do nothing
		}
		//I have reached the point position
		else if ( (index+1) == point)
		{
			//Print point
			print_char( row, col +pos, '.' );
			//Next pos
			pos++;						
			//Print chipher
			print_char( row, col +pos, temp );
			//Next pos
			pos++;
			//Stop blanking
			blank_flag = 0x00;
		}
		//write back the char and advance to the next position
		else
		{
			//Print chipher
			print_char( row, col +pos, temp );
			//Next pos
			pos++;
			//Stop blanking
			blank_flag = 0x00;
		}
	}	//end for any base
	//Add padding space at the bottom of the number to cover older entry
	//while: i have not reached the given padding size
	while (pos < size)
	{
		//print padding
		print_char( row, col +pos, ' ' );
		//next position
		pos++;
	}	//end while: i have not reached the given padding size

	//***************************************************************************
	//	RETURN
	//***************************************************************************

	return col;
}	//end function: print int16
