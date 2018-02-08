
#if DEV_BUS_TYPE == RT_USB_INTERFACE

#if 0
#include "rtl8188e/HalEfuseMask8188E_USB.h"
#endif

#if 1
#include "rtl8812a/HalEfuseMask8812A_USB.h"
#endif

#if 0
#include "rtl8812a/HalEfuseMask8821A_USB.h"
#endif

#if 0
#include "rtl8192e/HalEfuseMask8192E_USB.h"
#endif

#if 0
#include "rtl8723b/HalEfuseMask8723B_USB.h"
#endif


#elif DEV_BUS_TYPE == RT_PCI_INTERFACE

#if 0
#include "rtl8188e/HalEfuseMask8188E_PCIE.h"
#endif

#if 1
#include "rtl8812a/HalEfuseMask8812A_PCIE.h"
#endif

#if 0
#include "rtl8812a/HalEfuseMask8821A_PCIE.h"
#endif

#if 0
#include "rtl8192e/HalEfuseMask8192E_PCIE.h"
#endif

#if 0
#include "rtl8723b/HalEfuseMask8723B_PCIE.h"
#endif

#elif DEV_BUS_TYPE == RT_SDIO_INTERFACE

#if 0
#include "rtl8188e/HalEfuseMask8188E_SDIO.h"
#endif

#endif