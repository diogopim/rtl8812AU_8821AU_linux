EXTRA_CFLAGS += $(USER_EXTRA_CFLAGS)
EXTRA_CFLAGS += -O1
#EXTRA_CFLAGS += -O3
EXTRA_CFLAGS += -Wall
#EXTRA_CFLAGS += -Wextra
EXTRA_CFLAGS += -Werror
#EXTRA_CFLAGS += -pedantic
#EXTRA_CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
EXTRA_CFLAGS += -Wframe-larger-than=1536

#EXTRA_CFLAGS += -Wno-unused-variable
#EXTRA_CFLAGS += -Wno-unused-value
#EXTRA_CFLAGS += -Wno-unused-label
EXTRA_CFLAGS += -Wno-unused-parameter
#EXTRA_CFLAGS += -Wno-unused-function
#EXTRA_CFLAGS += -Wno-unused

#EXTRA_CFLAGS += -Wno-uninitialized

EXTRA_CFLAGS += -I$(src)/include

EXTRA_LDFLAGS += --strip-debug

########################## WIFI IC ############################
CONFIG_RTL8812A = y
######################### Interface ###########################
#CONFIG_USB_HCI = y
########################## Features ###########################
CONFIG_MP_INCLUDED = y
#CONFIG_POWER_SAVING = y
CONFIG_INTEL_WIDI = n
CONFIG_WAPI_SUPPORT = n
#CONFIG_RTW_ADAPTIVITY_EN = disable
#CONFIG_RTW_ADAPTIVITY_MODE = normal
######### Notify SDIO Host Keep Power During Syspend ##########
#CONFIG_RTW_SDIO_PM_KEEP_POWER = y // does not matter
###################### Platform Related #######################
#CONFIG_PLATFORM_I386_PC = y
#CONFIG_PLATFORM_ARM_RPI = n
#CONFIG_PLATFORM_ANDROID_X86 = n
#CONFIG_PLATFORM_ANDROID_INTEL_X86 = n
#CONFIG_PLATFORM_JB_X86 = n
#CONFIG_PLATFORM_ARM_S3C2K4 = n
#CONFIG_PLATFORM_ARM_PXA2XX = n
#CONFIG_PLATFORM_ARM_S3C6K4 = n
#CONFIG_PLATFORM_MIPS_RMI = n
#CONFIG_PLATFORM_RTD2880B = n
#CONFIG_PLATFORM_MIPS_AR9132 = n
#CONFIG_PLATFORM_RTK_DMP = n
#CONFIG_PLATFORM_MIPS_PLM = n
#CONFIG_PLATFORM_MSTAR389 = n
#CONFIG_PLATFORM_MT53XX = n
#CONFIG_PLATFORM_ARM_MX51_241H = n
#CONFIG_PLATFORM_FS_MX61 = n
#CONFIG_PLATFORM_ACTIONS_ATJ227X = n
#CONFIG_PLATFORM_TEGRA3_CARDHU = n
#CONFIG_PLATFORM_TEGRA4_DALMORE = n
#CONFIG_PLATFORM_ARM_TCC8900 = n
#CONFIG_PLATFORM_ARM_TCC8920 = n
#CONFIG_PLATFORM_ARM_TCC8920_JB42 = n
#CONFIG_PLATFORM_ARM_RK2818 = n
#CONFIG_PLATFORM_ARM_RK3066 = n
#CONFIG_PLATFORM_ARM_RK3188 = n
#CONFIG_PLATFORM_ARM_URBETTER = n
#CONFIG_PLATFORM_ARM_TI_PANDA = n
#CONFIG_PLATFORM_MIPS_JZ4760 = n
#CONFIG_PLATFORM_DMP_PHILIPS = n
#CONFIG_PLATFORM_MSTAR_TITANIA12 = n
#CONFIG_PLATFORM_MSTAR = n
#CONFIG_PLATFORM_SZEBOOK = n
#CONFIG_PLATFORM_ARM_SUNxI = n
#CONFIG_PLATFORM_ARM_SUN6I = n
#CONFIG_PLATFORM_ARM_SUN7I = n
#CONFIG_PLATFORM_ARM_SUN8I = n
#CONFIG_PLATFORM_ACTIONS_ATM702X = n
#CONFIG_PLATFORM_ACTIONS_ATV5201 = n
#CONFIG_PLATFORM_ACTIONS_ATM705X = n
#CONFIG_PLATFORM_ARM_RTD299X = n
#CONFIG_PLATFORM_ARM_SPREADTRUM_6820 = n
#CONFIG_PLATFORM_ARM_SPREADTRUM_8810 = n
#CONFIG_PLATFORM_ARM_WMT = n
#CONFIG_PLATFORM_TI_DM365 = n
#CONFIG_PLATFORM_MOZART = n
#CONFIG_PLATFORM_RTK119X = n
#CONFIG_PLATFORM_NOVATEK_NT72668 = n
########################## DEBUG ##############################
#CONFIG_DEBUG = n
#CONFIG_DEBUG_CFG80211 = n
#CONFIG_DEBUG_RTL871X = n
###############################################################

