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
#define _RTL8812A_CMD_C_

//#include <drv_types.h>
#include <rtl8812a_hal.h>
#include "hal_com_h2c.h"
#include <hal_com.h>
#define CONFIG_H2C_EF

#define RTL8812_MAX_H2C_BOX_NUMS	4
#define RTL8812_MAX_CMD_LEN	7
#define RTL8812_MESSAGE_BOX_SIZE		4
#define RTL8812_EX_MESSAGE_BOX_SIZE	4


static u8 _is_fw_read_cmd_down(_adapter* padapter, u8 msgbox_num)
{
	u8	read_down = _FALSE;
	int 	retry_cnts = 100;

	u8 valid;

	//DBG_8192C(" _is_fw_read_cmd_down ,reg_1cc(%x),msg_box(%d)...\n",rtw_read8(padapter,REG_HMETFR),msgbox_num);

	do {
		valid = rtw_read8(padapter,REG_HMETFR) & BIT(msgbox_num);
		if(0 == valid ) {
			read_down = _TRUE;
		} else
			rtw_msleep_os(1);
	} while( (!read_down) && (retry_cnts--));

	return read_down;

}


/*****************************************
* H2C Msg format :
* 0x1DF - 0x1D0
*| 31 - 8	| 7-5 	 4 - 0	|
*| h2c_msg 	|Class_ID CMD_ID	|
*
* Extend 0x1FF - 0x1F0
*|31 - 0	  |
*|ext_msg|
******************************************/
s32 FillH2CCmd_8812(PADAPTER padapter, u8 ElementID, u32 CmdLen, u8 *pCmdBuffer)
{
	u8 h2c_box_num;
	u32	msgbox_addr;
	u32 msgbox_ex_addr;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	u8 cmd_idx,ext_cmd_len;
	u32	h2c_cmd = 0;
	u32	h2c_cmd_ex = 0;
	s32 ret = _FAIL;

	_func_enter_;

	padapter = GET_PRIMARY_ADAPTER(padapter);
	pHalData = GET_HAL_DATA(padapter);


	if(padapter->bFWReady == _FALSE) {
		return ret;
	}

	_enter_critical_mutex(&(adapter_to_dvobj(padapter)->h2c_fwcmd_mutex), NULL);


	if (!pCmdBuffer) {
		goto exit;
	}
	if (CmdLen > RTL8812_MAX_CMD_LEN) {
		goto exit;
	}
	if (padapter->bSurpriseRemoved == _TRUE)
		goto exit;

	//pay attention to if  race condition happened in  H2C cmd setting.
	do {
		h2c_box_num = pHalData->LastHMEBoxNum;

		if(!_is_fw_read_cmd_down(padapter, h2c_box_num)) {
			DBG_8192C(" fw read cmd failed...\n");
			goto exit;
		}

		*(u8*)(&h2c_cmd) = ElementID;

		if(CmdLen<=3) {
			_rtw_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer, CmdLen );
		} else {
			_rtw_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer,3);
			ext_cmd_len = CmdLen-3;
			_rtw_memcpy((u8*)(&h2c_cmd_ex), pCmdBuffer+3,ext_cmd_len );

			//Write Ext command
			msgbox_ex_addr = REG_HMEBOX_EXT0_8812 + (h2c_box_num *RTL8812_EX_MESSAGE_BOX_SIZE);
#ifdef CONFIG_H2C_EF
			for(cmd_idx=0; cmd_idx<ext_cmd_len; cmd_idx++ ) {
				rtw_write8(padapter,msgbox_ex_addr+cmd_idx,*((u8*)(&h2c_cmd_ex)+cmd_idx));
			}
#else
			h2c_cmd_ex = le32_to_cpu( h2c_cmd_ex );
			rtw_write32(padapter, msgbox_ex_addr, h2c_cmd_ex);
#endif
		}
		// Write command
		msgbox_addr =REG_HMEBOX_0 + (h2c_box_num *RTL8812_MESSAGE_BOX_SIZE);
#ifdef CONFIG_H2C_EF
		for(cmd_idx=0; cmd_idx<RTL8812_MESSAGE_BOX_SIZE; cmd_idx++ ) {
			rtw_write8(padapter,msgbox_addr+cmd_idx,*((u8*)(&h2c_cmd)+cmd_idx));
		}
#else
		h2c_cmd = le32_to_cpu( h2c_cmd );
		rtw_write32(padapter,msgbox_addr, h2c_cmd);
#endif

		pHalData->LastHMEBoxNum = (h2c_box_num+1) % RTL8812_MAX_H2C_BOX_NUMS;

	} while(0);

	ret = _SUCCESS;

exit:

	_exit_critical_mutex(&(adapter_to_dvobj(padapter)->h2c_fwcmd_mutex), NULL);

	_func_exit_;

	return ret;
}

u8 rtl8812_set_rssi_cmd(_adapter*padapter, u8 *param)
{
	u8	res=_SUCCESS;
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	_func_enter_;

	*((u32*) param ) = cpu_to_le32( *((u32*) param ) );

	FillH2CCmd_8812(padapter, H2C_8812_RSSI_REPORT, 4, param);

	_func_exit_;

	return res;
}

u8	Get_VHT_ENI(
    u32		IOTAction,
    u32		WirelessMode,
    u32		ratr_bitmap
)
{
	u8	Ret = 0;

	if(WirelessMode == WIRELESS_11_24AC) {
		if(ratr_bitmap & 0xfff00000)	// Mix , 2SS
			Ret = 3;
		else 					// Mix, 1SS
			Ret = 2;
	} else if(WirelessMode == WIRELESS_11_5AC) {
		Ret = 1;					// VHT
	}

	return (Ret << 4);
}

