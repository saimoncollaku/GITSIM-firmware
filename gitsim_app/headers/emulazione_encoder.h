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

/**
 * @brief Resetta i conteggi di entrambi gli encoder E1 ed E2
 *
 * @details Questa funzione azzera i conteggi degli encoder E1 ed E2,
 * riportandoli a zero. È utile per inizializzare o reinizializzare
 * i dati per prevenire l'overflow delle variabili di conteggio.
 *
 * @see E1, E2
 * @see encoder.conteggio
 */
void reset_conteggi_encoder(void);

/**
 * @brief Restituisce la velocità corrente emulata dall'encoder E1
 *
 * @return double_t La velocità corrente dell'encoder E1 (in m/s)
 *
 * @details Questa funzione fornisce accesso al valore di velocità attuale
 * emulato per l'encoder E1, in metri al secondo. Fare attenzione al fatto
 * che il valore è un double_t.
 *
 * @see E1
 * @see encoder.vel
 */
double_t ritorna_velocita_encoder1(void);

/**
 * @brief Restituisce la velocità corrente emulata dall'encoder E2
 *
 * @return double_t La velocità corrente dell'encoder E2 (in m/s)
 *
 * @details Questa funzione fornisce accesso al valore di velocità attuale
 * emulato per l'encoder E2, in metri al secondo. Fare attenzione al fatto
 * che il valore è un double_t.
 *
 * @see E2
 * @see encoder.vel
 */
double_t ritorna_velocita_encoder2(void);

/**
 * @brief Restituisce il valore di conteggio corrente dell'encoder E1
 *
 * @return uint16_t Il valore di conteggio (con risoluzione x4) dell'encoder E1
 *
 * @details Questa funzione fornisce accesso al valore di conteggio attuale
 * dell'encoder E1 con risoluzione x4. Il conteggio rappresenta il numero
 * di impulsi rilevati dall'encoder dall'ultimo reset o dall'inizio dell'
 * emulazione.
 *
 * @see E1
 * @see encoder.conteggio
 * @see reset_conteggi_encoder()
 */
uint16_t ritorna_conteggio_encoder1(void);

/**
 * @brief Restituisce il valore di conteggio corrente dell'encoder E2
 *
 * @return uint16_t Il valore di conteggio (con risoluzione x4) dell'encoder E2
 *
 * @details Questa funzione fornisce accesso al valore di conteggio attuale
 * dell'encoder E2 con risoluzione x4. Il conteggio rappresenta il numero
 * di impulsi rilevati dall'encoder dall'ultimo reset o dall'inizio dell'
 * emulazione.
 *
 * @see E2
 * @see encoder.conteggio
 * @see reset_conteggi_encoder()
 */
uint16_t ritorna_conteggio_encoder2(void);

/**
 * @brief Assegna il valore di impulsi per rivoluzione (ppr) all'encoder E1
 *
 * @param ppr Il ppr da assegnare all'encoder E1
 *
 * @details Questa funzione imposta il valore ppr (impulsi per rivoluzione)
 * per l'encoder E1. Questo parametro è dato normalmente dal datasheet del
 * costruttore dell'encoder e per la misura dei passi corrisponderebbe
 * alla risoluzione x1 (cioè usare solo i fronti di salita di un canale).
 *
 * @see E1
 * @see encoder.ppr
 */
void assegna_ppr_encoder1(uint16_t ppr);

/**
 * @brief Assegna il valore di impulsi per rivoluzione (ppr) all'encoder E2
 *
 * @param ppr Il ppr da assegnare all'encoder E2
 *
 * @details Questa funzione imposta il valore ppr (impulsi per rivoluzione)
 * per l'encoder E2. Questo parametro è dato normalmente dal datasheet del
 * costruttore dell'encoder e per la misura dei passi corrisponderebbe
 * alla risoluzione x1 (cioè usare solo i fronti di salita di un canale).
 *
 * @see E2
 * @see encoder.ppr
 */
void assegna_ppr_encoder2(uint16_t ppr);