#CONFIG_DRVEXT_MODULE = n

export TopDIR ?= $(shell pwd)

########### COMMON  #################################

HCI_NAME = usb

_OS_INTFS_FILES :=	os_dep/osdep_service.o \
			os_dep/linux/os_intfs.o \
			os_dep/linux/$(HCI_NAME)_intf.o \
			os_dep/linux/$(HCI_NAME)_ops_linux.o \
			os_dep/linux/ioctl_linux.o \
			os_dep/linux/xmit_linux.o \
			os_dep/linux/mlme_linux.o \
			os_dep/linux/recv_linux.o \
			os_dep/linux/ioctl_cfg80211.o \
			os_dep/linux/rtw_cfgvendor.o \
			os_dep/linux/wifi_regd.o \
			os_dep/linux/rtw_android.o \
			os_dep/linux/rtw_proc.o

_HAL_INTFS_FILES :=	hal/hal_intf.o \
			hal/hal_com.o \
			hal/hal_com_phycfg.o \
			hal/hal_phy.o \
			hal/hal_dm.o \
			hal/hal_hci/hal_$(HCI_NAME).o \
			hal/led/hal_$(HCI_NAME)_led.o

_OUTSRC_FILES := hal/OUTSRC/phydm_debug.o	\
		hal/OUTSRC/phydm_AntDiv.o\
		hal/OUTSRC/phydm_AntDect.o\
		hal/OUTSRC/phydm_interface.o\
		hal/OUTSRC/phydm_HWConfig.o\
		hal/OUTSRC/phydm.o\
		hal/OUTSRC/HalPhyRf.o\
		hal/OUTSRC/phydm_EdcaTurboCheck.o\
		hal/OUTSRC/phydm_DIG.o\
		hal/OUTSRC/phydm_PathDiv.o\
		hal/OUTSRC/phydm_RaInfo.o\
		hal/OUTSRC/phydm_DynamicBBPowerSaving.o\
		hal/OUTSRC/phydm_PowerTracking.o\
		hal/OUTSRC/phydm_DynamicTxPower.o\
		hal/OUTSRC/PhyDM_Adaptivity.o\
		hal/OUTSRC/phydm_CfoTracking.o\
		hal/OUTSRC/phydm_NoiseMonitor.o\
		hal/OUTSRC/phydm_ACS.o

EXTRA_CFLAGS += -I$(src)/platform
_PLATFORM_FILES := platform/platform_ops.o

########### HAL_RTL8812A_RTL8821A #################################

RTL871X = rtl8812a
MODULE_NAME = 8812au

_HAL_INTFS_FILES +=  hal/HalPwrSeqCmd.o \
					hal/$(RTL871X)/Hal8812PwrSeq.o \
					hal/$(RTL871X)/Hal8821APwrSeq.o\
					hal/$(RTL871X)/$(RTL871X)_xmit.o\
					hal/$(RTL871X)/$(RTL871X)_sreset.o

_HAL_INTFS_FILES +=	hal/$(RTL871X)/$(RTL871X)_hal_init.o \
			hal/$(RTL871X)/$(RTL871X)_phycfg.o \
			hal/$(RTL871X)/$(RTL871X)_rf6052.o \
			hal/$(RTL871X)/$(RTL871X)_dm.o \
			hal/$(RTL871X)/$(RTL871X)_rxdesc.o \
			hal/$(RTL871X)/$(RTL871X)_cmd.o \
			hal/$(RTL871X)/$(HCI_NAME)/$(HCI_NAME)_halinit.o \
			hal/$(RTL871X)/$(HCI_NAME)/rtl$(MODULE_NAME)_led.o \
			hal/$(RTL871X)/$(HCI_NAME)/rtl$(MODULE_NAME)_xmit.o \
			hal/$(RTL871X)/$(HCI_NAME)/rtl$(MODULE_NAME)_recv.o

_HAL_INTFS_FILES += hal/$(RTL871X)/$(HCI_NAME)/$(HCI_NAME)_ops_linux.o

_HAL_INTFS_FILES += hal/$(RTL871X)/$(RTL871X)_mp.o

_HAL_INTFS_FILES +=hal/efuse/$(RTL871X)/HalEfuseMask8812A_USB.o

