/**
 ********************************************************************************
 * @file    gestione_uart.h
 * @author  wasab
 * @date    5 Jul 2024
 * @brief   
 ********************************************************************************
 */

#ifndef HEADERS_GESTIONE_UART_H_
#define HEADERS_GESTIONE_UART_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <stdio.h>
#include "xil_printf.h"
#include "xparameters.h"
#include "math.h"
#include <stdbool.h>

/************************************
 * MACROS AND DEFINES
 ************************************/

/************************************
 * TYPEDEFS
 ************************************/

/************************************
 * EXPORTED VARIABLES
 ************************************/



/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void inizializza_uart(void);
void manda_telegramma_di_risposta(void);
void leggi_telegramma(void);
bool ritorna_stato_connessione_app(void);

#ifdef __cplusplus
}
#endif

#endif 
