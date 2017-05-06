/*
 * internal_dac.c
 *
 *  Created on: 22 Apr 2017
 *      Author: dimtass
 */

#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_dma.h"
#include "stm32f4xx_dac.h"
#include "platform_config.h"
#include "internal_dac.h"
#include "dds.h"


#define APB1_TIM_FREQ		84000000	// TIM6 counter clock (prescaled APB1)
#define DAC_SAMPLE_RATE		DDS_SAMPLE_RATE

#define DAC_DHR12R1_ADDR    0x40007408	// DMA1 writes into this reg on every request
#define DAC_DHR12R2_ADDR    0x40007414	// DMA2 writes into this reg on every request


static DAC_InitTypeDef DAC1_InitStructure = {0};
static DMA_InitTypeDef DMA1_InitStructure = {0};
static DAC_InitTypeDef DAC2_InitStructure = {0};
static DMA_InitTypeDef DMA2_InitStructure = {0};

void DAC_GPIO_Init(void);
void DAC_set_defaults(uint8_t channel);
void TIM6_Init(void);

void DAC_init(void)
{
	DAC_GPIO_Init();
	DAC_set_defaults(DAC_CHANNEL_1);
//	DAC_set_defaults(DAC_CHANNEL_2);
	TIM6_Init();
}

void DAC_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Configure DAC1 pinout */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void DAC_set_defaults(uint8_t channel)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	DAC_InitTypeDef * DAC_InitStructure = NULL;
	DMA_InitTypeDef * DMA_InitStructure = NULL;
	DMA_Stream_TypeDef * dma_stream = NULL;
	uint32_t dac_channel = 0;
	uint32_t DMA_PeripheralBaseAddr = 0;
	uint8_t NVIC_IRQChannel = 0;

	/* Enable clocks for port A and DAC */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_DAC, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1, ENABLE);

	TRACE(("Initializing channel: %d\n", channel));

	/* initialize DMA struct */
	if (channel == DAC_CHANNEL_1) {
		DAC_InitStructure = &DAC1_InitStructure;
		DMA_InitStructure = &DMA1_InitStructure;
		dac_channel = DAC_Channel_1;
		dma_stream = DMA1_Stream5;
		DMA_PeripheralBaseAddr = (uint32_t)DAC_DHR12R1_ADDR;
		NVIC_IRQChannel = DMA1_Stream5_IRQn;
	}
	else if (channel == DAC_CHANNEL_2) {
		DAC_InitStructure = &DAC2_InitStructure;
		DMA_InitStructure = &DMA2_InitStructure;
		dac_channel = DAC_Channel_2;
		dma_stream = DMA1_Stream6;
		DMA_PeripheralBaseAddr = (uint32_t)DAC_DHR12R2_ADDR;
		NVIC_IRQChannel = DMA1_Stream6_IRQn;
	}
	else {
		return;
	}
	/* Initialize DMA & DAC structrs */
	DMA_StructInit(DMA_InitStructure);
	DAC_StructInit(DAC_InitStructure);

	/* DAC channel Configuration */
	DAC_InitStructure->DAC_Trigger        = DAC_Trigger_T6_TRGO;		// Select TIM6 for convertion timer
	DAC_InitStructure->DAC_WaveGeneration = DAC_WaveGeneration_None;	// Do not auto-generate output
	DAC_InitStructure->DAC_OutputBuffer   = DAC_OutputBuffer_Enable;
	DAC_Init(dac_channel, DAC_InitStructure);

	/* DMA configuration */
	DMA_DeInit(dma_stream);
	DMA_InitStructure->DMA_Channel            = DMA_Channel_7;
	DMA_InitStructure->DMA_PeripheralBaseAddr = DMA_PeripheralBaseAddr;
	DMA_InitStructure->DMA_Memory0BaseAddr    = (uint32_t)&glb.dds_buff_0[channel]; // First buffer
	DMA_InitStructure->DMA_BufferSize         = DDS_BUFF_SIZE;

	DMA_InitStructure->DMA_DIR                = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure->DMA_PeripheralInc      = DMA_PeripheralInc_Disable;
	DMA_InitStructure->DMA_MemoryInc          = DMA_MemoryInc_Enable;
	DMA_InitStructure->DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure->DMA_MemoryDataSize     = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure->DMA_Mode               = DMA_Mode_Circular;
	DMA_InitStructure->DMA_Priority           = DMA_Priority_High;
	DMA_InitStructure->DMA_FIFOMode           = DMA_FIFOMode_Enable;
	DMA_InitStructure->DMA_FIFOThreshold      = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure->DMA_MemoryBurst        = DMA_MemoryBurst_INC4;
	DMA_InitStructure->DMA_PeripheralBurst    = DMA_PeripheralBurst_Single;
	DMA_Init(dma_stream, DMA_InitStructure);

	/* Set secondary buffer */
	DMA_DoubleBufferModeConfig(dma_stream, (uint32_t)&glb.dds_buff_1[channel], DMA_Memory_0);	//0 buffer is the first
	/* Enable DMA interrupts */
	DMA_ITConfig(dma_stream, DMA_IT_TC, ENABLE);
	/* Enable double buffer mode */
	DMA_DoubleBufferModeCmd(dma_stream, ENABLE);

	/* Configure interrupts */
	NVIC_InitStructure.NVIC_IRQChannel = NVIC_IRQChannel;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	/* Enable DMA1_Stream5 */
	DMA_Cmd(dma_stream, ENABLE);
	/* Enable DAC Channel1 */
	DAC_Cmd(dac_channel, ENABLE);
	/* Enable DMA for DAC Channel1 */
	DAC_DMACmd(dac_channel, ENABLE);
}

