/**
 ******************************************************************************
 * @file    gestione_uart.c
 * @author  Saimon Collaku
 ******************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "xuartps.h"
#include "gestione_uart.h"
#include "emulazione_encoder.h"


/******************************************************************************
 * PRIVATE MACROS AND DEFINES
 *****************************************************************************/

/**
 * @brief Indirizzo base dell'UART
 *
 * Definisce l'indirizzo base dell'UART PS7 utilizzato per la comunicazione.
 */
#define UART_BASEADDR 			XPAR_PS7_UART_0_BASEADDR

/**
 * @brief Lunghezza del telegramma di connessione
 *
 * Definisce la lunghezza in byte del telegramma di connessione inviato dall'
 * applicazione.
 * Contiene: diametro ruota (4 byte), PPR encoder 1 (2 byte), PPR encoder 2
 * (2 byte).
 */
#define L_TELEGRAMMA_CONN   	(uint16_t) 8

/**
 * @brief Lunghezza del telegramma di funzionamento
 *
 * Definisce la lunghezza in byte del telegramma di funzionamento inviato dall'
 * applicazione.
 * Contiene comandi e parametri per il funzionamento del sistema.
 */
#define L_TELEGRAMMA_FUNZ   	(uint16_t) 14

/**
 * @brief Lunghezza del telegramma di risposta
 *
 * Definisce la lunghezza in byte del telegramma di risposta inviato al
 * dispositivo.
 * Contiene: velocità encoder 1 (4 byte), velocità encoder 2 (4 byte),
 * conteggio encoder 1 (2 byte), conteggio encoder 2 (2 byte).
 */
#define L_TELEGRAMMA_RISP   	(uint16_t) 13


/**
 * @brief Lunghezza della sezione valore nel telegramma di funzionamento
 *
 * Definisce la lunghezza in byte della sezione contenente i valori principali
 * nel telegramma di funzionamento. L'ultimo byte è riservato all'
 * identificatore.
 */
#define L_FUNZ_VALORE   		(uint16_t) 9

/**
 * @brief Lunghezza della sezione addon nel telegramma di funzionamento
 *
 * Definisce la lunghezza in byte della sezione contenente dati aggiuntivi
 * nel telegramma di funzionamento. L'ultimo byte è riservato all'
 * identificatore.
 */
#define L_FUNZ_ADDON   			(uint16_t) 5

/**
 * @brief Diametro massimo consentito per la ruota
 *
 * Definisce il valore massimo accettabile per il diametro della ruota, in
 * metri.
 */
#define MAX_DIAMETRO_RUOTA 		(float_t) 1.25

/**
 * @brief Diametro minimo consentito per la ruota
 *
 * Definisce il valore minimo accettabile per il diametro della ruota, in metri.
 */
#define MIN_DIAMETRO_RUOTA 		(float_t) 0.8

/**
 * @brief Massimo valore di PPR (Pulses Per Revolution) consentito per l'encoder
 *
 * Definisce il valore massimo accettabile per il numero di impulsi per giro
 * dell'encoder.
 */
#define MAX_PPR_ENCODER 		(uint16_t) 128

/**
 * @brief Minimo valore di PPR (Pulse Per Revolution) consentito per l'encoder
 *
 * Definisce il valore minimo accettabile per il numero di impulsi per giro
 * dell'encoder.
 */
#define MIN_PPR_ENCODER 		(uint16_t) 80

/**
 * @brief Valore fisso da mandare come ultimo byte nel telegramma di risposta
 */
#define IDENTIFICATIVO_RISPOSTA 	(uint8_t) 218


/**
 * @brief Unione per la conversione tra float e array di byte
 *
 * @details Questa unione permette di accedere a un valore float sia come
 * numero in virgola mobile che come array di byte. Utile per la
 * serializzazione e deserializzazione di dati float.
 */
union float_bytes {
	/* @brief Valore float */
    float_t value;
    /* @brief Array di byte rappresentante il float */
    uint8_t bytes[sizeof(float_t)];
};

/**
 * @brief Unione per la conversione tra uint16_t e array di byte
 *
 * @details Questa unione permette di accedere a un valore uint16_t sia come
 * intero a 16 bit che come array di byte. Utile per la serializzazione e
 * deserializzazione di dati uint16_t.
 */
