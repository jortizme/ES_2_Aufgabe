/*
 * balancer.cpp
 *
 *  Created on: 25.08.2016
 *      Author: Michael
 */

#include "balancer.h"

#define USE_ENHANCED

/* --Standard include. */
#include <cstdio>
#include <cstdlib>
#include <inttypes.h>

/* --Platform include. */
#include "cmsis_os.h"

using GPIO=Platform::BSP::DigitalInOut;
using Acelerometer=Platform::BSP::MMA7455;
using SAnzeige=Platform::BSP::SR74LVC595;
enum class ProgrammState{Start, Calibrate, ShowValues};


/*! @brief Check if SW2 is pressed, debounce implemented
	 *  @param[in] Center button as a class object
	 *  @return @c true if it´s pressed, false if not
	 */
bool SW2isPressed(GPIO& Taster);

/*! @brief Set the zero position values
	 *  @param[in] Acelerometer as a class object
	 *  @param[out] Array of zero position directions (x,y,z)
	 *  @return @c false if calibration failed
	 */
bool Calibrate(Acelerometer& ACC, int8_t *refValues);

/*! @brief Set the zero position values
	 *  @param[in] Accelerometer as a class object
	 *  @param[in] 7SegmentAnzeige as a class object
	 *  @param[out] Array of current directions(x,y,z)
	 *  param[out] Array of zero position directions (x,y,z)
	 *  @return @c false if the calculation failed
	 */
bool ShowValues(Acelerometer& ACC, SAnzeige& Anzeige, int8_t *actualValues, int8_t *refValues);

//
//TODO Für den zweiten Teil der Aufgabe ist eine eigene Klasse mit zusätzl. Selbtestfunktionalität zu entwerfen.
//


void balancerLoop(void const* arg) {
	printf("Board Shaker\r\n");

	GPIO Taster(0,17, GPIO::Direction::INPUT);
	Acelerometer ACC;
	SAnzeige Anzeige(SAnzeige::Interface::SSP1,2,2);

	ProgrammState State = ProgrammState::Start;
	ProgrammState NextState;


	if(!ACC.control())
	{
		printf("Accelerometer was not set correctly!\n");
		return;
	}

	/* Initialisierung von SAnzeige fehlt!!!!!!!!!!!!!!!!*/
	/*In mma7455.cpp  is the implementation hidden through a macro definition!!!*/


	int8_t refValues[3];			//x,y,z reference values
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
			while(!SW2isPressed(Taster));
			NextState = ProgrammState::Calibrate;
			break;

		case ProgrammState::Calibrate:

			printf("\r\nPlease lift the board and set it in your desired position\r\n");
			printf("\r\nAfter that, press the Button to set the zero values\r\n");

			while(!SW2isPressed(Taster));

			if(Calibrate(ACC,refValues))
				NextState = ProgrammState::ShowValues;
			else
			{
				printf("\r\n[ERROR] --> calibration failed\r\n");
				NextState = ProgrammState::Start;
			}
			break;

		case ProgrammState::ShowValues:

			if(!ShowValues(ACC,Anzeige,actualValues,refValues))
			{
				printf("\r\n[ERROR] --> accelerometer failure\r\n");
				NextState = ProgrammState::Start;
			}

			Anzeige.off();

			if(SW2isPressed(Taster))
				NextState = ProgrammState::Start;
			else
				NextState = ProgrammState::ShowValues;

			break;
		}

		State = NextState;

	}
	/* --Thank you for the fish. */
}

bool SW2isPressed(GPIO& Taster)
{
	if(!Taster.get())
	{
		osDelay(100);
		if(!Taster.get())
			return true;
	}
	return false;
}

bool Calibrate(Acelerometer&  ACC, int8_t *refValues)
{
	bool Test;
	Test = ACC.raw(refValues[0],refValues[1],refValues[2]);

	//Failure
	if(!Test)
		return Test;

	printf("(\r\n X has the zero position=" "%" PRId8 "\r\n",refValues[0]);
	printf("(\r\n Y has the zero position=" "%" PRId8 "\r\n",refValues[1]);
	printf("(\r\n Z has the zero position=" "%" PRId8 "\r\n",refValues[2]);

	return Test;
}

bool ShowValues(Acelerometer&  ACC, SAnzeige& Anzeige, int8_t *actualValues, int8_t *refValues)
{
	bool Test;
	bool isZeroPosition = true;
	/*I wasn't sure for the following values*/
	uint8_t G = 0x20;
	uint8_t AD = 0x90;
	uint8_t EFBC = 0x4b;

	Test = ACC.raw(actualValues[0],actualValues[1],actualValues[2]);

	//Failure
	if(!Test)
		return Test;

	int8_t difference[3];

	//Calculation
	for(int i = 0; i < 3; i++)
	{
		difference[i] = actualValues[i] - refValues[i];

		if(difference[i] != 0)
		{
			isZeroPosition = false;			//the z direction influences this variable too

			switch(i){

			case 0:
				Anzeige.set(AD);	//rollen
				break;

			case 1:
				Anzeige.set(EFBC);	//knicken
				break;

			//we don't care for the z position
			default:
				break;
			}
		}
	}

	if(isZeroPosition)
	{
		Anzeige.set(G);
	}

	return Test;
}

