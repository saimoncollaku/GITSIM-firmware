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
#define L_TELEGRAMMA_CONN   	8
#define L_TELEGRAMMA_FUNZ   	14
#define L_TELEGRAMMA_RISP   	12
#define L_FUNZ_VALORE   		9
#define L_FUNZ_ADDON   			5
#define MAX_DIAMETRO_RUOTA 		1.25
#define MIN_DIAMETRO_RUOTA 		0.8
#define MAX_PPR_ENCODER 		128
#define MIN_PPR_ENCODER 		80

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
void  leggi_telegramma_di_connessione(void);
void leggi_telegramma_funzionamento(void);
void azione_funzionamento_valore(uint8_t identificatore, uint8_t *array_stringa);


/************************************
 * STATIC FUNCTIONS
 ************************************/

void leggi_telegramma_di_connessione()
{
	uint16_t n_byte_ricevuti = 0;
	uint8_t byte;
	uint8_t byte_ricevuti[L_TELEGRAMMA_CONN];

	float_t diametro;
	uint16_t ppr1;
	uint16_t ppr2;

//	XUartPs_WriteReg(Uart_Ps.Config.BaseAddress, XUARTPS_CR_OFFSET,
//			XUartPs_ReadReg(Uart_Ps.Config.BaseAddress, XUARTPS_CR_OFFSET) |
//			XUARTPS_RXBS_MASK);


	do {
		/* Attendo che un byte arrivi */
		while (!(XUartPs_IsReceiveData(Uart_Ps.Config.BaseAddress)))
		{
			/* Wait */
		}

		/* Leggi byte ricevuto*/
		byte = XUartPs_ReadReg(Uart_Ps.Config.BaseAddress,
										XUARTPS_FIFO_OFFSET) & 0xFF;

		/* Salva byte ricevuto*/
		byte_ricevuti[n_byte_ricevuti] = byte;
		n_byte_ricevuti++;

	}while(n_byte_ricevuti != L_TELEGRAMMA_CONN);


	/* Estraggo diametro della ruota (little endian)*/
	memcpy(&diametro, byte_ricevuti, sizeof(float_t));

	/* Estraggo ppr encoder 1 (little endian)*/
	memcpy(&ppr1, &byte_ricevuti[4], sizeof(uint16_t));

	/* Estraggo ppr encoder 2 (little endian)*/
	memcpy(&ppr2, &byte_ricevuti[6], sizeof(uint16_t));

	/* Controllo se i parametri rientrano nei valori corretti */
	if((diametro > MAX_DIAMETRO_RUOTA) || (diametro < MIN_DIAMETRO_RUOTA))
	{
		stato_connessione_app = false;
	}
	else if((ppr1 > MAX_PPR_ENCODER) || (ppr1 < MIN_PPR_ENCODER))
	{
		stato_connessione_app = false;
	}
	else if((ppr2 > MAX_PPR_ENCODER) || (ppr2 < MIN_PPR_ENCODER))
	{
		stato_connessione_app = false;
	}
	else
	{
		/* I controlli sono passiti, assegno i parametri agli encoder */
		assegna_ppr_encoder1(ppr1);
		assegna_ppr_encoder2(ppr2);
		assegna_diametro_ruota(diametro);
		aggiorna_passo_encoder1();
		aggiorna_passo_encoder2();
		stato_connessione_app = true;
	}

}

void leggi_telegramma_funzionamento(void)
{
	uint16_t n_byte_ricevuti = 0;
	uint8_t byte;
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
		byte = XUartPs_ReadReg(Uart_Ps.Config.BaseAddress,
										XUARTPS_FIFO_OFFSET) & 0xFF;

		/* Salva byte ricevuto*/
		byte_ricevuti[n_byte_ricevuti] = byte;
		n_byte_ricevuti++;

	}while(n_byte_ricevuti != L_TELEGRAMMA_FUNZ);

	/* Estraggo identificatore telegramma valore */
	identificatore_valore = byte_ricevuti[L_FUNZ_VALORE - 1];
	memcpy(&stringa_valore, &byte_ricevuti[0], sizeof(uint8_t) * L_FUNZ_VALORE);
	azione_funzionamento_valore(identificatore_valore, stringa_valore);


	/* Estraggo identificatore telegramma addon */
	identificatore_addon = byte_ricevuti[L_FUNZ_ADDON - 1];
	memcpy(&stringa_valore, &byte_ricevuti[L_FUNZ_VALORE - 1], sizeof(uint8_t) * L_FUNZ_ADDON);


}

void azione_funzionamento_valore(uint8_t identificatore, uint8_t array_stringa[])
{
	float_t dato_valore1;
	float_t dato_valore2;

	if(identificatore == 0x00)
	{
		/* Non succede niente */
	}
	else if(identificatore == 0x01)
	{
		memcpy(&dato_valore1, &array_stringa[0], sizeof(float_t));
		assegna_velocita_encoder1(dato_valore1);
	}
	else if(identificatore == 0x02)
	{
		memcpy(&dato_valore2, &array_stringa[4], sizeof(float_t));
		assegna_velocita_encoder2(dato_valore2);
	}
	else if(identificatore == 0x03)
	{
		memcpy(&dato_valore1, &array_stringa[0], sizeof(float_t));
		memcpy(&dato_valore2, &array_stringa[4], sizeof(float_t));
		assegna_velocita_encoder1(dato_valore1);
		assegna_velocita_encoder2(dato_valore2);
	}
	else if(identificatore == 0x04)
	{
		memcpy(&dato_valore1, &array_stringa[0], sizeof(float_t));
		assegna_accelerazione_encoder1(dato_valore1);
	}
	else if(identificatore == 0x05)
	{
		memcpy(&dato_valore2, &array_stringa[4], sizeof(float_t));
		assegna_accelerazione_encoder2(dato_valore2);
	}
	else if(identificatore == 0x06)
	{
		memcpy(&dato_valore1, &array_stringa[0], sizeof(float_t));
		memcpy(&dato_valore2, &array_stringa[4], sizeof(float_t));
		assegna_accelerazione_encoder1(dato_valore1);
		assegna_accelerazione_encoder2(dato_valore2);
	}
	else if(identificatore == 0x07)
	{
		inizializza_variabili_encoder();
		stato_connessione_app = false;
		handshake_avvenuto = false;
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
    uint8_t buffer[L_TELEGRAMMA_RISP];  /* 96 bits = 12 bytes */
    float_t temp_vel;
    uint16_t temp_cont;

	if(stato_connessione_app == true && handshake_avvenuto == true)
	{
		/* Mando velocita' del GIT 1*/
		temp_vel = ritorna_velocita_encoder1();
    	memcpy(buffer, &temp_vel, sizeof(float_t));

    	/* Mando velocita' del GIT 2*/
    	temp_vel = ritorna_velocita_encoder2();
    	memcpy(buffer + 4, &temp_vel, sizeof(float_t));

    	/* Mando conteggio del GIT 1 */
    	temp_cont = ritorna_conteggio_encoder1();
    	buffer[8] = temp_cont & 0xFF;
    	buffer[9] = (temp_cont >> 8) & 0xFF;

    	/* Mando conteggio del GIT 2 */
    	temp_cont = ritorna_conteggio_encoder2();
    	buffer[10] = temp_cont & 0xFF;
    	buffer[11] = (temp_cont >> 8) & 0xFF;

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
