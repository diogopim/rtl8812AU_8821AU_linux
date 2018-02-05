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
#define _HAL_COM_C_

#include <drv_types.h>
#include "hal_com_h2c.h"

#include "hal_data.h"

//#define CONFIG_GTK_OL_DBG

#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
char	rtw_file_path[PATH_LENGTH_MAX];
#endif

u8 rtw_hal_data_init(_adapter *padapter)
{
	if(is_primary_adapter(padapter)) {
		padapter->hal_data_sz = sizeof(HAL_DATA_TYPE);
		padapter->HalData = rtw_zvmalloc(padapter->hal_data_sz);
		if(padapter->HalData == NULL) {
			DBG_8192C("cant not alloc memory for HAL DATA \n");
			return _FAIL;
		}
	}
	return _SUCCESS;
}

void rtw_hal_data_deinit(_adapter *padapter)
{
	if(is_primary_adapter(padapter)) {
		if (padapter->HalData) {
#ifdef CONFIG_LOAD_PHY_PARA_FROM_FILE
			phy_free_filebuf(padapter);
#endif
			rtw_vmfree(padapter->HalData, padapter->hal_data_sz);
			padapter->HalData = NULL;
			padapter->hal_data_sz = 0;
		}
	}
}

void dump_chip_info(HAL_VERSION	ChipVersion)
{
	int cnt = 0;
	u8 buf[128];

	if(IS_81XXC(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: %s_", IS_92C_SERIAL(ChipVersion)?"CHIP_8192C":"CHIP_8188C");
	} else if(IS_92D(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8192D_");
	} else if(IS_8723_SERIES(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8723A_");
	} else if(IS_8188E(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8188E_");
	} else if(IS_8812_SERIES(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8812_");
	} else if(IS_8192E(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8192E_");
	} else if(IS_8821_SERIES(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8821_");
	} else if(IS_8723B_SERIES(ChipVersion)) {
		cnt += sprintf((buf+cnt), "Chip Version Info: CHIP_8723B_");
	}

	cnt += sprintf((buf+cnt), "%s_", IS_NORMAL_CHIP(ChipVersion)?"Normal_Chip":"Test_Chip");
	if(IS_CHIP_VENDOR_TSMC(ChipVersion))
		cnt += sprintf((buf+cnt), "%s_","TSMC");
	else if(IS_CHIP_VENDOR_UMC(ChipVersion))
		cnt += sprintf((buf+cnt), "%s_","UMC");
	else if(IS_CHIP_VENDOR_SMIC(ChipVersion))
		cnt += sprintf((buf+cnt), "%s_","SMIC");

	if(IS_A_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "A_CUT_");
	else if(IS_B_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "B_CUT_");
	else if(IS_C_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "C_CUT_");
	else if(IS_D_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "D_CUT_");
	else if(IS_E_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "E_CUT_");
	else if(IS_I_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "I_CUT_");
	else if(IS_J_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "J_CUT_");
	else if(IS_K_CUT(ChipVersion)) cnt += sprintf((buf+cnt), "K_CUT_");
	else cnt += sprintf((buf+cnt), "UNKNOWN_CUT(%d)_", ChipVersion.CUTVersion);

	if(IS_1T1R(ChipVersion)) cnt += sprintf((buf+cnt), "1T1R_");
	else if(IS_1T2R(ChipVersion)) cnt += sprintf((buf+cnt), "1T2R_");
	else if(IS_2T2R(ChipVersion)) cnt += sprintf((buf+cnt), "2T2R_");
	else cnt += sprintf((buf+cnt), "UNKNOWN_RFTYPE(%d)_", ChipVersion.RFType);

	cnt += sprintf((buf+cnt), "RomVer(%d)\n", ChipVersion.ROMVer);

	DBG_871X("%s", buf);
}


#define	EEPROM_CHANNEL_PLAN_BY_HW_MASK	0x80

/*
 * Description:
 * 	Use hardware(efuse), driver parameter(registry) and default channel plan
 * 	to decide which one should be used.
 *
 * Parameters:
 *	padapter			pointer of adapter
 *	hw_channel_plan		channel plan from HW (efuse/eeprom)
 *						BIT[7] software configure mode; 0:Enable, 1:disable
 *						BIT[6:0] Channel Plan
 *	sw_channel_plan		channel plan from SW (registry/module param)
 *	def_channel_plan	channel plan used when HW/SW both invalid
 *	AutoLoadFail		efuse autoload fail or not
 *
 * Return:
 *	Final channel plan decision
 *
 */
u8
hal_com_config_channel_plan(
    IN	PADAPTER	padapter,
    IN	u8			hw_channel_plan,
    IN	u8			sw_channel_plan,
    IN	u8			def_channel_plan,
    IN	BOOLEAN		AutoLoadFail
)
{
	PHAL_DATA_TYPE	pHalData;
	//u8 hwConfig;
	u8 chnlPlan;


	pHalData = GET_HAL_DATA(padapter);
	pHalData->bDisableSWChannelPlan = _FALSE;
	chnlPlan = def_channel_plan;

	if (0xFF == hw_channel_plan)
		AutoLoadFail = _TRUE;

	if (_FALSE == AutoLoadFail) {
		u8 hw_chnlPlan;

		hw_chnlPlan = hw_channel_plan & (~EEPROM_CHANNEL_PLAN_BY_HW_MASK);
		if (rtw_is_channel_plan_valid(hw_chnlPlan)) {
#ifndef CONFIG_SW_CHANNEL_PLAN
			if (hw_channel_plan & EEPROM_CHANNEL_PLAN_BY_HW_MASK)
				pHalData->bDisableSWChannelPlan = _TRUE;
#endif // !CONFIG_SW_CHANNEL_PLAN

			chnlPlan = hw_chnlPlan;
		}
	}

	if ((_FALSE == pHalData->bDisableSWChannelPlan)
	    && rtw_is_channel_plan_valid(sw_channel_plan)) {
		chnlPlan = sw_channel_plan;
	}

	return chnlPlan;
}

BOOLEAN
HAL_IsLegalChannel(
    IN	PADAPTER	Adapter,
    IN	u32			Channel
)
{
	BOOLEAN bLegalChannel = _TRUE;

	if (Channel > 14) {
		if(IsSupported5G(Adapter->registrypriv.wireless_mode) == _FALSE) {
			bLegalChannel = _FALSE;
			DBG_871X("Channel > 14 but wireless_mode do not support 5G\n");
		}
	} else if ((Channel <= 14) && (Channel >=1)) {
		if(IsSupported24G(Adapter->registrypriv.wireless_mode) == _FALSE) {
			bLegalChannel = _FALSE;
			DBG_871X("(Channel <= 14) && (Channel >=1) but wireless_mode do not support 2.4G\n");
		}
	} else {
		bLegalChannel = _FALSE;
		DBG_871X("Channel is Invalid !!!\n");
	}

	return bLegalChannel;
}

u8	MRateToHwRate(u8 rate)
{
	u8	ret = DESC_RATE1M;

	switch(rate) {
	case MGN_1M:
		ret = DESC_RATE1M;
		break;
	case MGN_2M:
		ret = DESC_RATE2M;
		break;
	case MGN_5_5M:
		ret = DESC_RATE5_5M;
		break;
	case MGN_11M:
		ret = DESC_RATE11M;
		break;
	case MGN_6M:
		ret = DESC_RATE6M;
		break;
	case MGN_9M:
		ret = DESC_RATE9M;
		break;
	case MGN_12M:
		ret = DESC_RATE12M;
		break;
	case MGN_18M:
		ret = DESC_RATE18M;
		break;
	case MGN_24M:
		ret = DESC_RATE24M;
		break;
	case MGN_36M:
		ret = DESC_RATE36M;
		break;
	case MGN_48M:
		ret = DESC_RATE48M;
		break;
	case MGN_54M:
		ret = DESC_RATE54M;
		break;

	case MGN_MCS0:
		ret = DESC_RATEMCS0;
		break;
	case MGN_MCS1:
		ret = DESC_RATEMCS1;
		break;
	case MGN_MCS2:
		ret = DESC_RATEMCS2;
		break;
	case MGN_MCS3:
		ret = DESC_RATEMCS3;
		break;
	case MGN_MCS4:
		ret = DESC_RATEMCS4;
		break;
	case MGN_MCS5:
		ret = DESC_RATEMCS5;
		break;
	case MGN_MCS6:
		ret = DESC_RATEMCS6;
		break;
	case MGN_MCS7:
		ret = DESC_RATEMCS7;
		break;
	case MGN_MCS8:
		ret = DESC_RATEMCS8;
		break;
	case MGN_MCS9:
		ret = DESC_RATEMCS9;
		break;
	case MGN_MCS10:
		ret = DESC_RATEMCS10;
		break;
	case MGN_MCS11:
		ret = DESC_RATEMCS11;
		break;
	case MGN_MCS12:
		ret = DESC_RATEMCS12;
		break;
	case MGN_MCS13:
		ret = DESC_RATEMCS13;
		break;
	case MGN_MCS14:
		ret = DESC_RATEMCS14;
		break;
	case MGN_MCS15:
		ret = DESC_RATEMCS15;
		break;
	case MGN_MCS16:
		ret = DESC_RATEMCS16;
		break;
	case MGN_MCS17:
		ret = DESC_RATEMCS17;
		break;
	case MGN_MCS18:
		ret = DESC_RATEMCS18;
		break;
	case MGN_MCS19:
		ret = DESC_RATEMCS19;
		break;
	case MGN_MCS20:
		ret = DESC_RATEMCS20;
		break;
	case MGN_MCS21:
		ret = DESC_RATEMCS21;
		break;
	case MGN_MCS22:
		ret = DESC_RATEMCS22;
		break;
	case MGN_MCS23:
		ret = DESC_RATEMCS23;
		break;
	case MGN_MCS24:
		ret = DESC_RATEMCS24;
		break;
	case MGN_MCS25:
		ret = DESC_RATEMCS25;
		break;
	case MGN_MCS26:
		ret = DESC_RATEMCS26;
		break;
	case MGN_MCS27:
		ret = DESC_RATEMCS27;
		break;
	case MGN_MCS28:
		ret = DESC_RATEMCS28;
		break;
	case MGN_MCS29:
		ret = DESC_RATEMCS29;
		break;
	case MGN_MCS30:
		ret = DESC_RATEMCS30;
		break;
	case MGN_MCS31:
		ret = DESC_RATEMCS31;
		break;

	case MGN_VHT1SS_MCS0:
		ret = DESC_RATEVHTSS1MCS0;
		break;
	case MGN_VHT1SS_MCS1:
		ret = DESC_RATEVHTSS1MCS1;
		break;
	case MGN_VHT1SS_MCS2:
		ret = DESC_RATEVHTSS1MCS2;
		break;
	case MGN_VHT1SS_MCS3:
		ret = DESC_RATEVHTSS1MCS3;
		break;
	case MGN_VHT1SS_MCS4:
		ret = DESC_RATEVHTSS1MCS4;
		break;
	case MGN_VHT1SS_MCS5:
		ret = DESC_RATEVHTSS1MCS5;
		break;
	case MGN_VHT1SS_MCS6:
		ret = DESC_RATEVHTSS1MCS6;
		break;
	case MGN_VHT1SS_MCS7:
		ret = DESC_RATEVHTSS1MCS7;
		break;
	case MGN_VHT1SS_MCS8:
		ret = DESC_RATEVHTSS1MCS8;
		break;
	case MGN_VHT1SS_MCS9:
		ret = DESC_RATEVHTSS1MCS9;
		break;
	case MGN_VHT2SS_MCS0:
		ret = DESC_RATEVHTSS2MCS0;
		break;
	case MGN_VHT2SS_MCS1:
		ret = DESC_RATEVHTSS2MCS1;
		break;
	case MGN_VHT2SS_MCS2:
		ret = DESC_RATEVHTSS2MCS2;
		break;
	case MGN_VHT2SS_MCS3:
		ret = DESC_RATEVHTSS2MCS3;
		break;
	case MGN_VHT2SS_MCS4:
		ret = DESC_RATEVHTSS2MCS4;
		break;
	case MGN_VHT2SS_MCS5:
		ret = DESC_RATEVHTSS2MCS5;
		break;
	case MGN_VHT2SS_MCS6:
		ret = DESC_RATEVHTSS2MCS6;
		break;
	case MGN_VHT2SS_MCS7:
		ret = DESC_RATEVHTSS2MCS7;
		break;
	case MGN_VHT2SS_MCS8:
		ret = DESC_RATEVHTSS2MCS8;
		break;
	case MGN_VHT2SS_MCS9:
		ret = DESC_RATEVHTSS2MCS9;
		break;
	case MGN_VHT3SS_MCS0:
		ret = DESC_RATEVHTSS3MCS0;
		break;
	case MGN_VHT3SS_MCS1:
		ret = DESC_RATEVHTSS3MCS1;
		break;
	case MGN_VHT3SS_MCS2:
		ret = DESC_RATEVHTSS3MCS2;
		break;
	case MGN_VHT3SS_MCS3:
		ret = DESC_RATEVHTSS3MCS3;
		break;
	case MGN_VHT3SS_MCS4:
		ret = DESC_RATEVHTSS3MCS4;
		break;
	case MGN_VHT3SS_MCS5:
		ret = DESC_RATEVHTSS3MCS5;
		break;
	case MGN_VHT3SS_MCS6:
		ret = DESC_RATEVHTSS3MCS6;
		break;
	case MGN_VHT3SS_MCS7:
		ret = DESC_RATEVHTSS3MCS7;
		break;
	case MGN_VHT3SS_MCS8:
		ret = DESC_RATEVHTSS3MCS8;
		break;
	case MGN_VHT3SS_MCS9:
		ret = DESC_RATEVHTSS3MCS9;
		break;
	case MGN_VHT4SS_MCS0:
		ret = DESC_RATEVHTSS4MCS0;
		break;
	case MGN_VHT4SS_MCS1:
		ret = DESC_RATEVHTSS4MCS1;
		break;
	case MGN_VHT4SS_MCS2:
		ret = DESC_RATEVHTSS4MCS2;
		break;
	case MGN_VHT4SS_MCS3:
		ret = DESC_RATEVHTSS4MCS3;
		break;
	case MGN_VHT4SS_MCS4:
		ret = DESC_RATEVHTSS4MCS4;
		break;
	case MGN_VHT4SS_MCS5:
		ret = DESC_RATEVHTSS4MCS5;
		break;
	case MGN_VHT4SS_MCS6:
		ret = DESC_RATEVHTSS4MCS6;
		break;
	case MGN_VHT4SS_MCS7:
		ret = DESC_RATEVHTSS4MCS7;
		break;
	case MGN_VHT4SS_MCS8:
		ret = DESC_RATEVHTSS4MCS8;
		break;
	case MGN_VHT4SS_MCS9:
		ret = DESC_RATEVHTSS4MCS9;
		break;
	default:
		break;
	}

	return ret;
}

u8	HwRateToMRate(u8 rate)
{
	u8	ret_rate = MGN_1M;

	switch(rate) {

	case DESC_RATE1M:
		ret_rate = MGN_1M;
		break;
	case DESC_RATE2M:
		ret_rate = MGN_2M;
		break;
	case DESC_RATE5_5M:
		ret_rate = MGN_5_5M;
		break;
	case DESC_RATE11M:
		ret_rate = MGN_11M;
		break;
	case DESC_RATE6M:
		ret_rate = MGN_6M;
		break;
	case DESC_RATE9M:
		ret_rate = MGN_9M;
		break;
	case DESC_RATE12M:
		ret_rate = MGN_12M;
		break;
	case DESC_RATE18M:
		ret_rate = MGN_18M;
		break;
	case DESC_RATE24M:
		ret_rate = MGN_24M;
		break;
	case DESC_RATE36M:
		ret_rate = MGN_36M;
		break;
	case DESC_RATE48M:
		ret_rate = MGN_48M;
		break;
	case DESC_RATE54M:
		ret_rate = MGN_54M;
		break;
	case DESC_RATEMCS0:
		ret_rate = MGN_MCS0;
		break;
	case DESC_RATEMCS1:
		ret_rate = MGN_MCS1;
		break;
	case DESC_RATEMCS2:
		ret_rate = MGN_MCS2;
		break;
	case DESC_RATEMCS3:
		ret_rate = MGN_MCS3;
		break;
	case DESC_RATEMCS4:
		ret_rate = MGN_MCS4;
		break;
	case DESC_RATEMCS5:
		ret_rate = MGN_MCS5;
		break;
	case DESC_RATEMCS6:
		ret_rate = MGN_MCS6;
		break;
	case DESC_RATEMCS7:
		ret_rate = MGN_MCS7;
		break;
	case DESC_RATEMCS8:
		ret_rate = MGN_MCS8;
		break;
	case DESC_RATEMCS9:
		ret_rate = MGN_MCS9;
		break;
	case DESC_RATEMCS10:
		ret_rate = MGN_MCS10;
		break;
	case DESC_RATEMCS11:
		ret_rate = MGN_MCS11;
		break;
	case DESC_RATEMCS12:
		ret_rate = MGN_MCS12;
		break;
	case DESC_RATEMCS13:
		ret_rate = MGN_MCS13;
		break;
	case DESC_RATEMCS14:
		ret_rate = MGN_MCS14;
		break;
	case DESC_RATEMCS15:
		ret_rate = MGN_MCS15;
		break;
	case DESC_RATEMCS16:
		ret_rate = MGN_MCS16;
		break;
	case DESC_RATEMCS17:
		ret_rate = MGN_MCS17;
		break;
	case DESC_RATEMCS18:
		ret_rate = MGN_MCS18;
		break;
	case DESC_RATEMCS19:
		ret_rate = MGN_MCS19;
		break;
	case DESC_RATEMCS20:
		ret_rate = MGN_MCS20;
		break;
	case DESC_RATEMCS21:
		ret_rate = MGN_MCS21;
		break;
	case DESC_RATEMCS22:
		ret_rate = MGN_MCS22;
		break;
	case DESC_RATEMCS23:
		ret_rate = MGN_MCS23;
		break;
	case DESC_RATEMCS24:
		ret_rate = MGN_MCS24;
		break;
	case DESC_RATEMCS25:
		ret_rate = MGN_MCS25;
		break;
	case DESC_RATEMCS26:
		ret_rate = MGN_MCS26;
		break;
	case DESC_RATEMCS27:
		ret_rate = MGN_MCS27;
		break;
	case DESC_RATEMCS28:
		ret_rate = MGN_MCS28;
		break;
	case DESC_RATEMCS29:
		ret_rate = MGN_MCS29;
		break;
	case DESC_RATEMCS30:
		ret_rate = MGN_MCS30;
		break;
	case DESC_RATEMCS31:
		ret_rate = MGN_MCS31;
		break;
	case DESC_RATEVHTSS1MCS0:
		ret_rate = MGN_VHT1SS_MCS0;
		break;
	case DESC_RATEVHTSS1MCS1:
		ret_rate = MGN_VHT1SS_MCS1;
		break;
	case DESC_RATEVHTSS1MCS2:
		ret_rate = MGN_VHT1SS_MCS2;
		break;
	case DESC_RATEVHTSS1MCS3:
		ret_rate = MGN_VHT1SS_MCS3;
		break;
	case DESC_RATEVHTSS1MCS4:
		ret_rate = MGN_VHT1SS_MCS4;
		break;
	case DESC_RATEVHTSS1MCS5:
		ret_rate = MGN_VHT1SS_MCS5;
		break;
	case DESC_RATEVHTSS1MCS6:
		ret_rate = MGN_VHT1SS_MCS6;
		break;
	case DESC_RATEVHTSS1MCS7:
		ret_rate = MGN_VHT1SS_MCS7;
		break;
	case DESC_RATEVHTSS1MCS8:
		ret_rate = MGN_VHT1SS_MCS8;
		break;
	case DESC_RATEVHTSS1MCS9:
		ret_rate = MGN_VHT1SS_MCS9;
		break;
	case DESC_RATEVHTSS2MCS0:
		ret_rate = MGN_VHT2SS_MCS0;
		break;
	case DESC_RATEVHTSS2MCS1:
		ret_rate = MGN_VHT2SS_MCS1;
		break;
	case DESC_RATEVHTSS2MCS2:
		ret_rate = MGN_VHT2SS_MCS2;
		break;
	case DESC_RATEVHTSS2MCS3:
		ret_rate = MGN_VHT2SS_MCS3;
		break;
	case DESC_RATEVHTSS2MCS4:
		ret_rate = MGN_VHT2SS_MCS4;
		break;
	case DESC_RATEVHTSS2MCS5:
		ret_rate = MGN_VHT2SS_MCS5;
		break;
	case DESC_RATEVHTSS2MCS6:
		ret_rate = MGN_VHT2SS_MCS6;
		break;
	case DESC_RATEVHTSS2MCS7:
		ret_rate = MGN_VHT2SS_MCS7;
		break;
	case DESC_RATEVHTSS2MCS8:
		ret_rate = MGN_VHT2SS_MCS8;
		break;
	case DESC_RATEVHTSS2MCS9:
		ret_rate = MGN_VHT2SS_MCS9;
		break;
	case DESC_RATEVHTSS3MCS0:
		ret_rate = MGN_VHT3SS_MCS0;
		break;
	case DESC_RATEVHTSS3MCS1:
		ret_rate = MGN_VHT3SS_MCS1;
		break;
	case DESC_RATEVHTSS3MCS2:
		ret_rate = MGN_VHT3SS_MCS2;
		break;
	case DESC_RATEVHTSS3MCS3:
		ret_rate = MGN_VHT3SS_MCS3;
		break;
	case DESC_RATEVHTSS3MCS4:
		ret_rate = MGN_VHT3SS_MCS4;
		break;
	case DESC_RATEVHTSS3MCS5:
		ret_rate = MGN_VHT3SS_MCS5;
		break;
	case DESC_RATEVHTSS3MCS6:
		ret_rate = MGN_VHT3SS_MCS6;
		break;
	case DESC_RATEVHTSS3MCS7:
		ret_rate = MGN_VHT3SS_MCS7;
		break;
	case DESC_RATEVHTSS3MCS8:
		ret_rate = MGN_VHT3SS_MCS8;
		break;
	case DESC_RATEVHTSS3MCS9:
		ret_rate = MGN_VHT3SS_MCS9;
		break;
	case DESC_RATEVHTSS4MCS0:
		ret_rate = MGN_VHT4SS_MCS0;
		break;
	case DESC_RATEVHTSS4MCS1:
		ret_rate = MGN_VHT4SS_MCS1;
		break;
	case DESC_RATEVHTSS4MCS2:
		ret_rate = MGN_VHT4SS_MCS2;
		break;
	case DESC_RATEVHTSS4MCS3:
		ret_rate = MGN_VHT4SS_MCS3;
		break;
	case DESC_RATEVHTSS4MCS4:
		ret_rate = MGN_VHT4SS_MCS4;
		break;
	case DESC_RATEVHTSS4MCS5:
		ret_rate = MGN_VHT4SS_MCS5;
		break;
	case DESC_RATEVHTSS4MCS6:
		ret_rate = MGN_VHT4SS_MCS6;
		break;
	case DESC_RATEVHTSS4MCS7:
		ret_rate = MGN_VHT4SS_MCS7;
		break;
	case DESC_RATEVHTSS4MCS8:
		ret_rate = MGN_VHT4SS_MCS8;
		break;
	case DESC_RATEVHTSS4MCS9:
		ret_rate = MGN_VHT4SS_MCS9;
		break;

	default:
		DBG_871X("HwRateToMRate(): Non supported Rate [%x]!!!\n",rate );
		break;
	}

	return ret_rate;
}

void	HalSetBrateCfg(
    IN PADAPTER		Adapter,
    IN const u8		*mBratesOS,
    OUT u16			*pBrateCfg)
{
	u8	i, is_brate, brate;

	for(i=0; i<NDIS_802_11_LENGTH_RATES_EX; i++) {
		is_brate = mBratesOS[i] & IEEE80211_BASIC_RATE_MASK;
		brate = mBratesOS[i] & 0x7f;

		if( is_brate ) {
			switch(brate) {
			case IEEE80211_CCK_RATE_1MB:
				*pBrateCfg |= RATE_1M;
				break;
			case IEEE80211_CCK_RATE_2MB:
				*pBrateCfg |= RATE_2M;
				break;
			case IEEE80211_CCK_RATE_5MB:
				*pBrateCfg |= RATE_5_5M;
				break;
			case IEEE80211_CCK_RATE_11MB:
				*pBrateCfg |= RATE_11M;
				break;
			case IEEE80211_OFDM_RATE_6MB:
				*pBrateCfg |= RATE_6M;
				break;
			case IEEE80211_OFDM_RATE_9MB:
				*pBrateCfg |= RATE_9M;
				break;
			case IEEE80211_OFDM_RATE_12MB:
				*pBrateCfg |= RATE_12M;
				break;
			case IEEE80211_OFDM_RATE_18MB:
				*pBrateCfg |= RATE_18M;
				break;
			case IEEE80211_OFDM_RATE_24MB:
				*pBrateCfg |= RATE_24M;
				break;
			case IEEE80211_OFDM_RATE_36MB:
				*pBrateCfg |= RATE_36M;
				break;
			case IEEE80211_OFDM_RATE_48MB:
				*pBrateCfg |= RATE_48M;
				break;
			case IEEE80211_OFDM_RATE_54MB:
				*pBrateCfg |= RATE_54M;
				break;
			}
		}
	}
}

static VOID
_OneOutPipeMapping(
    IN	PADAPTER	pAdapter
)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(pAdapter);

	pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
	pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[0];//VI
	pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[0];//BE
	pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[0];//BK

	pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
	pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
	pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[0];//HIGH
	pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD
}

static VOID
_TwoOutPipeMapping(
    IN	PADAPTER	pAdapter,
    IN	BOOLEAN	 	bWIFICfg
)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(pAdapter);

	if(bWIFICfg) { //WMM

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  0, 	1, 	0, 	1, 	0, 	0, 	0, 	0, 		0	};
		//0:ep_0 num, 1:ep_1 num

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[1];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[0];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[1];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[0];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[0];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD

	} else { //typical setting


		//BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  1, 	1, 	0, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:ep_0 num, 1:ep_1 num

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[0];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[1];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[1];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[0];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD

	}

}

static VOID _ThreeOutPipeMapping(
    IN	PADAPTER	pAdapter,
    IN	BOOLEAN	 	bWIFICfg
)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(pAdapter);

	if(bWIFICfg) { //for WMM

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[1];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[2];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[1];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[0];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD

	} else { //typical setting


		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[1];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[2];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[2];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[0];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD
	}

}
static inline VOID _FourOutPipeMapping(
    IN	PADAPTER	pAdapter,
    IN	BOOLEAN	 	bWIFICfg
)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(pAdapter);

	if(bWIFICfg) { //for WMM

		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L ,3:E

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[1];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[2];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[1];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[3];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD

	} else { //typical setting


		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H, 1:N, 2:L

		pdvobjpriv->Queue2Pipe[0] = pdvobjpriv->RtOutPipe[0];//VO
		pdvobjpriv->Queue2Pipe[1] = pdvobjpriv->RtOutPipe[1];//VI
		pdvobjpriv->Queue2Pipe[2] = pdvobjpriv->RtOutPipe[2];//BE
		pdvobjpriv->Queue2Pipe[3] = pdvobjpriv->RtOutPipe[2];//BK

		pdvobjpriv->Queue2Pipe[4] = pdvobjpriv->RtOutPipe[0];//BCN
		pdvobjpriv->Queue2Pipe[5] = pdvobjpriv->RtOutPipe[0];//MGT
		pdvobjpriv->Queue2Pipe[6] = pdvobjpriv->RtOutPipe[3];//HIGH
		pdvobjpriv->Queue2Pipe[7] = pdvobjpriv->RtOutPipe[0];//TXCMD
	}

}
BOOLEAN
Hal_MappingOutPipe(
    IN	PADAPTER	pAdapter,
    IN	u8		NumOutPipe
)
{
	struct registry_priv *pregistrypriv = &pAdapter->registrypriv;

	BOOLEAN	 bWIFICfg = (pregistrypriv->wifi_spec) ?_TRUE:_FALSE;

	BOOLEAN result = _TRUE;

	switch(NumOutPipe) {
	case 2:
		_TwoOutPipeMapping(pAdapter, bWIFICfg);
		break;
	case 3:
	case 4:
		_ThreeOutPipeMapping(pAdapter, bWIFICfg);
		break;
	case 1:
		_OneOutPipeMapping(pAdapter);
		break;
	default:
		result = _FALSE;
		break;
	}

	return result;

}

void hal_init_macaddr(_adapter *adapter)
{
	rtw_hal_set_hwreg(adapter, HW_VAR_MAC_ADDR, adapter->eeprompriv.mac_addr);
#if 0
	if (adapter->pbuddy_adapter)
		rtw_hal_set_hwreg(adapter->pbuddy_adapter, HW_VAR_MAC_ADDR, adapter->pbuddy_adapter->eeprompriv.mac_addr);
#endif
}

void rtw_init_hal_com_default_value(PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	pHalData->AntDetection = 1;
}

/*
* C2H event format:
* Field	 TRIGGER		CONTENT	   CMD_SEQ 	CMD_LEN		 CMD_ID
* BITS	 [127:120]	[119:16]      [15:8]		  [7:4]	 	   [3:0]
*/

void c2h_evt_clear(_adapter *adapter)
{
	rtw_write8(adapter, REG_C2HEVT_CLEAR, C2H_EVT_HOST_CLOSE);
}

s32 c2h_evt_read(_adapter *adapter, u8 *buf)
{
	s32 ret = _FAIL;
	return ret;
}

/*
* C2H event format:
* Field    TRIGGER    CMD_LEN    CONTENT    CMD_SEQ    CMD_ID
* BITS    [127:120]   [119:112]    [111:16]	     [15:8]         [7:0]
*/
s32 c2h_evt_read_88xx(_adapter *adapter, u8 *buf)
{
	s32 ret = _FAIL;
	struct c2h_evt_hdr_88xx *c2h_evt;
	int i;
	u8 trigger;

	if (buf == NULL)
		goto exit;

#if defined(CONFIG_RTL8812A) || defined(CONFIG_RTL8821A)

	trigger = rtw_read8(adapter, REG_C2HEVT_CLEAR);

	if (trigger == C2H_EVT_HOST_CLOSE) {
		goto exit; /* Not ready */
	} else if (trigger != C2H_EVT_FW_CLOSE) {
		goto clear_evt; /* Not a valid value */
	}

	c2h_evt = (struct c2h_evt_hdr_88xx *)buf;

	_rtw_memset(c2h_evt, 0, 16);

	c2h_evt->id = rtw_read8(adapter, REG_C2HEVT_MSG_NORMAL);
	c2h_evt->seq = rtw_read8(adapter, REG_C2HEVT_CMD_SEQ_88XX);
	c2h_evt->plen = rtw_read8(adapter, REG_C2HEVT_CMD_LEN_88XX);

	RT_PRINT_DATA(_module_hal_init_c_, _drv_info_, "c2h_evt_read(): ",
	              &c2h_evt , sizeof(c2h_evt));

	if (0) {
		DBG_871X("%s id:%u, len:%u, seq:%u, trigger:0x%02x\n", __func__
		         , c2h_evt->id, c2h_evt->plen, c2h_evt->seq, trigger);
	}

	/* Read the content */
	for (i = 0; i < c2h_evt->plen; i++)
		c2h_evt->payload[i] = rtw_read8(adapter, REG_C2HEVT_MSG_NORMAL + 2 + i);

	RT_PRINT_DATA(_module_hal_init_c_, _drv_info_, "c2h_evt_read(): Command Content:\n",
	              c2h_evt->payload, c2h_evt->plen);

	ret = _SUCCESS;

clear_evt:
	/*
	* Clear event to notify FW we have read the command.
	* If this field isn't clear, the FW won't update the next command message.
	*/
	c2h_evt_clear(adapter);
#endif
exit:
	return ret;
}


u8  rtw_hal_networktype_to_raid(_adapter *adapter, struct sta_info *psta)
{
	if(IS_NEW_GENERATION_IC(adapter)) {
		return networktype_to_raid_ex(adapter,psta);
	} else {
		return networktype_to_raid(adapter,psta);
	}

}
u8 rtw_get_mgntframe_raid(_adapter *adapter,unsigned char network_type)
{

	u8 raid;
	if(IS_NEW_GENERATION_IC(adapter)) {

		raid = (network_type & WIRELESS_11B)	?RATEID_IDX_B
		       :RATEID_IDX_G;
	} else {
		raid = (network_type & WIRELESS_11B)	?RATR_INX_WIRELESS_B
		       :RATR_INX_WIRELESS_G;
	}
	return raid;
}

void rtw_hal_update_sta_rate_mask(PADAPTER padapter, struct sta_info *psta)
{
	u8	i, rf_type, limit;
	u32	tx_ra_bitmap;

	if(psta == NULL) {
		return;
	}

	tx_ra_bitmap = 0;

	//b/g mode ra_bitmap
	for (i=0; i<sizeof(psta->bssrateset); i++) {
		if (psta->bssrateset[i])
			tx_ra_bitmap |= rtw_get_bit_value_from_ieee_value(psta->bssrateset[i]&0x7f);
	}

#ifdef CONFIG_80211N_HT
#ifdef CONFIG_80211AC_VHT
	//AC mode ra_bitmap
	if(psta->vhtpriv.vht_option) {
		tx_ra_bitmap |= (rtw_vht_rate_to_bitmap(psta->vhtpriv.vht_mcs_map) << 12);
	} else
#endif //CONFIG_80211AC_VHT
	{
		//n mode ra_bitmap
		if(psta->htpriv.ht_option) {
			rf_type = RF_1T1R;
			rtw_hal_get_hwreg(padapter, HW_VAR_RF_TYPE, (u8 *)(&rf_type));
			if(rf_type == RF_2T2R)
				limit=16;// 2R
			else
				limit=8;//  1R

			for (i=0; i<limit; i++) {
				if (psta->htpriv.ht_cap.supp_mcs_set[i/8] & BIT(i%8))
					tx_ra_bitmap |= BIT(i+12);
			}
		}
	}
#endif //CONFIG_80211N_HT

	psta->ra_mask = tx_ra_bitmap;
	psta->init_rate = get_highest_rate_idx(tx_ra_bitmap)&0x3f;
}

void hw_var_port_switch(_adapter *adapter)
{
#if 0
#ifdef CONFIG_RUNTIME_PORT_SWITCH
	/*
	0x102: MSR
	0x550: REG_BCN_CTRL
	0x551: REG_BCN_CTRL_1
	0x55A: REG_ATIMWND
	0x560: REG_TSFTR
	0x568: REG_TSFTR1
	0x570: REG_ATIMWND_1
	0x610: REG_MACID
	0x618: REG_BSSID
	0x700: REG_MACID1
	0x708: REG_BSSID1
	*/

	int i;
	u8 msr;
	u8 bcn_ctrl;
	u8 bcn_ctrl_1;
	u8 atimwnd[2];
	u8 atimwnd_1[2];
	u8 tsftr[8];
	u8 tsftr_1[8];
	u8 macid[6];
	u8 bssid[6];
	u8 macid_1[6];
	u8 bssid_1[6];

	//u8 iface_type;

	msr = rtw_read8(adapter, MSR);
	bcn_ctrl = rtw_read8(adapter, REG_BCN_CTRL);
	bcn_ctrl_1 = rtw_read8(adapter, REG_BCN_CTRL_1);

	for (i=0; i<2; i++)
		atimwnd[i] = rtw_read8(adapter, REG_ATIMWND+i);
	for (i=0; i<2; i++)
		atimwnd_1[i] = rtw_read8(adapter, REG_ATIMWND_1+i);

	for (i=0; i<8; i++)
		tsftr[i] = rtw_read8(adapter, REG_TSFTR+i);
	for (i=0; i<8; i++)
		tsftr_1[i] = rtw_read8(adapter, REG_TSFTR1+i);

	for (i=0; i<6; i++)
		macid[i] = rtw_read8(adapter, REG_MACID+i);

	for (i=0; i<6; i++)
		bssid[i] = rtw_read8(adapter, REG_BSSID+i);

	for (i=0; i<6; i++)
		macid_1[i] = rtw_read8(adapter, REG_MACID1+i);

	for (i=0; i<6; i++)
		bssid_1[i] = rtw_read8(adapter, REG_BSSID1+i);

#ifdef DBG_RUNTIME_PORT_SWITCH
	DBG_871X(FUNC_ADPT_FMT" before switch\n"
	         "msr:0x%02x\n"
	         "bcn_ctrl:0x%02x\n"
	         "bcn_ctrl_1:0x%02x\n"
	         "atimwnd:0x%04x\n"
	         "atimwnd_1:0x%04x\n"
	         "tsftr:%llu\n"
	         "tsftr1:%llu\n"
	         "macid:"MAC_FMT"\n"
	         "bssid:"MAC_FMT"\n"
	         "macid_1:"MAC_FMT"\n"
	         "bssid_1:"MAC_FMT"\n"
	         , FUNC_ADPT_ARG(adapter)
	         , msr
	         , bcn_ctrl
	         , bcn_ctrl_1
	         , *((u16*)atimwnd)
	         , *((u16*)atimwnd_1)
	         , *((u64*)tsftr)
	         , *((u64*)tsftr_1)
	         , MAC_ARG(macid)
	         , MAC_ARG(bssid)
	         , MAC_ARG(macid_1)
	         , MAC_ARG(bssid_1)
	        );
#endif /* DBG_RUNTIME_PORT_SWITCH */

	/* disable bcn function, disable update TSF  */
	rtw_write8(adapter, REG_BCN_CTRL, (bcn_ctrl & (~EN_BCN_FUNCTION)) | DIS_TSF_UDT);
	rtw_write8(adapter, REG_BCN_CTRL_1, (bcn_ctrl_1 & (~EN_BCN_FUNCTION)) | DIS_TSF_UDT);

	/* switch msr */
	msr = (msr&0xf0) |((msr&0x03) << 2) | ((msr&0x0c) >> 2);
	rtw_write8(adapter, MSR, msr);

	/* write port0 */
	rtw_write8(adapter, REG_BCN_CTRL, bcn_ctrl_1 & ~EN_BCN_FUNCTION);
	for (i=0; i<2; i++)
		rtw_write8(adapter, REG_ATIMWND+i, atimwnd_1[i]);
	for (i=0; i<8; i++)
		rtw_write8(adapter, REG_TSFTR+i, tsftr_1[i]);
	for (i=0; i<6; i++)
		rtw_write8(adapter, REG_MACID+i, macid_1[i]);
	for (i=0; i<6; i++)
		rtw_write8(adapter, REG_BSSID+i, bssid_1[i]);

	/* write port1 */
	rtw_write8(adapter, REG_BCN_CTRL_1, bcn_ctrl & ~EN_BCN_FUNCTION);
	for (i=0; i<2; i++)
		rtw_write8(adapter, REG_ATIMWND_1+1, atimwnd[i]);
	for (i=0; i<8; i++)
		rtw_write8(adapter, REG_TSFTR1+i, tsftr[i]);
	for (i=0; i<6; i++)
		rtw_write8(adapter, REG_MACID1+i, macid[i]);
	for (i=0; i<6; i++)
		rtw_write8(adapter, REG_BSSID1+i, bssid[i]);

	/* write bcn ctl */
	rtw_write8(adapter, REG_BCN_CTRL, bcn_ctrl_1);
	rtw_write8(adapter, REG_BCN_CTRL_1, bcn_ctrl);

	if (adapter->iface_type == IFACE_PORT0) {
		adapter->iface_type = IFACE_PORT1;
		adapter->pbuddy_adapter->iface_type = IFACE_PORT0;
		DBG_871X_LEVEL(_drv_always_, "port switch - port0("ADPT_FMT"), port1("ADPT_FMT")\n",
		               ADPT_ARG(adapter->pbuddy_adapter), ADPT_ARG(adapter));
	} else {
		adapter->iface_type = IFACE_PORT0;
		adapter->pbuddy_adapter->iface_type = IFACE_PORT1;
		DBG_871X_LEVEL(_drv_always_, "port switch - port0("ADPT_FMT"), port1("ADPT_FMT")\n",
		               ADPT_ARG(adapter), ADPT_ARG(adapter->pbuddy_adapter));
	}

#ifdef DBG_RUNTIME_PORT_SWITCH
	msr = rtw_read8(adapter, MSR);
	bcn_ctrl = rtw_read8(adapter, REG_BCN_CTRL);
	bcn_ctrl_1 = rtw_read8(adapter, REG_BCN_CTRL_1);

	for (i=0; i<2; i++)
		atimwnd[i] = rtw_read8(adapter, REG_ATIMWND+i);
	for (i=0; i<2; i++)
		atimwnd_1[i] = rtw_read8(adapter, REG_ATIMWND_1+i);

	for (i=0; i<8; i++)
		tsftr[i] = rtw_read8(adapter, REG_TSFTR+i);
	for (i=0; i<8; i++)
		tsftr_1[i] = rtw_read8(adapter, REG_TSFTR1+i);

	for (i=0; i<6; i++)
		macid[i] = rtw_read8(adapter, REG_MACID+i);

	for (i=0; i<6; i++)
		bssid[i] = rtw_read8(adapter, REG_BSSID+i);

	for (i=0; i<6; i++)
		macid_1[i] = rtw_read8(adapter, REG_MACID1+i);

	for (i=0; i<6; i++)
		bssid_1[i] = rtw_read8(adapter, REG_BSSID1+i);

	DBG_871X(FUNC_ADPT_FMT" after switch\n"
	         "msr:0x%02x\n"
	         "bcn_ctrl:0x%02x\n"
	         "bcn_ctrl_1:0x%02x\n"
	         "atimwnd:%u\n"
	         "atimwnd_1:%u\n"
	         "tsftr:%llu\n"
	         "tsftr1:%llu\n"
	         "macid:"MAC_FMT"\n"
	         "bssid:"MAC_FMT"\n"
	         "macid_1:"MAC_FMT"\n"
	         "bssid_1:"MAC_FMT"\n"
	         , FUNC_ADPT_ARG(adapter)
	         , msr
	         , bcn_ctrl
	         , bcn_ctrl_1
	         , *((u16*)atimwnd)
	         , *((u16*)atimwnd_1)
	         , *((u64*)tsftr)
	         , *((u64*)tsftr_1)
	         , MAC_ARG(macid)
	         , MAC_ARG(bssid)
	         , MAC_ARG(macid_1)
	         , MAC_ARG(bssid_1)
	        );
#endif /* DBG_RUNTIME_PORT_SWITCH */

#endif /* CONFIG_RUNTIME_PORT_SWITCH */
#endif /**/
}

void rtw_hal_set_FwRsvdPage_cmd(PADAPTER padapter, PRSVDPAGE_LOC rsvdpageloc)
{
	struct	hal_ops *pHalFunc = &padapter->HalFunc;
	u8	u1H2CRsvdPageParm[H2C_RSVDPAGE_LOC_LEN]= {0};
	u8	ret = 0;

	DBG_871X("RsvdPageLoc: ProbeRsp=%d PsPoll=%d Null=%d QoSNull=%d BTNull=%d\n",
	         rsvdpageloc->LocProbeRsp, rsvdpageloc->LocPsPoll,
	         rsvdpageloc->LocNullData, rsvdpageloc->LocQosNull,
	         rsvdpageloc->LocBTQosNull);

	SET_H2CCMD_RSVDPAGE_LOC_PROBE_RSP(u1H2CRsvdPageParm, rsvdpageloc->LocProbeRsp);
	SET_H2CCMD_RSVDPAGE_LOC_PSPOLL(u1H2CRsvdPageParm, rsvdpageloc->LocPsPoll);
	SET_H2CCMD_RSVDPAGE_LOC_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocNullData);
	SET_H2CCMD_RSVDPAGE_LOC_QOS_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocQosNull);
	SET_H2CCMD_RSVDPAGE_LOC_BT_QOS_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocBTQosNull);

	if (pHalFunc->fill_h2c_cmd != NULL) {
		ret = pHalFunc->fill_h2c_cmd(padapter,
		                             H2C_RSVD_PAGE,
		                             H2C_RSVDPAGE_LOC_LEN,
		                             u1H2CRsvdPageParm);
	} else {
		DBG_871X("%s: Please hook fill_h2c_cmd first!\n", __func__);
		ret = _FAIL;
	}
}

