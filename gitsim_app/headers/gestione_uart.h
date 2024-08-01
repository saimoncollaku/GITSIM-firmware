/**
 ********************************************************************************
 * @file    gestione_uart.h
 * @author  Saimon Collaku
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
#include "xil_printf.h"
#include "math.h"
#include <stdbool.h>

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void inizializza_uart(void);

/**
 * @brief Invia un telegramma di risposta all'applicazione via uart
 *
 * Se la connessione con l'applicazione è stata stabilita e l'handshake è
 * avvenuto, questa funzione invia un telegramma di 12 byte contenente
 * le velocità e i conteggi attuali degli encoder e_1 ed e_2.
 *
 * @see e_1, e_2
 * @see encoder.vel
 * @see encoder.conteggio
 */
void manda_telegramma_di_risposta(void);

/**
 * @brief Legge un telegramma dalla UART
 *
 * Questa funzione determina se deve leggere un telegramma di connessione
 * o di funzionamento, basandosi sullo stato attuale della connessione.
 */
void leggi_telegramma(void);

/**
 * @brief Restituisce lo stato della connessione con l'applicazione
 *
 * @return bool True se la connessione è stabilita, False altrimenti
 */
bool ritorna_stato_connessione_app(void);

#ifdef __cplusplus
}
#endif

#endif 