BOOLEAN
Get_RA_ShortGI_8812(
    PADAPTER			Adapter,
    struct sta_info		*psta,
    u8					shortGIrate,
    u32					ratr_bitmap
)
{
	BOOLEAN		bShortGI;
	struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	bShortGI = shortGIrate;

#ifdef CONFIG_80211AC_VHT
	if(	bShortGI &&
	    IsSupportedVHT(psta->wireless_mode) &&
	    (pmlmeinfo->assoc_AP_vendor == HT_IOT_PEER_REALTEK_JAGUAR_CCUTAP) &&
	    TEST_FLAG(psta->vhtpriv.ldpc_cap, LDPC_VHT_ENABLE_TX)
	  ) {
		if(ratr_bitmap & 0xC0000000)
			bShortGI = _FALSE;
	}
#endif

	return bShortGI;
}


void
Set_RA_LDPC_8812(
    struct sta_info	*psta,
    BOOLEAN			bLDPC
)
{
	if(psta == NULL)
		return;
#ifdef CONFIG_80211AC_VHT
	if(psta->wireless_mode == WIRELESS_11_5AC) {
		if(bLDPC && TEST_FLAG(psta->vhtpriv.ldpc_cap, LDPC_VHT_CAP_TX))
			SET_FLAG(psta->vhtpriv.ldpc_cap, LDPC_VHT_ENABLE_TX);
		else
			CLEAR_FLAG(psta->vhtpriv.ldpc_cap, LDPC_VHT_ENABLE_TX);
	} else if(IsSupportedHT(psta->wireless_mode) || IsSupportedVHT(psta->wireless_mode)) {
		if(bLDPC && TEST_FLAG(psta->htpriv.ldpc_cap, LDPC_HT_CAP_TX))
			SET_FLAG(psta->htpriv.ldpc_cap, LDPC_HT_ENABLE_TX);
		else
			CLEAR_FLAG(psta->htpriv.ldpc_cap, LDPC_HT_ENABLE_TX);
	}

	update_ldpc_stbc_cap(psta);
#endif
}


u8
Get_RA_LDPC_8812(
    struct sta_info		*psta
)
{
	u8	bLDPC = 0;

	if (psta != NULL) {
		if(psta->mac_id == 1) {
			bLDPC = 0;
		} else {
#ifdef CONFIG_80211AC_VHT
			if(IsSupportedVHT(psta->wireless_mode)) {
				if(TEST_FLAG(psta->vhtpriv.ldpc_cap, LDPC_VHT_CAP_TX))
					bLDPC = 1;
				else
					bLDPC = 0;
			} else if(IsSupportedHT(psta->wireless_mode)) {
				if(TEST_FLAG(psta->htpriv.ldpc_cap, LDPC_HT_CAP_TX))
					bLDPC =1;
				else
					bLDPC =0;
			} else
#endif
				bLDPC = 0;
		}
	}

	return (bLDPC << 2);
}

void rtl8812_set_raid_cmd(PADAPTER padapter, u32 bitmap, const u8* arg)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct sta_info	*psta;
	u8 macid, init_rate, raid, shortGIrate=_FALSE;

	_func_enter_;

	macid = arg[0];
	raid = arg[1];
	shortGIrate = arg[2];
	init_rate = arg[3];

	psta = pmlmeinfo->FW_sta_info[macid].psta;
	if(psta == NULL) {
		return;
	}

	if(pHalData->fw_ractrl == _TRUE) {
		u8	H2CCommand[7] = {0};

		shortGIrate = Get_RA_ShortGI_8812(padapter, psta, shortGIrate, bitmap);

		H2CCommand[0] = macid;
		H2CCommand[1] = (raid & 0x1F) | (shortGIrate?0x80:0x00) ;
		H2CCommand[2] = (psta->bw_mode & 0x3) |Get_RA_LDPC_8812(psta) |Get_VHT_ENI(0, psta->wireless_mode, bitmap);

		//DisableTXPowerTraining
		if(pHalData->bDisableTXPowerTraining) {
			H2CCommand[2] |= BIT6;
			DBG_871X("%s,Disable PWT by driver\n",__FUNCTION__);
		} else {
			PDM_ODM_T	pDM_OutSrc = &pHalData->odmpriv;

			if(pDM_OutSrc->bDisablePowerTraining) {
				H2CCommand[2] |= BIT6;
				DBG_871X("%s,Disable PWT by DM\n",__FUNCTION__);
			}
		}

		H2CCommand[3] = (u8)(bitmap & 0x000000ff);
		H2CCommand[4] = (u8)((bitmap & 0x0000ff00) >>8);
		H2CCommand[5] = (u8)((bitmap & 0x00ff0000) >> 16);
		H2CCommand[6] = (u8)((bitmap & 0xff000000) >> 24);

		//DBG_871X("rtl8812_set_raid_cmd, bitmap=0x%x, mac_id=0x%x, raid=0x%x, shortGIrate=%x\n", bitmap, macid, raid, shortGIrate);

		FillH2CCmd_8812(padapter, H2C_8812_RA_MASK, 7, H2CCommand);
	}

	if (shortGIrate==_TRUE)
		init_rate |= BIT(7);

	pdmpriv->INIDATA_RATE[macid] = init_rate;

	_func_exit_;

}

