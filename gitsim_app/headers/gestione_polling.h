/**
 ********************************************************************************
 * @file    gestione_polling.h
 * @author  wasab
 * @date    5 Jul 2024
 * @brief   
 ********************************************************************************
 */

#ifndef HEADERS_GESTIONE_POLLING_H_
#define HEADERS_GESTIONE_POLLING_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include "stdio.h"
#include "stdint.h"
#include "xscutimer.h"
#include "ps7_init.h"

/************************************
 * MACROS AND DEFINES
 ************************************/

/*
 * Formula per ottenere il periodo di polling corretto:
 * T = {(PSC + 1)*(LV + 1)} / {CPU_CLK/2}
 */


#define TIMER_PRESCALER			(19 + 1) /**< Prescaler del timer, serve a
                                definire il valore massimo su cui il timer
                                si resetta */

#define TIMER_LOAD_VALUE 		(64 + 1) /**< Valore di load del timer, serve a
                                definire il valore massimo su cui il timer
                                si resetta */
#define NUM_T_POLLING	((float_t) (TIMER_PRESCALER * TIMER_LOAD_VALUE))
#define DEN_T_POLLING	((float_t) (APU_FREQ / 2))
#define T_POLLING 		NUM_T_POLLING / DEN_T_POLLING


/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXPORTED VARIABLES
 ************************************/
extern XScuTimer IstanzaTimer;

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
int32_t inizializza_polling_timer(void);

#ifdef __cplusplus
}
#endif

#endif 
