/**
 ******************************************************************************
 * @file    emulazione_encoder.c
 * @author  Saimon Collaku
 ******************************************************************************
 */


/******************************************************************************
 * INCLUDES
 *****************************************************************************/
#include "emulazione_encoder.h"
#include "gestione_uart.h"
#include "gestione_polling.h"


/******************************************************************************
 * PRIVATE MACROS AND DEFINES
 *****************************************************************************/

/** @brief Costante pigreco */
#define PI_GRECO 3.14159265358

/** @brief Velocita' massima lineare emulabile dal GIT, in m/s */
#define VELOCITA_MAX (700/3.6)

/******************************************************************************
 * TYPEDEFS
 *****************************************************************************/

/** @brief Rappresenta lo stato dell'encoder basato sui livelli dei canali A e B.
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
   *  Misurato in m/s^2.
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
 * STATIC VARIABLES
 *****************************************************************************/

/** @brief Tempo di aggiornamento delle variabili di encoder */
static float_t t_update;

/**
 *  @brief Struttura dati utilizzata per contenere tutte le variabili
 * di stato dell'encoder 1
 */
static encoder e_1;

/**
 *  @brief Struttura dati utilizzata per contenere tutte le variabili
 * di stato dell'encoder 2
 */
static encoder e_2;


/******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/
static void aggiorna_encoder(encoder *e_x);
static void emula_encoder(encoder *e_x);
static void inizializza_encoder(encoder *e_x);
static void valuta_stato_encoder(encoder *e_x, bool statoA, bool statoB);
static void reset_gpio(encoder *e_x);


/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

/**
 * @brief Aggiorna lo stato di un encoder
 *
 * @param e_x Puntatore alla struttura dell'encoder da aggiornare
 *
 * @details Questa funzione aggiorna la velocità, la posizione e gestisce
 * la saturazione per un encoder. Implementa un modello di movimento
 * basato su accelerazione e velocità, considerando lo sfasamento tra
 * i canali A e B dell'encoder.
 *
 * Il processo di aggiornamento include:
 * 1. Integrazione dell'accelerazione per ottenere la velocità
 * 2. Saturazione della velocità entro i limiti da VELOCITA_MAX
 * 3. Integrazione della velocità per ottenere la posizione
 * 4. Calcolo della posizione per entrambi i canali A e B
 * 5. Gestione della saturazione dello spazio
 *
 * @note
 * - La variabile t_update rappresenta l'intervallo di tempo per
 * l'integrazione
 *
 * @see encoder
 * @see VELOCITA_MAX
 * @see t_update
 */
