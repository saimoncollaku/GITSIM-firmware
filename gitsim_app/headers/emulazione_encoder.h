/**
 ******************************************************************************
 * @file    emulazione_encoder.h
 * @author  Saimon Collaku
 ******************************************************************************
 */

#ifndef HEADERS_EMULAZIONE_ENCODER_H_
#define HEADERS_EMULAZIONE_ENCODER_H_

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include <math.h>
#include "xgpio.h"
#include <stdbool.h>


/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/** @brief Rappresenta lo stato dell'encoder basato sui livelli dei canali A e
 *  B.
 *
 *  Utilizzato per decodificare la posizione e la generare il conteggio.
 */
typedef enum
{
	/** @brief Stato 0: Canale A = 0, Canale B = 0 */
	zero,
	/** @brief Stato 1: Canale A = 0, Canale B = 1 */
	uno,
	/** @brief Stato 2: Canale A = 1, Canale B = 0 */
	due,
	/** @brief Stato 3: Canale A = 1, Canale B = 1 */
	tre,
	/** @brief Stato incerto, usato per l'inizializzazione */
	incerto
}stato_encoder;


/** @brief Struttura che rappresenta un encoder incrementale emulato.
 *
 *  Questa struttura contiene tutti i parametri e le informazioni necessarie
 *  per emulare e gestire un encoder incrementale in un sistema embedded.
 *  Include caratteristiche fisiche, stati operativi/errore, e parametri di
 *  output.
 */
typedef struct
{
  /** @brief Duty cycle del segnale in uscita al canale A.
   *  Espresso come intero percentuale. Moltiplicare per 0.01 per ottenere il
   *   valore effettivo.
   */
  uint16_t duty_A;

  /** @brief Duty cycle del segnale in uscita al canale B.
   *  Espresso come intero percentuale. Moltiplicare per 0.01 per ottenere il
   *   valore effettivo.
   */
  uint16_t duty_B;

  /** @brief Flag per indicare l'incollaggio del canale A.
   *  true se il canale è incollato, false altrimenti.
   */
  bool incollaggio_A;

  /** @brief Flag per indicare l'incollaggio del canale B.
   *  true se il canale è incollato, false altrimenti.
   */
  bool incollaggio_B;

  /** @brief Flag per indicare errore di frequenza sul canale A.
   *  true se c'è un errore di frequenza, false altrimenti.
   */
  bool err_freq_A;

  /** @brief Flag per indicare errore di frequenza sul canale B.
   *  true se c'è un errore di frequenza, false altrimenti.
   */
  bool err_freq_B;

  /** @brief Errore di frequenza per passo.
   *  Rappresenta la deviazione dalla frequenza attesa per ogni passo dell'
   *  encoder.
   */
  float_t err_freq_passo;

  /** @brief Sfasamento dei due segnali dell'encoder.
   *  Misurato in gradi.
   */
  int16_t fase;

  /** @brief Impulsi per giro dell'encoder.
   *  Numero di suddivisioni del disco o impulsi per giro, misurato in PPR
   *  (Pulses Per Revolution).
   */
  uint16_t ppr;

  /** @brief Diametro della ruota collegata al GIT.
   *  Misurato in metri.
   */
  float_t diametro;

  /** @brief Lunghezza del passo di ruota rilevato da un canale dell'encoder.
   *  Misurato in metri. Ricalcolato quando diametro o risoluzione variano.
   *  Corrisponde alla lunghezza con risoluzione x2.
   */
  double_t l_passo;

  /** @brief Posizione del sensore A.
   *  Misurato in metri. Rappresenta la posizione nell'intervallo di 4 passi,
   *  con risoluzione x2
   */
  double_t pos_A;

  /** @brief Posizione del sensore B.
   *  Misurato in metri. Rappresenta la posizione nell'intervallo di 4 passi,
   *  con risoluzione x2
   */
  double_t pos_B;

  /** @brief Velocità del GIT.
   *  Misurato in m/s.
   */
  double_t vel;

  /** @brief Accelerazione del GIT.
   *  Misurato in m/s<sup>2</sup>
   */
  double_t acc;

  /** @brief Numero di passi svolti dall'ultimo reset.
   *  Conteggio contato con i passi a risoluzione x4.
   *
   *  @note quindi la lunghezza del passo equivalente per il
   *  calcolo dei conteggi e la metà rispetto a quello della
   *  variabile l_passo.
   */
  uint16_t conteggio;

  /** @brief Indirizzo GPIO per il canale A.
   *  Struttura XGpio per la gestione del GPIO del canale A.
   */
  XGpio indirizzo_gpio_A;

  /** @brief Indirizzo GPIO per il canale B.
   *  Struttura XGpio per la gestione del GPIO del canale B.
   */
  XGpio indirizzo_gpio_B;

  /** @brief Stato corrente dell'encoder.
   *  Enumerazione che rappresenta lo stato operativo dell'encoder.
   */
  stato_encoder stato;

} encoder;


