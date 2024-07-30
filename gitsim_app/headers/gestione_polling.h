/**
 ********************************************************************************
 * @file    gestione_polling.h
 * @author  Saimon Collaku
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

#include "xscutimer.h"
#include "math.h"

/************************************
 * MACROS AND DEFINES
 ************************************/

/*
 * Formula per ottenere il periodo di polling corretto:
 * T = {(PSC + 1)*(LV + 1)} / {CPU_CLK/2}
 */


#define TIMER_PSC		(19U + 1U) /**< Prescaler del timer, serve a
                                definire il valore massimo su cui il timer
                                si resetta */

#define TIMER_LV 		(64U + 1U) /**< Valore di load del timer, serve a
                                definire il valore massimo su cui il timer
                                si resetta */

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void inizializza_polling_timer(void);
XScuTimer ritorna_istanza_timer(void);
float_t ritorna_tempo_del_polling(void);


#ifdef __cplusplus
}
#endif

#endif 