/**
 * @brief Assegna il diametro della ruota a entrambi gli encoder E1 ed E2
 *
 * @param diametro Il diametro della ruota da assegnare (in metri)
 *
 * @details Questa funzione imposta il diametro della ruota per entrambi gli encoder
 * E1 ed E2. Il valore del diametro viene assegnato direttamente senza conversioni.
 *
 * @see E1, E2
 * @see encoder.diametro
 */
void assegna_diametro_ruota(float_t diametro);

/**
 * @brief Assegna un valore di velocità all'encoder E1
 *
 * @param vel Valore di velocità da assegnare (in m/s)
 *
 * @details Questa funzione converte il valore di velocità fornito
 * da float_t a double_t e lo assegna al campo 'vel' dell'encoder E1.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see E1
 * @see encoder.vel
 */
void assegna_velocita_encoder1(float_t vel);

/**
 * @brief Assegna un valore di velocità all'encoder E2
 *
 * @param vel Valore di velocità da assegnare (in m/s)
 *
 * @details Questa funzione converte il valore di velocità fornito
 * da float_t a double_t e lo assegna al campo 'vel' dell'encoder E2.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see E2
 * @see encoder.vel
 */
void assegna_velocita_encoder2(float_t vel);

/**
 * @brief Assegna un valore di accelerazione all'encoder E1
 *
 * @param acc Valore di accelerazione da assegnare (in m/s<sup>2</sup>)
 *
 * @details Questa funzione converte il valore di accelerazione fornito
 * da float_t a double_t e lo assegna al campo 'acc' dell'encoder E1.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see E1
 * @see encoder.acc
 */
void assegna_accelerazione_encoder1(float_t acc);

/**
 * @brief Assegna un valore di accelerazione all'encoder E2
 *
 * @param acc Valore di accelerazione da assegnare (in m/s<sup>2</sup>)
 *
 * @details Questa funzione converte il valore di accelerazione fornito
 * da float_t a double_t e lo assegna al campo 'acc' dell'encoder E2.
 *
 * @note La conversione da float_t a double_t può comportare un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see E2
 * @see encoder.acc
 */
void assegna_accelerazione_encoder2(float_t acc);

/**
 * @brief Aggiorna il passo dell'encoder E1
 *
 * Questa funzione calcola e aggiorna la  lunghezza del passo dell'encoder E1
 * basandosi sul suo diametro e sul numero di impulsi per rivoluzione (ppr).
 *
 * La formula utilizzata è:
 * @f[
 *    l_{passo} = \frac{diametro \cdot \pi}{ppr \cdot 2}
 * @f]
 * dove:
 * - @f$l_{passo}@f$ è il passo dell'encoder
 * - @f$diametro@f$ è il diametro della ruota dell'encoder
 * - @f$\pi@f$ è il valore di PI greco
 * - @f$ppr@f$ è il numero di impulsi per rivoluzione dell'encoder
 * - Il fattore 2 nel denominatore è usato per ottenere la risoluzione x2 del
 * 	 passo
 *
 * @see E1
 * @see encoder.l_passo
 */
void aggiorna_passo_encoder1(void);

/**
 * @brief Aggiorna il passo dell'encoder E2
 *
 * Questa funzione calcola e aggiorna la  lunghezza del passo dell'encoder E2
 * basandosi sul suo diametro e sul numero di impulsi per rivoluzione (ppr).
 *
 * La formula utilizzata è:
 * @f[
 *    l_{passo} = \frac{diametro \cdot \pi}{ppr \cdot 2}
 * @f]
 * dove:
 * - @f$l_{passo}@f$ è il passo dell'encoder
 * - @f$diametro@f$ è il diametro della ruota dell'encoder
 * - @f$\pi@f$ è il valore di PI greco
 * - @f$ppr@f$ è il numero di impulsi per rivoluzione dell'encoder
 * - Il fattore 2 nel denominatore è usato per ottenere la risoluzione x2 del
 * 	 passo
 *
 * @see E2
 * @see encoder.l_passo
 */
void aggiorna_passo_encoder2(void);

#ifdef __cplusplus
}
#endif

#endif 
