/**
 ********************************************************************************
 * @file    side.c
 * @author  Saimon Collaku
 * @date
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "side.h"
#include "gestione_polling.h"
#include "gestione_uart.h"
#include "emulazione_encoder.h"


/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define T_SIDE_LENTO 		0.05
#define N_CLOCK_SIDE_LENTO	((float_t) T_SIDE_LENTO) / ((float_t) T_POLLING)




/************************************
 * STATIC VARIABLES
 ************************************/

static uint32_t counter_side_secondario = 0;


/************************************
 * SIDE
 ************************************/
void side(void *CallBack_Timer)
{
	/* Rimuovi flag di interrupt */
	XScuTimer_ClearInterruptStatus(&IstanzaTimer);

	if (counter_side_secondario == N_CLOCK_SIDE_LENTO - 1)
	{
		manda_telegramma_di_risposta();
		reset_conteggi_encoder();
		counter_side_secondario = 0;
	}
	else
	{
		counter_side_secondario++;
	}

	aggiorna_variabili_encoder();
	emula_sensori_encoder();
}