static void aggiorna_encoder(encoder *e_x)
{
	/*
	 * Contiene la posizione MINORE tra quella
	*  del canale A e B, serve per la correzione
	* dello spazio
	*/
	double_t pos_minore;

	/*
	 * Contiene la posizione MAGGIORE tra quella
	*  del canale A e B, serve per la correzione
	* dello spazio
	*/
	double_t pos_maggiore;

	/* Integrazione dell'accelerazione */
	e_x->vel = (e_x->acc * t_update) + e_x->vel;

	/* Saturo la velocità se va oltre la soglia fissata */
	if (e_x->vel > VELOCITA_MAX)
	{
		e_x->vel = VELOCITA_MAX;
	}
	else if (e_x->vel < -VELOCITA_MAX)
	{
		e_x->vel = -VELOCITA_MAX;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	/*
	 * Integro la velocità, assegno lo spazio ad A
	 * (è una scelta arbitraria)
	 */
	e_x->pos_A = e_x->pos_A + (e_x->vel * t_update);

	/*
	 * Estrapolo il fattore di sfasamento, cio� converto i gradi nella
	 * quantit� di spazio da cui il canale B si discosta dal canale A
	 */
	float_t k_fase = ((float_t) - (e_x->fase) / 360);
	e_x->pos_B = e_x->pos_A + (2 * e_x->l_passo * k_fase);

	/*
	 * Indico chi è il sensore con la posizione maggiore e quale con la
	 * minore, mi serve per fare il controllo sulla correzione di spazio
	 */
	if (k_fase <= 0)
	{
		/* Posizione del B è minore dell'A */
		pos_minore = e_x->pos_B;
		pos_maggiore = e_x->pos_A;
	}
	else
	{
		/* Posizione dell'A è minore del B */
		pos_minore = e_x->pos_A;
		pos_maggiore = e_x->pos_B;
	}

	/* Satura lo spazio se uno dei due sensori va oltre la
	soglia [-2*passi, 2*passi] */
	if (pos_minore <= (-2 * e_x->l_passo))
	{
		/*
		 * Il MINORE ha sforato, esso ritorna a 0 mentre il maggiore
		 * va ad un valore maggiore di 0 (dipende dalla fase)
		 */
		e_x->pos_A = e_x->pos_A + (2 * e_x->l_passo);
		e_x->pos_B = e_x->pos_B + (2 * e_x->l_passo);
	}
	else if (pos_maggiore >= (2 * e_x->l_passo))
	{
		/*
		 * Il MAGGIORE ha sforato, esso ritorna a 0 mentre il minore
		 * va ad un valore minore di 0 (dipende dalla fase)
		 */
		e_x->pos_A = e_x->pos_A - (2 * e_x->l_passo);
		e_x->pos_B = e_x->pos_B - (2 * e_x->l_passo);
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	return;
}

/**
 * @brief Emula il comportamento di un encoder
 *
 * @param e_x Puntatore alla struttura dell'encoder da emulare
 *
 * @details
 * Questa funzione simula il comportamento di un encoder generando segnali
 * per i canali A e B basati sulla posizione attuale dell'encoder. Il processo
 * include:
 * 1. Calcolo delle soglie basate sul duty cycle per ciascun canale
 * 2. Generazione dei segnali per i canali A e B usando GPIO
 * 3. Valutazione dello stato dell'encoder basata sui segnali generati
 *
 * @note
 * - Assume l'uso di XGpio per la scrittura dei segnali
 * - Il comportamento è influenzato dai duty cycle e dalle posizioni dei canali
 * A e B
 * - Utilizza la funzione valuta_stato_encoder per aggiornare lo stato dell'
 * encoder
 *
 * @see encoder, valuta_stato_encoder, XGpio_DiscreteWrite
 */
static void emula_encoder(encoder *e_x)
{
	/* Stato del canale, equivalente al valore logico di GPIO corrispondente */
	bool stato_sensoreA;
	bool stato_sensoreB;

	/* Porto i duty cycle da percentuale intera a decimale */
	float_t k_dutyA = ((float_t) e_x->duty_A) * 0.01;
	float_t k_dutyB = ((float_t) e_x->duty_B) * 0.01;

	/*
	 * Genero le soglie su cui fare il confronto, per stimare in che
	 * posizione è il sensore
	 */
	double_t soglia_max_def = 2 * e_x->l_passo; /* 2passi */
	double_t soglia_min_def = - soglia_max_def; /* -2passi */
	/* Duty% * 2passi */
	double_t soglia_pos_A = k_dutyA * soglia_max_def;
	double_t soglia_pos_B = k_dutyB * soglia_max_def;
	/* (1-Duty%) * 2passi */
	double_t soglia_neg_A = (1 - k_dutyA) * soglia_min_def;
	double_t soglia_neg_B = (1 - k_dutyB) * soglia_min_def;

	/*
	 * Generazione segnale per canale A da GPIO,
	 * se il treno va a velocit� positiva allora lo spazio sar� positivo,
	 * quindi verranno usati solo gli ultimi 2 if dei 4 totali, che fanno
	 * il check per posizioni positive. Ogni paio di if dice se la GPIO è
	 * alta o bassa, la soglia che decide questo dipende dal duty cycle.
	 * Esempio: il range dello spazio � 0 -> 2 passi, se duty = 50%
	 * la soglia di transizione alto/basso sar� 1 passo.
	 */
	if ((e_x->pos_A >= soglia_min_def) && (e_x->pos_A < soglia_neg_A))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_A, 1, 0x01);
		stato_sensoreA = true;
	}
	else if ((e_x->pos_A >= soglia_neg_A) && (e_x->pos_A < 0))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_A, 1, 0x00);
		stato_sensoreA = false;
	}
	else if  ((e_x->pos_A >= 0) && (e_x->pos_A < soglia_pos_A))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_A, 1, 0x01);
		stato_sensoreA = true;
	}
	else if ((e_x->pos_A >= soglia_pos_A) && (e_x->pos_A < soglia_max_def))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_A, 1, 0x00);
		stato_sensoreA = false;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	/*
	 * Generazione segnale per canale B da GPIO, valgono gli stessi
	 * ragionamenti del canale A
	 */
	if ((e_x->pos_B >= soglia_min_def) && (e_x->pos_B < soglia_neg_B))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_B, 1, 0x01);
		stato_sensoreB = true;
	}
	else if ((e_x->pos_B >= soglia_neg_B) && (e_x->pos_B < 0))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_B, 1, 0x00);
		stato_sensoreB = false;
	}
	else if  ((e_x->pos_B >= 0) && (e_x->pos_B < soglia_pos_B))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_B, 1, 0x01);
		stato_sensoreB = true;
	}
	else if ((e_x->pos_B >= soglia_pos_B) && (e_x->pos_B < soglia_max_def))
	{
		XGpio_DiscreteWrite(&e_x->indirizzo_gpio_B, 1, 0x00);
		stato_sensoreB = false;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	valuta_stato_encoder(e_x, stato_sensoreA, stato_sensoreB);
}

