/**
 ********************************************************************************
 * @file    main.c
 * @author  Saimon Collaku
 ********************************************************************************
 */


/************************************
 * INCLUDES
 ************************************/
#include "gestione_uart.h"
#include "gestione_polling.h"
#include "emulazione_encoder.h"
#include "side.h"
#include "platform.h"

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
