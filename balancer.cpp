//*
 * balancer.cpp
 *
 *  Created on: 25.08.2016
 *      Author: Michael
 */

#define USE_ENHANCED

/* --Standard include. */
#include <cstdio>
#include <cstdlib>
#include <cinttypes>

#include "balancer.h"

/* --Platform include. */
#include "cmsis_os.h"

using GPIO=Platform::BSP::DigitalInOut;
using SAnzeige=Platform::BSP::SR74LVC595;
enum class ProgrammState{Start,SelfTest, Calibrate, ShowValues};


void balancerLoop(void const* arg) {
	printf("Board Shaker\r\n");

	GPIO Taster(0,17, GPIO::Direction::INPUT);
	ACCSystem ACC;
	SAnzeige Anzeige(SAnzeige::Interface::SSP1,2,2);

	ProgrammState State = ProgrammState::Start;
	ProgrammState NextState;


	if(!ACC.control())
	{
		printf("Accelerometer was not set correctly!\n");
		return;
	}


	int8_t refValues[3];		//x,y,z reference values
	int8_t actualValues[3];		//x,y,z actual values

	for(int i = 0; i < 3; i++)
	{
		refValues[i] = 0;
		actualValues[i] = 0;
	}

	/* --Main loop. */
	while (true) {

		/* State Diagram */
		switch ( State ){

		case ProgrammState::Start:

			printf("\r\nPress the button to start.\r\n");
			while(!ACC.SW2isPressed(Taster));
			NextState = ProgrammState::SelfTest;
			break;

		case ProgrammState::SelfTest:

			printf("\r\nSelfTest is running .....\r\n");

			if(ACC.selftest())
			{
				printf("\r\nTest passed!\r\n");
				NextState = ProgrammState::Calibrate;
			}
			else
			{
				printf("\r\n[ERROR] --> test failed\r\n");
				NextState = ProgrammState::Start;
			}

			break;

		case ProgrammState::Calibrate:

			printf("\r\nPlease lift the board and set it in your desired position\r\n");
			printf("\r\nAfter that, press the Button to set the zero values\r\n");

			while(!ACC.SW2isPressed(Taster));

			if(ACC.Calibrate(refValues)){
				printf("\r\n Calibration completed. \r\n");
				NextState = ProgrammState::ShowValues;
			}
				else
			{
				printf("\r\n[ERROR] --> calibration failed\r\n");
				NextState = ProgrammState::Start;
			}
			break;

		case ProgrammState::ShowValues:

			if(!ACC.ShowValues(Anzeige,actualValues,refValues))
			{
				printf("\r\n[ERROR] --> accelerometer failure\r\n");
				NextState = ProgrammState::Start;
			}

			//Anzeige.off();

			if(ACC.SW2isPressed(Taster))
				NextState = ProgrammState::Start;
			else
				NextState = ProgrammState::ShowValues;

			break;
		}

		State = NextState;

	}
	/* --Thank you for the fish. */
}

ACCSystem::ACCSystem():MMA7455(){};

bool ACCSystem::SW2isPressed(GPIO& Taster)
{
	if(!Taster.get())
	{
		osDelay(100);
		if(!Taster.get())
			return true;
	}
	return false;
}

bool ACCSystem::selftest(){

	bool retvalue = false;

	uint8_t mode= modeControl();
	int8_t valuesBefore[3];
	int8_t valuesAfter[3];

	if(mode != 0xff)
	{
		raw(valuesBefore[0],valuesBefore[1],valuesBefore[2]);
		mode |= (1<<4);
		if(!cmd(Register::MCTL,mode))
				return retvalue;

		osDelay(STResponseTime);

		raw(valuesAfter[0],valuesAfter[1],valuesAfter[2]);
		mode &=~ (1<<4);
		if(!cmd(Register::MCTL,mode))
				return retvalue;

		if((valuesAfter[2] - valuesBefore[2]) > STOutMin && (valuesAfter[2] - valuesBefore[2]) <  STOutMax )
		{
			retvalue = true;
			return retvalue;
		}
	}

	return retvalue;
}

bool ACCSystem::Calibrate(int8_t *refValues)
{
	bool Test;
	Test = raw(refValues[0],refValues[1],refValues[2]);

	//Failure
	if(!Test)
		return Test;


	return Test;
}

bool ACCSystem::ShowValues(SAnzeige& Anzeige, int8_t *actualValues, int8_t *refValues)
{
	bool Test;
	bool isZeroPosition = true;
	/*I wasn't sure for the following values*/
	uint8_t roll_positiv = 0xF7; //A->Bit 3 -> 1000 ->8 oder 0x8
	uint8_t roll_negativ = 0xFE; //D->Bit 0 ->1-> 1 oder 0x1
	uint8_t pitch_positiv = 0x7D; //BC-> Bit 4 und 6 ->0x7D
	uint8_t pitch_negativ = 0xAF; //FE -> Bit 7 und 1 -> 0xAF
	uint8_t zero_Position = 0xFB; //G -> Bit 2 -> 4 oder 0x4


	Test = raw(actualValues[0],actualValues[1],actualValues[2]);

	//Failure
	if(!Test)
		return Test;

	int8_t difference[3];
	Anzeige.off();

	//Calculation
	for(int i = 0; i < 3; i++)
	{
		difference[i] = actualValues[i] - refValues[i];

		if(difference[i] >= 5)
		{
			isZeroPosition = false;			//the z direction influences this variable too

			switch(i){

			case 0:
				Anzeige.set(pitch_negativ);	//pitch negativ
				break;

			case 1:
				Anzeige.set(roll_positiv);	//roll negativ
				break;

			//we don't care for the z position
			default:
				break;
			}
		}
		if(difference[i] <= -5)
				{
					isZeroPosition = false;			//the z direction influences this variable too

					switch(i){

					case 0:
						Anzeige.set(pitch_positiv);	//pitch positiv
						break;

					case 1:
						Anzeige.set(roll_negativ);	//roll positiv
						break;

					//we don't care for the z position
					default:
						break;
					}
				}

	}

	if(isZeroPosition)
	{
		Anzeige.set(zero_Position);
	}

	return Test;
}