union uint16_bytes {
	/* @brief Valore uint16_t */
	uint16_t value;
	/* @brief Array di byte rappresentante il uint16_t */
    uint8_t bytes[sizeof(uint16_t)];
};


/******************************************************************************
 * STATIC VARIABLES
 *****************************************************************************/

/**
 * @brief Istanza della struttura di configurazione UART
 *
 * Questa variabile contiene la configurazione e lo stato dell'UART del Zynq.
 * Viene utilizzata per tutte le operazioni di comunicazione UART nel sistema.
 */
static XUartPs Uart_Ps;

/**
 * @brief Stato della connessione con l'applicazione
 *
 * Indica se la connessione con l'applicazione esterna è attualmente stabilita.
 * - true: la connessione è attiva
 * - false: la connessione non è attiva
 */
static bool stato_connessione_app = false;

/**
 * @brief Flag di avvenuto handshake
 *
 * Indica se l'handshake con l'applicazione è avvenuto.
 * - true: l'handshake è avvenuto con successo
 * - false: l'handshake non è ancora avvenuto
 *
 * Questo flag viene usato per gestire la sequenza della comunicazione con l'
 * app.
 * La comunicazione è fatta in modo che la task di scrittura venga attivata se
 * si è fatta una lettura, e si vada in lettura se si è fatta una scrittura.
 */
static bool handshake_avvenuto = false;


/******************************************************************************
 * STATIC FUNCTION PROTOTYPES
 *****************************************************************************/
static void  leggi_telegramma_di_connessione(void);
static void leggi_telegramma_funzionamento(void);
static void azione_funzionamento_valore(uint8_t identificatore,
										const uint8_t *array_stringa);


/******************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/

/**
 * @brief Legge il telegramma di connessione dall'applicazione
 *
 * Questa funzione riceve e processa il telegramma di connessione inviato dall'
 * applicazione. Il telegramma contiene informazioni sul diametro della ruota
 * e sui ppr degli encoder. La funzione verifica che questi valori rientrino
 * nei limiti accettabili  e, se validi, li assegna agli encoder e aggiorna
 * lo stato della connessione.
 */
static void leggi_telegramma_di_connessione()
{
	uint16_t n_byte_ricevuti = 0;
	uint8_t byte_ricevuti[L_TELEGRAMMA_CONN];
	union float_bytes diametro;
	union uint16_bytes ppr1;
	union uint16_bytes ppr2;

	do {
		/* Attendo che un byte arrivi */
		while (!(XUartPs_IsReceiveData(Uart_Ps.Config.BaseAddress)))
		{
			/* Wait */
		}

		/* Leggi byte ricevuto*/
		uint8_t byte = XUartPs_ReadReg(Uart_Ps.Config.BaseAddress,
										XUARTPS_FIFO_OFFSET) & 0xFF;

		/* Salva byte ricevuto nel buffer */
		byte_ricevuti[n_byte_ricevuti] = byte;
		n_byte_ricevuti++;

	}while(n_byte_ricevuti != L_TELEGRAMMA_CONN);


	/* Estraggo diametro della ruota (little endian)*/
	(void) memcpy(diametro.bytes, byte_ricevuti, sizeof(float_t));

	/* Estraggo ppr encoder 1 (little endian)*/
	(void) memcpy(ppr1.bytes, &byte_ricevuti[4], sizeof(uint16_t));

	/* Estraggo ppr encoder 2 (little endian)*/
	(void) memcpy(ppr2.bytes, &byte_ricevuti[6], sizeof(uint16_t));

	/* Controllo se i parametri rientrano nei valori corretti */
	if(	(diametro.value > MAX_DIAMETRO_RUOTA) ||
		(diametro.value < MIN_DIAMETRO_RUOTA) )
	{
		/* Diametro fuori dai limiti accettabili */
		stato_connessione_app = false;
	}
	else if( (ppr1.value > MAX_PPR_ENCODER) ||
			  (ppr1.value < MIN_PPR_ENCODER) )
	{
		/* ppr 1 fuori dai limiti accettabili */
		stato_connessione_app = false;
	}
	else if( (ppr2.value > MAX_PPR_ENCODER) ||
			 (ppr2.value < MIN_PPR_ENCODER)	)
	{
		/* ppr 2 fuori dai limiti accettabili */
		stato_connessione_app = false;
	}
	else
	{
		/* I controlli sono passati, assegno i parametri agli encoder */
		assegna_ppr_encoder1(ppr1.value);
		assegna_ppr_encoder2(ppr2.value);
		assegna_diametro_ruota(diametro.value);
		aggiorna_passo_encoder1();
		aggiorna_passo_encoder2();
		/* Imposta lo stato di connessione a true */
		stato_connessione_app = true;
	}

}

