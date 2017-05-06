/*
 * internal_dac.h
 *
 *  Created on: 22 Apr 2017
 *      Author: dimtass
 */

#ifndef INTERNAL_DAC_H_
#define INTERNAL_DAC_H_

typedef enum {
	DAC_CHANNEL_1 = 0,
	DAC_CHANNEL_2
} en_dac_channel;

void DAC_init(void);

#endif /* INTERNAL_DAC_H_ */
