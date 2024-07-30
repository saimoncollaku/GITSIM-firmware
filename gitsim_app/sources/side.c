/**
 ********************************************************************************
 * @file    side.c
 * @author  Saimon Collaku
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
#define T_SIDE_LENTO 		0.05f


/************************************
 * STATIC VARIABLES
 ************************************/
static uint32_t counter_side_secondario;
static XScuTimer istanza_timer;
static uint32_t n_polls_side_lento = 999999999;


/************************************
 * SIDE
 ************************************/
void side_loop(void *CallBack_Timer)
{
	/* Rimuovi flag di interrupt */
	XScuTimer_ClearInterruptStatus(&istanza_timer);

	if (counter_side_secondario == (n_polls_side_lento - 1U))
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


/************************************
 * INIZIALIZZAZIONE SIDE
 ************************************/
void inizializza_side_loop()
{
	counter_side_secondario = 0;
	istanza_timer = ritorna_istanza_timer();
	float_t t_polling_side = ritorna_tempo_del_polling();
	float_t n_polls_misra = T_SIDE_LENTO / t_polling_side;
	n_polls_side_lento = (uint32_t) n_polls_misra;
}
