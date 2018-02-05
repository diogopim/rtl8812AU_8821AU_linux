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
#define _HCI_OPS_OS_C_

#include <rtl8812a_hal.h>

static inline s32 pre_recv_entry(union recv_frame *precvframe, u8 *pphy_status)
{
	return _SUCCESS; //TODO
}

int recvbuf2recvframe(PADAPTER padapter, void *ptr)
{
	u8	*pbuf;
	u8	pkt_cnt = 0;
	u32	pkt_offset;
	s32	transfer_len;
	u8				*pphy_status = NULL;
	union recv_frame	*precvframe = NULL;
	struct rx_pkt_attrib	*pattrib = NULL;
	//HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct recv_priv	*precvpriv = &padapter->recvpriv;
	_queue			*pfree_recv_queue = &precvpriv->free_recv_queue;
	_pkt *pskb;

#ifdef CONFIG_USE_USB_BUFFER_ALLOC_RX
	pskb = NULL;
	transfer_len = (s32)((struct recv_buf*)ptr)->transfer_len;
	pbuf = ((struct recv_buf*)ptr)->pbuf;
#else
	pskb = (_pkt*)ptr;
	transfer_len = (s32)pskb->len;
	pbuf = pskb->data;
#endif//CONFIG_USE_USB_BUFFER_ALLOC_RX


#ifdef CONFIG_USB_RX_AGGREGATION
	pkt_cnt = GET_RX_STATUS_DESC_USB_AGG_PKTNUM_8812(pbuf);
#endif

	do {
		precvframe = rtw_alloc_recvframe(pfree_recv_queue);
		if(precvframe==NULL) {
			RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,("recvbuf2recvframe: precvframe==NULL\n"));
			DBG_8192C("%s()-%d: rtw_alloc_recvframe() failed! RX Drop!\n", __FUNCTION__, __LINE__);
			goto _exit_recvbuf2recvframe;
		}

		_rtw_init_listhead(&precvframe->u.hdr.list);
		precvframe->u.hdr.precvbuf = NULL;	//can't access the precvbuf for new arch.
		precvframe->u.hdr.len=0;

		rtl8812_query_rx_desc_status(precvframe, pbuf);

		pattrib = &precvframe->u.hdr.attrib;

		if ((padapter->registrypriv.mp_mode == 0) && ((pattrib->crc_err) || (pattrib->icv_err))) {
			DBG_8192C("%s: RX Warning! crc_err=%d icv_err=%d, skip!\n", __FUNCTION__, pattrib->crc_err, pattrib->icv_err);

			rtw_free_recvframe(precvframe, pfree_recv_queue);
			goto _exit_recvbuf2recvframe;
		}

		pkt_offset = RXDESC_SIZE + pattrib->drvinfo_sz + pattrib->shift_sz + pattrib->pkt_len;

		if((pattrib->pkt_len<=0) || (pkt_offset>transfer_len)) {
			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("recvbuf2recvframe: pkt_len<=0\n"));
			DBG_8192C("%s()-%d: RX Warning!,pkt_len<=0 or pkt_offset> transfer_len \n", __FUNCTION__, __LINE__);
			rtw_free_recvframe(precvframe, pfree_recv_queue);
			goto _exit_recvbuf2recvframe;
		}

#ifdef CONFIG_RX_PACKET_APPEND_FCS
		if(pattrib->pkt_rpt_type == NORMAL_RX)
			pattrib->pkt_len -= IEEE80211_FCS_LEN;
#endif
		if(rtw_os_alloc_recvframe(padapter, precvframe,
		                          (pbuf + pattrib->shift_sz + pattrib->drvinfo_sz + RXDESC_SIZE), pskb) == _FAIL) {
			rtw_free_recvframe(precvframe, pfree_recv_queue);

			goto _exit_recvbuf2recvframe;
		}

		recvframe_put(precvframe, pattrib->pkt_len);
		//recvframe_pull(precvframe, drvinfo_sz + RXDESC_SIZE);

		if(pattrib->pkt_rpt_type == NORMAL_RX) { //Normal rx packet
			if(pattrib->physt)
				pphy_status = (pbuf + RXDESC_OFFSET);

			if(pattrib->physt && pphy_status)
				rx_query_phy_status(precvframe, pphy_status);

			if(rtw_recv_entry(precvframe) != _SUCCESS) {
				RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,
				         ("recvbuf2recvframe: rtw_recv_entry(precvframe) != _SUCCESS\n"));
			}

		} else { // pkt_rpt_type == TX_REPORT1-CCX, TX_REPORT2-TX RTP,HIS_REPORT-USB HISR RTP
			if (pattrib->pkt_rpt_type == C2H_PACKET) {
				//DBG_8192C("rx C2H_PACKET \n");
				C2HPacketHandler_8812(padapter,precvframe->u.hdr.rx_data,pattrib->pkt_len);
			}
			rtw_free_recvframe(precvframe, pfree_recv_queue);
		}

#ifdef CONFIG_USB_RX_AGGREGATION
		// jaguar 8-byte alignment
		pkt_offset = (u16)_RND8(pkt_offset);
		pkt_cnt--;
		pbuf += pkt_offset;
#endif
		transfer_len -= pkt_offset;
		precvframe = NULL;

	} while(transfer_len>0);

_exit_recvbuf2recvframe:

	return _SUCCESS;
}


void rtl8812au_xmit_tasklet(void *priv)
{
	int ret = _FALSE;
	_adapter *padapter = (_adapter*)priv;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;

	while(1) {
		if (RTW_CANNOT_TX(padapter)) {
			DBG_8192C("xmit_tasklet => bDriverStopped or bSurpriseRemoved or bWritePortCancel\n");
			break;
		}

		if(check_fwstate(&padapter->mlmepriv, _FW_UNDER_SURVEY) == _TRUE ) {
			break;
		}

		ret = rtl8812au_xmitframe_complete(padapter, pxmitpriv, NULL);

		if(ret==_FALSE)
			break;

	}

}

void rtl8812au_set_intf_ops(struct _io_ops	*pops)
{
	_func_enter_;

	_rtw_memset((u8 *)pops, 0, sizeof(struct _io_ops));

	pops->_read8 = &usb_read8;
	pops->_read16 = &usb_read16;
	pops->_read32 = &usb_read32;
	pops->_read_mem = &usb_read_mem;
	pops->_read_port = &usb_read_port;

	pops->_write8 = &usb_write8;
	pops->_write16 = &usb_write16;
	pops->_write32 = &usb_write32;
	pops->_writeN = &usb_writeN;

#ifdef CONFIG_USB_SUPPORT_ASYNC_VDN_REQ
	pops->_write8_async= &usb_async_write8;
	pops->_write16_async = &usb_async_write16;
	pops->_write32_async = &usb_async_write32;
#endif
	pops->_write_mem = &usb_write_mem;
	pops->_write_port = &usb_write_port;

	pops->_read_port_cancel = &usb_read_port_cancel;
	pops->_write_port_cancel = &usb_write_port_cancel;

	_func_exit_;

}

void rtl8812au_set_hw_type(_adapter *padapter)
{
	if(padapter->chip_type == RTL8812) {
		padapter->HardwareType = HARDWARE_TYPE_RTL8812AU;
		DBG_871X("CHIP TYPE: RTL8812\n");
	} else if(padapter->chip_type == RTL8821) {
		//padapter->HardwareType = HARDWARE_TYPE_RTL8811AU;
		padapter->HardwareType = HARDWARE_TYPE_RTL8821U;
		DBG_871X("CHIP TYPE: RTL8811AU or RTL8821U\n");
	}
}

