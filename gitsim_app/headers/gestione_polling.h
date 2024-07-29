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
 * TYPEDEFS
 ************************************/

/************************************
 * EXPORTED VARIABLES
 ************************************/

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