void rtw_hal_set_FwAoacRsvdPage_cmd(PADAPTER padapter, PRSVDPAGE_LOC rsvdpageloc)
{
}

static void rtw_hal_construct_beacon(_adapter *padapter,
                                     u8 *pframe, u32 *pLength)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					rate_len, pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	SetFrameSubType(pframe, WIFI_BEACON);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pktlen = sizeof (struct rtw_ieee80211_hdr_3addr);

	//timestamp will be inserted by hardware
	pframe += 8;
	pktlen += 8;

	// beacon interval: 2 bytes
	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_beacon_interval_from_ie(cur_network->IEs)), 2);

	pframe += 2;
	pktlen += 2;

	// capability info: 2 bytes
	_rtw_memcpy(pframe, (unsigned char *)(rtw_get_capability_from_ie(cur_network->IEs)), 2);

	pframe += 2;
	pktlen += 2;

	if( (pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE) {
		//DBG_871X("ie len=%d\n", cur_network->IELength);
		pktlen += cur_network->IELength - sizeof(NDIS_802_11_FIXED_IEs);
		_rtw_memcpy(pframe, cur_network->IEs+sizeof(NDIS_802_11_FIXED_IEs), pktlen);

		goto _ConstructBeacon;
	}

	//below for ad-hoc mode

	// SSID
	pframe = rtw_set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pktlen);

	// supported rates...
	rate_len = rtw_get_rateset_len(cur_network->SupportedRates);
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &pktlen);

	// DS parameter set
	pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pktlen);

	if( (pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE) {
		u32 ATIMWindow;
		// IBSS Parameter Set...
		//ATIMWindow = cur->Configuration.ATIMWindow;
		ATIMWindow = 0;
		pframe = rtw_set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pktlen);
	}


	//todo: ERP IE


	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8) {
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pktlen);
	}


	//todo:HT for adhoc