void rtl8812_Add_RateATid(PADAPTER pAdapter, u32 bitmap, const u8* arg, u8 rssi_level)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u8	macid;

	macid = arg[0];

	if(rssi_level != DM_RATR_STA_INIT)
		bitmap = ODM_Get_Rate_Bitmap(&pHalData->odmpriv, macid, bitmap, rssi_level);

	rtl8812_set_raid_cmd(pAdapter, bitmap, arg);
}

void rtl8812_set_FwPwrMode_cmd(PADAPTER padapter, u8 PSMode)
{
	u8	u1H2CSetPwrMode[H2C_PWRMODE_LEN]= {0};
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
	u8	Mode = 0, RLBM = 0, PowerState = 0, LPSAwakeIntvl = 2, pwrModeByte5 = 0;
	_func_enter_;

	DBG_871X("%s: Mode=%d SmartPS=%d UAPSD=%d\n", __FUNCTION__,
	         PSMode, pwrpriv->smart_ps, padapter->registrypriv.uapsd_enable);

	switch(PSMode) {
	case PS_MODE_ACTIVE:
		Mode = 0;
		break;
	case PS_MODE_MIN:
		Mode = 1;
		break;
	case PS_MODE_MAX:
		RLBM = 1;
		Mode = 1;
		break;
	case PS_MODE_DTIM:
		RLBM = 2;
		Mode = 1;
		break;
	case PS_MODE_UAPSD_WMM:
		Mode = 2;
		break;
	default:
		Mode = 0;
		break;
	}

	if (Mode > PS_MODE_ACTIVE) {
		{
			PowerState = 0x00;// AllON(0x0C), RFON(0x04), RFOFF(0x00)
			pwrModeByte5 = 0x40;
		}

	} else {
		PowerState = 0x0C;// AllON(0x0C), RFON(0x04), RFOFF(0x00)
		pwrModeByte5 = 0x40;
	}

	// 0: Active, 1: LPS, 2: WMMPS
	SET_8812_H2CCMD_PWRMODE_PARM_MODE(u1H2CSetPwrMode, Mode);

	// 0:Min, 1:Max , 2:User define
	SET_8812_H2CCMD_PWRMODE_PARM_RLBM(u1H2CSetPwrMode, RLBM);

	// (LPS) smart_ps:  0: PS_Poll, 1: PS_Poll , 2: NullData
	// (WMM)smart_ps: 0:PS_Poll, 1:NullData
	SET_8812_H2CCMD_PWRMODE_PARM_SMART_PS(u1H2CSetPwrMode, pwrpriv->smart_ps);

	// AwakeInterval: Unit is beacon interval, this field is only valid in PS_DTIM mode
	SET_8812_H2CCMD_PWRMODE_PARM_BCN_PASS_TIME(u1H2CSetPwrMode, LPSAwakeIntvl);

	// (WMM only)bAllQueueUAPSD
	SET_8812_H2CCMD_PWRMODE_PARM_ALL_QUEUE_UAPSD(u1H2CSetPwrMode, padapter->registrypriv.uapsd_enable);

	// AllON(0x0C), RFON(0x04), RFOFF(0x00)
	SET_8812_H2CCMD_PWRMODE_PARM_PWR_STATE(u1H2CSetPwrMode, PowerState);

	SET_8812_H2CCMD_PWRMODE_PARM_BYTE5(u1H2CSetPwrMode, pwrModeByte5);

	//DBG_871X("u1H2CSetPwrMode="MAC_FMT"\n", MAC_ARG(u1H2CSetPwrMode));
	FillH2CCmd_8812(padapter, H2C_8812_SETPWRMODE, sizeof(u1H2CSetPwrMode), u1H2CSetPwrMode);

	_func_exit_;
}

void rtl8812_set_FwMediaStatus_cmd(PADAPTER padapter, u16 mstatus_rpt )
{
	u8	u1JoinBssRptParm[3]= {0};
	u8	mstatus, macId, macId_Ind = 0, macId_End = 0;

	mstatus = (u8) (mstatus_rpt & 0xFF);
	macId = (u8)(mstatus_rpt >> 8)  ;

	SET_8812_H2CCMD_MSRRPT_PARM_OPMODE(u1JoinBssRptParm, mstatus);
	SET_8812_H2CCMD_MSRRPT_PARM_MACID_IND(u1JoinBssRptParm, macId_Ind);

	SET_8812_H2CCMD_MSRRPT_PARM_MACID(u1JoinBssRptParm, macId);
	SET_8812_H2CCMD_MSRRPT_PARM_MACID_END(u1JoinBssRptParm, macId_End);

	DBG_871X("[MacId],  Set MacId Ctrl(original) = 0x%x \n", u1JoinBssRptParm[0]<<16|u1JoinBssRptParm[1]<<8|u1JoinBssRptParm[2]);

	FillH2CCmd_8812(padapter, H2C_8812_MSRRPT, 3, u1JoinBssRptParm);
}

void ConstructBeacon(_adapter *padapter, u8 *pframe, u32 *pLength)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					rate_len, pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


	//DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	//pmlmeext->mgnt_seq++;
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

	//DBG_871X("%s bcn_sz=%d\n", __FUNCTION__, pktlen);

}

void ConstructPSPoll(_adapter *padapter, u8 *pframe, u32 *pLength)
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

