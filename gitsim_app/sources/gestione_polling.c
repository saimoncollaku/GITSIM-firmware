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

/** @brief ID dell'interrupt controller */
#define INTC_DEVICE_ID       XPAR_SCUGIC_SINGLE_DEVICE_ID

/** @brief ID del timer SCU */
#define TIMER_DEVICE_ID		 XPAR_XSCUTIMER_0_DEVICE_ID

/** @brief Numero di interrupt associato al timer  */
#define TIMER_IRPT_INTR		 XPAR_SCUTIMER_INTR

/************************************
 * STATIC VARIABLES
 ************************************/

/** @brief Istanza dell'interrupt controller */
static XScuGic istanza_interrupt_gic;

/** @brief Istanza del timer SCU */
static XScuTimer istanza_timer_scu;

/************************************
 * STATIC FUNCTION PROTOTYPES
 ************************************/
static void connetti_interrupt_su_timer(XScuGic *istanza_interrupt_gic,
								XScuTimer *istanza_timer,
								uint16_t n_timer_interrupt);
static s32 configura_gic_system(XScuGic *IstanzaGIC);


/************************************
 * STATIC FUNCTIONS
 ************************************/

/**
 * @brief Configura il sistema di interrupt GIC
 *
 * @param istanza_gic Puntatore all'istanza XScuGic da inizializzare
 * @return s32 XST_SUCCESS se la configurazione ha successo, XST_FAILURE altrimenti
 */
static s32 configura_gic_system(XScuGic *istanza_gic)
{
	s32 status = XST_SUCCESS;

	/* Configurazione dell'interrupt controller */
	XScuGic_Config *gic_config;

	/* Inizializzo il sistema di eccezioni */
	Xil_ExceptionInit();

	/* Ottengo la configurazione dell'interrupt controller */
	gic_config = XScuGic_LookupConfig(INTC_DEVICE_ID);
	if (NULL != gic_config)
	{
		/* Inizializza l'istanza dell'interrupt controller */
		status = XScuGic_CfgInitialize(istanza_gic, gic_config,
						gic_config->CpuBaseAddress);

		/*
		 * Collega l'handler dell'interrupt alla logica di gestione di
		 * di interrupt hw del processore
		 */
		Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
					(Xil_ExceptionHandler)XScuGic_InterruptHandler,
					istanza_gic);

		/* Abilita l'interrupt del processore */
		Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
	}
	else
	{
		status = XST_FAILURE;
	}

	return status;
}


/**
 * @brief Connette e configura l'interrupt per il timer SCU
 *
 * @param istanza_interrupt Puntatore all'istanza XScuGic (interrupt GIC)
 * @param istanza_timer Puntatore all'istanza XScuTimer (timer SCU)
 * @param n_timer_interrupt Numero dell'interrupt del timer
 */
static void connetti_interrupt_su_timer(XScuGic *istanza_interrupt,
								XScuTimer *istanza_timer,
								uint16_t n_timer_interrupt)
{
	/* Inizializzo il sistema di eccezioni */
	Xil_ExceptionInit();

	/* Associo il side loop al timer */
	XScuGic_Connect(istanza_interrupt, n_timer_interrupt,
			(Xil_ExceptionHandler)side_loop,
			(void *)istanza_timer);

	/* Abilito l'interrupt specifico per il GIC */
	XScuGic_Enable(istanza_interrupt, n_timer_interrupt);

	 /* Abilito la generazione di interrupt nel timer SCU */
	XScuTimer_EnableInterrupt(istanza_timer);

	/* Abilito il sistema globale di gestione delle eccezioni */
	Xil_ExceptionEnable();
}


/************************************
 * GLOBAL FUNCTIONS
 ************************************/

/**
 * @brief Inizializza e configura il timer per il polling
 *
 * Questa funzione configura l'interrupt GIC,
 * inizializza il timer SCU, esegue un self-test, connette l'interrupt,
 * e avvia il timer con le impostazioni specificate.
 */
void inizializza_polling_timer()
{
	s32 status = XST_SUCCESS;
	XScuTimer_Config *scu_config_pointer;

	/* Configura il Generic Interrupt Controller (GIC) */
	status = configura_gic_system(&istanza_interrupt_gic);
	while(status != XST_SUCCESS)
	{
		/* Configurazione interrupt fallita, mi blocco qui */
	}

	/* Inizializza il driver del timer SCU */
	scu_config_pointer = XScuTimer_LookupConfig(TIMER_DEVICE_ID);
	status = XScuTimer_CfgInitialize(&istanza_timer_scu, scu_config_pointer,
									 scu_config_pointer->BaseAddr);
	while(status != XST_SUCCESS)
	{
		/* Configurazione timer fallita, mi blocco qui */
	}

	/* Eseguo un self-test del timer */
	status = XScuTimer_SelfTest(&istanza_timer_scu);
	while(status != XST_SUCCESS)
	{
		/* Self-test del timer fallito, mi blocco qui */
	}

	/* Configuro l'interrupt sul reset del timer */
	connetti_interrupt_su_timer(&istanza_interrupt_gic, &istanza_timer_scu, TIMER_IRPT_INTR);

	/* Configuro il timer per ripartire ad ogni overflow */
	XScuTimer_EnableAutoReload(&istanza_timer_scu);

	/* Imposto il prescaler del timer */
	XScuTimer_SetPrescaler(&istanza_timer_scu, TIMER_PSC - 1U);

	/* Imposto il valore di ricarica del timer */
	XScuTimer_LoadTimer(&istanza_timer_scu, TIMER_LV - 1U);

	/* Avvio il conteggio del timer */
	XScuTimer_Start(&istanza_timer_scu);
}

/**
 * @brief Restituisce l'istanza del timer SCU utilizzato.
 *
 * Questa funzione fornisce accesso all'istanza globale del timer SCU,
 * utilizzata nel side loop per dare il timing all'aggiornamento ed
 * emulazione delle variabili di encoder.
 *
 * @return XScuTimer L'istanza corrente del timer SCU
 */
XScuTimer ritorna_istanza_timer(void)
{
	return istanza_timer_scu;
}

/**
 * @brief Calcola e restituisce il tempo di polling del timer
 *
 * Questa funzione calcola il tempo di polling basandosi sui valori di
 * TIMER_PSC (prescaler), TIMER_LV (valore di ricarica), e la frequenza
 * della CPU.
 *
 * Viene utilizzato nel side loop per dare il timing all'aggiornamento ed
 * emulazione delle variabili di encoder.
 *
 * @return float_t Il tempo di polling calcolato in secondi
 *
 * @note La formula utilizzata e':
 * \f[
 * 		T_{polling} = 2 \times \frac{prescaler
 * 		\times valore_{ricarica}}{frequenza_{CPU}}
 * \f]
 */
float_t ritorna_tempo_del_polling(void)
{
	float_t t_polling = (TIMER_PSC * TIMER_LV) / (APU_FREQ * 0.5);
	return t_polling;
}