/******************************************************************************
 * GLOBAL FUNCTION PROTOTYPES
 *****************************************************************************/

/**
 * @brief Inizializza le variabili e i GPIO per gli encoder
 *
 * @details
 * Questa funzione esegue le seguenti operazioni:
 * 1. Imposta il tempo di aggiornamento (t_update)
 * 2. Inizializza le strutture degli encoder e_1 e e_2
 * 3. Inizializza i GPIO associati a ciascun encoder
 * 4. Resetta lo stato dei GPIO
 *
 * @note
 * - Utilizza funzioni esterne come ritorna_tempo_del_polling()
 * - Utilizza costanti XPAR per l'identificazione dei dispositivi GPIO
 *
 * @see inizializza_encoder, XGpio_Initialize, reset_gpio
 */
void inizializza_variabili_encoder(void);


/**
 * @brief Emula le uscite degli encoder e_1 ed e_2
 *
 * @details Questa funzione gestisce l'emulazione dei sensori per gli encoder
 *  e_1 ed e_2.
 * L'emulazione viene eseguita solo se l'applicazione GITSIM è connessa,
 * altrimenti la funzione non esegue alcuna operazione.
 *
 * @note L'emulazione dei sensori è utile per test e debug, permettendo di
 * simulare il comportamento degli encoder senza hardware fisico.
 *
 * @see e_1, e_2, ritorna_stato_connessione_app, emula_encoder
 */
void emula_sensori_encoder(void);

/**
 * @brief Aggiorna le variabili degli encoder se l'applicazione è connessa
 *
 * @details
 * Verifica lo stato di connessione dell'applicazione e, se connessa,
 * aggiorna le variabili per entrambi gli encoder e_1 e e_2.
 *
 * @note
 * - Dipende dalla funzione ritorna_stato_connessione_app()
 * - Non esegue alcuna azione se l'applicazione non è connessa
 *
 * @see aggiorna_encoder, ritorna_stato_connessione_app
 */
void aggiorna_variabili_encoder(void);

/**
 * @brief Resetta i conteggi di entrambi gli encoder e_1 ed e_2
 *
 * @details Questa funzione azzera i conteggi degli encoder e_1 ed e_2,
 * riportandoli a zero. È utile per inizializzare o reinizializzare
 * i dati per prevenire l'overflow delle variabili di conteggio.
 *
 * @see e_1, e_2
 * @see encoder.conteggio
 */
void reset_conteggi_encoder(void);

/**
 * @brief Restituisce la velocità corrente emulata dall'encoder e_1
 *
 * @return double_t La velocità corrente dell'encoder e_1 (in m/s)
 *
 * @details Questa funzione fornisce accesso al valore di velocità attuale
 * emulato per l'encoder e_1, in metri al secondo. Fare attenzione al fatto
 * che il valore è un double_t.
 *
 * @see e_1
 * @see encoder.vel
 */
double_t ritorna_velocita_encoder1(void);

/**
 * @brief Restituisce la velocità corrente emulata dall'encoder e_2
 *
 * @return double_t La velocità corrente dell'encoder e_2 (in m/s)
 *
 * @details Questa funzione fornisce accesso al valore di velocità attuale
 * emulato per l'encoder e_2, in metri al secondo. Fare attenzione al fatto
 * che il valore è un double_t.
 *
 * @see e_2
 * @see encoder.vel
 */
double_t ritorna_velocita_encoder2(void);

/**
 * @brief Restituisce il valore di conteggio corrente dell'encoder e_1
 *
 * @return uint16_t Il valore di conteggio (con risoluzione x4) dell'encoder
 * e_1
 *
 * @details Questa funzione fornisce accesso al valore di conteggio attuale
 * dell'encoder e_1 con risoluzione x4. Il conteggio rappresenta il numero
 * di impulsi rilevati dall'encoder dall'ultimo reset o dall'inizio dell'
 * emulazione.
 *
 * @see e_1
 * @see encoder.conteggio
 * @see reset_conteggi_encoder()
 */
uint16_t ritorna_conteggio_encoder1(void);

/**
 * @brief Restituisce il valore di conteggio corrente dell'encoder e_2
 *
 * @return uint16_t Il valore di conteggio (con risoluzione x4) dell'encoder
 * e_2
 *
 * @details Questa funzione fornisce accesso al valore di conteggio attuale
 * dell'encoder e_2 con risoluzione x4. Il conteggio rappresenta il numero
 * di impulsi rilevati dall'encoder dall'ultimo reset o dall'inizio dell'
 * emulazione.
 *
 * @see e_2
 * @see encoder.conteggio
 * @see reset_conteggi_encoder()
 */