void ConstructNullFunctionData(
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

void ConstructProbeRsp(_adapter *padapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bHideSSID)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u8					*mac, *bssid;
	u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);


	//DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = myid(&(padapter->eeprompriv));
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, 0);
	SetFrameSubType(fctrl, WIFI_PROBERSP);

	pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	pframe += pktlen;

	if(cur_network->IELength>MAX_IE_SZ)
		return;

	_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
	pframe += cur_network->IELength;
	pktlen += cur_network->IELength;

	*pLength = pktlen;
}

#ifdef CONFIG_GTK_OL
static void ConstructGTKResponse(
    PADAPTER padapter,
    u8			*pframe,
    u32			*pLength
)
{
	struct rtw_ieee80211_hdr	*pwlanhdr;
	u16						*fctrl;
	u32						pktlen;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct wlan_network		*cur_network = &pmlmepriv->cur_network;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct security_priv *psecuritypriv = &padapter->securitypriv;
	static u8			LLCHeader[8] = {0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x88, 0x8E};
	static u8			GTKbody_a[11] = {0x01, 0x03, 0x00, 0x5F, 0x02, 0x03, 0x12, 0x00, 0x10, 0x42, 0x0B};
	u8				*pGTKRspPkt = pframe;
	u8			EncryptionHeadOverhead = 0;
	//DBG_871X("%s:%d\n", __FUNCTION__, bForcePowerSave);

	pwlanhdr = (struct rtw_ieee80211_hdr*)pframe;

	fctrl = &pwlanhdr->frame_ctl;
	*(fctrl) = 0;

	//-------------------------------------------------------------------------
	// MAC Header.
	//-------------------------------------------------------------------------
	SetFrameType(fctrl, WIFI_DATA);
	//SetFrameSubType(fctrl, 0);
	SetToDs(fctrl);
	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0);
	SetDuration(pwlanhdr, 0);

	*pLength = 24;

	//-------------------------------------------------------------------------
	// Security Header: leave space for it if necessary.
	//-------------------------------------------------------------------------

	switch (psecuritypriv->dot11PrivacyAlgrthm) {
	case _WEP40_:
	case _WEP104_:
		EncryptionHeadOverhead = 4;
		break;
	case _TKIP_:
		EncryptionHeadOverhead = 8;
		break;
	case _AES_:
		EncryptionHeadOverhead = 8;
		break;
	default:
		EncryptionHeadOverhead = 0;
	}

	if(EncryptionHeadOverhead > 0) {
		_rtw_memset(&(pframe[*pLength]), 0,EncryptionHeadOverhead);
		*pLength += EncryptionHeadOverhead;
		//SET_80211_HDR_WEP(pGTKRspPkt, 1);  //Suggested by CCW.
		//GTK's privacy bit is done by FW
		//SetPrivacy(fctrl);
	}
	//-------------------------------------------------------------------------
	// Frame Body.
	//-------------------------------------------------------------------------
	pGTKRspPkt =  (u8*)(pframe+ *pLength);
	// LLC header
	_rtw_memcpy(pGTKRspPkt, LLCHeader, 8);
	*pLength += 8;

	// GTK element
	pGTKRspPkt += 8;

	//GTK frame body after LLC, part 1
	_rtw_memcpy(pGTKRspPkt, GTKbody_a, 11);
	*pLength += 11;
	pGTKRspPkt += 11;
	//GTK frame body after LLC, part 2
	_rtw_memset(&(pframe[*pLength]), 0, 88);
	*pLength += 88;
	pGTKRspPkt += 88;

}
#endif //CONFIG_GTK_OL

// To check if reserved page content is destroyed by beacon beacuse beacon is too large.
// 2010.06.23. Added by tynli.
VOID
CheckFwRsvdPageContent(
    IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);

	if(pHalData->FwRsvdPageStartOffset != 0) {}
}

//
// Description: Get the reserved page number in Tx packet buffer.
// Retrun value: the page number.
// 2012.08.09, by tynli.
//
u8
GetTxBufferRsvdPageNum8812(_adapter *Adapter, bool bWoWLANBoundary)
{
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	RsvdPageNum=0;
	u8	TxPageBndy= LAST_ENTRY_OF_TX_PKT_BUFFER_8812; // default reseved 1 page for the IC type which is undefined.

	if(bWoWLANBoundary) {
		rtw_hal_get_def_var(Adapter, HAL_DEF_TX_PAGE_BOUNDARY_WOWLAN, (u8 *)&TxPageBndy);
	} else {
		rtw_hal_get_def_var(Adapter, HAL_DEF_TX_PAGE_BOUNDARY, (u8 *)&TxPageBndy);
	}

	RsvdPageNum = LAST_ENTRY_OF_TX_PKT_BUFFER_8812 -TxPageBndy + 1;

	return RsvdPageNum;
}

