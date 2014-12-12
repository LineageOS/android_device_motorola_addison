/******************************************************************************
 *
 *  $Id: AKMD_Driver.h 877 2012-11-30 07:46:10Z yamada.rj $
 *
 * -- Copyright Notice --
 *
 * Copyright (c) 2004 Asahi Kasei Microdevices Corporation, Japan
 * All Rights Reserved.
 *
 * This software program is the proprietary program of Asahi Kasei Microdevices
 * Corporation("AKM") licensed to authorized Licensee under the respective
 * agreement between the Licensee and AKM only for use with AKM's electronic
 * compass IC.
 *
 * THIS SOFTWARE IS PROVIDED TO YOU "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABLITY, FITNESS FOR A PARTICULAR PURPOSE AND NON INFRINGEMENT OF
 * THIRD PARTY RIGHTS, AND WE SHALL NOT BE LIABLE FOR ANY LOSSES AND DAMAGES
 * WHICH MAY OCCUR THROUGH USE OF THIS SOFTWARE.
 *
 * -- End Asahi Kasei Microdevices Copyright Notice --
 *
 ******************************************************************************/
#ifndef AKMD_INC_AKMD_DRIVER_H
#define AKMD_INC_AKMD_DRIVER_H

/* Device driver */
#ifdef AKMD_FOR_AK8963
#include "akm8963.h"

#elif AKMD_FOR_AK8975
#include "akm8975.h"

#elif AKMD_FOR_AK09911
#include "akm09911.h"

#elif AKMD_FOR_AK09912
#include "akm09912.h"

#else
#error "No devices are defined."
#endif

#include <stdint.h>			/* int8_t, int16_t etc. */

/*** Constant definition ******************************************************/
#define AKD_TRUE	1		/*!< Represents true */
#define AKD_FALSE	0		/*!< Represents false */
#define AKD_SUCCESS	1		/*!< Represents success.*/
#define AKD_FAIL	0		/*!< Represents fail. */
#define AKD_ERROR	-1		/*!< Represents error. */

/*! 0:Don't Output data, 1:Output data */
#define AKD_DBG_DATA	0
/*! Typical interval in ns */
#define AKM_MEASUREMENT_TIME_NS	((AKM_MEASURE_TIME_US) * 1000)


/*** Type declaration *********************************************************/
typedef unsigned char BYTE;


/*** Global variables *********************************************************/

/*** Prototype of Function  ***************************************************/

int16_t AKD_InitDevice(void);

void AKD_DeinitDevice(void);

int16_t AKD_TxData(
		const BYTE address,
		const BYTE* data,
		const uint16_t numberOfBytesToWrite);

int16_t AKD_RxData(
		const BYTE address,
		BYTE* data,
		const uint16_t numberOfBytesToRead);

int16_t AKD_Reset(void);

int16_t AKD_GetSensorInfo(BYTE data[AKM_SENSOR_INFO_SIZE]);

int16_t AKD_GetSensorConf(BYTE data[AKM_SENSOR_CONF_SIZE]);

int16_t AKD_GetMagneticData(BYTE data[AKM_SENSOR_DATA_SIZE]);

void AKD_SetYPR(const int buf[AKM_YPR_DATA_SIZE]);

int16_t AKD_GetOpenStatus(int* status);

int16_t AKD_GetCloseStatus(int* status);

int16_t AKD_SetMode(const BYTE mode);

int16_t AKD_GetDelay(int64_t* delay);

int16_t AKD_GetLayout(int16_t* layout);

#endif //AKMD_INC_AKMD_DRIVER_H