_ConstructBeacon:

	if ((pktlen + TXDESC_SIZE) > 512) {
		DBG_871X("beacon frame too large\n");
		return;
	}

	*pLength = pktlen;
}

static void rtw_hal_construct_PSPoll(_adapter *padapter,
                                     u8 *pframe, u32 *pLength)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	//u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	//DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	// Frame control.
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	SetPwrMgt(fctrl);
	SetFrameSubType(pframe, WIFI_PSPOLL);

	// AID.
	SetDuration(pframe, (pmlmeinfo->aid | 0xc000));

	// BSSID.
	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	// TA.
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);

	*pLength = 16;
}

static void rtw_hal_construct_NullFunctionData(
    PADAPTER padapter,
    u8		*pframe,
    u32		*pLength,
    u8		*StaAddr,
    u8		bQoS,
    u8		AC,
    u8		bEosp,
    u8		bForcePowerSave)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16						*fctrl;
	u32						pktlen;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct wlan_network		*cur_network = &pmlmepriv->cur_network;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);


	//DBG_871X("%s:%d\n", __FUNCTION__, bForcePowerSave);

	pwlanhdr = (struct rtw_ieee80211_hdr*)pframe;

	fctrl = &pwlanhdr->frame_ctl;
	*(fctrl) = 0;
	if (bForcePowerSave) {
		SetPwrMgt(fctrl);
	}

	switch(cur_network->network.InfrastructureMode) {
	case Ndis802_11Infrastructure:
		SetToDs(fctrl);
		_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, StaAddr, ETH_ALEN);
		break;
	case Ndis802_11APMode:
		SetFrDs(fctrl);
		_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);
		break;
	case Ndis802_11IBSS:
	default:
		_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		break;
	}

	SetSeqNum(pwlanhdr, 0);

	if (bQoS == _TRUE) {
		struct rtw_ieee80211_hdr_3addr_qos *pwlanqoshdr;

		SetFrameSubType(pframe, WIFI_QOS_DATA_NULL);

		pwlanqoshdr = (struct rtw_ieee80211_hdr_3addr_qos*)pframe;
		SetPriority(&pwlanqoshdr->qc, AC);
		SetEOSP(&pwlanqoshdr->qc, bEosp);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr_qos);
	} else {
		SetFrameSubType(pframe, WIFI_DATA_NULL);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	}

	*pLength = pktlen;
}

