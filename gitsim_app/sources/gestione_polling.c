/**
 ********************************************************************************
 * @file    gestione_polling.c
 * @author  wasab
 * @date    5 Jul 2024
 * @brief   
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "gestione_polling.h"
#include "xil_exception.h"
#include "xscugic.h"
#include "side.h"

/************************************
 * EXTERN VARIABLES
 ************************************/
XScuTimer IstanzaTimer; /**< Istanza del timer hardware */

/************************************
 * PRIVATE MACROS AND DEFINES
 ************************************/
#define INTC_DEVICE_ID       XPAR_SCUGIC_SINGLE_DEVICE_ID /**<ID dell'interrupt
                                                          controller */
#define TIMER_DEVICE_ID		 XPAR_XSCUTIMER_0_DEVICE_ID /**< ID del timer
                                                        hardware */
#define TIMER_IRPT_INTR		 XPAR_SCUTIMER_INTR /**< Numero di interrupt del
                                                timer associato allo SCU */

/************************************
 * STATIC VARIABLES
 ************************************/
static XScuGic IstanzaInterrupt; /**< Istanza dell'interrupt*/

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
void connetti_interrupt_su_timer(XScuGic *IstanzaInterrupt,
								XScuTimer *IstanzaTimer,
								uint16_t ID_Timer_Interrupt);
int setup_interrupt_system(XScuGic *IstanzaGIC);


/************************************
 * STATIC FUNCTIONS
 ************************************/
int setup_interrupt_system(XScuGic *IstanzaGIC)
{
	int Status;

	XScuGic_Config *IntcConfig; /* Istanza dell'interrupt controller */

	Xil_ExceptionInit();

	/* Inizializza i driver dell'interrupt controller */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL == IntcConfig) {
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IstanzaGIC, IntcConfig,
					IntcConfig->CpuBaseAddress);

	/* Collecga l'handler dell'interrupt alla logica di gestione di
	 * di interrupt hw del processore */
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
				(Xil_ExceptionHandler)XScuGic_InterruptHandler,
				IstanzaGIC);

	/* Abilita l'interrupt del processore */
	Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);

	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}
void connetti_interrupt_su_timer(XScuGic *IstanzaInterrupt,
								XScuTimer *IstanzaTimer,
								uint16_t ID_Timer_Interrupt)
{
	Xil_ExceptionInit();

	/* Associazione funzione di interrupt */
	XScuGic_Connect(IstanzaInterrupt, ID_Timer_Interrupt,
			(Xil_ExceptionHandler)side,
			(void *)IstanzaTimer);

	/* Abilita GIC interrupt*/
	XScuGic_Enable(IstanzaInterrupt, ID_Timer_Interrupt);

	/* Abilita interrupt sul timer */
	XScuTimer_EnableInterrupt(IstanzaTimer);
	Xil_ExceptionEnable();
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
int32_t inizializza_polling_timer()
{
	int Status;
	XScuTimer_Config *ConfigPtr;

	/* Inizializza l'interrupt */
	setup_interrupt_system(&IstanzaInterrupt);

	/* Inizializza il driver del timer */
	ConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);

	Status = XScuTimer_CfgInitialize(&IstanzaTimer, ConfigPtr,
									 ConfigPtr->BaseAddr);

	/* Test */
	Status = XScuTimer_SelfTest(&IstanzaTimer);

	/* Connetti l'interrupt sul reset del timer */
	connetti_interrupt_su_timer(&IstanzaInterrupt, &IstanzaTimer, TIMER_IRPT_INTR);

	/* Abilita l'auto reset del timer */
	XScuTimer_EnableAutoReload(&IstanzaTimer);

	/* Imposta il prescaler */
	XScuTimer_SetPrescaler(&IstanzaTimer, TIMER_PRESCALER - 1);

	/* Carica valore a cui il timer si resetta */
	XScuTimer_LoadTimer(&IstanzaTimer, TIMER_LOAD_VALUE - 1);

	/* Inizia il conteggio */
	XScuTimer_Start(&IstanzaTimer);

	return Status;
}