//
// Description: Fill the reserved packets that FW will use to RSVD page.
//			Now we just send 4 types packet to rsvd page.
//			(1)Beacon, (2)Ps-poll, (3)Null data, (4)ProbeRsp.
//	Input:
//	    bDLFinished - FALSE: At the first time we will send all the packets as a large packet to Hw,
//				 		so we need to set the packet length to total lengh.
//			      TRUE: At the second time, we should send the first packet (default:beacon)
//						to Hw again and set the lengh in descriptor to the real beacon lengh.
// 2009.10.15 by tynli.
static inline void SetFwRsvdPagePkt_8812(PADAPTER padapter, BOOLEAN bDLFinished)
{
	PHAL_DATA_TYPE pHalData;
	struct xmit_frame	*pcmdframe;
	struct pkt_attrib	*pattrib;
	struct xmit_priv	*pxmitpriv;
	struct mlme_ext_priv	*pmlmeext;
	struct mlme_ext_info	*pmlmeinfo;
	u32	PSPollLength, NullFunctionDataLength, QosNullLength;
	u32	BcnLen;
	u8	TotalPageNum=0, CurtPktPageNum=0, TxDescLen=0, RsvdPageNum=0;
	u8	*ReservedPagePacket;
	u8	RsvdPageLoc[5] = {0};
	u16	BufIndex=0, PageSize = 256;
	u32	TotalPacketLen, MaxRsvdPageBufSize=0;;


	//DBG_871X("%s\n", __FUNCTION__);

	pHalData = GET_HAL_DATA(padapter);
	pxmitpriv = &padapter->xmitpriv;
	pmlmeext = &padapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;

	if(IS_HARDWARE_TYPE_8812(padapter))
		PageSize = 512;
	else if (IS_HARDWARE_TYPE_8821(padapter))
		PageSize = PAGE_SIZE_TX_8821A;

	// <tynli_note> The function SetFwRsvdPagePkt_8812() input must be added a value "bDLWholePackets" to
	// decide if download wowlan packets, and use "bDLWholePackets" to be GetTxBufferRsvdPageNum8812() 2nd input value.
	RsvdPageNum = GetTxBufferRsvdPageNum8812(padapter, _FALSE);
	MaxRsvdPageBufSize = RsvdPageNum*PageSize;

	pcmdframe = rtw_alloc_cmdxmitframe(pxmitpriv);
	if (pcmdframe == NULL) {
		return;
	}

	ReservedPagePacket = pcmdframe->buf_addr;

	TxDescLen = TXDESC_SIZE;//The desc lengh in Tx packet buffer of 8812A is 40 bytes.

	//(1) beacon
	BufIndex = TXDESC_OFFSET;
	ConstructBeacon(padapter, &ReservedPagePacket[BufIndex], &BcnLen);

	// When we count the first page size, we need to reserve description size for the RSVD
	// packet, it will be filled in front of the packet in TXPKTBUF.
	CurtPktPageNum = (u8)PageNum(BcnLen+TxDescLen, PageSize);

	if(bDLFinished) {
		TotalPageNum += CurtPktPageNum;
		TotalPacketLen = (TotalPageNum*PageSize);
		DBG_871X("%s(): Beacon page size = %d\n",__FUNCTION__,TotalPageNum);
	} else {
		TotalPageNum += CurtPktPageNum;

		pHalData->FwRsvdPageStartOffset = TotalPageNum;

		BufIndex += (CurtPktPageNum*PageSize);

		if(BufIndex > MaxRsvdPageBufSize) {
			DBG_871X("%s(): Beacon: The rsvd page size is not enough!!BufIndex %d, MaxRsvdPageBufSize %d\n",__FUNCTION__,
			         BufIndex,MaxRsvdPageBufSize);
			goto error;
		}

		//(2) ps-poll
		ConstructPSPoll(padapter, &ReservedPagePacket[BufIndex], &PSPollLength);
		rtl8812a_fill_fake_txdesc(padapter, &ReservedPagePacket[BufIndex-TxDescLen], PSPollLength, _TRUE, _FALSE, _FALSE);

		SET_8812_H2CCMD_RSVDPAGE_LOC_PSPOLL(RsvdPageLoc, TotalPageNum);

		CurtPktPageNum = (u8)PageNum(PSPollLength+TxDescLen, PageSize);

		BufIndex += (CurtPktPageNum*PageSize);

		TotalPageNum += CurtPktPageNum;

		if(BufIndex > MaxRsvdPageBufSize) {
			DBG_871X("%s(): ps-poll: The rsvd page size is not enough!!BufIndex %d, MaxRsvdPageBufSize %d\n",__FUNCTION__,
			         BufIndex,MaxRsvdPageBufSize);
			goto error;
		}

		//(3) null data
		ConstructNullFunctionData(
		    padapter,
		    &ReservedPagePacket[BufIndex],
		    &NullFunctionDataLength,
		    get_my_bssid(&pmlmeinfo->network),
		    _FALSE, 0, 0, _FALSE);
		rtl8812a_fill_fake_txdesc(padapter, &ReservedPagePacket[BufIndex-TxDescLen], NullFunctionDataLength, _FALSE, _FALSE, _FALSE);

		SET_8812_H2CCMD_RSVDPAGE_LOC_NULL_DATA(RsvdPageLoc, TotalPageNum);

		CurtPktPageNum = (u8)PageNum(NullFunctionDataLength+TxDescLen, PageSize);

		BufIndex += (CurtPktPageNum*PageSize);

		TotalPageNum += CurtPktPageNum;

		if(BufIndex > MaxRsvdPageBufSize) {
			DBG_871X("%s(): Null-data: The rsvd page size is not enough!!BufIndex %d, MaxRsvdPageBufSize %d\n",__FUNCTION__,
			         BufIndex,MaxRsvdPageBufSize);
			goto error;
		}

		//(5) Qos null data
		ConstructNullFunctionData(
		    padapter,
		    &ReservedPagePacket[BufIndex],
		    &QosNullLength,
		    get_my_bssid(&pmlmeinfo->network),
		    _TRUE, 0, 0, _FALSE);
		rtl8812a_fill_fake_txdesc(padapter, &ReservedPagePacket[BufIndex-TxDescLen], QosNullLength, _FALSE, _FALSE, _FALSE);

		SET_8812_H2CCMD_RSVDPAGE_LOC_QOS_NULL_DATA(RsvdPageLoc, TotalPageNum);

		CurtPktPageNum = (u8)PageNum(QosNullLength+TxDescLen, PageSize);

		BufIndex += (CurtPktPageNum*PageSize);

		TotalPageNum += CurtPktPageNum;

		TotalPacketLen = (TotalPageNum * PageSize);
	}


	if(TotalPacketLen > MaxRsvdPageBufSize) {
		DBG_871X("%s(): ERROR: The rsvd page size is not enough!!TotalPacketLen %d, MaxRsvdPageBufSize %d\n",__FUNCTION__,
		         TotalPacketLen,MaxRsvdPageBufSize);
		goto error;
	} else {
		// update attribute
		pattrib = &pcmdframe->attrib;
		update_mgntframe_attrib(padapter, pattrib);
		pattrib->qsel = QSLT_BEACON;
		pattrib->pktlen = pattrib->last_txcmdsz = TotalPacketLen - TxDescLen;
		dump_mgntframe_and_wait(padapter, pcmdframe, 100);
	}

	if(!bDLFinished) {
		DBG_871X("%s: Set RSVD page location to Fw ,TotalPacketLen(%d), TotalPageNum(%d)\n", __FUNCTION__,TotalPacketLen,TotalPageNum);
		FillH2CCmd_8812(padapter, H2C_8812_RSVDPAGE, 5, RsvdPageLoc);
	}

	return;

error:
	rtw_free_xmitframe(pxmitpriv, pcmdframe);
}