void rtw_hal_fill_fake_txdesc(_adapter* padapter, u8* pDesc, u32 BufferLen,
                              u8 IsPsPoll, u8 IsBTQosNull, u8 bDataFrame)
{
	struct hal_ops *pHalFunc = &padapter->HalFunc;
	if (pHalFunc->fill_fake_txdesc == NULL) {
		DBG_871X_LEVEL(_drv_err_,
		               "%s missing fill_fake_txdesc\n", __func__);
		return;
	} else {
		pHalFunc->fill_fake_txdesc(padapter, pDesc, BufferLen,
		                           IsPsPoll, IsBTQosNull, bDataFrame);
	}
}

//
// Description: Fill the reserved packets that FW will use to RSVD page.
//			Now we just send 4 types packet to rsvd page.
//			(1)Beacon, (2)Ps-poll, (3)Null data, (4)ProbeRsp.
// Input:
// finished - FALSE:At the first time we will send all the packets as a large packet to Hw,
//		    so we need to set the packet length to total lengh.
//	      TRUE: At the second time, we should send the first packet (default:beacon)
//		    to Hw again and set the lengh in descriptor to the real beacon lengh.
// 2009.10.15 by tynli.
//
//Page Size = 128: 8188e, 8723a/b, 8192c/d,
//Page Size = 256: 8192e, 8821a
//Page Size = 512: 8812a
void rtw_hal_set_fw_rsvd_page(_adapter* adapter, bool finished)
{
	PHAL_DATA_TYPE pHalData;
	struct xmit_frame	*pcmdframe;
	struct pkt_attrib	*pattrib;
	struct xmit_priv	*pxmitpriv;
	struct mlme_ext_priv	*pmlmeext;
	struct mlme_ext_info	*pmlmeinfo;
	struct pwrctrl_priv *pwrctl;
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct hal_ops *pHalFunc = &adapter->HalFunc;
	u32	BeaconLength = 0, PSPollLength = 0;
	u32	NullDataLength = 0, QosNullLength = 0;
	u8	TxDescLen = TXDESC_SIZE, TxDescOffset = TXDESC_OFFSET;
	u8	TotalPageNum=0, CurtPktPageNum=0, RsvdPageNum=0;
	u8	*ReservedPagePacket;
	u16	BufIndex = 0;
	u32	TotalPacketLen = 0, MaxRsvdPageBufSize = 0, PageSize = 0;
	RSVDPAGE_LOC	RsvdPageLoc;

#ifdef DBG_CONFIG_ERROR_DETECT
	struct sreset_priv *psrtpriv;
#endif // DBG_CONFIG_ERROR_DETECT

	pHalData = GET_HAL_DATA(adapter);
#ifdef DBG_CONFIG_ERROR_DETECT
	psrtpriv = &pHalData->srestpriv;
#endif
	pxmitpriv = &adapter->xmitpriv;
	pmlmeext = &adapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;
	pwrctl = adapter_to_pwrctl(adapter);

	rtw_hal_get_def_var(adapter, HAL_DEF_TX_PAGE_SIZE, (u8 *)&PageSize);

	if (PageSize == 0) {
		DBG_871X("[Error]: %s, PageSize is zero!!\n", __func__);
		return;
	}

	if (pHalFunc->hal_get_tx_buff_rsvd_page_num != NULL) {
		RsvdPageNum =
		    pHalFunc->hal_get_tx_buff_rsvd_page_num(adapter, _TRUE);
		DBG_871X("%s PageSize: %d, RsvdPageNUm: %d\n",
		         __func__, PageSize, RsvdPageNum);
	} else {
		DBG_871X("[Error]: %s, missing tx_buff_rsvd_page_num func!!\n",
		         __func__);
		return;
	}

	MaxRsvdPageBufSize = RsvdPageNum*PageSize;

	pcmdframe = rtw_alloc_cmdxmitframe(pxmitpriv);
	if (pcmdframe == NULL) {
		DBG_871X("%s: alloc ReservedPagePacket fail!\n", __FUNCTION__);
		return;
	}

	ReservedPagePacket = pcmdframe->buf_addr;
	_rtw_memset(&RsvdPageLoc, 0, sizeof(RSVDPAGE_LOC));

	//beacon * 2 pages
	BufIndex = TxDescOffset;
	rtw_hal_construct_beacon(adapter,
	                         &ReservedPagePacket[BufIndex], &BeaconLength);

	// When we count the first page size, we need to reserve description size for the RSVD
	// packet, it will be filled in front of the packet in TXPKTBUF.
	CurtPktPageNum = (u8)PageNum_128(TxDescLen + BeaconLength);
	//If we don't add 1 more page, the WOWLAN function has a problem. Baron thinks it's a bug of firmware
	if (CurtPktPageNum == 1)
		CurtPktPageNum += 1;

	TotalPageNum += CurtPktPageNum;

	BufIndex += (CurtPktPageNum*PageSize);

	//ps-poll * 1 page
	RsvdPageLoc.LocPsPoll = TotalPageNum;
	DBG_871X("LocPsPoll: %d\n", RsvdPageLoc.LocPsPoll);
	rtw_hal_construct_PSPoll(adapter,
	                         &ReservedPagePacket[BufIndex], &PSPollLength);
	rtw_hal_fill_fake_txdesc(adapter,
	                         &ReservedPagePacket[BufIndex-TxDescLen],
	                         PSPollLength, _TRUE, _FALSE, _FALSE);

	CurtPktPageNum = (u8)PageNum((TxDescLen + PSPollLength), PageSize);

	TotalPageNum += CurtPktPageNum;

	BufIndex += (CurtPktPageNum*PageSize);

	//null data * 1 page
	RsvdPageLoc.LocNullData = TotalPageNum;
	DBG_871X("LocNullData: %d\n", RsvdPageLoc.LocNullData);
	rtw_hal_construct_NullFunctionData(
	    adapter,
	    &ReservedPagePacket[BufIndex],
	    &NullDataLength,
	    get_my_bssid(&pmlmeinfo->network),
	    _FALSE, 0, 0, _FALSE);
	rtw_hal_fill_fake_txdesc(adapter,
	                         &ReservedPagePacket[BufIndex-TxDescLen],
	                         NullDataLength, _FALSE, _FALSE, _FALSE);

	CurtPktPageNum = (u8)PageNum(TxDescLen + NullDataLength, PageSize);

	TotalPageNum += CurtPktPageNum;

	BufIndex += (CurtPktPageNum*PageSize);

	//Qos null data * 1 page
	RsvdPageLoc.LocQosNull = TotalPageNum;
	DBG_871X("LocQosNull: %d\n", RsvdPageLoc.LocQosNull);
	rtw_hal_construct_NullFunctionData(
	    adapter,
	    &ReservedPagePacket[BufIndex],
	    &QosNullLength,
	    get_my_bssid(&pmlmeinfo->network),
	    _TRUE, 0, 0, _FALSE);
	rtw_hal_fill_fake_txdesc(adapter,
	                         &ReservedPagePacket[BufIndex-TxDescLen],
	                         QosNullLength, _FALSE, _FALSE, _FALSE);

	CurtPktPageNum = (u8)PageNum(TxDescLen + QosNullLength, PageSize);
	TotalPageNum += CurtPktPageNum;
	BufIndex += (CurtPktPageNum*PageSize);
	TotalPacketLen = BufIndex + QosNullLength;

	if(TotalPacketLen > MaxRsvdPageBufSize) {
		DBG_871X("%s(ERROR): rsvd page size is not enough!!TotalPacketLen %d, MaxRsvdPageBufSize %d\n",
		         __FUNCTION__, TotalPacketLen,MaxRsvdPageBufSize);
		goto error;
	} else {
		// update attribute
		pattrib = &pcmdframe->attrib;
		update_mgntframe_attrib(adapter, pattrib);
		pattrib->qsel = 0x10;
		pattrib->pktlen = pattrib->last_txcmdsz = TotalPacketLen - TxDescOffset;
		dump_mgntframe_and_wait(adapter, pcmdframe, 100);
	}

	DBG_871X("%s: Set RSVD page location to Fw ,TotalPacketLen(%d), TotalPageNum(%d)\n",
	         __func__,TotalPacketLen,TotalPageNum);

	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) {
		rtw_hal_set_FwRsvdPage_cmd(adapter, &RsvdPageLoc);
		if (pwrctl->wowlan_mode == _TRUE)
			rtw_hal_set_FwAoacRsvdPage_cmd(adapter, &RsvdPageLoc);
	}
	return;
error:
	rtw_free_xmitframe(pxmitpriv, pcmdframe);
}

