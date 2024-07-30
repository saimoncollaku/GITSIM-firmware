/**
 ********************************************************************************
 * @file    gestione_polling.c
 * @author  Saimon Collaku
 ********************************************************************************
 */

/************************************
 * INCLUDES
 ************************************/
#include "gestione_polling.h"
#include "xil_exception.h"
#include "ps7_init.h"
#include "xscugic.h"
#include "side.h"

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
//const static float_t t_polling_zynq = (TIMER_PSC * TIMER_LV) / (APU_FREQ * 0.5);
static XScuGic istanza_interrupt; /**< Istanza dell'interrupt*/
static XScuTimer istanza_timer_zynq; /**< Istanza del timer hardware */

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static void connetti_interrupt_su_timer(XScuGic *istanza_interrupt,
								XScuTimer *istanza_timer,
								uint16_t ID_Timer_Interrupt);
static s32 setup_interrupt_system(XScuGic *IstanzaGIC);


/************************************
 * STATIC FUNCTIONS
 ************************************/
static s32 setup_interrupt_system(XScuGic *IstanzaGIC)
{
	s32 status = XST_SUCCESS;

	XScuGic_Config *IntcConfig; /* Istanza dell'interrupt controller */

	Xil_ExceptionInit();

	/* Inizializza i driver dell'interrupt controller */
	IntcConfig = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL != IntcConfig)
	{
		status = XScuGic_CfgInitialize(IstanzaGIC, IntcConfig,
						IntcConfig->CpuBaseAddress);

		/* Collega l'handler dell'interrupt alla logica di gestione di
		 * di interrupt hw del processore */
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
					(Xil_ExceptionHandler)XScuGic_InterruptHandler,
					IstanzaGIC);

		/* Abilita l'interrupt del processore */
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	}
	else
	{
		status = XST_FAILURE;
	}

	return status;
}

static void connetti_interrupt_su_timer(XScuGic *istanza_interrupt,
								XScuTimer *istanza_timer,
								uint16_t ID_Timer_Interrupt)
{
	Xil_ExceptionInit();

	/* Associazione funzione di interrupt */
	XScuGic_Connect(istanza_interrupt, ID_Timer_Interrupt,
			(Xil_ExceptionHandler)side,
			(void *)istanza_timer);

	/* Abilita GIC interrupt*/
	XScuGic_Enable(istanza_interrupt, ID_Timer_Interrupt);

	/* Abilita interrupt sul timer */
	XScuTimer_EnableInterrupt(istanza_timer);
	Xil_ExceptionEnable();
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/
void inizializza_polling_timer()
{
	s32 status = XST_SUCCESS;
	XScuTimer_Config *ConfigPtr;

	/* Inizializza l'interrupt */
	status = setup_interrupt_system(&istanza_interrupt);
	while(status != XST_SUCCESS)
	{
		/* Interrupt setup fail */
	}

	/* Inizializza il driver del timer */
	ConfigPtr = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	status = XScuTimer_CfgInitialize(&istanza_timer_zynq, ConfigPtr,
									 ConfigPtr->BaseAddr);
	while(status != XST_SUCCESS)
	{
		/* Timer setup fail */
	}

	/* Test */
	status = XScuTimer_SelfTest(&istanza_timer_zynq);
	while(status != XST_SUCCESS)
	{
		/* Timer selftest fail */
	}

	/* Connetti l'interrupt sul reset del timer */
	connetti_interrupt_su_timer(&istanza_interrupt, &istanza_timer_zynq, TIMER_IRPT_INTR);

	/* Abilita l'auto reset del timer */
	XScuTimer_EnableAutoReload(&istanza_timer_zynq);

	/* Imposta il prescaler */
	XScuTimer_SetPrescaler(&istanza_timer_zynq, TIMER_PSC - 1U);

	/* Carica valore a cui il timer si resetta */
	XScuTimer_LoadTimer(&istanza_timer_zynq, TIMER_LV - 1U);

	/* Inizia il conteggio */
	XScuTimer_Start(&istanza_timer_zynq);
}

XScuTimer ritorna_istanza_timer(void)
{
	return istanza_timer_zynq;
}

float_t ritorna_tempo_del_polling(void)
{
	float_t t_polling_zynq = (TIMER_PSC * TIMER_LV) / (APU_FREQ * 0.5);
	return t_polling_zynq;
}
