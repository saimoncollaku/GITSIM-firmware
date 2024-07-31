/**
 ******************************************************************************
 * @file    side.c
 * @author  Saimon Collaku
 ******************************************************************************
 */

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "side.h"
#include "gestione_polling.h"
#include "gestione_uart.h"
#include "emulazione_encoder.h"


/******************************************************************************
 * PRIVATE MACROS AND DEFINES
 *****************************************************************************/

/** @brief Tempo di campionamento del side loop secondario */
#define T_SIDE_SECONDARIO 		0.05f


/******************************************************************************
 * STATIC VARIABLES
 *****************************************************************************/

/**
 * @brief Contatore di side loop fatti, usato per entrare nel side loop
 * secondario
 */
static uint32_t counter_side_secondario;

/** @brief Istanza del timer che dà il callback al side loop */
static XScuTimer istanza_timer_da_resettare;

/**
 * @brief Numero di ingressi in side loop che bisogna fare per entrare nel
 * side secondario (lento)
 */
static uint32_t n_loop_side_secondario = UINT32_MAX;


/******************************************************************************
 * SIDE LOOP
 *****************************************************************************/
void side_loop(void *CallBack_Timer)
{
	/* Resetto il flag di interrupt dal timer */
	XScuTimer_ClearInterruptStatus(&istanza_timer_da_resettare);

	/* Controllo se ho fatto abbastanza loop per entrane nel secondario */
	if (counter_side_secondario == (n_loop_side_secondario - 1U))
	{
		/* Azioni del side loop secondario */
		manda_telegramma_di_risposta();
		reset_conteggi_encoder();
		counter_side_secondario = 0;
	}
	else
	{
		counter_side_secondario++;
	}

	/* Azioni del side loop principale */
	aggiorna_variabili_encoder();
	emula_sensori_encoder();
}


/******************************************************************************
 * INIZIALIZZAZIONE SIDE
 *****************************************************************************/
void inizializza_side_loop()
{
	counter_side_secondario = 0;
	istanza_timer_da_resettare = ritorna_istanza_timer();
	float_t t_polling_side = ritorna_tempo_del_polling();

	/* Creo la variabile temporanea per MISRA-2023 */
	float_t n_temp = T_SIDE_SECONDARIO / t_polling_side;
	n_loop_side_secondario = (uint32_t) n_temp;
}


/****************** (C) COPYRIGHT GITSIM ***** END OF FILE *******************/