void rtl8812_set_FwJoinBssReport_cmd(PADAPTER padapter, u8 mstatus)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	BOOLEAN		bSendBeacon=_FALSE;
	BOOLEAN		bcn_valid = _FALSE;
	u8	DLBcnCount=0;
	u32 poll = 0;

	_func_enter_;

	DBG_871X("%s mstatus(%x)\n", __FUNCTION__,mstatus);

	if(mstatus == 1) {
		// We should set AID, correct TSF, HW seq enable before set JoinBssReport to Fw in 88/92C.
		// Suggested by filen. Added by tynli.
		rtw_write16(padapter, REG_BCN_PSR_RPT, (0xC000|pmlmeinfo->aid));
		// Do not set TSF again here or vWiFi beacon DMA INT will not work.
		//correct_TSF(padapter, pmlmeext);
		// Hw sequende enable by dedault. 2010.06.23. by tynli.

		//Set REG_CR bit 8. DMA beacon by SW.
		pHalData->RegCR_1 |= BIT0;
		rtw_write8(padapter,  REG_CR+1, pHalData->RegCR_1);

		// Disable Hw protection for a time which revserd for Hw sending beacon.
		// Fix download reserved page packet fail that access collision with the protection time.
		// 2010.05.11. Added by tynli.
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~BIT(3)));
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(4));

		if(pHalData->RegFwHwTxQCtrl&BIT6) {
			DBG_871X("HalDownloadRSVDPage(): There is an Adapter is sending beacon.\n");
			bSendBeacon = _TRUE;
		}

		// Set FWHW_TXQ_CTRL 0x422[6]=0 to tell Hw the packet is not a real beacon frame.
		rtw_write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl&(~BIT6)));
		pHalData->RegFwHwTxQCtrl &= (~BIT6);

		// Clear beacon valid check bit.
		rtw_hal_set_hwreg(padapter, HW_VAR_BCN_VALID, NULL);
		DLBcnCount = 0;
		poll = 0;
		do {
			// download rsvd page.
			rtw_hal_set_fw_rsvd_page(padapter, _FALSE);
			DLBcnCount++;
			do {
				rtw_yield_os();
				//rtw_mdelay_os(10);
				// check rsvd page download OK.
				rtw_hal_get_hwreg(padapter, HW_VAR_BCN_VALID, (u8*)(&bcn_valid));
				poll++;
			} while(!bcn_valid && (poll%10)!=0 && !padapter->bSurpriseRemoved && !padapter->bDriverStopped);

		} while(!bcn_valid && DLBcnCount<=100 && !padapter->bSurpriseRemoved && !padapter->bDriverStopped);

		//RT_ASSERT(bcn_valid, ("HalDownloadRSVDPage88ES(): 1 Download RSVD page failed!\n"));
		if(padapter->bSurpriseRemoved || padapter->bDriverStopped) {
		} else if(!bcn_valid)
			DBG_871X(ADPT_FMT": 1 DL RSVD page failed! DLBcnCount:%u, poll:%u\n",
			         ADPT_ARG(padapter) ,DLBcnCount, poll);
		else {
			struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(padapter);
			pwrctl->fw_psmode_iface_id = padapter->iface_id;
			DBG_871X(ADPT_FMT": 1 DL RSVD page success! DLBcnCount:%u, poll:%u\n",
			         ADPT_ARG(padapter), DLBcnCount, poll);
		}
		// Enable Bcn
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)|BIT(3));
		rtw_write8(padapter, REG_BCN_CTRL, rtw_read8(padapter, REG_BCN_CTRL)&(~BIT(4)));

		// To make sure that if there exists an adapter which would like to send beacon.
		// If exists, the origianl value of 0x422[6] will be 1, we should check this to
		// prevent from setting 0x422[6] to 0 after download reserved page, or it will cause
		// the beacon cannot be sent by HW.
		// 2010.06.23. Added by tynli.
		if(bSendBeacon) {
			rtw_write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl|BIT6));
			pHalData->RegFwHwTxQCtrl |= BIT6;
		}

		//
		// Update RSVD page location H2C to Fw.
		//
		if(bcn_valid) {
			rtw_hal_set_hwreg(padapter, HW_VAR_BCN_VALID, NULL);
			DBG_871X("Set RSVD page location to Fw.\n");
		}

		// Do not enable HW DMA BCN or it will cause Pcie interface hang by timing issue. 2011.11.24. by tynli.
		//if(!padapter->bEnterPnpSleep)
		{
			// Clear CR[8] or beacon packet will not be send to TxBuf anymore.
			pHalData->RegCR_1 &= (~BIT0);
			rtw_write8(padapter,  REG_CR+1, pHalData->RegCR_1);
		}
	}
	_func_exit_;
}

