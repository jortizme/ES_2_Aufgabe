*
 * balancer.h
 *
 *  Created on: 25.08.2016
 *      Author: Michael
 */

#ifndef BALANCER_H_
#define BALANCER_H_

#include "digitalinout.h"
#include "mma7455.h"
#include "sr74lvc595.h"

class ACCSystem	: public Platform::BSP::MMA7455 {

private:

	uint32_t STResponseTime = 20000;
	uint8_t STOutMin = 32;
	uint8_t STOutMax = 83;

public:

	ACCSystem();

/*! @brief Runs the Selft-Test function of the Accelerometer
		*  @return @c true if test passed, false if not
		 */
	bool selftest();

	/*! @brief Check if SW2 is pressed, debounce implemented
		 *  @param[in] Center button as a class object
		 *  @return @c true if it´s pressed, false if not
		 */
	bool SW2isPressed(Platform::BSP::DigitalInOut& Taster);

	/*! @brief Set the zero position values
		 *  @param[out] Array of zero position directions (x,y,z)
		 *  @return @c false if calibration failed
		 */
	bool Calibrate(int8_t *refValues);

	/*! @brief Set the zero position values
		 *  @param[in] 7SegmentAnzeige as a class object
		 *  @param[out] Array of current directions(x,y,z)
		 *  param[out] Array of zero position directions (x,y,z)
		 *  @return @c false if the calculation failed
		 */
	bool ShowValues(Platform::BSP::SR74LVC595& Anzeige, int8_t *actualValues, int8_t *refValues);

};
/* --Single Thread Loop. */
void balancerLoop(void const* arg);

#endif /* BALANCER_H_ */
