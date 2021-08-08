#ifndef AT_LCD_H
	//header envroiment variabile, is used to detect multiple inclusion
	//of the same header, and can be used in the c file to detect the
	//included library
	#define AT_LCD_H

	/****************************************************************************
	**	ENVROIMENT VARIABILE
	****************************************************************************/

	/****************************************************************************
	**	GLOBAL INCLUDE
	**	TIPS: you can put here the library common to all source file
	****************************************************************************/

	/****************************************************************************
	**	DEFINE
	****************************************************************************/

	#define LCD_BUFFER_SIZE		32

		///*********************************************************************/
		///	INTERFACE
		///*********************************************************************/

	//If defined, enable the 4 bit communication
	#define MODE_4BIT

	//If dfined, the D7..D4 of the LCD are connected to D3...D0 of the port
	//#define MODE_LOW_BIT

		///*********************************************************************/
		///	PIN
		///*********************************************************************/
		
		//EN
	#define EN_PORT				PORTC
	#define EN_PIN				PC2
	
		//RS				
	#define RS_PORT				PORTC
	#define RS_PIN				PC3

		//DATA
	#define LCD_PORT			PORTC


	#define INIT_8B2L58F		( MASK(5) | MASK(4) | MASK(3) )
	#define INIT_4B2L58F		( MASK(5) | MASK(3) )
	#define DISPLAY_CLEAR		( MASK(0) )
	#define DISPLAY_ON_CURSOR	( MASK(3) | MASK(2) | MASK(1) )

	#define SEND_COMMAND		0x00
	#define SEND_DATA			0x01


	/****************************************************************************
	**	MACRO
	****************************************************************************/

	//Strobe the enable pin, time specify the delay after each edge
	#define STROBE_EN( time )			\
		SET_BIT( EN_PORT, EN_PIN );		\
		_delay_us( time );				\
		CLEAR_BIT( EN_PORT, EN_PIN );	\
		_delay_us( time )

	#define SET_RS()					\
		SET_BIT( RS_PORT, RS_PIN )

	#define CLEAR_RS()					\
		CLEAR_BIT( RS_PORT, RS_PIN )

		
		///*********************************************************************/
		///	BUFFER
		///*********************************************************************/

	//push a data into the buffer
	#define LCD_BUFFER_PUSH( data )	\
		( ( lcd_buffer[ lcd_buffer_top ] = data ), ( ( lcd_buffer_top < (LCD_BUFFER_SIZE -1) ) ? (lcd_buffer_top++) : (lcd_buffer_top = 0) ) )
	
	//number of element in the buffer
	#define LCD_BUFFER_NUMELEM()	\
		( (lcd_buffer_top > lcd_buffer_bot) ? (lcd_buffer_top - lcd_buffer_bot) : (lcd_buffer_bot - lcd_buffer_top) )

	//peek the oldest byte in the buffer
	#define LCD_BUFFER_PEEK()	\
		( lcd_buffer[ lcd_buffer_bot ] )

	//kick out the oldest element in the buffer
	#define LCD_BUFFER_KICK()	\
		( ( lcd_buffer_bot < (LCD_BUFFER_SIZE -1) ) ? (lcd_buffer_bot++) : (lcd_buffer_bot = 0) )  


	/****************************************************************************
	**	TYPEDEF
	****************************************************************************/

	typedef struct _Lcd_status Lcd_status;

	/****************************************************************************
	**	STRUCTURE
	****************************************************************************/

	struct _Lcd_status
	{
		uint8_t en 	: 1;			//Status of the enable pin
		uint8_t rs 	: 1;			//Status of the RS pin
		uint8_t ist	: 1;			//Instruction or data sent last?
		uint8_t low	: 1;			//Low part sent last?
		uint8_t exe	: 4;			//execution step
	};

	/****************************************************************************
	**	PROTOTYPE: FUNCTION
	****************************************************************************/

	//Initialise Display
	extern void display_initialisation( void );
	//Hardwired command send
	extern void send_cmd(uint8_t cmd);
	//Function to be called every Tlcd uS
	extern void display_update( void );
	//Print a char on screen
	extern void print_char( uint8_t row, uint8_t col, uint8_t data );
	//Print a string on screen
	extern void print_str( uint8_t row, uint8_t col, uint8_t *str );
	//Print a string on screen starting from row, col, with point and blanking charter up to position size
	extern uint8_t print_int8( uint8_t row, uint8_t col, uint8_t size, uint8_t point, int8_t num );
	//Print a string on screen starting from row, col, with point and blanking charter up to position size
	extern uint8_t print_int16( uint8_t row, uint8_t col, uint8_t point, uint8_t size, int16_t num );

	/****************************************************************************
	**	PROTOTYPE: GLOBAL VARIABILE
	****************************************************************************/

	//command buffer
	extern uint8_t lcd_buffer[ LCD_BUFFER_SIZE ];
	extern uint8_t lcd_buffer_bot;
	extern uint8_t lcd_buffer_top;
	//Low when there is nothing to do (allow for further MCU cycle saving)
	extern uint8_t lcd_busy_flag;

#else
	#warning "multiple inclusion of the header file at_lcd.h"
#endif