void SetHwReg(_adapter *adapter, u8 variable, const u8 *val)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);

	_func_enter_;

	switch (variable) {
	case HW_VAR_INITIAL_GAIN: {
		u8 rx_gain = *((u8 *)(val));
		//printk("rx_gain:%x\n",rx_gain);
		if(rx_gain == 0xff) { //restore rx gain
			//ODM_Write_DIG(podmpriv,pDigTable->BackupIGValue);
			odm_PauseDIG(odm, ODM_RESUME_DIG,rx_gain);
		} else {
			//pDigTable->BackupIGValue = pDigTable->CurIGValue;
			//ODM_Write_DIG(podmpriv,rx_gain);
			odm_PauseDIG(odm, ODM_PAUSE_DIG,rx_gain);
		}
	}
	break;
	case HW_VAR_PORT_SWITCH:
		hw_var_port_switch(adapter);
		break;
	case HW_VAR_INIT_RTS_RATE: {
		u16 brate_cfg = *((u16*)val);
		u8 rate_index = 0;
		HAL_VERSION *hal_ver = &hal_data->VersionID;

		if (IS_81XXC(*hal_ver) ||IS_92D(*hal_ver) || IS_8723_SERIES(*hal_ver) || IS_8188E(*hal_ver)) {

			while (brate_cfg > 0x1) {
				brate_cfg = (brate_cfg >> 1);
				rate_index++;
			}
			rtw_write8(adapter, REG_INIRTS_RATE_SEL, rate_index);
		} else {
			rtw_warn_on(1);
		}
	}
	break;
	case HW_VAR_SEC_CFG: {
#if 0
		// enable tx enc and rx dec engine, and no key search for MC/BC
		rtw_write8(adapter, REG_SECCFG, SCR_NoSKMC|SCR_RxDecEnable|SCR_TxEncEnable);
#elif defined(DYNAMIC_CAMID_ALLOC)
		u16 reg_scr;

		reg_scr = rtw_read16(adapter, REG_SECCFG);
		rtw_write16(adapter, REG_SECCFG, reg_scr|SCR_CHK_KEYID|SCR_RxDecEnable|SCR_TxEncEnable);
#else
		rtw_write8(adapter, REG_SECCFG, *((u8*)val));
#endif
	}
	break;
	case HW_VAR_SEC_DK_CFG: {
		struct security_priv *sec = &adapter->securitypriv;
		u8 reg_scr = rtw_read8(adapter, REG_SECCFG);

		if (val) { /* Enable default key related setting */
			reg_scr |= SCR_TXBCUSEDK;
			if (sec->dot11AuthAlgrthm != dot11AuthAlgrthm_8021X)
				reg_scr |= (SCR_RxUseDK|SCR_TxUseDK);
		} else { /* Disable default key related setting */
			reg_scr &= ~(SCR_RXBCUSEDK|SCR_TXBCUSEDK|SCR_RxUseDK|SCR_TxUseDK);
		}

		rtw_write8(adapter, REG_SECCFG, reg_scr);
	}
	break;
	case HW_VAR_DM_FLAG:
		odm->SupportAbility = *((u32*)val);
		break;
	case HW_VAR_DM_FUNC_OP:
		if (*((u8*)val) == _TRUE) {
			/* save dm flag */
			odm->BK_SupportAbility = odm->SupportAbility;
		} else {
			/* restore dm flag */
			odm->SupportAbility = odm->BK_SupportAbility;
		}
		break;
	case HW_VAR_DM_FUNC_SET:
		if(*((u32*)val) == DYNAMIC_ALL_FUNC_ENABLE) {
			struct dm_priv	*dm = &hal_data->dmpriv;
			dm->DMFlag = dm->InitDMFlag;
			odm->SupportAbility = dm->InitODMFlag;
		} else {
			odm->SupportAbility |= *((u32 *)val);
		}
		break;
	case HW_VAR_DM_FUNC_CLR:
		/*
		* input is already a mask to clear function
		* don't invert it again! George,Lucas@20130513
		*/
		odm->SupportAbility &= *((u32 *)val);
		break;
	case HW_VAR_ASIX_IOT:
		// enable  ASIX IOT function
		if (*((u8*)val) == _TRUE) {
			// 0xa2e[0]=0 (disable rake receiver)
			rtw_write8(adapter, rCCK0_FalseAlarmReport+2,
			           rtw_read8(adapter, rCCK0_FalseAlarmReport+2) & ~(BIT0));
			//  0xa1c=0xa0 (reset channel estimation if signal quality is bad)
			rtw_write8(adapter, rCCK0_DSPParameter2, 0xa0);
		} else {
			// restore reg:0xa2e,   reg:0xa1c
			rtw_write8(adapter, rCCK0_FalseAlarmReport+2,
			           rtw_read8(adapter, rCCK0_FalseAlarmReport+2)|(BIT0));
			rtw_write8(adapter, rCCK0_DSPParameter2, 0x00);
		}
		break;
	default:
		break;
	}

	_func_exit_;
}

void GetHwReg(_adapter *adapter, u8 variable, u8 *val)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);

	_func_enter_;

	switch (variable) {
	case HW_VAR_BASIC_RATE:
		*((u16*)val) = hal_data->BasicRateSet;
		break;
	case HW_VAR_DM_FLAG:
		*((u32*)val) = odm->SupportAbility;
		break;
	case HW_VAR_RF_TYPE:
		*((u8*)val) = hal_data->rf_type;
		break;
	default:
		if (0)
			DBG_871X_LEVEL(_drv_always_, FUNC_ADPT_FMT" variable(%d) not defined!\n",
			               FUNC_ADPT_ARG(adapter), variable);
		break;
	}

	_func_exit_;
}




u8 SetHalDefVar(_adapter *adapter, HAL_DEF_VARIABLE variable, const void *value)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);
	u8 bResult = _SUCCESS;

	switch(variable) {
	case HW_DEF_FA_CNT_DUMP:
		//ODM_COMP_COMMON
		if(*((u8*)value))
			odm->DebugComponents |= (ODM_COMP_DIG |ODM_COMP_FA_CNT);
		else
			odm->DebugComponents &= ~(ODM_COMP_DIG |ODM_COMP_FA_CNT);
		break;
	case HAL_DEF_DBG_RX_INFO_DUMP: {
		PFALSE_ALARM_STATISTICS FalseAlmCnt = (PFALSE_ALARM_STATISTICS)PhyDM_Get_Structure( odm , PHYDM_FALSEALMCNT);
		pDIG_T	pDM_DigTable = &odm->DM_DigTable;

		DBG_871X("============ Rx Info dump ===================\n");
		DBG_871X("bLinked = %d, RSSI_Min = %d(%%), CurrentIGI = 0x%x \n",
		         odm->bLinked, odm->RSSI_Min, pDM_DigTable->CurIGValue);
		DBG_871X("Cnt_Cck_fail = %d, Cnt_Ofdm_fail = %d, Total False Alarm = %d\n",
		         FalseAlmCnt->Cnt_Cck_fail, FalseAlmCnt->Cnt_Ofdm_fail, FalseAlmCnt->Cnt_all);

		if(odm->bLinked) {
			DBG_871X("RxRate = %s, RSSI_A = %d(%%), RSSI_B = %d(%%)\n",
			         HDATA_RATE(odm->RxRate), odm->RSSI_A, odm->RSSI_B);

#ifdef DBG_RX_SIGNAL_DISPLAY_RAW_DATA
			rtw_dump_raw_rssi_info(adapter);
#endif
		}
	}
	break;
	case HW_DEF_ODM_DBG_FLAG:
		ODM_CmnInfoUpdate(odm, ODM_CMNINFO_DBG_COMP, *((u8Byte*)value));
		break;
	case HW_DEF_ODM_DBG_LEVEL:
		ODM_CmnInfoUpdate(odm, ODM_CMNINFO_DBG_LEVEL, *((u4Byte*)value));
		break;
	case HAL_DEF_DBG_DM_FUNC: {
		u8 dm_func = *((u8*)value);
		struct dm_priv *dm = &hal_data->dmpriv;

		if(dm_func == 0) { //disable all dynamic func
			pDIG_T	pDM_DigTable = &odm->DM_DigTable;
			odm->SupportAbility = DYNAMIC_FUNC_DISABLE;
			pDM_DigTable->bStopDIG = _TRUE;
			DBG_8192C("==> Disable all dynamic function...\n");
		} else if(dm_func == 1) { //disable DIG
			pDIG_T	pDM_DigTable = &odm->DM_DigTable;
			odm->SupportAbility  &= (~DYNAMIC_BB_DIG);
			pDM_DigTable->bStopDIG = _TRUE;
			DBG_8192C("==> Disable DIG...\n");
		} else if(dm_func == 2) { //disable High power
			odm->SupportAbility  &= (~DYNAMIC_BB_DYNAMIC_TXPWR);
		} else if(dm_func == 3) { //disable tx power tracking
			odm->SupportAbility  &= (~DYNAMIC_RF_CALIBRATION);
			DBG_8192C("==> Disable tx power tracking...\n");
		} else if(dm_func == 4) { //disable BT coexistence
			dm->DMFlag &= (~DYNAMIC_FUNC_BT);
		} else if(dm_func == 5) { //disable antenna diversity
			odm->SupportAbility  &= (~DYNAMIC_BB_ANT_DIV);
		} else if(dm_func == 6) { //turn on all dynamic func
			if(!(odm->SupportAbility  & DYNAMIC_BB_DIG)) {
				DIG_T	*pDigTable = &odm->DM_DigTable;
				pDigTable->CurIGValue= rtw_read8(adapter, 0xc50);
				pDigTable->bStopDIG = _FALSE;
			}
			dm->DMFlag |= DYNAMIC_FUNC_BT;
			odm->SupportAbility = DYNAMIC_ALL_FUNC_ENABLE;

			DBG_8192C("==> Turn on all dynamic function...\n");
		}
	}
	break;
	case HAL_DEF_DBG_DUMP_RXPKT:
		hal_data->bDumpRxPkt = *((u8*)value);
		break;
	case HAL_DEF_DBG_DUMP_TXPKT:
		hal_data->bDumpTxPkt = *((u8*)value);
		break;
	case HAL_DEF_ANT_DETECT:
		hal_data->AntDetection = *((u8 *)value);
		break;
	case HAL_DEF_DBG_DIS_PWT:
		hal_data->bDisableTXPowerTraining = *((u8*)value);
		break;
	default:
		DBG_871X_LEVEL(_drv_always_, "%s: [WARNING] HAL_DEF_VARIABLE(%d) not defined!\n", __FUNCTION__, variable);
		bResult = _FAIL;
		break;
	}

	return bResult;
}

u8
GetHalDefVar(_adapter *adapter, HAL_DEF_VARIABLE variable, void *value)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);
	u8 bResult = _SUCCESS;

	switch(variable) {
	case HAL_DEF_UNDERCORATEDSMOOTHEDPWDB: {
		struct mlme_priv *pmlmepriv;
		struct sta_priv *pstapriv;
		struct sta_info *psta;

		pmlmepriv = &adapter->mlmepriv;
		pstapriv = &adapter->stapriv;
		psta = rtw_get_stainfo(pstapriv, pmlmepriv->cur_network.network.MacAddress);
		if (psta) {
			*((int*)value) = psta->rssi_stat.UndecoratedSmoothedPWDB;
		}
	}
	break;
	case HW_DEF_ODM_DBG_FLAG:
		*((u8Byte*)value) = odm->DebugComponents;
		break;
	case HW_DEF_ODM_DBG_LEVEL:
		*((u4Byte*)value) = odm->DebugLevel;
		break;
	case HAL_DEF_DBG_DM_FUNC:
		*(( u32*)value) =hal_data->odmpriv.SupportAbility;
		break;
	case HAL_DEF_DBG_DUMP_RXPKT:
		*((u8*)value) = hal_data->bDumpRxPkt;
		break;
	case HAL_DEF_DBG_DUMP_TXPKT:
		*((u8*)value) = hal_data->bDumpTxPkt;
		break;
	case HAL_DEF_ANT_DETECT:
		*((u8 *)value) = hal_data->AntDetection;
		break;
	case HAL_DEF_MACID_SLEEP:
		*(u8*)value = _FALSE;
		break;
	case HAL_DEF_TX_PAGE_SIZE:
		*(( u32*)value) = PAGE_SIZE_128;
		break;
	case HAL_DEF_DBG_DIS_PWT:
		*(u8*)value = hal_data->bDisableTXPowerTraining;
		break;
	default:
		DBG_871X_LEVEL(_drv_always_, "%s: [WARNING] HAL_DEF_VARIABLE(%d) not defined!\n", __FUNCTION__, variable);
		bResult = _FAIL;
		break;
	}

	return bResult;
}

void GetHalODMVar(
    PADAPTER				Adapter,
    HAL_ODM_VARIABLE		eVariable,
    PVOID					pValue1,
    PVOID					pValue2)
{
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	//PDM_ODM_T podmpriv = &pHalData->odmpriv;
	switch(eVariable) {
#if defined(CONFIG_SIGNAL_DISPLAY_DBM) && defined(CONFIG_BACKGROUND_NOISE_MONITOR)
	case HAL_ODM_NOISE_MONITOR: {
		u8 chan = *(u8*)pValue1;
		*(s16 *)pValue2 = pHalData->noise[chan];
#ifdef DBG_NOISE_MONITOR
		DBG_8192C("### Noise monitor chan(%d)-noise:%d (dBm) ###\n",
		          chan,pHalData->noise[chan]);
#endif

	}
	break;
#endif//#ifdef CONFIG_BACKGROUND_NOISE_MONITOR
	default:
		break;
	}
}

void SetHalODMVar(
    PADAPTER				Adapter,
    HAL_ODM_VARIABLE		eVariable,
    PVOID					pValue1,
    BOOLEAN					bSet)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PDM_ODM_T podmpriv = &pHalData->odmpriv;
	//_irqL irqL;
	switch(eVariable) {
	case HAL_ODM_STA_INFO: {
		struct sta_info *psta = (struct sta_info *)pValue1;
		if(bSet) {
			DBG_8192C("### Set STA_(%d) info ###\n",psta->mac_id);
			ODM_CmnInfoPtrArrayHook(podmpriv, ODM_CMNINFO_STA_STATUS,psta->mac_id,psta);
		} else {
			DBG_8192C("### Clean STA_(%d) info ###\n",psta->mac_id);
			//_enter_critical_bh(&pHalData->odm_stainfo_lock, &irqL);
			ODM_CmnInfoPtrArrayHook(podmpriv, ODM_CMNINFO_STA_STATUS,psta->mac_id,NULL);

			//_exit_critical_bh(&pHalData->odm_stainfo_lock, &irqL);
		}
	}
	break;
	case HAL_ODM_P2P_STATE:
		ODM_CmnInfoUpdate(podmpriv,ODM_CMNINFO_WIFI_DIRECT,bSet);
		break;
	case HAL_ODM_WIFI_DISPLAY_STATE:
		ODM_CmnInfoUpdate(podmpriv,ODM_CMNINFO_WIFI_DISPLAY,bSet);
		break;
	case HAL_ODM_REGULATION:
		ODM_CmnInfoInit(podmpriv, ODM_CMNINFO_DOMAIN_CODE_2G, pHalData->Regulation2_4G);
		ODM_CmnInfoInit(podmpriv, ODM_CMNINFO_DOMAIN_CODE_5G, pHalData->Regulation5G);
		break;
#if defined(CONFIG_SIGNAL_DISPLAY_DBM) && defined(CONFIG_BACKGROUND_NOISE_MONITOR)
	case HAL_ODM_NOISE_MONITOR: {
		struct noise_info *pinfo = (struct noise_info *)pValue1;

#ifdef DBG_NOISE_MONITOR
		DBG_8192C("### Noise monitor chan(%d)-bPauseDIG:%d,IGIValue:0x%02x,max_time:%d (ms) ###\n",
		          pinfo->chan,pinfo->bPauseDIG,pinfo->IGIValue,pinfo->max_time);
#endif

		pHalData->noise[pinfo->chan] = ODM_InbandNoise_Monitor(podmpriv,pinfo->bPauseDIG,pinfo->IGIValue,pinfo->max_time);
		DBG_871X("chan_%d, noise = %d (dBm)\n",pinfo->chan,pHalData->noise[pinfo->chan]);
#ifdef DBG_NOISE_MONITOR
		DBG_871X("noise_a = %d, noise_b = %d  noise_all:%d \n",
		         podmpriv->noise_level.noise[ODM_RF_PATH_A],
		         podmpriv->noise_level.noise[ODM_RF_PATH_B],
		         podmpriv->noise_level.noise_all);
#endif
	}
	break;
#endif//#ifdef CONFIG_BACKGROUND_NOISE_MONITOR

	default:
		break;
	}
}


