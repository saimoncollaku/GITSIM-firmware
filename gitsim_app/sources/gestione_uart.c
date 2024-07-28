/**
 ********************************************************************************
 * @file    gestione_uart.c
 * @author  wasab
 * @date    5 Jul 2024
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "xuartps.h"
#include "gestione_uart.h"
#include "emulazione_encoder.h"

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define UART_BASEADDR 			XPAR_PS7_UART_0_BASEADDR
#define L_TELEGRAMMA_CONN   	(uint16_t) 8
#define L_TELEGRAMMA_FUNZ   	(uint16_t) 14
#define L_TELEGRAMMA_RISP   	(uint16_t) 12
#define L_FUNZ_VALORE   		(uint16_t) 9
#define L_FUNZ_ADDON   			(uint16_t) 5
#define MAX_DIAMETRO_RUOTA 		(float_t) 1.25
#define MIN_DIAMETRO_RUOTA 		(float_t) 0.8
#define MAX_PPR_ENCODER 		(uint16_t) 128
#define MIN_PPR_ENCODER 		(uint16_t) 80

/************************************
 * PRIVATE TYPEDEFS
 ************************************/

/************************************
 * STATIC VARIABLES
 ************************************/
static XUartPs Uart_Ps;
static bool stato_connessione_app = false;
static bool handshake_avvenuto = false;

/************************************
 * GLOBAL VARIABLES
 ************************************/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static void  leggi_telegramma_di_connessione( void );
static void leggi_telegramma_funzionamento( void );
static void azione_funzionamento_valore(uint8_t identificatore, const uint8_t *array_stringa);


/************************************
 * STATIC FUNCTIONS
 ************************************/

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

		/* Salva byte ricevuto*/
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
	if((diametro.value > MAX_DIAMETRO_RUOTA) || (diametro.value < MIN_DIAMETRO_RUOTA))
	{
		stato_connessione_app = false;
	}
	else if((ppr1.value > MAX_PPR_ENCODER) || (ppr1.value < MIN_PPR_ENCODER))
	{
		stato_connessione_app = false;
	}
	else if((ppr2.value > MAX_PPR_ENCODER) || (ppr2.value < MIN_PPR_ENCODER))
	{
		stato_connessione_app = false;
	}
	else
	{
		/* I controlli sono passiti, assegno i parametri agli encoder */
		assegna_ppr_encoder1(ppr1.value);
		assegna_ppr_encoder2(ppr2.value);
		assegna_diametro_ruota(diametro.value);
		aggiorna_passo_encoder1();
		aggiorna_passo_encoder2();
		stato_connessione_app = true;
	}

}

static void leggi_telegramma_funzionamento()
{
	uint16_t n_byte_ricevuti = 0;
	uint8_t byte_ricevuti[L_TELEGRAMMA_FUNZ];
	uint8_t identificatore_valore;
	uint8_t stringa_valore[L_FUNZ_VALORE];
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
	(void) memcpy(&stringa_valore, &byte_ricevuti[0], sizeof(uint8_t) * L_FUNZ_VALORE);
	azione_funzionamento_valore(identificatore_valore, stringa_valore);


	/* Estraggo identificatore telegramma addon */
	identificatore_addon = byte_ricevuti[L_FUNZ_ADDON - 1U];
	(void) memcpy(&stringa_valore, &byte_ricevuti[L_FUNZ_VALORE - 1U], sizeof(uint8_t) * L_FUNZ_ADDON);


}

static void azione_funzionamento_valore(uint8_t identificatore, const uint8_t array_stringa[])
{
	union float_bytes dato_valore1;
	union float_bytes dato_valore2;

	if(identificatore == 0x00U)
	{
		/* Non succede niente */
	}
	else if(identificatore == 0x01U)
	{
		(void) memcpy(dato_valore1.bytes, &array_stringa[0], sizeof(float_t));
		assegna_velocita_encoder1(dato_valore1.value);
	}
	else if(identificatore == 0x02U)
	{
		(void) memcpy(dato_valore2.bytes, &array_stringa[4], sizeof(float_t));
		assegna_velocita_encoder2(dato_valore2.value);
	}
	else if(identificatore == 0x03U)
	{
		(void) memcpy(dato_valore1.bytes, &array_stringa[0], sizeof(float_t));
		(void) memcpy(dato_valore2.bytes, &array_stringa[4], sizeof(float_t));
		assegna_velocita_encoder1(dato_valore1.value);
		assegna_velocita_encoder2(dato_valore2.value);
	}
	else if(identificatore == 0x04U)
	{
		(void) memcpy(dato_valore1.bytes, &array_stringa[0], sizeof(float_t));
		assegna_accelerazione_encoder1(dato_valore1.value);
	}
	else if(identificatore == 0x05U)
	{
		(void) memcpy(dato_valore2.bytes, &array_stringa[4], sizeof(float_t));
		assegna_accelerazione_encoder2(dato_valore2.value);
	}
	else if(identificatore == 0x06U)
	{
		(void) memcpy(dato_valore1.bytes, &array_stringa[0], sizeof(float_t));
		(void) memcpy(dato_valore2.bytes, &array_stringa[4], sizeof(float_t));
		assegna_accelerazione_encoder1(dato_valore1.value);
		assegna_accelerazione_encoder2(dato_valore2.value);
	}
	else if(identificatore == 0x07U)
	{
		inizializza_variabili_encoder();
		stato_connessione_app = false;
		handshake_avvenuto = false;
	}
	else if(identificatore == 0x08U)
	{
		assegna_accelerazione_encoder1(0);
		assegna_accelerazione_encoder2(0);
		assegna_velocita_encoder1(0);
		assegna_velocita_encoder2(0);
	}
	else
	{
		/* Non succede niente */
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
