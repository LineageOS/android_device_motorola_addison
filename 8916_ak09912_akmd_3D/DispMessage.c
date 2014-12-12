/******************************************************************************
 *
 *  $Id: DispMessage.c 877 2012-11-30 07:46:10Z yamada.rj $
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
#include "DispMessage.h"
#include "AKCommon.h"

/*!
 Print startup message to Android Log daemon.
 */
void Disp_StartMessage(void)
{
	ALOGI("AKMD 3D v20130531 (Library for AK%d: v%d.%d.%d.%d) started.",
		 AKSC_GetVersion_Device(),
		 AKSC_GetVersion_Major(),
		 AKSC_GetVersion_Minor(),
		 AKSC_GetVersion_Revision(),
		 AKSC_GetVersion_DateCode());
	ALOGI("Debug: %s", ((ENABLE_AKMDEBUG)?("ON"):("OFF")));
}

/*!
 Print ending message to Android Log daemon.
 */
void Disp_EndMessage(int ret)
{
	ALOGI("AKMD end (%d).", ret);
}

/*!
 Print calculated result.
 @param[in] prms A pointer to a #AKSCPRMS structure. The value of member
 variables of this structure will be printed.
 */
void Disp_MeasurementResult(AKSCPRMS* prms)
{
	AKMDEBUG(AKMDBG_DISP2, "FORMATION = %d\n", prms->m_form);

	// hr is in AKSC format,  i.e. 1LSB = 0.06uT
	AKMDEBUG(AKMDBG_DISP1, "HVEC[uT]:  x=%8.1f, y=%8.1f, z=%8.1f\n",
				DISP_CONV_AKSCF((int32)prms->m_hvec.u.x),
				DISP_CONV_AKSCF((int32)prms->m_hvec.u.y),
				DISP_CONV_AKSCF((int32)prms->m_hvec.u.z));

	AKMDEBUG(AKMDBG_DISP2, "HDOE Parameter Set:%s\n",
		((prms->m_hdoev.hthIdx == AKSC_HDFI_SMA)?"Small":"Normal"));
	AKMDEBUG(AKMDBG_DISP1, "LEVEL=%2d\n", prms->m_hdst);
	AKMDEBUG(AKMDBG_DISP2, "HOFFSET[uT]:  x=%8.1f, y=%8.1f, z=%8.1f\n",
				DISP_CONV_AKSCF((int32)prms->m_ho.u.x + prms->m_hbase.u.x),
				DISP_CONV_AKSCF((int32)prms->m_ho.u.y + prms->m_hbase.u.y),
				DISP_CONV_AKSCF((int32)prms->m_ho.u.z + prms->m_hbase.u.z));
	AKMDEBUG(AKMDBG_DISP2, "DOE HR[uT]=%5.1f\n",
				DISP_CONV_AKSCF(prms->m_hdoev.hrdoeHR));

	AKMDEBUG(AKMDBG_DISP1, "\n");
}

/*!
 Output main menu to stdout and wait for user input from stdin.
 @return Selected mode.
 */
MODE Menu_Main(void)
{
	char msg[20];
	memset(msg, 0, sizeof(msg));

	AKMDEBUG(AKMDBG_DISP1, " ------------------  AKMD main menu ------------------ \n");
	AKMDEBUG(AKMDBG_DISP1, "   T. Start Factory Shipment Test. \n");
	AKMDEBUG(AKMDBG_DISP1, "   1. Start Single Measurement. \n");
	AKMDEBUG(AKMDBG_DISP1, "   Q. Quit application. \n");
	AKMDEBUG(AKMDBG_DISP1, " ----------------------------------------------------- \n\n");
	AKMDEBUG(AKMDBG_DISP1, " Please select a number.\n");
	AKMDEBUG(AKMDBG_DISP1, "   ---> ");
	fgets(msg, 10, stdin);
	AKMDEBUG(AKMDBG_DISP1, "\n");

	// BUG : If 2-digits number is input,
	//    only the first character is compared.
	if (strncmp(msg, "T", 1) == 0 || strncmp(msg, "t", 1) == 0) {
		return MODE_FST;
	} else if (!strncmp(msg, "1", 1)) {
		return MODE_MeasureSNG;
	} else if (strncmp(msg, "Q", 1) == 0 || strncmp(msg, "q", 1) == 0) {
		return MODE_Quit;
	} else {
		return MODE_ERROR;
	}
}