/**
 * @brief Inizializza una struttura encoder con valori predefiniti
 *
 * @param e_x Puntatore alla struttura dell'encoder da inizializzare
 *
 * @details
 * Imposta i valori iniziali per tutti i campi della struttura encoder,
 * inclusi:
 * velocità, accelerazione, posizioni, duty cycle, fase, conteggio, stato di
 * incollaggio,
 * impulsi per rivoluzione (ppr), diametro e lunghezza del passo.
 *
 * @note
 * - Il valore di PI_GRECO deve essere definito altrove nel codice
 * - I valori iniziali sono pensati per un encoder standard a quadratura
 *
 * @see encoder
 */
static void inizializza_encoder(encoder *e_x)
{
	e_x->vel = 0;
	e_x->acc = 0;
	e_x->pos_A = 0;
	e_x->pos_B = PI_GRECO / 512;
	e_x->duty_A = 50;
	e_x->duty_B = 50;
	e_x->fase = 90;
	e_x->conteggio = 0;
	e_x->incollaggio_A = false;
	e_x->incollaggio_B = false;
	e_x->ppr = 128;
	e_x->diametro = 1;
	e_x->l_passo = PI_GRECO / 256;
}

/**
 * @brief Resetta lo stato dei GPIO associati all'encoder
 *
 * @param e_x Puntatore alla struttura dell'encoder
 *
 * @details
 * Configura i GPIO associati ai canali A e B dell'encoder come output
 * e li imposta a un valore basso (0).
 *
 * @note
 * - Utilizza le funzioni XGpio per la configurazione e il controllo dei GPIO
 * - Assume che i campi indirizzo_gpio_A e indirizzo_gpio_B siano correttamente
 *  inizializzati
 *
 * @see encoder, XGpio_SetDataDirection, XGpio_DiscreteClear
 */
static void reset_gpio(encoder *e_x)
{

    XGpio_SetDataDirection(&e_x->indirizzo_gpio_A, 1, 0x00);
    XGpio_DiscreteClear(&e_x->indirizzo_gpio_A, 1, 0x01);
    XGpio_SetDataDirection(&e_x->indirizzo_gpio_B, 1, 0x00);
    XGpio_DiscreteClear(&e_x->indirizzo_gpio_B, 1, 0x01);
}

/**
 * @brief Valuta e aggiorna lo stato dell'encoder basato sui segnali dei canali
 *  A e B
 *
 * @param e_x Puntatore alla struttura dell'encoder
 * @param statoA Stato logico del canale A
 * @param statoB Stato logico del canale B
 *
 * @details
 * Questa funzione determina lo stato dell'encoder basandosi sui segnali dei
 * canali A e B.
 * Aggiorna il conteggio dell'encoder quando viene rilevato un cambiamento di
 * stato valido.
 * Gli stati possibili sono: zero, uno, due, tre e incerto.
 *
 * @note
 * - Il conteggio viene incrementato solo quando lo stato cambia da uno stato
 * valido a un altro
 * - Lo stato 'incerto' è usato per gestire l'inizializzazione
 *
 * @see encoder
 */