/**
 * @brief Legge il telegramma di funzionamento dall'UART
 *
 * @details Questa funzione legge un telegramma di funzionamento dall'UART,
 * estrae i dati e gli identificatori per il valore e l'addon, ed esegue
 * l'azione corrispondente al valore ricevuto.
 */
static void leggi_telegramma_funzionamento()
{
	uint16_t n_byte_ricevuti = 0;
	uint8_t byte_ricevuti[L_TELEGRAMMA_FUNZ];
	uint8_t identificatore_valore;
	uint8_t stringa_valore[L_FUNZ_VALORE];

	/* TODO Aggiungere le funzioni per l'addon */
	uint8_t stringa_addon[L_FUNZ_ADDON];
	uint8_t identificatore_addon;


	do {
		/* Attendo che un byte arrivi */
		while (!(XUartPs_IsReceiveData(Uart_Ps.Config.BaseAddress)))
		{
			/* Wait */
		}

		/* Leggi byte ricevuto*/
		uint8_t byte = XUartPs_ReadReg(Uart_Ps.Config.BaseAddress,
										XUARTPS_FIFO_OFFSET) & 0xFF;

		/* Salva byte ricevuto*/
		byte_ricevuti[n_byte_ricevuti] = byte;
		n_byte_ricevuti++;

	}while(n_byte_ricevuti != L_TELEGRAMMA_FUNZ);

	/* Estraggo identificatore telegramma valore */
	identificatore_valore = byte_ricevuti[L_FUNZ_VALORE - 1U];
	(void) memcpy(&stringa_valore, &byte_ricevuti[0],
			sizeof(uint8_t) * L_FUNZ_VALORE);
	azione_funzionamento_valore(identificatore_valore, stringa_valore);


	/* Estraggo identificatore telegramma addon */
	identificatore_addon = byte_ricevuti[L_FUNZ_ADDON - 1U];
	(void) memcpy(&stringa_valore, &byte_ricevuti[L_FUNZ_VALORE - 1U],
			sizeof(uint8_t) * L_FUNZ_ADDON);


}

/**
 * @brief Esegue l'azione corrispondente al valore ricevuto nel telegramma
 *
 * @param identificatore Byte che identifica il tipo di azione da eseguire
 * @param array_stringa Array contenente i dati del telegramma
 *
 * @details Questa funzione interpreta l'identificatore ricevuto e esegue
 * l'azione corrispondente, che può includere l'assegnazione di velocità o
 * accelerazione agli encoder, la disconnessione dall'applicazione o il reset
 * della cinematica degli encoder.
 */