#ifdef CONFIG_P2P_PS
void rtl8812_set_p2p_ps_offload_cmd(_adapter* padapter, u8 p2p_ps_state)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct wifidirect_info	*pwdinfo = &( padapter->wdinfo );
	u8	*p2p_ps_offload = (u8 *)&pHalData->p2p_ps_offload;
	u8	i;

	_func_enter_;

	switch(p2p_ps_state) {
	case P2P_PS_DISABLE:
		DBG_8192C("P2P_PS_DISABLE \n");
		_rtw_memset(p2p_ps_offload, 0, 1);
		break;
	case P2P_PS_ENABLE:
		DBG_8192C("P2P_PS_ENABLE \n");
		// update CTWindow value.
		if( pwdinfo->ctwindow > 0 ) {
			SET_8812_H2CCMD_P2P_PS_OFFLOAD_CTWINDOW_EN(p2p_ps_offload, 1);
			rtw_write8(padapter, REG_P2P_CTWIN, pwdinfo->ctwindow);
		}

		// hw only support 2 set of NoA
		for( i=0 ; i<pwdinfo->noa_num ; i++) {
			// To control the register setting for which NOA
			rtw_write8(padapter, REG_NOA_DESC_SEL, (i << 4));
			if(i == 0) {
				SET_8812_H2CCMD_P2P_PS_OFFLOAD_NOA0_EN(p2p_ps_offload, 1);
			} else {
				SET_8812_H2CCMD_P2P_PS_OFFLOAD_NOA1_EN(p2p_ps_offload, 1);
			}

			// config P2P NoA Descriptor Register
			rtw_write32(padapter, REG_NOA_DESC_DURATION, pwdinfo->noa_duration[i]);
			rtw_write32(padapter, REG_NOA_DESC_INTERVAL, pwdinfo->noa_interval[i]);
			rtw_write32(padapter, REG_NOA_DESC_START, pwdinfo->noa_start_time[i]);
			rtw_write8(padapter, REG_NOA_DESC_COUNT, pwdinfo->noa_count[i]);
		}

		if( (pwdinfo->opp_ps == 1) || (pwdinfo->noa_num > 0) ) {
			// rst p2p circuit
			rtw_write8(padapter, REG_DUAL_TSF_RST, BIT(4));

			SET_8812_H2CCMD_P2P_PS_OFFLOAD_ENABLE(p2p_ps_offload, 1);

			if(pwdinfo->role == P2P_ROLE_GO) {
				// 1: Owner, 0: Client
				SET_8812_H2CCMD_P2P_PS_OFFLOAD_ROLE(p2p_ps_offload, 1);
				SET_8812_H2CCMD_P2P_PS_OFFLOAD_ALLSTASLEEP(p2p_ps_offload, 0);
			} else {
				// 1: Owner, 0: Client
				SET_8812_H2CCMD_P2P_PS_OFFLOAD_ROLE(p2p_ps_offload, 0);
			}

			SET_8812_H2CCMD_P2P_PS_OFFLOAD_DISCOVERY(p2p_ps_offload, 0);
		}
		break;
	case P2P_PS_SCAN:
		DBG_8192C("P2P_PS_SCAN \n");
		SET_8812_H2CCMD_P2P_PS_OFFLOAD_DISCOVERY(p2p_ps_offload, 1);
		break;
	case P2P_PS_SCAN_DONE:
		DBG_8192C("P2P_PS_SCAN_DONE \n");
		SET_8812_H2CCMD_P2P_PS_OFFLOAD_DISCOVERY(p2p_ps_offload, 0);
		pwdinfo->p2p_ps_state = P2P_PS_ENABLE;
		break;
	default:
		break;
	}

	DBG_871X("P2P_PS_OFFLOAD : %x\n", p2p_ps_offload[0]);
	FillH2CCmd_8812(padapter, H2C_8812_P2P_PS_OFFLOAD, 1, p2p_ps_offload);

	_func_exit_;

}
#endif //CONFIG_P2P


