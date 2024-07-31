/**
 ********************************************************************************
 * @file    emulazione_encoder.c
 * @author  Saimon Collaku
 ********************************************************************************
 */


/************************************
 * INCLUDES
 ************************************/
#include "emulazione_encoder.h"
#include "gestione_uart.h"
#include "gestione_polling.h"


/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/

/** @brief Costante pigreco */
#define PI_GRECO 3.14159265358

/** @brief Velocita' massima lineare emulabile dal GIT, in m/s */
#define VELOCITA_MAX (700/3.6)


/************************************
 * STATIC VARIABLES
 ************************************/

/** @brief Tempo di aggiornamento delle variabili di encoder */
static float_t t_update;

/**
 *  @brief Struttura dati utilizzata per contenere tutte le variabili
 * di stato dell'encoder 1
 */
static encoder E1;

/**
 *  @brief Struttura dati utilizzata per contenere tutte le variabili
 * di stato dell'encoder 2
 */
static encoder E2;


/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static void aggiorna_encoder(encoder *encoder);
static void emula_encoder(encoder *encoder);
static void inizializza_encoder(encoder *encoder);
static void valuta_stato_encoder(encoder *encoder, bool statoA, bool statoB);
static void reset_gpio(encoder *encoder);


/************************************
 * STATIC FUNCTIONS
 ************************************/

static void aggiorna_encoder(encoder *encoder)
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
	encoder->vel = (encoder->acc * t_update) + encoder->vel;

	/* Saturo la velocit� se va oltre la soglia fissata */
	if (encoder->vel > VELOCITA_MAX)
	{
		encoder->vel = VELOCITA_MAX;
	}
	else if (encoder->vel < -VELOCITA_MAX)
	{
		encoder->vel = -VELOCITA_MAX;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	/*
	 * Integrazione della velocit�, assegno lo spazio ad A
	 * (che � una scelta arbitraria)
	 */
	encoder->pos_A = encoder->pos_A + (encoder->vel * t_update);

	/*
	 * Estrapolo il fattore di sfasamento, cio� converto i gradi nella
	 * quantit� di spazio da cui il canale B si discosta dal canale A
	 */
	float_t k_fase = ((float_t) - (encoder->fase) / 360);
	encoder->pos_B = encoder->pos_A + (2 * encoder->l_passo * k_fase);

	/*
	 * Indico chi � il sensore con la posizione maggiore e quale con la
	 * minore, mi serve per fare il controllo sulla correzione di spazio
	 */
	if (k_fase <= 0)
	{
		/* Posizione del B � minore dell'A */
		pos_minore = encoder->pos_B;
		pos_maggiore = encoder->pos_A;
	}
	else
	{
		/* Posizione dell'A � minore del B */
		pos_minore = encoder->pos_A;
		pos_maggiore = encoder->pos_B;
	}

	/* Satura lo spazio se uno dei due sensori va oltre la
	soglia [-2*passi, 2*passi] */
	if (pos_minore <= (-2 * encoder->l_passo))
	{
		/*
		 * Il MINORE ha sforato, esso ritorna a 0 mentre il maggiore
		 * va ad un valore maggiore di 0 (dipende dalla fase)
		 */
		encoder->pos_A = encoder->pos_A + (2 * encoder->l_passo);
		encoder->pos_B = encoder->pos_B + (2 * encoder->l_passo);
	}
	else if (pos_maggiore >= (2 * encoder->l_passo))
	{
		/*
		 * Il MAGGIORE ha sforato, esso ritorna a 0 mentre il minore
		 * va ad un valore minore di 0 (dipende dalla fase)
		 */
		encoder->pos_A = encoder->pos_A - (2 * encoder->l_passo);
		encoder->pos_B = encoder->pos_B - (2 * encoder->l_passo);
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	return;

}

static void emula_encoder(encoder *encoder)
{
	/* Stato del canale, equivalente al valore logico di GPIO corrispondente */
	bool stato_sensoreA;
	bool stato_sensoreB;

	/* Porto i duty cycle da percentuale intera a decimale */
	float_t k_dutyA = ((float_t) encoder->duty_A) * 0.01;
	float_t k_dutyB = ((float_t) encoder->duty_B) * 0.01;


	/*
	 * Genero le soglie su cui fare il confronto, per stimare in che
	 * posizione � il sensore
	 */
	double_t soglia_max_def = 2 * encoder->l_passo; /* 2passi */
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
	 * il check per posizioni positive. Ogni paio di if dice se la GPIO �
	 * alto o basso, la soglia che decide questo dipende dal duty cycle.
	 * Esempio: il range dello spazio � 0 -> 2 passi, se duty = 50%
	 * la soglia di transizione alto/basso sar� 1 passo.
	 */
	if ((encoder->pos_A >= soglia_min_def) && (encoder->pos_A < soglia_neg_A))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_A, 1, 0x01);
		stato_sensoreA = true;
	}
	else if ((encoder->pos_A >= soglia_neg_A) && (encoder->pos_A < 0))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_A, 1, 0x00);
		stato_sensoreA = false;
	}
	else if  ((encoder->pos_A >= 0) && (encoder->pos_A < soglia_pos_A))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_A, 1, 0x01);
		stato_sensoreA = true;
	}
	else if ((encoder->pos_A >= soglia_pos_A) && (encoder->pos_A < soglia_max_def))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_A, 1, 0x00);
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
	if ((encoder->pos_B >= soglia_min_def) && (encoder->pos_B < soglia_neg_B))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_B, 1, 0x01);
		stato_sensoreB = true;
	}
	else if ((encoder->pos_B >= soglia_neg_B) && (encoder->pos_B < 0))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_B, 1, 0x00);
		stato_sensoreB = false;
	}
	else if  ((encoder->pos_B >= 0) && (encoder->pos_B < soglia_pos_B))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_B, 1, 0x01);
		stato_sensoreB = true;
	}
	else if ((encoder->pos_B >= soglia_pos_B) && (encoder->pos_B < soglia_max_def))
	{
		XGpio_DiscreteWrite(&encoder->indirizzo_gpio_B, 1, 0x00);
		stato_sensoreB = false;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

	valuta_stato_encoder(encoder, stato_sensoreA, stato_sensoreB);
}