static void azione_funzionamento_valore(uint8_t identificatore,
		const uint8_t array_stringa[])
{
	union float_bytes dato_valore1;
	union float_bytes dato_valore2;

	switch (identificatore)
	{
		/* Telegramma vuoto */
	    case 0x00U:
	        /* Non succede niente */
	        break;

	    /* Telegramma assegnazione velocità su encoder 1 */
	    case 0x01U:
	        (void) memcpy(dato_valore1.bytes, &array_stringa[0],
	        		sizeof(float_t));
	        assegna_velocita_encoder1(dato_valore1.value);
	        break;

		/* Telegramma assegnazione velocità su encoder 2 */
	    case 0x02U:
	        (void) memcpy(dato_valore2.bytes, &array_stringa[4],
	        		sizeof(float_t));
	        assegna_velocita_encoder2(dato_valore2.value);
	        break;

		/* Telegramma assegnazione velocità su entrambi gli encoder */
	    case 0x03U:
	        (void) memcpy(dato_valore1.bytes, &array_stringa[0],
	        		sizeof(float_t));
	        (void) memcpy(dato_valore2.bytes, &array_stringa[4],
	        		sizeof(float_t));
	        assegna_velocita_encoder1(dato_valore1.value);
	        assegna_velocita_encoder2(dato_valore2.value);
	        break;

		/* Telegramma assegnazione accelerazione su encoder 1 */
	    case 0x04U:
	        (void) memcpy(dato_valore1.bytes, &array_stringa[0],
	        		sizeof(float_t));
	        assegna_accelerazione_encoder1(dato_valore1.value);
	        break;

	    /* Telegramma assegnazione accelerazione su encoder 2 */
	    case 0x05U:
	        (void) memcpy(dato_valore2.bytes, &array_stringa[4],
	        		sizeof(float_t));
	        assegna_accelerazione_encoder2(dato_valore2.value);
	        break;

	    /* Telegramma assegnazione accelerazione su entrambi gli encoder */
	    case 0x06U:
	        (void) memcpy(dato_valore1.bytes, &array_stringa[0],
	        		sizeof(float_t));
	        (void) memcpy(dato_valore2.bytes, &array_stringa[4],
	        		sizeof(float_t));
	        assegna_accelerazione_encoder1(dato_valore1.value);
	        assegna_accelerazione_encoder2(dato_valore2.value);
	        break;

		/* Telegramma per disconnettersi dall'applicazione */
	    case 0x07U:
	        inizializza_variabili_encoder();
	        stato_connessione_app = false;
	        handshake_avvenuto = false;
	        break;

	    /* Telegramma per resettare la cinematica degli encoder */
	    case 0x08U:
	        assegna_accelerazione_encoder1(0);
	        assegna_accelerazione_encoder2(0);
	        assegna_velocita_encoder1(0);
	        assegna_velocita_encoder2(0);
	        break;

	    default:
	        /* Non succede niente */
	        break;
	}
}



/************************************
 * GLOBAL FUNCTIONS
 ************************************/

void leggi_telegramma()
{
	if(stato_connessione_app == false)
	{
		leggi_telegramma_di_connessione();
	}
	else
	{
		leggi_telegramma_funzionamento();
	}
	handshake_avvenuto = true;
}

void inizializza_uart()
{
	XUartPs_Config *Config;

	Config = XUartPs_LookupConfig(UART_BASEADDR);

	XUartPs_CfgInitialize(&Uart_Ps, Config, Config->BaseAddress);
}

void manda_telegramma_di_risposta()
{

	if((stato_connessione_app == true) && (handshake_avvenuto == true))
	{
		uint8_t buffer[L_TELEGRAMMA_RISP];  /* 96 bits = 12 bytes */
		union float_bytes temp_vel;
		uint16_t temp_cont;

		/* Mando velocita' del GIT 1*/
		temp_vel.value = ritorna_velocita_encoder1();
		(void) memcpy(buffer, temp_vel.bytes, sizeof(float_t));

    	/* Mando velocita' del GIT 2*/
		temp_vel.value = ritorna_velocita_encoder2();
    	(void) memcpy(&buffer[4], temp_vel.bytes, sizeof(float_t));

    	/* Mando conteggio del GIT 1 */
    	temp_cont = ritorna_conteggio_encoder1();
    	buffer[8] = temp_cont & 0xFFU;
    	buffer[9] = (temp_cont >> 8U) & 0xFFU;

    	/* Mando conteggio del GIT 2 */
    	temp_cont = ritorna_conteggio_encoder2();
    	buffer[10] = temp_cont & 0xFFU;
    	buffer[11] = (temp_cont >> 8U) & 0xFFU;

    	/* Mando identificativo del telegramma della risposta (fisso) */
    	buffer[12] = IDENTIFICATIVO_RISPOSTA;

    	// Send the buffer over UART
    	for(uint16_t indice = 0; indice < L_TELEGRAMMA_RISP; indice++)
    	{
    		XUartPs_SendByte(UART_BASEADDR, buffer[indice]);
    	}

    	handshake_avvenuto = false;
	}
	else
	{
		/* Non succede nulla */
	}
}

bool ritorna_stato_connessione_app()
{
	return stato_connessione_app;
}
