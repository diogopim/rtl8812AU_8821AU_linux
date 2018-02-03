/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
//============================================================
// Description:
//
// This file is for 92CE/92CU dynamic mechanism only
//
//
//============================================================
#define _RTL8812A_DM_C_

//============================================================
// include files
//============================================================
//#include <drv_types.h>
#include <rtl8812a_hal.h>

//============================================================
// Global var
//============================================================


static inline VOID dm_CheckProtection(
    IN	PADAPTER	Adapter
)
{
}

static VOID
dm_CheckStatistics(
    IN	PADAPTER	Adapter
)
{
}
#ifdef CONFIG_SUPPORT_HW_WPS_PBC
static void dm_CheckPbcGPIO(_adapter *padapter)
{
	u8	tmp1byte;
	u8	bPbcPressed = _FALSE;

	if(!padapter->registrypriv.hw_wps_pbc)
		return;

#if defined(CONFIG_USB_HCI)
	if (IS_HARDWARE_TYPE_8812(padapter)) {
		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
		tmp1byte |= (HAL_8192C_HW_GPIO_WPS_BIT);
		rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as output mode

		tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
		rtw_write8(padapter,  GPIO_IN, tmp1byte);		//reset the floating voltage level

		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL);
		tmp1byte &= ~(HAL_8192C_HW_GPIO_WPS_BIT);
		rtw_write8(padapter, GPIO_IO_SEL, tmp1byte);	//enable GPIO[2] as input mode

		tmp1byte =rtw_read8(padapter, GPIO_IN);

		if (tmp1byte == 0xff)
			return ;

		if (tmp1byte&HAL_8192C_HW_GPIO_WPS_BIT) {
			bPbcPressed = _TRUE;
		}
	} else if (IS_HARDWARE_TYPE_8821(padapter)) {
		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL_8811A);
		tmp1byte |= (BIT4);
		rtw_write8(padapter, GPIO_IO_SEL_8811A, tmp1byte);	//enable GPIO[2] as output mode

		tmp1byte &= ~(BIT4);
		rtw_write8(padapter,  GPIO_IN_8811A, tmp1byte);		//reset the floating voltage level

		tmp1byte = rtw_read8(padapter, GPIO_IO_SEL_8811A);
		tmp1byte &= ~(BIT4);
		rtw_write8(padapter, GPIO_IO_SEL_8811A, tmp1byte);	//enable GPIO[2] as input mode

		tmp1byte =rtw_read8(padapter, GPIO_IN_8811A);

		if (tmp1byte == 0xff)
			return ;

		if (tmp1byte&BIT4) {
			bPbcPressed = _TRUE;
		}
	}
#else

#endif

	if( _TRUE == bPbcPressed) {
		// Here we only set bPbcPressed to true
		// After trigger PBC, the variable will be set to false
		DBG_8192C("CheckPbcGPIO - PBC is pressed\n");

		rtw_request_wps_pbc_event(padapter);
	}
}
#endif //#ifdef CONFIG_SUPPORT_HW_WPS_PBC

//
// Initialize GPIO setting registers
//
static void
dm_InitGPIOSetting(
    IN	PADAPTER	Adapter
)
{
	//PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);

	u8	tmp1byte;

	tmp1byte = rtw_read8(Adapter, REG_GPIO_MUXCFG);
	tmp1byte &= (GPIOSEL_GPIO | ~GPIOSEL_ENBT);

	rtw_write8(Adapter, REG_GPIO_MUXCFG, tmp1byte);
}

//============================================================
// functions
//============================================================
static void Init_ODM_ComInfo_8812(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);

	u8	cut_ver,fab_ver;

	Init_ODM_ComInfo(Adapter);

	if (IS_HARDWARE_TYPE_8812(Adapter))
		ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_IC_TYPE, ODM_RTL8812);
	else if (IS_HARDWARE_TYPE_8821(Adapter))
		ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_IC_TYPE, ODM_RTL8821);

	fab_ver = ODM_TSMC;
	if(IS_A_CUT(pHalData->VersionID))
		cut_ver = ODM_CUT_A;
	else if(IS_B_CUT(pHalData->VersionID))
		cut_ver = ODM_CUT_B;
	else if(IS_C_CUT(pHalData->VersionID))
		cut_ver = ODM_CUT_C;
	else if(IS_D_CUT(pHalData->VersionID))
		cut_ver = ODM_CUT_D;
	else if(IS_E_CUT(pHalData->VersionID))
		cut_ver = ODM_CUT_E;
	else
		cut_ver = ODM_CUT_A;

	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_FAB_VER,fab_ver);
	ODM_CmnInfoInit(pDM_Odm,ODM_CMNINFO_CUT_VER,cut_ver);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_RF_ANTENNA_TYPE, pHalData->TRxAntDivType);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_IQKFWOFFLOAD, pHalData->RegIQKFWOffload);

#ifdef CONFIG_DISABLE_ODM
	pdmpriv->InitODMFlag = 0;
#else
	pdmpriv->InitODMFlag =	ODM_RF_CALIBRATION		|
	                        ODM_RF_TX_PWR_TRACK	//|
	                        ;
	//if(pHalData->AntDivCfg)
	//	pdmpriv->InitODMFlag |= ODM_BB_ANT_DIV;