BOOLEAN
eqNByte(
    u8*	str1,
    u8*	str2,
    u32	num
)
{
	if(num==0)
		return _FALSE;
	while(num>0) {
		num--;
		if(str1[num]!=str2[num])
			return _FALSE;
	}
	return _TRUE;
}

//
//	Description:
//		Return TRUE if chTmp is represent for hex digit and
//		FALSE otherwise.
//
//
BOOLEAN
IsHexDigit(
    IN		char		chTmp
)
{
	if( (chTmp >= '0' && chTmp <= '9') ||
	    (chTmp >= 'a' && chTmp <= 'f') ||
	    (chTmp >= 'A' && chTmp <= 'F') ) {
		return _TRUE;
	} else {
		return _FALSE;
	}
}


//
//	Description:
//		Translate a character to hex digit.
//
u32
MapCharToHexDigit(
    IN		char		chTmp
)
{
	if(chTmp >= '0' && chTmp <= '9')
		return (chTmp - '0');
	else if(chTmp >= 'a' && chTmp <= 'f')
		return (10 + (chTmp - 'a'));
	else if(chTmp >= 'A' && chTmp <= 'F')
		return (10 + (chTmp - 'A'));
	else
		return 0;
}



//
//	Description:
//		Parse hex number from the string pucStr.
//
BOOLEAN
GetHexValueFromString(
    IN		char*			szStr,
    IN OUT	u32*			pu4bVal,
    IN OUT	u32*			pu4bMove
)
{
	char*		szScan = szStr;

	// Check input parameter.
	if(szStr == NULL || pu4bVal == NULL || pu4bMove == NULL) {
		DBG_871X("GetHexValueFromString(): Invalid inpur argumetns! szStr: %p, pu4bVal: %p, pu4bMove: %p\n", szStr, pu4bVal, pu4bMove);
		return _FALSE;
	}

	// Initialize output.
	*pu4bMove = 0;
	*pu4bVal = 0;

	// Skip leading space.
	while(	*szScan != '\0' &&
	        (*szScan == ' ' || *szScan == '\t') ) {
		szScan++;
		(*pu4bMove)++;
	}

	// Skip leading '0x' or '0X'.
	if(*szScan == '0' && (*(szScan+1) == 'x' || *(szScan+1) == 'X')) {
		szScan += 2;
		(*pu4bMove) += 2;
	}

	// Check if szScan is now pointer to a character for hex digit,
	// if not, it means this is not a valid hex number.
	if(!IsHexDigit(*szScan)) {
		return _FALSE;
	}

	// Parse each digit.
	do {
		(*pu4bVal) <<= 4;
		*pu4bVal += MapCharToHexDigit(*szScan);

		szScan++;
		(*pu4bMove)++;
	} while(IsHexDigit(*szScan));

	return _TRUE;
}

BOOLEAN
GetFractionValueFromString(
    IN		char*			szStr,
    IN OUT	u8*				pInteger,
    IN OUT	u8*				pFraction,
    IN OUT	u32*			pu4bMove
)
{
	char	*szScan = szStr;

	// Initialize output.
	*pu4bMove = 0;
	*pInteger = 0;
	*pFraction = 0;

	// Skip leading space.
	while (	*szScan != '\0' && 	(*szScan == ' ' || *szScan == '\t') ) {
		++szScan;
		++(*pu4bMove);
	}

	// Parse each digit.
	do {
		(*pInteger) *= 10;
		*pInteger += ( *szScan - '0' );

		++szScan;
		++(*pu4bMove);

		if ( *szScan == '.' ) {
			++szScan;
			++(*pu4bMove);

			if ( *szScan < '0' || *szScan > '9' )
				return _FALSE;
			else {
				*pFraction = *szScan - '0';
				++szScan;
				++(*pu4bMove);
				return _TRUE;
			}
		}
	} while(*szScan >= '0' && *szScan <= '9');

	return _TRUE;
}

//
//	Description:
//		Return TRUE if szStr is comment out with leading "//".
//
BOOLEAN
IsCommentString(
    IN		char			*szStr
)
{
	if(*szStr == '/' && *(szStr+1) == '/') {
		return _TRUE;
	} else {
		return _FALSE;
	}
}

BOOLEAN
GetU1ByteIntegerFromStringInDecimal(
    IN		char*	Str,
    IN OUT	u8*		pInt
)
{
	u16 i = 0;
	*pInt = 0;

	while ( Str[i] != '\0' ) {
		if ( Str[i] >= '0' && Str[i] <= '9' ) {
			*pInt *= 10;
			*pInt += ( Str[i] - '0' );
		} else {
			return _FALSE;
		}
		++i;
	}

	return _TRUE;
}

// <20121004, Kordan> For example,
// ParseQualifiedString(inString, 0, outString, '[', ']') gets "Kordan" from a string "Hello [Kordan]".
// If RightQualifier does not exist, it will hang on in the while loop
BOOLEAN
ParseQualifiedString(
    IN		char*	In,
    IN OUT	u32*	Start,
    OUT		char*	Out,
    IN		char		LeftQualifier,
    IN		char		RightQualifier
)
{
	u32	i = 0, j = 0;
	char	c = In[(*Start)++];

	if (c != LeftQualifier)
		return _FALSE;

	i = (*Start);
	while ((c = In[(*Start)++]) != RightQualifier)
		; // find ']'
	j = (*Start) - 2;
	strncpy((char *)Out, (const char*)(In+i), j-i+1);

	return _TRUE;
}

BOOLEAN
isAllSpaceOrTab(
    u8*	data,
    u8	size
)
{
	u8	cnt = 0, NumOfSpaceAndTab = 0;

	while( size > cnt ) {
		if ( data[cnt] == ' ' || data[cnt] == '\t' || data[cnt] == '\0' )
			++NumOfSpaceAndTab;

		++cnt;
	}

	return size == NumOfSpaceAndTab;
}


void rtw_hal_check_rxfifo_full(_adapter *adapter)
{
	struct dvobj_priv *psdpriv = adapter->dvobj;
	struct debug_priv *pdbgpriv = &psdpriv->drv_dbg;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(adapter);
	int save_cnt=_FALSE;

	//switch counter to RX fifo
	if(IS_81XXC(pHalData->VersionID) || IS_92D(pHalData->VersionID)
	   || IS_8188E(pHalData->VersionID) || IS_8723_SERIES(pHalData->VersionID)
	   || IS_8812_SERIES(pHalData->VersionID) || IS_8821_SERIES(pHalData->VersionID)
	   || IS_8723B_SERIES(pHalData->VersionID) || IS_8192E(pHalData->VersionID)) {
		rtw_write8(adapter, REG_RXERR_RPT+3, rtw_read8(adapter, REG_RXERR_RPT+3)|0xa0);
		save_cnt = _TRUE;
	} else {
		//todo: other chips
	}


	if(save_cnt) {
		//rtw_write8(adapter, REG_RXERR_RPT+3, rtw_read8(adapter, REG_RXERR_RPT+3)|0xa0);
		pdbgpriv->dbg_rx_fifo_last_overflow = pdbgpriv->dbg_rx_fifo_curr_overflow;
		pdbgpriv->dbg_rx_fifo_curr_overflow = rtw_read16(adapter, REG_RXERR_RPT);
		pdbgpriv->dbg_rx_fifo_diff_overflow = pdbgpriv->dbg_rx_fifo_curr_overflow-pdbgpriv->dbg_rx_fifo_last_overflow;
	}
}

void linked_info_dump(_adapter *padapter,u8 benable)
{
	struct pwrctrl_priv *pwrctrlpriv = adapter_to_pwrctl(padapter);

	if(padapter->bLinkInfoDump == benable)
		return;

	DBG_871X("%s %s \n",__FUNCTION__,(benable)?"enable":"disable");

	if(benable) {
#ifdef CONFIG_LPS
		pwrctrlpriv->org_power_mgnt = pwrctrlpriv->power_mgnt;//keep org value
		rtw_pm_set_lps(padapter,PS_MODE_ACTIVE);
#endif

#ifdef CONFIG_IPS
		pwrctrlpriv->ips_org_mode = pwrctrlpriv->ips_mode;//keep org value
		rtw_pm_set_ips(padapter,IPS_NONE);
#endif
	} else {
#ifdef CONFIG_IPS
		rtw_pm_set_ips(padapter, pwrctrlpriv->ips_org_mode);
#endif // CONFIG_IPS

#ifdef CONFIG_LPS
		rtw_pm_set_lps(padapter, pwrctrlpriv->org_power_mgnt );
#endif // CONFIG_LPS
	}
	padapter->bLinkInfoDump = benable ;
}

#ifdef DBG_RX_SIGNAL_DISPLAY_RAW_DATA
void rtw_get_raw_rssi_info(void *sel, _adapter *padapter)
{
	u8 isCCKrate,rf_path;
	PHAL_DATA_TYPE	pHalData =  GET_HAL_DATA(padapter);
	struct rx_raw_rssi *psample_pkt_rssi = &padapter->recvpriv.raw_rssi_info;

	DBG_871X_SEL_NL(sel,"RxRate = %s, PWDBALL = %d(%%), rx_pwr_all = %d(dBm)\n",
	                HDATA_RATE(psample_pkt_rssi->data_rate), psample_pkt_rssi->pwdball, psample_pkt_rssi->pwr_all);
	isCCKrate = (psample_pkt_rssi->data_rate <= DESC_RATE11M)?TRUE :FALSE;

	if(isCCKrate)
		psample_pkt_rssi->mimo_singal_strength[0] = psample_pkt_rssi->pwdball;

	for(rf_path = 0; rf_path<pHalData->NumTotalRFPath; rf_path++) {
		DBG_871X_SEL_NL(sel,"RF_PATH_%d=>singal_strength:%d(%%),singal_quality:%d(%%)\n"
		                ,rf_path,psample_pkt_rssi->mimo_singal_strength[rf_path],psample_pkt_rssi->mimo_singal_quality[rf_path]);

		if(!isCCKrate) {
			DBG_871X_SEL_NL(sel,"\trx_ofdm_pwr:%d(dBm),rx_ofdm_snr:%d(dB)\n",
			                psample_pkt_rssi->ofdm_pwr[rf_path],psample_pkt_rssi->ofdm_snr[rf_path]);
		}
	}
}

void rtw_dump_raw_rssi_info(_adapter *padapter)
{
	u8 isCCKrate,rf_path;
	PHAL_DATA_TYPE	pHalData =  GET_HAL_DATA(padapter);
	struct rx_raw_rssi *psample_pkt_rssi = &padapter->recvpriv.raw_rssi_info;
	DBG_871X("============ RAW Rx Info dump ===================\n");
	DBG_871X("RxRate = %s, PWDBALL = %d(%%), rx_pwr_all = %d(dBm)\n",
	         HDATA_RATE(psample_pkt_rssi->data_rate), psample_pkt_rssi->pwdball, psample_pkt_rssi->pwr_all);

	isCCKrate = (psample_pkt_rssi->data_rate <= DESC_RATE11M)?TRUE :FALSE;

	if(isCCKrate)
		psample_pkt_rssi->mimo_singal_strength[0] = psample_pkt_rssi->pwdball;

	for(rf_path = 0; rf_path<pHalData->NumTotalRFPath; rf_path++) {
		DBG_871X("RF_PATH_%d=>singal_strength:%d(%%),singal_quality:%d(%%)"
		         ,rf_path,psample_pkt_rssi->mimo_singal_strength[rf_path],psample_pkt_rssi->mimo_singal_quality[rf_path]);

		if(!isCCKrate) {
			printk(",rx_ofdm_pwr:%d(dBm),rx_ofdm_snr:%d(dB)\n",
			       psample_pkt_rssi->ofdm_pwr[rf_path],psample_pkt_rssi->ofdm_snr[rf_path]);
		} else {
			printk("\n");
		}
	}
}

void rtw_store_phy_info(_adapter *padapter, union recv_frame *prframe)
{
	u8 isCCKrate,rf_path;
	PHAL_DATA_TYPE	pHalData =  GET_HAL_DATA(padapter);
	struct rx_pkt_attrib *pattrib = &prframe->u.hdr.attrib;

	PODM_PHY_INFO_T pPhyInfo  = (PODM_PHY_INFO_T)(&pattrib->phy_info);
	struct rx_raw_rssi *psample_pkt_rssi = &padapter->recvpriv.raw_rssi_info;

	psample_pkt_rssi->data_rate = pattrib->data_rate;
	isCCKrate = (pattrib->data_rate <= DESC_RATE11M)?TRUE :FALSE;

	psample_pkt_rssi->pwdball = pPhyInfo->RxPWDBAll;
	psample_pkt_rssi->pwr_all = pPhyInfo->RecvSignalPower;

	for(rf_path = 0; rf_path<pHalData->NumTotalRFPath; rf_path++) {
		psample_pkt_rssi->mimo_singal_strength[rf_path] = pPhyInfo->RxMIMOSignalStrength[rf_path];
		psample_pkt_rssi->mimo_singal_quality[rf_path] = pPhyInfo->RxMIMOSignalQuality[rf_path];
		if(!isCCKrate) {
			psample_pkt_rssi->ofdm_pwr[rf_path] = pPhyInfo->RxPwr[rf_path];
			psample_pkt_rssi->ofdm_snr[rf_path] = pPhyInfo->RxSNR[rf_path];
		}
	}
}
#endif

#ifdef CONFIG_RF_GAIN_OFFSET
u32 Array_kfreemap[] = {
	0x08,0xe,
	0x06,0xc,
	0x04,0xa,
	0x02,0x8,
	0x00,0x6,
	0x03,0x4,
	0x05,0x2,
	0x07,0x0,
	0x09,0x0,
	0x0c,0x0,
};

void rtw_bb_rf_gain_offset(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8		value = padapter->eeprompriv.EEPROMRFGainOffset;
	u8		tmp = 0x3e;
	u32 	res,i=0;
	u4Byte	   ArrayLen    = sizeof(Array_kfreemap)/sizeof(u32);
	pu4Byte    Array	   = Array_kfreemap;
	u4Byte v1=0,v2=0,GainValue,target=0;
	//DBG_871X("+%s value: 0x%02x+\n", __func__, value);

	if (!(value & 0x01)) {
		//DBG_871X("Offset RF Gain.\n");
		res = rtw_hal_read_rfreg(padapter, RF_PATH_A, REG_RF_BB_GAIN_OFFSET, 0xffffffff);
		value &= tmp;
		res = value << 14;
		rtw_hal_write_rfreg(padapter, RF_PATH_A, REG_RF_BB_GAIN_OFFSET, RF_GAIN_OFFSET_MASK, res);
	} else {
		DBG_871X("Using the default RF gain.\n");
	}
}
#endif //CONFIG_RF_GAIN_OFFSET

