/*
 * synth.h
 *
 *  Created on: 22 Apr 2017
 *      Author: dimtass
 */

#ifndef DDS_SYNTH_H_
#define DDS_SYNTH_H_

#include "dds_defs.h"

void DDS_Init(void);
void DDS_calculate(uint16_t * buffer, uint16_t buffer_size, float frequency);

#endif /* SYNTH_SYNTH_H_ */
