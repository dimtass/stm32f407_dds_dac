/*
 * synth.c
 *
 *  Created on: 22 Apr 2017
 *      Author: dimtass
 */

#include <math.h>
#include "platform_config.h"
#include "dds.h"

#define SAMPLE_RATE		DDS_SAMPLE_RATE
#define TABLE_SIZE 		1024						// size of wavetable
#define TABLE_FREQ		(SAMPLE_RATE / TABLE_SIZE)	// frequency of one wavetable period

/* Math definitions */
#define _2PI            6.283185307f
#define _PI             3.14159265f

/* Q16.16 */
#define FRAC		16
#define Q16(X)		(X * (float)(1<<FRAC))
#define QFLOAT(X)	(X / (float)(1<<FRAC))
#define Q16_TO_UINT(X)	((uint32_t)X / (1<<FRAC))


static uint32_t _phase_accumulator = 0; // fixed-point (16.16) phase accumulator
static float _sine[TABLE_SIZE+1];	// +1 for interpolation

void sine_init(void)
{
	// populate table table[k]=sin(2*pi*k/N)
	int i = 0;
	for(i = 0; i < TABLE_SIZE; i++) {
		// calculating sine wave
		_sine[i] = sinf(_2PI * ((float)i/TABLE_SIZE));
	}
	/* set the last byte equal to first for interpolation */
	_sine[TABLE_SIZE] = _sine[0];
	_phase_accumulator = 0;
}

void DDS_Init(void)
{
	sine_init();
}

void DDS_calculate(uint16_t * buffer, uint16_t buffer_size, float frequency)
{
	uint32_t phaseIncrement = Q16((float)frequency*TABLE_SIZE / SAMPLE_RATE);
	uint32_t index = 0;
	int i = 0;

	for(i=0; i<buffer_size; i++)
    {
		/* Increment the phase accumulator */
		_phase_accumulator += phaseIncrement;

		_phase_accumulator &= TABLE_SIZE*(1<<16) - 1;

		index = _phase_accumulator >> 16;

		/* interpolation */
		float v1 = _sine[index];
		float v2 = _sine[index+1];
		float fmul = (_phase_accumulator & 65535)/65536.0f;
		float out = v1 + (v2-v1)*fmul;

		buffer[i] = (uint16_t)(2048 * out + 2047) ;
    }
}