static inline void rtl8812_set_FwRsvdPage_cmd(PADAPTER padapter, PRSVDPAGE_LOC rsvdpageloc)
{
	u8 u1H2CRsvdPageParm[H2C_RSVDPAGE_LOC_LEN]= {0};

	DBG_871X("8812au/8821/8811 RsvdPageLoc: ProbeRsp=%d PsPoll=%d Null=%d QoSNull=%d BTNull=%d\n",
	         rsvdpageloc->LocProbeRsp, rsvdpageloc->LocPsPoll,
	         rsvdpageloc->LocNullData, rsvdpageloc->LocQosNull,
	         rsvdpageloc->LocBTQosNull);

	SET_H2CCMD_RSVDPAGE_LOC_PROBE_RSP(u1H2CRsvdPageParm, rsvdpageloc->LocProbeRsp);
	SET_H2CCMD_RSVDPAGE_LOC_PSPOLL(u1H2CRsvdPageParm, rsvdpageloc->LocPsPoll);
	SET_H2CCMD_RSVDPAGE_LOC_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocNullData);
	SET_H2CCMD_RSVDPAGE_LOC_QOS_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocQosNull);
	SET_H2CCMD_RSVDPAGE_LOC_BT_QOS_NULL_DATA(u1H2CRsvdPageParm, rsvdpageloc->LocBTQosNull);

	RT_PRINT_DATA(_module_hal_init_c_, _drv_always_, "u1H2CRsvdPageParm:", u1H2CRsvdPageParm, H2C_RSVDPAGE_LOC_LEN);
	FillH2CCmd_8812(padapter, H2C_RSVD_PAGE, H2C_RSVDPAGE_LOC_LEN, u1H2CRsvdPageParm);
}

int rtl8812_iqk_wait(_adapter* padapter, u32 timeout_ms)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct submit_ctx	*iqk_sctx = &pHalData->iqk_sctx;

	iqk_sctx->submit_time = rtw_get_current_time();
	iqk_sctx->timeout_ms = timeout_ms;
	iqk_sctx->status = RTW_SCTX_SUBMITTED;

	return rtw_sctx_wait(iqk_sctx, __func__);
}

void rtl8812_iqk_done(_adapter* padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct submit_ctx	*iqk_sctx = &pHalData->iqk_sctx;

	rtw_sctx_done(&iqk_sctx);
}

static VOID
C2HTxBeamformingHandler_8812(
    IN	PADAPTER		Adapter,
    IN	u8*				CmdBuf,
    IN	u8				CmdLen
)
{
	u8	status = CmdBuf[0] & BIT0;
#ifdef CONFIG_BEAMFORMING
	beamforming_check_sounding_success(Adapter, status);
#endif
}

static VOID
C2HTxFeedbackHandler_8812(
    IN	PADAPTER	Adapter,
    IN	u8			*CmdBuf,
    IN	u8			CmdLen
)
{
#ifdef CONFIG_XMIT_ACK
	if (GET_8812_C2H_TX_RPT_RETRY_OVER(CmdBuf) | GET_8812_C2H_TX_RPT_LIFE_TIME_OVER(CmdBuf)) {
		rtw_ack_tx_done(&Adapter->xmitpriv, RTW_SCTX_DONE_CCX_PKT_FAIL);
	} else {
		rtw_ack_tx_done(&Adapter->xmitpriv, RTW_SCTX_DONE_SUCCESS);
	}
#endif
}

static VOID
C2HRaReportHandler_8812(
    IN	PADAPTER	Adapter,
    IN	u8*			CmdBuf,
    IN	u8			CmdLen
)
{
	u8 	Rate = CmdBuf[0] & 0x3F;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	ODM_UpdateInitRate(&pHalData->odmpriv, Rate);
}

s32
_C2HContentParsing8812(
    IN	PADAPTER	Adapter,
    IN	u8			c2hCmdId,
    IN	u8			c2hCmdLen,
    IN	u8 			*tmpBuf
)
{
	s32 ret = _SUCCESS;

	switch(c2hCmdId) {
	case C2H_8812_DBG:
		DBG_871X("[C2H], C2H_8812_DBG!!\n");
		break;

	case C2H_8812_TXBF:
		DBG_871X("[C2H], C2H_8812_TXBF!!\n");
		C2HTxBeamformingHandler_8812(Adapter, tmpBuf, c2hCmdLen);
		break;

	case C2H_8812_TX_REPORT:
		C2HTxFeedbackHandler_8812(Adapter, tmpBuf, c2hCmdLen);
		break;

	case C2H_8812_BT_MP:
		DBG_871X("[C2H], C2H_8812_BT_MP!!\n");
		break;

	case C2H_8812_RA_RPT:
		C2HRaReportHandler_8812(Adapter, tmpBuf, c2hCmdLen);
		break;

	case C2H_8812_FW_SWCHNL:
		break;

	case C2H_8812_IQK_FINISH:
		DBG_871X("== IQK Finish ==\n");
		rtl8812_iqk_done(Adapter);
		break;

	case C2H_8812_MAILBOX_STATUS:
		DBG_871X("[C2H], mailbox status:%u\n", *tmpBuf);
		break;

#ifdef CONFIG_FW_C2H_DEBUG
	case C2H_8812_FW_DEBUG:
		DBG_871X("[C2H], FW_DEBUG.\n");
		Debug_FwC2H(Adapter, tmpBuf, c2hCmdLen);
		break;
#endif // CONFIG_FW_C2H_DEBUG
	default:
		DBG_871X("%s: [WARNING] unknown C2H(0x%02x)\n", __FUNCTION__, c2hCmdId);
		ret = _FAIL;
		break;
	}

	return ret;
}


VOID
C2HPacketHandler_8812(
    IN	PADAPTER	Adapter,
    IN	u8			*Buffer,
    IN	u8			Length
)
{
	u8	c2hCmdId=0, c2hCmdSeq=0, c2hCmdLen=0;
	u8	*tmpBuf=NULL;

	c2hCmdId = Buffer[0];
	c2hCmdSeq = Buffer[1];
	c2hCmdLen = Length -2;
	tmpBuf = Buffer+2;
	/* handle directly */
		_C2HContentParsing8812(Adapter, c2hCmdId, c2hCmdLen, tmpBuf);
}