static void inizializza_encoder(encoder *encoder)
{
	encoder->vel = 0;
	encoder->acc = 0;
	encoder->pos_A = 0;
	encoder->pos_B = 0;
	encoder->duty_A = 50;
	encoder->duty_B = 50;
	encoder->fase = 90;
	encoder->conteggio = 0;
	encoder->incollaggio_A = false;
	encoder->incollaggio_B = false;
	encoder->ppr = 128;
	encoder->diametro = 1;
	encoder->l_passo = PI_GRECO / 256;
}

static void reset_gpio(encoder *encoder)
{

    XGpio_SetDataDirection(&encoder->indirizzo_gpio_A, 1, 0x00);
    XGpio_DiscreteClear(&encoder->indirizzo_gpio_A, 1, 0x01);
    XGpio_SetDataDirection(&encoder->indirizzo_gpio_B, 1, 0x00);
    XGpio_DiscreteClear(&encoder->indirizzo_gpio_B, 1, 0x01);
}

static void valuta_stato_encoder(encoder *encoder, bool statoA, bool statoB)
{

	if((statoA == false) && (statoB == false) && (encoder->stato != zero))
	{
		if(encoder->stato != incerto)
		{
			encoder->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		encoder->stato = zero;
	}
	else if((statoA == false) && (statoB == true) && (encoder->stato != uno))
	{
		if(encoder->stato != incerto)
		{
			encoder->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		encoder->stato = uno;
	}
	else if((statoA == true) && (statoB == false) && (encoder->stato != due))
	{
		if(encoder->stato != incerto)
		{
			encoder->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		encoder->stato = due;
	}
	else if((statoA == true) && (statoB == true) && (encoder->stato != tre))
	{
		if(encoder->stato != incerto)
		{
			encoder->conteggio++;
		}
		else
		{
			/* Non succede niente, MISRA-2023-15.7 */
		}
		encoder->stato = tre;
	}
	else
	{
		/* Non succede niente, MISRA-2023-15.7 */
	}

}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void inizializza_variabili_encoder()
{
	t_update = ritorna_tempo_del_polling();

	/* Inizializzo variabili */
	inizializza_encoder(&E1);
	inizializza_encoder(&E2);

	/* Inizializzo gpio */
	XGpio_Initialize(&E1.indirizzo_gpio_A, XPAR_AXI_GPIO_E1_A_DEVICE_ID);
	XGpio_Initialize(&E1.indirizzo_gpio_B, XPAR_AXI_GPIO_E1_B_DEVICE_ID);
	XGpio_Initialize(&E2.indirizzo_gpio_A, XPAR_AXI_GPIO_E2_A_DEVICE_ID);
	XGpio_Initialize(&E2.indirizzo_gpio_B, XPAR_AXI_GPIO_E2_B_DEVICE_ID);

	/* Reset gpio */
	reset_gpio(&E1);
	reset_gpio(&E2);
}

void aggiorna_variabili_encoder()
{
	bool stato_connessione_app = ritorna_stato_connessione_app();

	if(stato_connessione_app == true)
	{
		aggiorna_encoder(&E1);
		aggiorna_encoder(&E2);
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
		emula_encoder(&E1);
		emula_encoder(&E2);
	}
	else
	{
		/* Non succede niente */
	}
}

void reset_conteggi_encoder()
{
	E1.conteggio = 0;
	E2.conteggio = 0;
}

double_t ritorna_velocita_encoder1()
{
	return E1.vel;
}

double_t ritorna_velocita_encoder2()
{
	return E2.vel;
}

uint16_t ritorna_conteggio_encoder1()
{
	return E1.conteggio;
}

uint16_t ritorna_conteggio_encoder2()
{
	return E2.conteggio;
}

void assegna_ppr_encoder1(uint16_t ppr)
{
	E1.ppr = ppr;
}

void assegna_ppr_encoder2(uint16_t ppr)
{
	E2.ppr = ppr;
}

void assegna_diametro_ruota(float_t diametro)
{
	E1.diametro = diametro;
	E2.diametro = diametro;
}

void assegna_velocita_encoder1(float_t vel)
{
	E1.vel = ((double_t) vel);
}

void assegna_velocita_encoder2(float_t vel)
{
	E2.vel = ((double_t) vel);
}

void assegna_accelerazione_encoder1(float_t acc)
{
	E1.acc = ((double_t) acc);
}

void assegna_accelerazione_encoder2(float_t acc)
{
	E2.acc = ((double_t) acc);
}

void aggiorna_passo_encoder1(void)
{
	E1.l_passo = (((double_t) E1.diametro * PI_GRECO) / ((double_t) E1.ppr)) * 0.5;
}

void aggiorna_passo_encoder2(void)
{
	E2.l_passo = (((double_t) E2.diametro * PI_GRECO) / ((double_t) E2.ppr)) * 0.5;
}