static void valuta_stato_encoder(encoder *e_x, bool statoA, bool statoB)
{

	if((statoA == false) && (statoB == false) && (e_x->stato != zero))
	{
		if(e_x->stato != incerto)
		{
			e_x->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		e_x->stato = zero;
	}
	else if((statoA == false) && (statoB == true) && (e_x->stato != uno))
	{
		if(e_x->stato != incerto)
		{
			e_x->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		e_x->stato = uno;
	}
	else if((statoA == true) && (statoB == false) && (e_x->stato != due))
	{
		if(e_x->stato != incerto)
		{
			e_x->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		e_x->stato = due;
	}
	else if((statoA == true) && (statoB == true) && (e_x->stato != tre))
	{
		if(e_x->stato != incerto)
		{
			e_x->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		e_x->stato = tre;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

}


/******************************************************************************
 * GLOBAL FUNCTIONS
 *****************************************************************************/

void inizializza_variabili_encoder()
{
	t_update = ritorna_tempo_del_polling();

	/* Inizializzo variabili */
	inizializza_encoder(&e_1);
	inizializza_encoder(&e_2);

	/* Inizializzo gpio */
	XGpio_Initialize(&e_1.indirizzo_gpio_A, XPAR_AXI_GPIO_E1_A_DEVICE_ID);
	XGpio_Initialize(&e_1.indirizzo_gpio_B, XPAR_AXI_GPIO_E1_B_DEVICE_ID);
	XGpio_Initialize(&e_2.indirizzo_gpio_A, XPAR_AXI_GPIO_E2_A_DEVICE_ID);
	XGpio_Initialize(&e_2.indirizzo_gpio_B, XPAR_AXI_GPIO_E2_B_DEVICE_ID);

	/* Reset gpio */
	reset_gpio(&e_1);
	reset_gpio(&e_2);
}

void aggiorna_variabili_encoder()
{
	bool stato_connessione_app = ritorna_stato_connessione_app();

	if(stato_connessione_app == true)
	{
		aggiorna_encoder(&e_1);
		aggiorna_encoder(&e_2);
	}
	else
	{
		/* Non succede niente */
	}
}

void emula_sensori_encoder()
{
	bool stato_connessione_app = ritorna_stato_connessione_app();

	if(stato_connessione_app == true)
	{
		emula_encoder(&e_1);
		emula_encoder(&e_2);
	}
	else
	{
		/* Non succede niente */
	}
}

void reset_conteggi_encoder()
{
	e_1.conteggio = 0;
	e_2.conteggio = 0;
}

double_t ritorna_velocita_encoder1()
{
	return e_1.vel;
}

double_t ritorna_velocita_encoder2()
{
	return e_2.vel;
}

uint16_t ritorna_conteggio_encoder1()
{
	return e_1.conteggio;
}

uint16_t ritorna_conteggio_encoder2()
{
	return e_2.conteggio;
}

void assegna_ppr_encoder1(uint16_t ppr)
{
	e_1.ppr = ppr;
}

void assegna_ppr_encoder2(uint16_t ppr)
{
	e_2.ppr = ppr;
}

void assegna_diametro_ruota(float_t diametro)
{
	e_1.diametro = diametro;
	e_2.diametro = diametro;
}

void assegna_velocita_encoder1(float_t vel)
{
	e_1.vel = ((double_t) vel);
}

void assegna_velocita_encoder2(float_t vel)
{
	e_2.vel = ((double_t) vel);
}

void assegna_accelerazione_encoder1(float_t acc)
{
	e_1.acc = ((double_t) acc);
}

void assegna_accelerazione_encoder2(float_t acc)
{
	e_2.acc = ((double_t) acc);
}

void aggiorna_passo_encoder1(void)
{
	double_t numeratore = ((double_t) e_1.diametro) * PI_GRECO;
	double_t denominatore = ((double_t) e_1.ppr) * 2;
	e_1.l_passo = numeratore / denominatore;
}

void aggiorna_passo_encoder2(void)
{
	double_t numeratore = ((double_t) e_2.diametro) * PI_GRECO;
	double_t denominatore = ((double_t) e_2.ppr) * 2;
	e_2.l_passo = numeratore / denominatore;
}

/****************** (C) COPYRIGHT GITSIM ***** END OF FILE *******************/