#ifdef CONFIG_USB_RX_AGGREGATION
void rtw_set_usb_agg_by_mode(_adapter *padapter, u8 cur_wireless_mode)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	if(cur_wireless_mode < WIRELESS_11_24N
	   && cur_wireless_mode > 0) { //ABG mode
		if(0x6 != pHalData->RegAcUsbDmaSize || 0x10 !=pHalData->RegAcUsbDmaTime) {
			pHalData->RegAcUsbDmaSize = 0x6;
			pHalData->RegAcUsbDmaTime = 0x10;
			rtw_write16(padapter, REG_RXDMA_AGG_PG_TH,
			            pHalData->RegAcUsbDmaSize | (pHalData->RegAcUsbDmaTime<<8));
		}

	} else if(cur_wireless_mode >= WIRELESS_11_24N
	          && cur_wireless_mode <= WIRELESS_MODE_MAX) { //N AC mode
		if(0x5 != pHalData->RegAcUsbDmaSize || 0x20 !=pHalData->RegAcUsbDmaTime) {
			pHalData->RegAcUsbDmaSize = 0x5;
			pHalData->RegAcUsbDmaTime = 0x20;
			rtw_write16(padapter, REG_RXDMA_AGG_PG_TH,
			            pHalData->RegAcUsbDmaSize | (pHalData->RegAcUsbDmaTime<<8));
		}

	} else {
		DBG_871X("%s: Unknow wireless mode(0x%x)\n",__func__,padapter->mlmeextpriv.cur_wireless_mode);
	}
}
#endif //CONFIG_USB_RX_AGGREGATION

//To avoid RX affect TX throughput
void dm_DynamicUsbTxAgg(_adapter *padapter, u8 from_timer)
{
	struct dvobj_priv	*pdvobjpriv = adapter_to_dvobj(padapter);
	struct mlme_priv		*pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeextpriv = &(padapter->mlmeextpriv);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 cur_wireless_mode = pmlmeextpriv->cur_wireless_mode;
#if 0
	struct mlme_ext_priv	*pbuddymlmeextpriv = &(padapter->pbuddy_adapter->mlmeextpriv);
#endif //

#ifdef CONFIG_USB_RX_AGGREGATION
	if(IS_HARDWARE_TYPE_8821U(padapter) ) { //|| IS_HARDWARE_TYPE_8192EU(padapter))
		//This AGG_PH_TH only for UsbRxAggMode == USB_RX_AGG_USB
		if((pHalData->UsbRxAggMode == USB_RX_AGG_USB) && (check_fwstate(pmlmepriv, _FW_LINKED)== _TRUE)) {
			if(pdvobjpriv->traffic_stat.cur_tx_tp > 2 && pdvobjpriv->traffic_stat.cur_rx_tp < 30)
				rtw_write16(padapter, REG_RXDMA_AGG_PG_TH,0x1010);
			else if (pdvobjpriv->traffic_stat.last_tx_bytes > 220000 && pdvobjpriv->traffic_stat.cur_rx_tp < 30)
				rtw_write16(padapter , REG_RXDMA_AGG_PG_TH , 0x1006);
			else
				rtw_write16(padapter, REG_RXDMA_AGG_PG_TH,0x2005); //dmc agg th 20K

			//DBG_871X("TX_TP=%u, RX_TP=%u \n", pdvobjpriv->traffic_stat.cur_tx_tp, pdvobjpriv->traffic_stat.cur_rx_tp);
		}
	} else if(IS_HARDWARE_TYPE_8812(padapter)) {
#if 0
		if(rtw_linked_check(padapter) == _TRUE && rtw_linked_check(padapter->pbuddy_adapter) == _TRUE) {
			if(pbuddymlmeextpriv->cur_wireless_mode >= pmlmeextpriv->cur_wireless_mode)
				cur_wireless_mode = pbuddymlmeextpriv->cur_wireless_mode;
			else
				cur_wireless_mode = pmlmeextpriv->cur_wireless_mode;

			rtw_set_usb_agg_by_mode(padapter,cur_wireless_mode);
		} else if (rtw_linked_check(padapter) == _TRUE && rtw_linked_check(padapter->pbuddy_adapter) == _FALSE) {
			rtw_set_usb_agg_by_mode(padapter,cur_wireless_mode);
		}
#else
		rtw_set_usb_agg_by_mode(padapter,cur_wireless_mode);
#endif //
	}
#endif
}

//bus-agg check for SoftAP mode
inline u8 rtw_hal_busagg_qsel_check(_adapter *padapter,u8 pre_qsel,u8 next_qsel)
{
	struct mlme_priv *pmlmepriv = &(padapter->mlmepriv);
	u8 chk_rst = _SUCCESS;

	if(check_fwstate(pmlmepriv, WIFI_AP_STATE) != _TRUE)
		return chk_rst;

	//if((pre_qsel == 0xFF)||(next_qsel== 0xFF))
	//	return chk_rst;

	if(	((pre_qsel == QSLT_HIGH)||((next_qsel== QSLT_HIGH)))
	    && (pre_qsel != next_qsel )) {
		//DBG_871X("### bus-agg break cause of qsel misatch, pre_qsel=0x%02x,next_qsel=0x%02x ###\n",
		//	pre_qsel,next_qsel);
		chk_rst = _FAIL;
	}
	return chk_rst;
}

/*
 * Description:
 * dump_TX_FIFO: This is only used to dump TX_FIFO for debug WoW mode offload
 * contant.
 *
 * Input:
 * adapter: adapter pointer.
 * page_num: The max. page number that user want to dump.
 * page_size: page size of each page. eg. 128 bytes, 256 bytes, 512byte.
 */
void dump_TX_FIFO(_adapter* padapter, u8 page_num, u16 page_size)
{

	int i;
	u8 val = 0;
	u8 base = 0;
	u32 addr = 0;
	u32 count = (page_size / 8);

	if (page_num <= 0) {
		DBG_871X("!!%s: incorrect input page_num paramter!\n", __func__);
		return;
	}

	if (page_size < 128 || page_size > 512) {
		DBG_871X("!!%s: incorrect input page_size paramter!\n", __func__);
		return;
	}

	DBG_871X("+%s+\n", __func__);
	val = rtw_read8(padapter, 0x106);
	rtw_write8(padapter, 0x106, 0x69);
	DBG_871X("0x106: 0x%02x\n", val);
	base = rtw_read8(padapter, 0x209);
	DBG_871X("0x209: 0x%02x\n", base);

	addr = ((base) * page_size)/8;
	for (i = 0 ; i < page_num * count ; i+=2) {
		rtw_write32(padapter, 0x140, addr + i);
		printk(" %08x %08x ", rtw_read32(padapter, 0x144), rtw_read32(padapter, 0x148));
		rtw_write32(padapter, 0x140, addr + i + 1);
		printk(" %08x %08x \n", rtw_read32(padapter, 0x144), rtw_read32(padapter, 0x148));
	}
}

#ifdef CONFIG_GPIO_API
u8 rtw_hal_get_gpio(_adapter* adapter, u8 gpio_num)
{
	u8 value;
	u8 direction;
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);

	rtw_ps_deny(adapter, PS_DENY_IOCTL);

	DBG_871X("rf_pwrstate=0x%02x\n", pwrpriv->rf_pwrstate);
	LeaveAllPowerSaveModeDirect(adapter);

	/* Read GPIO Direction */
	direction = (rtw_read8(adapter,REG_GPIO_PIN_CTRL + 2) & BIT(gpio_num)) >> gpio_num;

	/* According the direction to read register value */
	if( direction )
		value =  (rtw_read8(adapter, REG_GPIO_PIN_CTRL + 1)& BIT(gpio_num)) >> gpio_num;
	else
		value =  (rtw_read8(adapter, REG_GPIO_PIN_CTRL)& BIT(gpio_num)) >> gpio_num;

	rtw_ps_deny_cancel(adapter, PS_DENY_IOCTL);
	DBG_871X("%s direction=%d value=%d\n",__FUNCTION__,direction,value);

	return value;
}

int  rtw_hal_set_gpio_output_value(_adapter* adapter, u8 gpio_num, bool isHigh)
{
	u8 direction = 0;
	u8 res = -1;
	if (IS_HARDWARE_TYPE_8188E(adapter)) {
		/* Check GPIO is 4~7 */
		if( gpio_num > 7 || gpio_num < 4) {
			DBG_871X("%s The gpio number does not included 4~7.\n",__FUNCTION__);
			return -1;
		}
	}

	rtw_ps_deny(adapter, PS_DENY_IOCTL);

	LeaveAllPowerSaveModeDirect(adapter);

	/* Read GPIO direction */
	direction = (rtw_read8(adapter,REG_GPIO_PIN_CTRL + 2) & BIT(gpio_num)) >> gpio_num;

	/* If GPIO is output direction, setting value. */
	if( direction ) {
		if(isHigh)
			rtw_write8(adapter, REG_GPIO_PIN_CTRL + 1, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 1) | BIT(gpio_num));
		else
			rtw_write8(adapter, REG_GPIO_PIN_CTRL + 1, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 1) & ~BIT(gpio_num));

		DBG_871X("%s Set gpio %x[%d]=%d\n",__FUNCTION__,REG_GPIO_PIN_CTRL+1,gpio_num,isHigh );
		res = 0;
	} else {
		DBG_871X("%s The gpio is input,not be set!\n",__FUNCTION__);
		res = -1;
	}

	rtw_ps_deny_cancel(adapter, PS_DENY_IOCTL);
	return res;
}

int rtw_hal_config_gpio(_adapter* adapter, u8 gpio_num, bool isOutput)
{
	if (IS_HARDWARE_TYPE_8188E(adapter)) {
		if( gpio_num > 7 || gpio_num < 4) {
			DBG_871X("%s The gpio number does not included 4~7.\n",__FUNCTION__);
			return -1;
		}
	}

	DBG_871X("%s gpio_num =%d direction=%d\n",__FUNCTION__,gpio_num,isOutput);

	rtw_ps_deny(adapter, PS_DENY_IOCTL);

	LeaveAllPowerSaveModeDirect(adapter);

	if( isOutput ) {
		rtw_write8(adapter, REG_GPIO_PIN_CTRL + 2, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 2) | BIT(gpio_num));
	} else {
		rtw_write8(adapter, REG_GPIO_PIN_CTRL + 2, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 2) & ~BIT(gpio_num));
	}

	rtw_ps_deny_cancel(adapter, PS_DENY_IOCTL);

	return 0;
}
int rtw_hal_register_gpio_interrupt(_adapter* adapter, int gpio_num, void(*callback)(u8 level))
{
	u8 value;
	u8 direction;
	PHAL_DATA_TYPE phal = GET_HAL_DATA(adapter);

	if (IS_HARDWARE_TYPE_8188E(adapter)) {
		if(gpio_num > 7 || gpio_num < 4) {
			DBG_871X_LEVEL(_drv_always_, "%s The gpio number does not included 4~7.\n",__FUNCTION__);
			return -1;
		}
	}

	rtw_ps_deny(adapter, PS_DENY_IOCTL);

	LeaveAllPowerSaveModeDirect(adapter);

	/* Read GPIO direction */
	direction = (rtw_read8(adapter,REG_GPIO_PIN_CTRL + 2) & BIT(gpio_num)) >> gpio_num;
	if(direction) {
		DBG_871X_LEVEL(_drv_always_, "%s Can't register output gpio as interrupt.\n",__FUNCTION__);
		return -1;
	}

	/* Config GPIO Mode */
	rtw_write8(adapter, REG_GPIO_PIN_CTRL + 3, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 3) | BIT(gpio_num));

	/* Register GPIO interrupt handler*/
	adapter->gpiointpriv.callback[gpio_num] = callback;

	/* Set GPIO interrupt mode, 0:positive edge, 1:negative edge */
	value = rtw_read8(adapter, REG_GPIO_PIN_CTRL) & BIT(gpio_num);
	adapter->gpiointpriv.interrupt_mode = rtw_read8(adapter, REG_HSIMR + 2)^value;
	rtw_write8(adapter, REG_GPIO_INTM, adapter->gpiointpriv.interrupt_mode);

	/* Enable GPIO interrupt */
	adapter->gpiointpriv.interrupt_enable_mask = rtw_read8(adapter, REG_HSIMR + 2) | BIT(gpio_num);
	rtw_write8(adapter, REG_HSIMR + 2, adapter->gpiointpriv.interrupt_enable_mask);

	rtw_hal_update_hisr_hsisr_ind(adapter, 1);

	rtw_ps_deny_cancel(adapter, PS_DENY_IOCTL);

	return 0;
}
int rtw_hal_disable_gpio_interrupt(_adapter* adapter, int gpio_num)
{
	u8 value;
	u8 direction;
	PHAL_DATA_TYPE phal = GET_HAL_DATA(adapter);

	if (IS_HARDWARE_TYPE_8188E(adapter)) {
		if(gpio_num > 7 || gpio_num < 4) {
			DBG_871X("%s The gpio number does not included 4~7.\n",__FUNCTION__);
			return -1;
		}
	}

	rtw_ps_deny(adapter, PS_DENY_IOCTL);

	LeaveAllPowerSaveModeDirect(adapter);

	/* Config GPIO Mode */
	rtw_write8(adapter, REG_GPIO_PIN_CTRL + 3, rtw_read8(adapter, REG_GPIO_PIN_CTRL + 3) &~ BIT(gpio_num));

	/* Unregister GPIO interrupt handler*/
	adapter->gpiointpriv.callback[gpio_num] = NULL;

	/* Reset GPIO interrupt mode, 0:positive edge, 1:negative edge */
	adapter->gpiointpriv.interrupt_mode = rtw_read8(adapter, REG_GPIO_INTM) &~ BIT(gpio_num);
	rtw_write8(adapter, REG_GPIO_INTM, 0x00);

	/* Disable GPIO interrupt */
	adapter->gpiointpriv.interrupt_enable_mask = rtw_read8(adapter, REG_HSIMR + 2) &~ BIT(gpio_num);
	rtw_write8(adapter, REG_HSIMR + 2, adapter->gpiointpriv.interrupt_enable_mask);

	if(!adapter->gpiointpriv.interrupt_enable_mask)
		rtw_hal_update_hisr_hsisr_ind(adapter, 0);

	rtw_ps_deny_cancel(adapter, PS_DENY_IOCTL);

	return 0;
}
#endif