uint16_t ritorna_conteggio_encoder2(void);

/**
 * @brief Assegna il valore di impulsi per rivoluzione (ppr) all'encoder e_1
 *
 * @param ppr Il ppr da assegnare all'encoder e_1
 *
 * @details Questa funzione imposta il valore ppr (impulsi per rivoluzione)
 * per l'encoder e_1. Questo parametro è dato normalmente dal datasheet del
 * costruttore dell'encoder e per la misura dei passi corrisponderebbe
 * alla risoluzione x1 (cioè usare solo i fronti di salita di un canale).
 *
 * @see e_1
 * @see encoder.ppr
 */
void assegna_ppr_encoder1(uint16_t ppr);

/**
 * @brief Assegna il valore di impulsi per rivoluzione (ppr) all'encoder e_2
 *
 * @param ppr Il ppr da assegnare all'encoder e_2
 *
 * @details Questa funzione imposta il valore ppr (impulsi per rivoluzione)
 * per l'encoder e_2. Questo parametro è dato normalmente dal datasheet del
 * costruttore dell'encoder e per la misura dei passi corrisponderebbe
 * alla risoluzione x1 (cioè usare solo i fronti di salita di un canale).
 *
 * @see e_2
 * @see encoder.ppr
 */
void assegna_ppr_encoder2(uint16_t ppr);

/**
 * @brief Assegna il diametro della ruota a entrambi gli encoder e_1 ed e_2
 *
 * @param diametro Il diametro della ruota da assegnare (in metri)
 *
 * @details Questa funzione imposta il diametro della ruota per entrambi gli
 * encoder e_1 ed e_2. Il valore del diametro viene assegnato direttamente
 * senza conversioni.
 *
 * @see e_1, e_2
 * @see encoder.diametro
 */
void assegna_diametro_ruota(float_t diametro);

/**
 * @brief Assegna un valore di velocità all'encoder e_1
 *
 * @param vel Valore di velocità da assegnare (in m/s)
 *
 * @details Questa funzione converte il valore di velocità fornito
 * da float_t a double_t e lo assegna al campo 'vel' dell'encoder e_1.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see e_1
 * @see encoder.vel
 */
void assegna_velocita_encoder1(float_t vel);

/**
 * @brief Assegna un valore di velocità all'encoder e_2
 *
 * @param vel Valore di velocità da assegnare (in m/s)
 *
 * @details Questa funzione converte il valore di velocità fornito
 * da float_t a double_t e lo assegna al campo 'vel' dell'encoder e_2.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see e_2
 * @see encoder.vel
 */
void assegna_velocita_encoder2(float_t vel);

/**
 * @brief Assegna un valore di accelerazione all'encoder e_1
 *
 * @param acc Valore di accelerazione da assegnare (in m/s<sup>2</sup>)
 *
 * @details Questa funzione converte il valore di accelerazione fornito
 * da float_t a double_t e lo assegna al campo 'acc' dell'encoder e_1.
 *
 * @note La conversione da float_t a double_t comporta un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see e_1
 * @see encoder.acc
 */
void assegna_accelerazione_encoder1(float_t acc);

/**
 * @brief Assegna un valore di accelerazione all'encoder e_2
 *
 * @param acc Valore di accelerazione da assegnare (in m/s<sup>2</sup>)
 *
 * @details Questa funzione converte il valore di accelerazione fornito
 * da float_t a double_t e lo assegna al campo 'acc' dell'encoder e_2.
 *
 * @note La conversione da float_t a double_t può comportare un
 * cambiamento di precisione, dipendente dalla libreria math.h
 *
 * @see e_2
 * @see encoder.acc
 */
void assegna_accelerazione_encoder2(float_t acc);

/**
 * @brief Aggiorna il passo dell'encoder e_1
 *
 * Questa funzione calcola e aggiorna la  lunghezza del passo dell'encoder e_1
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
 * @see e_1
 * @see encoder.l_passo
 */
void aggiorna_passo_encoder1(void);

/**
 * @brief Aggiorna il passo dell'encoder e_2
 *
 * Questa funzione calcola e aggiorna la  lunghezza del passo dell'encoder e_2
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
 * @see e_2
 * @see encoder.l_passo
 */
void aggiorna_passo_encoder2(void);

#ifdef __cplusplus
}
#endif

#endif 

/****************** (C) COPYRIGHT GITSIM ***** END OF FILE *******************/
