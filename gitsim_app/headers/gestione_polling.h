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

/** @brief Prescaler del timer SCU */
#define TIMER_PSC		(19U + 1U)

/** @brief Valore di ricarica (load value) del timer SCU */
#define TIMER_LV 		(64U + 1U)

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
