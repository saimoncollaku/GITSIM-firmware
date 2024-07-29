/**
 ********************************************************************************
 * @file    main.c
 * @author  wasab
 * @date    5 Jul 2024
 * @brief
 ********************************************************************************
 */


/************************************
 * INCLUDES
 ************************************/
#include "gestione_uart.h"
#include "gestione_polling.h"
#include "emulazione_encoder.h"
#include "main.h"
#include "side.h"
#include "platform.h"
#include "xparameters.h"
#include <stdbool.h>


/************************************
 * EXTERN VARIABLES
 ************************************/


/************************************
 * MAIN
 ************************************/
int main()
{
	/* Inizializzazione*/
	init_platform();
	inizializza_polling_timer();
	inizializza_side();
	inizializza_uart();
	inizializza_variabili_encoder();

	/* Main loop */
	while(1)
	{
		leggi_telegramma();
	};

	cleanup_platform();
	return 0;
}