#endif

	ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_ABILITY,pdmpriv->InitODMFlag);

}
static void Update_ODM_ComInfo_8812(PADAPTER	Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	pdmpriv->InitODMFlag = 0
	                       | ODM_BB_DIG
	                       | ODM_BB_RA_MASK
	                       | ODM_BB_FA_CNT
	                       | ODM_BB_RSSI_MONITOR
	                       | ODM_BB_CFO_TRACKING
	                       | ODM_RF_TX_PWR_TRACK
	                       | ODM_MAC_EDCA_TURBO
	                       | ODM_BB_NHM_CNT
//		| ODM_BB_PWR_TRAIN
	                       ;

	if (rtw_odm_adaptivity_needed(Adapter) == _TRUE)
		pdmpriv->InitODMFlag |= ODM_BB_ADAPTIVITY;

	if(pHalData->AntDivCfg)
		pdmpriv->InitODMFlag |= ODM_BB_ANT_DIV;

#if (MP_DRIVER==1)
	if (Adapter->registrypriv.mp_mode == 1) {
		pdmpriv->InitODMFlag = 0
		                       | ODM_RF_CALIBRATION
		                       | ODM_RF_TX_PWR_TRACK
		                       ;
	}
#endif//(MP_DRIVER==1)

#ifdef CONFIG_DISABLE_ODM
	pdmpriv->InitODMFlag = 0;
#endif//CONFIG_DISABLE_ODM

	ODM_CmnInfoUpdate(pDM_Odm,ODM_CMNINFO_ABILITY,pdmpriv->InitODMFlag);

	ODM_CmnInfoInit(pDM_Odm, ODM_CMNINFO_RF_ANTENNA_TYPE, pHalData->TRxAntDivType);
}

void
rtl8812_InitHalDm(
    IN	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
	//u8	i;

#ifdef CONFIG_USB_HCI
	dm_InitGPIOSetting(Adapter);
#endif

	pdmpriv->DM_Type = DM_Type_ByDriver;
	pdmpriv->DMFlag = DYNAMIC_FUNC_DISABLE;

	Update_ODM_ComInfo_8812(Adapter);
	ODM_DMInit(pDM_Odm);

	//Adapter->fix_rate = 0xFF;

}


VOID
rtl8812_HalDmWatchDog(
    IN	PADAPTER	Adapter
)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;
	u8 hw_init_completed = _FALSE;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	//struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	//PDM_ODM_T		pDM_Odm = &(pHalData->odmpriv);
#ifdef CONFIG_CONCURRENT_MODE
	PADAPTER pbuddy_adapter = Adapter->pbuddy_adapter;
#endif //CONFIG_CONCURRENT_MODE

	_func_enter_;

	hw_init_completed = Adapter->hw_init_completed;

	if (hw_init_completed == _FALSE)
		goto skip_dm;

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = adapter_to_pwrctl(Adapter)->bFwCurrentInPSMode;
	rtw_hal_get_hwreg(Adapter, HW_VAR_FWLPS_RF_ON, (u8 *)(&bFwPSAwake));
#endif

#ifdef CONFIG_P2P_PS
	// Fw is under p2p powersaving mode, driver should stop dynamic mechanism.
	// modifed by thomas. 2011.06.11.
	if(Adapter->wdinfo.p2p_ps_mode)
		bFwPSAwake = _FALSE;
#endif //CONFIG_P2P_PS

	if( (hw_init_completed == _TRUE)
	    && ((!bFwCurrentInPSMode) && bFwPSAwake)) {
		//
		// Calculate Tx/Rx statistics.
		//
		dm_CheckStatistics(Adapter);
		rtw_hal_check_rxfifo_full(Adapter);
		//
		// Dynamically switch RTS/CTS protection.
		//
		//dm_CheckProtection(Adapter);

	}

	//ODM
	if (hw_init_completed == _TRUE) {
		u8	bLinked=_FALSE;
		u8	bsta_state=_FALSE;
		u8	bBtDisabled = _TRUE;

#ifdef CONFIG_DISABLE_ODM
		pHalData->odmpriv.SupportAbility = 0;
#endif

		if(rtw_linked_check(Adapter)) {
			bLinked = _TRUE;
			if (check_fwstate(&Adapter->mlmepriv, WIFI_STATION_STATE))
				bsta_state = _TRUE;
		}

#ifdef CONFIG_CONCURRENT_MODE
		if(pbuddy_adapter && rtw_linked_check(pbuddy_adapter)) {
			bLinked = _TRUE;
			if(pbuddy_adapter && check_fwstate(&pbuddy_adapter->mlmepriv, WIFI_STATION_STATE))
				bsta_state = _TRUE;
		}
#endif //CONFIG_CONCURRENT_MODE

		ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_LINK, bLinked);
		ODM_CmnInfoUpdate(&pHalData->odmpriv ,ODM_CMNINFO_STATION_STATE, bsta_state);

		ODM_CmnInfoUpdate(&pHalData->odmpriv, ODM_CMNINFO_BT_ENABLED, ((bBtDisabled == _TRUE)?_FALSE:_TRUE));

		ODM_DMWatchdog(&pHalData->odmpriv);

	}

skip_dm:

#ifdef CONFIG_SUPPORT_HW_WPS_PBC
	// Check GPIO to determine current Pbc status.
	dm_CheckPbcGPIO(Adapter);
#endif

	return;
}

void rtl8812_init_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T 		podmpriv = &pHalData->odmpriv;


	_rtw_memset(pdmpriv, 0, sizeof(struct dm_priv));
	//_rtw_spinlock_init(&(pHalData->odm_stainfo_lock));

	{
		pHalData->RegIQKFWOffload = 1;
		rtw_sctx_init(&pHalData->iqk_sctx, 0);
	}

	Init_ODM_ComInfo_8812(Adapter);
	ODM_InitAllTimers(podmpriv );
	PHYDM_InitDebugSetting(podmpriv);

	pHalData->TxPwrInPercentage = TX_PWR_PERCENTAGE_3;

}

void rtl8812_deinit_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	//struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PDM_ODM_T 		podmpriv = &pHalData->odmpriv;
	//_rtw_spinlock_free(&pHalData->odm_stainfo_lock);
	ODM_CancelAllTimers(podmpriv);
}
