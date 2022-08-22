/******************************************************************************
 *
 * File Name: MC2.c
 *
 * Description: Source file for micro controller 2
 *
 * Author: Omar hossam ElDin
 *
 *******************************************************************************/


#include "uart.h"
#include "buzzer.h"
#include "motor.h"
#include "timer.h"
#include "external_eeprom.h"
#include <avr/io.h>
#include <util/delay.h>

/*******************************************************************************
 *                      Preprocessor Macros                                    *
 *******************************************************************************/
#define SUCCESS 1
#define FAILURE 0
#define MC1_READY 0x10
#define MC2_READY 0x10
#define PASSWORD_ADDRESS 0x0004
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
uint8 g_doorState;
uint8 g_loginCounter=0;
uint8 g_password[8];
/*******************************************************************************
 *                      Functions Prototypes                                   *
 *******************************************************************************/
uint8 checkPassword(void);
void openDoor(void);
void buzzerOn(void);
void changeDoorState(void);
void changeSystemState(void);

int main(void){
	UART_ConfigType config_uart = {BITS_8,DISABLED,BIT_1,9600};
		UART_init(&config_uart);

	UART_init(&config_uart);
	SREG |= (1<<7);
	if(g_loginCounter == 0){
		g_systemState = NEW_PASSWORD;
		g_loginCounter++;
		/******************** g_logincounter must be saved in eeprom ************************/
	}
	else{
		g_systemState = VIEW_OPTIONS;
		EEPROM_readString(PASSWORD_ADDRESS,g_password);
	}
	while(1)
	{while(UART_receiveByte() !=MC1_READY);
	UART_sendByte(g_systemState);
	switch(g_systemState){
	case NEW_PASSWORD:
		UART_sendByte(MC2_READY);
		UART_receiveString(g_password);
		EEPROM_writeString(PASSWORD_ADDRESS,g_password);
		g_systemState = VIEW_OPTIONS;
		break;
	case VIEW_OPTIONS:
		UART_sendByte(MC2_READY);
		if(UART_receiveByte() == OPEN_DOOR){
			g_systemState =CHECK_PASSWORD_FOR_OPEN_DOOR;
		}
		else{
			g_systemState = CHECK_PASSWORD_FOR_NEW_PASSWORD;
		}
		break;
	case CHECK_PASSWORD_FOR_OPEN_DOOR:
		if(checkPassword() == SUCCESS){
			openDoor();
		}
		break;
	case CHECK_PASSWORD_FOR_NEW_PASSWORD:
		if(checkPassword() == SUCCESS){
			g_systemState = NEW_PASSWORD;
		}
		break;
	case BUZZER_ON:
		buzzerOn();
		break;

	}
	}
}

/*******************************************************************************
 *                      Functions Definitions                                  *
 *******************************************************************************/
uint8 checkPassword(void){
	static uint8 fail_time=0;
	uint8 passCounter;
	uint8 userpassword[8];
	UART_sendByte(MC2_READY);
	UART_receiveString(userpassword);
	EEPROM_readString(PASSWORD_ADDRESS,g_password);
	for(passCounter = 0;passCounter < 6;passCounter++){
		if(g_password[passCounter] != userpassword[passCounter])
		{
			fail_time++;
			while(UART_receiveByte()!= MC1_READY);
			UART_sendByte(WRONG_PASSWORD);
			if(fail_time == 3)
			{
				g_systemState = BUZZER_ON;
				fail_time = 0;
			}
			return FAILURE;

		}
	}
	while(UART_receiveByte()!= MC1_READY);
	UART_sendByte(CORRECT_PASSWORD);
	fail_time = 0;
	return SUCCESS;
}

void openDoor(void){
	DcMotor_Init();
	TIMER_setCallBackT1(changeDoorState);
	TIMER_ConfigType timer1_config = {TIMER1,COMPARE,F_CPU_1024,OCN_DISCONNECTED,COMPARE_A,0,4000};
	TIMER_init(&timer1_config);
	g_doorState = UNLOCKING;
	while(UART_receiveByte() != MC1_READY);
	UART_sendByte(OPEN_DOOR);
	while(UART_receiveByte() != MC1_READY);
	UART_sendByte(g_doorState);
	DcMotor_Rotate(CW);
	while(g_doorState == UNLOCKING);
	DcMotor_Rotate(STOP);
	while(UART_receiveByte() != MC1_READY);
	UART_sendByte(g_doorState);
	while(g_doorState == OPENED);
	while(UART_receiveByte() != MC1_READY);
	UART_sendByte(g_doorState);
	DcMotor_Rotate(A_CW);
	while(g_doorState == LOCKING);
	DcMotor_Rotate(STOP);
	TIMER_DeInit(TIMER1);
	g_systemState = VIEW_OPTIONS;
}

void buzzerOn(void){
	BUZZER_on();
	TIMER_setCallBackT1(changeSystemState);
	TIMER_ConfigType timer1_config = {TIMER1,COMPARE,F_CPU_1024,OCN_DISCONNECTED,COMPARE_A,0,4000};
	TIMER_init(&timer1_config);
	_delay_ms(100);
	while(g_systemState == BUZZER_ON);
	BUZZER_off();
	TIMER_DeInit(TIMER1);
}

void changeDoorState(void){
	static uint8 ticks = 0;
	ticks++;
	if(ticks == 15)
	{
		g_doorState = OPENED;
	}
	else if(ticks == 18)
	{
		g_doorState = LOCKING;
	}
	else if(ticks == 33){
		g_doorState = CLOSED;
		ticks = 0;
	}
}

void changeSystemState(void){
	static uint8 ticks1 =0;
	ticks1++;
	if(ticks1 == 60)
	{g_systemState = VIEW_OPTIONS;}
}