void DMA1_Stream5_IRQHandler(void)
{
	if (DMA_GetFlagStatus(DMA1_Stream5, DMA_FLAG_TCIF5) != RESET)
	{
		/* Enable DMA1_Stream5 */
		DMA_ClearFlag(DMA1_Stream5, DMA_FLAG_TCIF5);
		if ((DMA1_Stream5->CR & DMA_SxCR_CT) == 0)//get number of current buffer
		{
			PIN_DEBUG_PORT->ODR |= PIN_DEBUG1;
			/* calculate new buffer 0, while buffer 1 is transmitting */
			DDS_calculate(glb.dds_buff_1[DAC_CHANNEL_1], DDS_BUFF_SIZE, glb.ch1_freq);
		}
		else {
			PIN_DEBUG_PORT->ODR &= ~PIN_DEBUG1;
			/* calculate new buffer 0, while buffer 0 is transmitting */
			DDS_calculate(glb.dds_buff_0[DAC_CHANNEL_1], DDS_BUFF_SIZE, glb.ch1_freq);
		}
	}
}

void DMA1_Stream6_IRQHandler(void)
{
	if (DMA_GetFlagStatus(DMA1_Stream6, DMA_FLAG_TCIF6) != RESET)
	{
		DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);
		if ((DMA1_Stream6->CR & DMA_SxCR_CT) == 0)//get number of current buffer
		{
			PIN_DEBUG_PORT->ODR |= PIN_DEBUG2;
			DDS_calculate(glb.dds_buff_1[DAC_CHANNEL_2], DDS_BUFF_SIZE, glb.ch2_freq);
		}
		else {
			PIN_DEBUG_PORT->ODR &= ~PIN_DEBUG2;
			/* calculate new buffer 0, while buffer 0 is transmitting */
			DDS_calculate(glb.dds_buff_0[DAC_CHANNEL_2], DDS_BUFF_SIZE, glb.ch2_freq);
		}
	}
}

void TIM6_Init(void)
{
  TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);

  TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
  TIM_TimeBaseStructure.TIM_Period        = (uint16_t) (APB1_TIM_FREQ/DAC_SAMPLE_RATE) - 1; //TIM_PERIOD;
  TIM_TimeBaseStructure.TIM_Prescaler     = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode   = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);

  /* TIM6 TRGO selection */
  TIM_SelectOutputTrigger(TIM6, TIM_TRGOSource_Update);

  /* TIM6 enable counter */
  TIM_Cmd(TIM6, ENABLE);
}

