#include "stm32f4xx_usb_extra.h"
#include "usbh_conf.h"
/**
  * @brief  Stop Host Channel
  * @param  USBx : Selected device
  * @param  chnum : Channel number  
  * @retval HAL state
  */
HAL_StatusTypeDef USB_StopHostChannel(USB_OTG_GlobalTypeDef *USBx, uint8_t chnum)
{
  uint32_t count = 0;
  uint32_t value;
  
  USB_DisableGlobalInt(USBx);

  /* Flush out any leftover queued requests. */
  value = USBx_HC(chnum)->HCCHAR ;
  value |=  USB_OTG_HCCHAR_CHDIS;
  value &= ~USB_OTG_HCCHAR_CHENA;
  value &= ~USB_OTG_HCCHAR_EPDIR;
  USBx_HC(chnum)->HCCHAR = value;

  /* Halt all channels to put them into a known state. */
  value = USBx_HC(chnum)->HCCHAR;

  value |= USB_OTG_HCCHAR_CHDIS;
  value |= USB_OTG_HCCHAR_CHENA;
  value &= ~USB_OTG_HCCHAR_EPDIR;

  USBx_HC(chnum)->HCCHAR = value;
  do
  {
    if (++count > 1000)
    {
      break;
    }
  }
  while ((USBx_HC(chnum)->HCCHAR & USB_OTG_HCCHAR_CHENA) == USB_OTG_HCCHAR_CHENA);

  USB_EnableGlobalInt(USBx);

  return HAL_OK;
}

// hal_hcd

HAL_StatusTypeDef HAL_HCD_StopHC(HCD_HandleTypeDef *hhcd, uint8_t chnum)
{
  USBH_DbgLog("HAL_HCD_StopHC ch=%d", chnum);
  __HAL_LOCK(hhcd);
  USB_StopHostChannel(hhcd->Instance, chnum);
  __HAL_UNLOCK(hhcd);
  return HAL_OK;
}