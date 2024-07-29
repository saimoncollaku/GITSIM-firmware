/**
 ********************************************************************************
 * @file    side.h
 * @author  wasab
 * @date    5 Jul 2024
 * @brief   
 ********************************************************************************
 */

#ifndef HEADERS_SIDE_H_
#define HEADERS_SIDE_H_

#ifdef __cplusplus
extern "C" {
#endif

/************************************
 * INCLUDES
 ************************************/
#include "xil_io.h"
#include "stdio.h"
#include <stdint.h>

/************************************
 * GLOBAL FUNCTION PROTOTYPES
 ************************************/
void side(void *CallBack_Timer);
void inizializza_side(void);



#ifdef __cplusplus
}
#endif

#endif 