EXTRA_CFLAGS += -DCONFIG_RTL8812A
_OUTSRC_FILES += hal/OUTSRC/$(RTL871X)/HalHWImg8812A_FW.o\
		hal/OUTSRC/$(RTL871X)/HalHWImg8812A_MAC.o\
		hal/OUTSRC/$(RTL871X)/HalHWImg8812A_BB.o\
		hal/OUTSRC/$(RTL871X)/HalHWImg8812A_RF.o\
		hal/OUTSRC/$(RTL871X)/HalPhyRf_8812A.o\
		hal/OUTSRC/$(RTL871X)/phydm_RegConfig8812A.o\
		hal/OUTSRC/$(RTL871X)/phydm_RTL8812A.o

########### END OF PATH  #################################

EXTRA_CFLAGS += -DCONFIG_MP_INCLUDED
EXTRA_CFLAGS += -DCONFIG_POWER_SAVING
EXTRA_CFLAGS += -DCONFIG_TRAFFIC_PROTECT

EXTRA_CFLAGS += -DCONFIG_LOAD_PHY_PARA_FROM_FILE
EXTRA_CFLAGS += -DREALTEK_CONFIG_PATH=\"\"

EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_EN=0

EXTRA_CFLAGS += -DCONFIG_RTW_ADAPTIVITY_MODE=0

BR_NAME = br0
EXTRA_CFLAGS += -DCONFIG_BR_EXT
EXTRA_CFLAGS += '-DCONFIG_BR_EXT_BRNAME="'$(BR_NAME)'"'

EXTRA_CFLAGS += -DCONFIG_LITTLE_ENDIAN
#EXTRA_CFLAGS += -DCONFIG_CONCURRENT_MODE
EXTRA_CFLAGS += -DCONFIG_IOCTL_CFG80211 -DRTW_USE_CFG80211_STA_EVENT
#EXTRA_CFLAGS += -DCONFIG_P2P_IPS
SUBARCH := $(shell uname -m | sed -e s/i.86/i386/)
ARCH ?= $(SUBARCH)
CROSS_COMPILE ?=
ifndef KVER
KVER ?= $(shell uname -r)
endif
KSRC := /lib/modules/$(KVER)/build
MODDESTDIR := /lib/modules/$(KVER)/kernel/drivers/net/wireless/
INSTALL_PREFIX :=

USER_MODULE_NAME ?= rtl$(MODULE_NAME)
ifneq ($(USER_MODULE_NAME),)
MODULE_NAME := $(USER_MODULE_NAME)
endif

ifneq ($(KERNELRELEASE),)
KERNELRELEASE := $(shell uname -r)
endif

rtk_core :=	core/rtw_cmd.o \
		core/rtw_security.o \
		core/rtw_debug.o \
		core/rtw_io.o \
		core/rtw_ioctl_query.o \
		core/rtw_ioctl_set.o \
		core/rtw_ieee80211.o \
		core/rtw_mlme.o \
		core/rtw_mlme_ext.o \
		core/rtw_wlan_util.o \
		core/rtw_vht.o \
		core/rtw_pwrctrl.o \
		core/rtw_rf.o \
		core/rtw_recv.o \
		core/rtw_sta_mgt.o \
		core/rtw_ap.o \
		core/rtw_xmit.o	\
		core/rtw_p2p.o \
		core/rtw_tdls.o \
		core/rtw_br_ext.o \
		core/rtw_iol.o \
		core/rtw_sreset.o \
		core/rtw_beamforming.o \
		core/rtw_odm.o \
		core/efuse/rtw_efuse.o

$(MODULE_NAME)-y += $(rtk_core)

$(MODULE_NAME)-$(CONFIG_INTEL_WIDI) += core/rtw_intel_widi.o

$(MODULE_NAME)-$(CONFIG_WAPI_SUPPORT) += core/rtw_wapi.o	\
					core/rtw_wapi_sms4.o

$(MODULE_NAME)-y += $(_OS_INTFS_FILES)
$(MODULE_NAME)-y += $(_HAL_INTFS_FILES)
$(MODULE_NAME)-y += $(_OUTSRC_FILES)
$(MODULE_NAME)-y += $(_PLATFORM_FILES)

$(MODULE_NAME)-$(CONFIG_MP_INCLUDED) += core/rtw_mp.o \
					core/rtw_mp_ioctl.o

ifneq ($(CONFIG_RTL8812AU),)
obj-$(CONFIG_RTL8812AU) := $(MODULE_NAME).o
else
obj-m := $(MODULE_NAME).o

all: modules

modules:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

strip:
	$(CROSS_COMPILE)strip $(MODULE_NAME).ko --strip-unneeded

install:
	install -p -m 644 $(MODULE_NAME).ko  $(MODDESTDIR)
	/sbin/depmod -a ${KVER}

uninstall:
	rm -f $(MODDESTDIR)/$(MODULE_NAME).ko
	/sbin/depmod -a ${KVER}

config_r:
	@echo "make config"
	/bin/bash script/Configure script/config.in


.PHONY: modules clean

clean:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd) clean
endif
