/***************************************************************************//**
 * @file
 * @brief FreeRTOS Blink Demo for Energy Micro EFM32GG_STK3700 Starter Kit
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
//#include <my_file_I2C.c>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "croutine.h"

#include "em_chip.h"
#include "bsp.h"
#include "bsp_trace.h"

#include "sleep.h"

#include "semphr.h"
#include "my_file.h"

#define STACK_SIZE_FOR_TASK    (configMINIMAL_STACK_SIZE + 10)
#define TASK_PRIORITY          (tskIDLE_PRIORITY + 1)

// Implementaciones
#define QUEUE_LENGTH 10
QueueHandle_t queue1;
QueueHandle_t queue2;

/* Structure with parameters for LedBlink */
typedef struct {
  /* Delay between blink of led */
  portTickType delay;
  /* Number of led */
  int          ledNo;
} TaskParams_t;

/***************************************************************************//**
 * @brief Simple task which is blinking led
 * @param *pParameters pointer to parameters passed to the function
 ******************************************************************************/
static void LedBlink(void *pParameters)
{
  TaskParams_t     * pData = (TaskParams_t*) pParameters;
  const portTickType delay = pData->delay;

  for (;; ) {
    BSP_LedToggle(pData->ledNo);
    vTaskDelay(delay);
  }
}

static void comprovacio(void *pParameters)
{

  for (;; ) {
	bool funciona=I2C_Test_1(0xFF);
	if(funciona){
    printf("funciona\n");
	}
	else{
		printf("no funciona\n");
	}
  }
}


static void lectura(void *pParameters)
{
	uint8_t fifo=10;
	for (;; ) {

		xQueueSend(queue1,&fifo,portMAX_DELAY);

		if (fifo<150){
			fifo+=10;
		}
		else{
			fifo=10;
		}

		vTaskDelay(pdMS_TO_TICKS(1000));



	}
}
static void interpretacio_dades(void *pParameters)
{
	for (;; ) {

		uint8_t read;
		bool led_active=false;
		xQueueReceive(queue1,&read,portMAX_DELAY);

		if( read<100){

			xQueueSend(queue2,&led_active,portMAX_DELAY);

		}
		else{
			led_active=true;
			xQueueSend(queue2,&led_active,portMAX_DELAY);
		}


	}
}

static void activacio(void *pParameters)
{
	for (;; ) {
		bool activar_led;
		xQueueReceive(queue2,&activar_led,portMAX_DELAY);


		I2C_WriteRegister(0x09, 0x07);

		I2C_WriteRegister(0x11, 0x03); // Activar bit para encender LED verde


		if(activar_led==true){

		I2C_WriteRegister(0x11, 0x10);// Activar bit para encender LED red

		I2C_WriteRegister(0x0C, 0x7F);//Intensidad LED rojo
		}
		else{

			I2C_WriteRegister(0x11, 0x03); // Activar bit para encender LED verde

			I2C_WriteRegister(0x0E, 0x7F); //Intensidad LED verde
		}



	}

}

/***************************************************************************//**
 * @brief  Main function
 ******************************************************************************/


int main(void)
{
  /* Chip errata */
  CHIP_Init();
  /* If first word of user data page is non-zero, enable Energy Profiler trace */
  BSP_TraceProfilerSetup();
  BSP_TraceSwoSetup();

  /* Initialize LED driver */
  BSP_LedsInit();
  /* Setting state of leds*/
  BSP_LedSet(0);
  BSP_LedSet(1);

  /* Initialize SLEEP driver, no calbacks are used */
  SLEEP_Init(NULL, NULL);
  #if (configSLEEP_MODE < 3)
    /* do not let to sleep deeper than define */
    SLEEP_SleepBlockBegin((SLEEP_EnergyMode_t)(configSLEEP_MODE + 1));
  #endif
  uint8_t addr = 0xAE;
  BSP_I2C_Init(addr);

  /* Parameters value for taks*/
  static TaskParams_t parametersToTask1 = { pdMS_TO_TICKS(1000), 0 };
  static TaskParams_t parametersToTask2 = { pdMS_TO_TICKS(500), 1 };


  /*Create two task for blinking leds*/
  xTaskCreate(LedBlink, (const char *) "LedBlink1", STACK_SIZE_FOR_TASK, &parametersToTask1, TASK_PRIORITY, NULL);
  xTaskCreate(LedBlink, (const char *) "LedBlink2", STACK_SIZE_FOR_TASK, &parametersToTask2, TASK_PRIORITY, NULL);

 // xTaskCreate(comprovacio, (const char *) "Comprovacio1", STACK_SIZE_FOR_TASK, NULL, TASK_PRIORITY, NULL);

  queue1 = xQueueCreate(QUEUE_LENGTH, sizeof(uint8_t));
  queue2 = xQueueCreate(QUEUE_LENGTH, sizeof(uint8_t));


  xTaskCreate(lectura, (const char *) "lectura1", STACK_SIZE_FOR_TASK, NULL, TASK_PRIORITY, NULL);
  xTaskCreate(interpretacio_dades, (const char *) "interpretacio1", STACK_SIZE_FOR_TASK, NULL, TASK_PRIORITY, NULL);
  xTaskCreate(activacio, (const char *) "activacio1", STACK_SIZE_FOR_TASK, NULL, TASK_PRIORITY, NULL);
 // xTaskCreate(llegir_dades, (const char *) "lectura", STACK_SIZE_FOR_TASK, NULL, TASK_PRIORITY, NULL);


  /*Start FreeRTOS Scheduler*/
  vTaskStartScheduler();

  return 0;
}

int _write(int file, const char *ptr, int len) {
    int x;
    for (x = 0; x < len; x++) {
       ITM_SendChar (*ptr++);
    }
    return (len);
}
