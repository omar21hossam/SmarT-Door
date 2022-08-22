/******************************************************************************
 *
 * File Name: MC1.c
 *
 * Description: Source file for micro controller 1
 *
 * Author: Omar hossam ElDin
 *
 *******************************************************************************/

#include "lcd.h"
#include "uart.h"
#include "keypad.h"
#include <util/delay.h>

/*******************************************************************************
 *                      Preprocessor Macros                                    *
 *******************************************************************************/
#define SUCCESS 1
#define FAILURE 0
#define MC1_READY 0x10
#define MC2_READY 0x10
/*******************************************************************************
 *                         Types Declaration                                   *
 *******************************************************************************/
typedef enum
{
	NEW_PASSWORD,CHECK_PASSWORD_FOR_NEW_PASSWORD,CHECK_PASSWORD_FOR_OPEN_DOOR,VIEW_OPTIONS,BUZZER_ON,CHANGE_PASSWORD,OPEN_DOOR
}SystemState;


typedef enum
{
	UNLOCKING=7,LOCKING,OPENED,CLOSED
}Door_State;

typedef enum
{
	WRONG_PASSWORD=11,CORRECT_PASSWORD
}PasswordState;
/*******************************************************************************
 *                           Global Variables                                  *
 *******************************************************************************/
uint8 g_systemState;
/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/
static uint8 createNewPassword(void);
static void viewOptions(void);
static void getPassword(void);
static void opendoor(void);




int main(void){
	LCD_init();
	UART_ConfigType config_uart = {BITS_8,DISABLED,BIT_1,9600};
	UART_init(&config_uart);
	while(1){
		UART_sendByte(MC1_READY);
		g_systemState = UART_receiveByte();
		LCD_clearScreen();
		switch(g_systemState){
		case NEW_PASSWORD:
			createNewPassword();
			break;
		case VIEW_OPTIONS:
			viewOptions();
			break;
		case CHECK_PASSWORD_FOR_OPEN_DOOR:
		case CHECK_PASSWORD_FOR_NEW_PASSWORD:
			getPassword();
			break;
		case OPEN_DOOR:
			opendoor();
			break;
		case BUZZER_ON:
			LCD_clearScreen();
			LCD_displayString("Calling 911...");
			LCD_moveCursor(1,0);
			LCD_displayString("RUN!!!!!!");
			break;

		}
	}
}

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/

static uint8 createNewPassword(void){
	uint8 passCounter,state = FALSE;
	uint8 password[8];
	uint8 confirmPass[8];

	do{
		LCD_clearScreen();
		LCD_displayString("Enter New Pass");
		LCD_moveCursor(1,0);
		/* Take the password from user*/
		for(passCounter = 0;passCounter<5;passCounter++)
		{
			password[passCounter] = KEYPAD_getPressedKey();
			_delay_ms(350);
			LCD_displayCharacter('*');
		}
		password[passCounter]='#';
		password[++passCounter]='\0';
		LCD_clearScreen();
		LCD_displayString("Confirm Pass");
		LCD_moveCursor(1,0);
		/* Re-take the password to confirm it*/
		for(passCounter = 0;passCounter<5;passCounter++)
		{
			confirmPass[passCounter]=KEYPAD_getPressedKey();
			_delay_ms(350);
			LCD_displayCharacter('*');
		}
		confirmPass[passCounter]='#';
		confirmPass[++passCounter]='\0';
		state=FALSE;
		for(passCounter =0;passCounter<5;passCounter++){
			/*check the password is correct or not*/
			if(password[passCounter] != confirmPass[passCounter])
			{
				state=TRUE;
			}
		}
		if(state)
		{
			LCD_clearScreen();
			LCD_displayStringRowColumn(0,2,"Password Not");
			LCD_moveCursor(1,0);
			LCD_displayString("Match try again");
			_delay_ms(1500);
		}
	}while(state);

	/*Send the password to MC2*/
	LCD_clearScreen();
	LCD_displayString("Saving Password");
	_delay_ms(1500);
	while(UART_receiveByte() !=MC2_READY);
	UART_sendString(password);
	LCD_clearScreen();
	LCD_displayStringRowColumn(0,5,"Saved");
	LCD_displayStringRowColumn(1,2,"Successfully");
	_delay_ms(1500);
	return SUCCESS;

}

static void viewOptions(void){
	uint8 option;
	LCD_clearScreen();
	LCD_displayString("0:Open Door");
	LCD_moveCursor(1,0);
	LCD_displayString("1:Change Pass");
	/*loop until the user enter one of the two options*/
	do{
		option = KEYPAD_getPressedKey();
		_delay_ms(350);
	}while(option != 22 && option != 1);

	/*send the option to MC2*/
	while(UART_receiveByte() != MC2_READY);
	if(option == 1){
		UART_sendByte(CHECK_PASSWORD_FOR_NEW_PASSWORD);
	}
	else{
		UART_sendByte(OPEN_DOOR);
	}

}

static void getPassword(void){
	uint8 passCounter;
	uint8 password[8];
	LCD_clearScreen();
	LCD_displayString("Enter The Pass");
	LCD_moveCursor(1,0);
	/*Taking the password from the user*/
	for(passCounter = 0;passCounter<5;passCounter++)
	{
		password[passCounter] = KEYPAD_getPressedKey();
		_delay_ms(350);
		LCD_displayCharacter('*');
	}
	password[passCounter]='#';
	password[++passCounter]='\0';
	/*Sending the password to MC2 to check it */
	while(UART_receiveByte() !=MC2_READY);
	UART_sendString(password);
	/*Receiving the password confirmation*/
	UART_sendByte(MC1_READY);
	if(UART_receiveByte() == WRONG_PASSWORD)
	{
		LCD_clearScreen();
		LCD_displayString("WRONG PASSWORD!");
		LCD_moveCursor(1,0);
		_delay_ms(1500);
	}

}

static void opendoor(void){
	uint8 state;
	UART_sendByte(MC1_READY);
	state = UART_receiveByte();
	if(state == UNLOCKING)
	{
		LCD_clearScreen();
		LCD_displayString("UNLOCKING...");
		LCD_moveCursor(1,0);
	}
	UART_sendByte(MC1_READY);
	state = UART_receiveByte();
	if(state == OPENED)
	{
		LCD_clearScreen();
		LCD_displayString("Door Opened");
	}
	UART_sendByte(MC1_READY);
	state = UART_receiveByte();
	if(state == LOCKING)
	{
		LCD_clearScreen();
		LCD_displayString("LOCKING...");
	}
}



