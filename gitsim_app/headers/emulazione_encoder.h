/**
 ********************************************************************************
 * @file    emulazione_encoder.h
 * @author  Saimon Collaku
 ********************************************************************************
 */

#ifndef HEADERS_EMULAZIONE_ENCODER_H_
#define HEADERS_EMULAZIONE_ENCODER_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include <math.h>
#include "xgpio.h"
#include <stdbool.h>


/************************************
 * TYPEDEFS
 ************************************/
typedef enum
{
	zero,
	uno,
	due,
	tre,
	incerto
}stato_encoder;


typedef struct
{
  uint16_t duty_A;          /**< Duty cycle del segnale in uscita al canale A,
                                espresso come intero percentuale (fare
                                attenzione perchè bisogna convertirlo in numero
                                moltiplicando per 0.01). */

  uint16_t duty_B;          /**< Duty cycle del segnale in uscita al canale B,
                                espresso come percentuale (fare
                                attenzione perchè bisogna convertirlo in numero
                                moltiplicando per 0.01). */
  bool incollaggio_A;

  bool incollaggio_B;

  bool err_freq_A;

  bool err_freq_B;

  float_t err_freq_passo;



  int16_t fase;           /**< Sfasamento dei due segnali dal GIT, misurato
                                in gradi. */

  uint16_t ppr;            /**< Risoluzione del GIT riconducibile come numero
                                di suddivisioni del disco o al numero di
                                impulsi per giro, misurato in ppr. */

  float_t diametro;              /**< Diametro della ruota il cui asse è
                                collegato al GIT, misurato in metri. */

  double_t l_passo;             /**< Lunghezza del passo di ruota recepito dal
                                GIT, misurato in metri.  Esso deve essere
                                ricalcolato ogni volta che il diametro o
                                la risoluzione variano utilizzando la
                                formula: \f$ passo=\frac{\pi \cdot diametro}
                                {4 \cdot risoluzione}\f$. */

  double_t pos_A;       /**< Posizione del sensore A, misurato in metri. */

  double_t pos_B;       /**< Posizione del sensore B nell'intervallo di 4
                                passi, misurato in metri */

  double_t vel;         /**< Velocità del GIT, in m/s */

  double_t acc;         /**< Accelerazione del GIT, in m/s^2 */

  uint16_t conteggio;        /**< Numero di passi svolti dall'ultimo reset,
                                i passi sono quelli ottenuti tramite la
                                tecnica di risoluzione x4 */
  XGpio indirizzo_gpio_A;
  XGpio indirizzo_gpio_B;

  stato_encoder stato;
} encoder;


/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/

void inizializza_variabili_encoder(void);
void emula_sensori_encoder(void);
void aggiorna_variabili_encoder(void);
void reset_conteggi_encoder(void);

double_t ritorna_velocita_encoder1(void);
double_t ritorna_velocita_encoder2(void);
uint16_t ritorna_conteggio_encoder1(void);
uint16_t ritorna_conteggio_encoder2(void);

void assegna_ppr_encoder1(uint16_t ppr);
void assegna_ppr_encoder2(uint16_t ppr);
void assegna_diametro_ruota(float_t diametro);

void assegna_velocita_encoder1(float_t vel);
void assegna_velocita_encoder2(float_t vel);

void assegna_accelerazione_encoder1(float_t acc);
void assegna_accelerazione_encoder2(float_t acc);

void aggiorna_passo_encoder1(void);
void aggiorna_passo_encoder2(void);

#ifdef __cplusplus
}
#endif

#endif 