void rtw_dump_mac_rx_counters(_adapter* padapter,struct dbg_rx_counter *rx_counter)
{
	u32	mac_cck_ok=0, mac_ofdm_ok=0, mac_ht_ok=0, mac_vht_ok=0;
	u32	mac_cck_err=0, mac_ofdm_err=0, mac_ht_err=0, mac_vht_err=0;
	u32	mac_cck_fa=0, mac_ofdm_fa=0, mac_ht_fa=0;
	u32	DropPacket=0;

	if(!rx_counter) {
		rtw_warn_on(1);
		return;
	}

	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x3);
	mac_cck_ok	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x0);
	mac_ofdm_ok	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x6);
	mac_ht_ok	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	mac_vht_ok	= 0;
	if (IS_HARDWARE_TYPE_JAGUAR(padapter)) {
		PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x0);
		PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT26, 0x1);
		mac_vht_ok	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	}

	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x4);
	mac_cck_err	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x1);
	mac_ofdm_err	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x7);
	mac_ht_err	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	mac_vht_err	= 0;
	if (IS_HARDWARE_TYPE_JAGUAR(padapter)) {
		PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x1);
		PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT26, 0x1);
		mac_vht_err	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	}

	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x5);
	mac_cck_fa	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x2);
	mac_ofdm_fa	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT28|BIT29|BIT30|BIT31, 0x9);
	mac_ht_fa	= PHY_QueryMacReg(padapter, REG_RXERR_RPT, bMaskLWord);// [15:0]

	//Mac_DropPacket
	rtw_write32(padapter, REG_RXERR_RPT, (rtw_read32(padapter, REG_RXERR_RPT)& 0x0FFFFFFF)| Mac_DropPacket);
	DropPacket = rtw_read32(padapter, REG_RXERR_RPT)& 0x0000FFFF;

	rx_counter->rx_pkt_ok = mac_cck_ok+mac_ofdm_ok+mac_ht_ok+mac_vht_ok;
	rx_counter->rx_pkt_crc_error = mac_cck_err+mac_ofdm_err+mac_ht_err+mac_vht_err;
	rx_counter->rx_cck_fa = mac_cck_fa;
	rx_counter->rx_ofdm_fa = mac_ofdm_fa;
	rx_counter->rx_ht_fa = mac_ht_fa;
	rx_counter->rx_pkt_drop = DropPacket;
}
void rtw_reset_mac_rx_counters(_adapter* padapter)
{
	//reset mac counter
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT27, 0x1);
	PHY_SetMacReg(padapter, REG_RXERR_RPT, BIT27, 0x0);
}

void rtw_dump_phy_rx_counters(_adapter* padapter,struct dbg_rx_counter *rx_counter)
{
	u32 cckok=0,cckcrc=0,ofdmok=0,ofdmcrc=0,htok=0,htcrc=0,OFDM_FA=0,CCK_FA=0,vht_ok=0,vht_err=0;
	if(!rx_counter) {
		rtw_warn_on(1);
		return;
	}
	if (IS_HARDWARE_TYPE_JAGUAR(padapter)) {
		cckok	= PHY_QueryBBReg(padapter, 0xF04, 0x3FFF);	     // [13:0]
		ofdmok	= PHY_QueryBBReg(padapter, 0xF14, 0x3FFF);	     // [13:0]
		htok		= PHY_QueryBBReg(padapter, 0xF10, 0x3FFF);     // [13:0]
		vht_ok	= PHY_QueryBBReg(padapter, 0xF0C, 0x3FFF);     // [13:0]
		cckcrc	= PHY_QueryBBReg(padapter, 0xF04, 0x3FFF0000); // [29:16]
		ofdmcrc	= PHY_QueryBBReg(padapter, 0xF14, 0x3FFF0000); // [29:16]
		htcrc	= PHY_QueryBBReg(padapter, 0xF10, 0x3FFF0000); // [29:16]
		vht_err	= PHY_QueryBBReg(padapter, 0xF0C, 0x3FFF0000); // [29:16]
		CCK_FA	= PHY_QueryBBReg(padapter, 0xA5C, bMaskLWord);
		OFDM_FA	= PHY_QueryBBReg(padapter, 0xF48, bMaskLWord);
	} else {
		cckok	= PHY_QueryBBReg(padapter, 0xF88, bMaskDWord);
		ofdmok	= PHY_QueryBBReg(padapter, 0xF94, bMaskLWord);
		htok		= PHY_QueryBBReg(padapter, 0xF90, bMaskLWord);
		vht_ok	= 0;
		cckcrc	= PHY_QueryBBReg(padapter, 0xF84, bMaskDWord);
		ofdmcrc	= PHY_QueryBBReg(padapter, 0xF94, bMaskHWord);
		htcrc	= PHY_QueryBBReg(padapter, 0xF90, bMaskHWord);
		vht_err	= 0;
		OFDM_FA = PHY_QueryBBReg(padapter, 0xCF0, bMaskLWord) + PHY_QueryBBReg(padapter, 0xCF2, bMaskLWord) +
		          PHY_QueryBBReg(padapter, 0xDA2, bMaskLWord) + PHY_QueryBBReg(padapter, 0xDA4, bMaskLWord) +
		          PHY_QueryBBReg(padapter, 0xDA6, bMaskLWord) + PHY_QueryBBReg(padapter, 0xDA8, bMaskLWord);

		CCK_FA=(rtw_read8(padapter, 0xA5B )<<8 ) | (rtw_read8(padapter, 0xA5C));
	}

	rx_counter->rx_pkt_ok = cckok+ofdmok+htok+vht_ok;
	rx_counter->rx_pkt_crc_error = cckcrc+ofdmcrc+htcrc+vht_err;
	rx_counter->rx_ofdm_fa = OFDM_FA;
	rx_counter->rx_cck_fa = CCK_FA;

}

void rtw_reset_phy_rx_counters(_adapter* padapter)
{
	//reset phy counter
	if (IS_HARDWARE_TYPE_JAGUAR(padapter)) {
		PHY_SetBBReg(padapter, 0xB58, BIT0, 0x1);
		PHY_SetBBReg(padapter, 0xB58, BIT0, 0x0);

		PHY_SetBBReg(padapter, 0x9A4, BIT17, 0x1);//reset  OFDA FA counter
		PHY_SetBBReg(padapter, 0x9A4, BIT17, 0x0);

		PHY_SetBBReg(padapter, 0xA2C, BIT15, 0x0);//reset  CCK FA counter
		PHY_SetBBReg(padapter, 0xA2C, BIT15, 0x1);
	} else {
		PHY_SetBBReg(padapter, 0xF14, BIT16, 0x1);
		rtw_msleep_os(10);
		PHY_SetBBReg(padapter, 0xF14, BIT16, 0x0);

		PHY_SetBBReg(padapter, 0xD00, BIT27, 0x1);//reset  OFDA FA counter
		PHY_SetBBReg(padapter, 0xC0C, BIT31, 0x1);//reset  OFDA FA counter
		PHY_SetBBReg(padapter, 0xD00, BIT27, 0x0);
		PHY_SetBBReg(padapter, 0xC0C, BIT31, 0x0);

		PHY_SetBBReg(padapter, 0xA2C, BIT15, 0x0);//reset  CCK FA counter
		PHY_SetBBReg(padapter, 0xA2C, BIT15, 0x1);
	}
}
#ifdef DBG_RX_COUNTER_DUMP
void rtw_dump_drv_rx_counters(_adapter* padapter,struct dbg_rx_counter *rx_counter)
{
	struct recv_priv *precvpriv = &padapter->recvpriv;
	if(!rx_counter) {
		rtw_warn_on(1);
		return;
	}
	rx_counter->rx_pkt_ok = padapter->drv_rx_cnt_ok;
	rx_counter->rx_pkt_crc_error = padapter->drv_rx_cnt_crcerror;
	rx_counter->rx_pkt_drop = precvpriv->rx_drop - padapter->drv_rx_cnt_drop;
}
void rtw_reset_drv_rx_counters(_adapter* padapter)
{
	struct recv_priv *precvpriv = &padapter->recvpriv;
	padapter->drv_rx_cnt_ok = 0;
	padapter->drv_rx_cnt_crcerror = 0;
	padapter->drv_rx_cnt_drop = precvpriv->rx_drop;
}
void rtw_dump_phy_rxcnts_preprocess(_adapter* padapter,u8 rx_cnt_mode)
{
	u8 initialgain;
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(padapter);
	DM_ODM_T *odm = &(hal_data->odmpriv);
	DIG_T	*pDigTable = &odm->DM_DigTable;

	if((!(padapter->dump_rx_cnt_mode& DUMP_PHY_RX_COUNTER)) && (rx_cnt_mode & DUMP_PHY_RX_COUNTER)) {
		initialgain = pDigTable->CurIGValue;
		DBG_871X("%s CurIGValue:0x%02x\n",__FUNCTION__,initialgain);
		rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));
		//disable dynamic functions, such as high power, DIG
		Save_DM_Func_Flag(padapter);
		Switch_DM_Func(padapter, ~(DYNAMIC_BB_DIG|DYNAMIC_BB_BB_FA_CNT), _FALSE);
	} else if((padapter->dump_rx_cnt_mode& DUMP_PHY_RX_COUNTER) &&(!(rx_cnt_mode & DUMP_PHY_RX_COUNTER ))) {
		//turn on phy-dynamic functions
		Restore_DM_Func_Flag(padapter);
		initialgain = 0xff; //restore RX GAIN
		rtw_hal_set_hwreg(padapter, HW_VAR_INITIAL_GAIN, (u8 *)(&initialgain));

	}
}

void rtw_dump_rx_counters(_adapter* padapter)
{
	struct dbg_rx_counter rx_counter;

	if( padapter->dump_rx_cnt_mode & DUMP_DRV_RX_COUNTER ) {
		_rtw_memset(&rx_counter,0,sizeof(struct dbg_rx_counter));
		rtw_dump_drv_rx_counters(padapter,&rx_counter);
		DBG_871X( "Drv Received packet OK:%d CRC error:%d Drop Packets: %d\n",
		          rx_counter.rx_pkt_ok,rx_counter.rx_pkt_crc_error,rx_counter.rx_pkt_drop);
		rtw_reset_drv_rx_counters(padapter);
	}

	if( padapter->dump_rx_cnt_mode & DUMP_MAC_RX_COUNTER ) {
		_rtw_memset(&rx_counter,0,sizeof(struct dbg_rx_counter));
		rtw_dump_mac_rx_counters(padapter,&rx_counter);
		DBG_871X( "Mac Received packet OK:%d CRC error:%d FA Counter: %d Drop Packets: %d\n",
		          rx_counter.rx_pkt_ok,rx_counter.rx_pkt_crc_error,
		          rx_counter.rx_cck_fa+rx_counter.rx_ofdm_fa+rx_counter.rx_ht_fa,
		          rx_counter.rx_pkt_drop);
		rtw_reset_mac_rx_counters(padapter);
	}

	if(padapter->dump_rx_cnt_mode & DUMP_PHY_RX_COUNTER ) {
		_rtw_memset(&rx_counter,0,sizeof(struct dbg_rx_counter));
		rtw_dump_phy_rx_counters(padapter,&rx_counter);
		//DBG_871X("%s: OFDM_FA =%d\n", __FUNCTION__, rx_counter.rx_ofdm_fa);
		//DBG_871X("%s: CCK_FA =%d\n", __FUNCTION__, rx_counter.rx_cck_fa);
		DBG_871X("Phy Received packet OK:%d CRC error:%d FA Counter: %d\n",rx_counter.rx_pkt_ok,rx_counter.rx_pkt_crc_error,
		         rx_counter.rx_ofdm_fa+rx_counter.rx_cck_fa);
		rtw_reset_phy_rx_counters(padapter);
	}
}
#endif
void rtw_get_noise(_adapter* padapter)
{
#if defined(CONFIG_SIGNAL_DISPLAY_DBM) && defined(CONFIG_BACKGROUND_NOISE_MONITOR)
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct noise_info info;
	if(rtw_linked_check(padapter)) {
		info.bPauseDIG = _TRUE;
		info.IGIValue = 0x1e;
		info.max_time = 100;//ms
		info.chan = pmlmeext->cur_channel ;//rtw_get_oper_ch(padapter);
		rtw_ps_deny(padapter, PS_DENY_IOCTL);
		LeaveAllPowerSaveModeDirect(padapter);

		rtw_hal_set_odm_var(padapter, HAL_ODM_NOISE_MONITOR,&info, _FALSE);
		//ODM_InbandNoise_Monitor(podmpriv,_TRUE,0x20,100);
		rtw_ps_deny_cancel(padapter, PS_DENY_IOCTL);
		rtw_hal_get_odm_var(padapter, HAL_ODM_NOISE_MONITOR,&(info.chan), &(padapter->recvpriv.noise));
#ifdef DBG_NOISE_MONITOR
		DBG_871X("chan:%d,noise_level:%d\n",info.chan,padapter->recvpriv.noise);
#endif
	}
#endif

}

#ifdef CONFIG_FW_C2H_DEBUG

/*	C2H RX package original is 128.
if enable CONFIG_FW_C2H_DEBUG, it should increase to 256.
 C2H FW debug message:
 without aggregate:
 {C2H_CmdID,Seq,SubID,Len,Content[0~n]}
 Content[0~n]={'a','b','c',...,'z','\n'}
 with aggregate:
 {C2H_CmdID,Seq,SubID,Len,Content[0~n]}
 Content[0~n]={'a','b','c',...,'z','\n',Extend C2H pkt 2...}
 Extend C2H pkt 2={C2H CmdID,Seq,SubID,Len,Content = {'a','b','c',...,'z','\n'}}
 Author: Isaac	*/

void Debug_FwC2H(PADAPTER padapter, u8 *pdata, u8 len)
{
	int i = 0;
	int cnt = 0, total_length = 0;
	u8 buf[128] = {0};
	u8 more_data = _FALSE;
	u8 *nextdata = NULL;
	u8 test = 0;

	u8 data_len;
	u8 seq_no;

	nextdata = pdata;
	do {
		data_len = *(nextdata + 1);
		seq_no = *(nextdata + 2);

		for (i = 0 ; i < data_len - 2 ; i++) {
			cnt += sprintf((buf+cnt), "%c", nextdata[3 + i]);

			if (nextdata[3 + i] == 0x0a && nextdata[4 + i] == 0xff)
				more_data = _TRUE;
			else if (nextdata[3 + i] == 0x0a && nextdata[4 + i] != 0xff)
				more_data = _FALSE;
		}

		DBG_871X("[RTKFW, SEQ=%d]: %s", seq_no, buf);
		data_len += 3;
		total_length += data_len;

		if (more_data == _TRUE) {
			_rtw_memset(buf, '\0', 128);
			cnt = 0;
			nextdata = (pdata + total_length);
		}
	} while (more_data == _TRUE);
}
#endif /*CONFIG_FW_C2H_DEBUG*/
