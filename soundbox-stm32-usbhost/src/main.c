
#include "stm32f4xx.h"
#include "stm32f4_generic.h"
			
#include "usbh_core.h"
#include "usbh_hid.h"
#include "usbh_hub.h"

#include "log.h"


USBH_HandleTypeDef hUSBHost[5];

static void USBH_UserProcess (USBH_HandleTypeDef *pHost, uint8_t vId);
static void hub_process();
void SystemClock_Config(void);
static void update_mon_out_interface();

static uint32_t i2c_latest_active_tick = 0;

static uint8_t latest_ossc_power_state = 0;

volatile void OnOSSCPowerChanged()
{
	update_mon_out_interface();
}

static void update_mon_out_interface()
{
	latest_ossc_power_state = ReadGPIO(PIN_IN_OSSC_POWER);
	WriteGPIO(PIN_OUT_MONOUT_INTERFACE_ENABLE, 1);
}

static void update_usb_host_power()
{
	WriteGPIO(PIN_OUT_USB_ENABLE, 1);
}

static void init_gpio_value()
{
	WriteGPIO(PIN_OUT_NEXT_POWERSW, 0);
	WriteGPIO(PIN_OUT_SPI_SS, 0);
	WriteGPIO(PIN_OUT_STATUS_LED, 0);
	update_usb_host_power();
	update_mon_out_interface();
}

static uint8_t i2c_recv_buf[3];
static uint8_t i2c_buf;

static void recv_i2c_slave()
{
	HAL_I2C_Slave_Receive_IT(&hi2c2, i2c_recv_buf, 3);
}

static void send_i2c_slave()
{
	HAL_I2C_Slave_Transmit_IT(&hi2c2, &i2c_buf, 1);
}

void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c2.Instance) {
		clear_i2c_intr_counter();
		i2c_latest_active_tick = HAL_GetTick();
		LOG("i2c recv done. data0 = %d, data1 = %d, data2 = %d, tick = %d", i2c_recv_buf[0],
		i2c_recv_buf[1], i2c_recv_buf[2],
		HAL_GetTick());
		// send_i2c_slave();
		recv_i2c_slave();
	}
}

void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
	if (hi2c->Instance == hi2c2.Instance) {
		clear_i2c_intr_counter();
		i2c_latest_active_tick = HAL_GetTick();
		LOG("i2c sent. data = %d, tick = %d", i2c_buf, HAL_GetTick());
		recv_i2c_slave();
	}
}

void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c)
{
	LOG("i2c abort");
	if (hi2c->Instance == hi2c2.Instance) {
		recv_i2c_slave();
	}
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
	LOG("i2c error");
	if (hi2c->Instance == hi2c2.Instance) {
		recv_i2c_slave();
	}
}

static void start_i2c()
{
	MX_I2C2_Init();
	recv_i2c_slave();
	i2c_latest_active_tick = HAL_GetTick();
}

static void check_i2c_activity()
{
	uint32_t current = HAL_GetTick();
	// LOG("tick %d, %d", i2c_latest_active_tick, current);
	if (current >= i2c_latest_active_tick) {
		if (current - i2c_latest_active_tick > 5*1000) { // no activity threshold
			LOG("no i2c activity, resetting...");
			// reset i2c
			MX_I2C2_DeInit(); // disable i2c pins
			HAL_Delay(3*1000);
			NVIC_SystemReset();
		}
	} else {
		// tick reset, check for next tick
		i2c_latest_active_tick = current;
		return;
	}
}

int main(void)
{
	uint32_t i = 0;

	HAL_Init();
	SystemClock_Config();

	MX_GPIO_Init();
	init_gpio_value();
	LOG_INIT(USARTx, 115200);

	LOG("wait for ossc boot up");
	HAL_Delay(3*1000);

	// i2c slave
	start_i2c();

	// spi master
	MX_SPI1_Init();

	// LOG("\033[2J\033[H");
	LOG(" ");
	LOG("APP RUNNING...");
	LOG("MCU-ID %08X", DBGMCU->IDCODE);

	memset(&hUSBHost[0], 0, sizeof(USBH_HandleTypeDef));

	hUSBHost[0].valid   = 1;
	hUSBHost[0].address = USBH_DEVICE_ADDRESS;
	hUSBHost[0].Pipes   = USBH_malloc(sizeof(uint32_t) * USBH_MAX_PIPES_NBR);

	USBH_Init(&hUSBHost[0], USBH_UserProcess, ID_USB_HOST_FS);
	USBH_RegisterClass(&hUSBHost[0], USBH_HID_CLASS);
	USBH_RegisterClass(&hUSBHost[0], USBH_HUB_CLASS);

	USBH_Start(&hUSBHost[0]);
	WriteGPIO(PIN_OUT_USB_ENABLE, 1);

	LOG("start");

	int j = 0;
	//uint8_t i2c_buf;
	while(1)
	{
		check_i2c_activity();

		if (i++ > 150000) {
			i = 0;
			j++;
			LOG("test %d, usb fault=%d, ossc power=%d", j, 
				ReadGPIO(PIN_IN_USB_FAULT),
				latest_ossc_power_state
			);			
		}

		if(i > 0 && i <= 150000/2)
			WriteGPIO(PIN_OUT_STATUS_LED, 0);
		else
			WriteGPIO(PIN_OUT_STATUS_LED, 1);

		// LOG("test\n");
		hub_process();
	}
}

void hub_process()
{
	// LOG("hub_process");
	static uint8_t current_loop = -1;
	static USBH_HandleTypeDef *_phost = 0;

	if(_phost != NULL && _phost->valid == 1)
	{
		USBH_Process(_phost);

		if(_phost->busy)
			return;
	}

	while(1)
	{
		// LOG("hub loop %d", current_loop);
		current_loop++;

		if(current_loop > MAX_HUB_PORTS)
			current_loop = 0;

		if(hUSBHost[current_loop].valid)
		{
			_phost = &hUSBHost[current_loop];
			USBH_LL_SetupEP0(_phost);

			if(_phost->valid == 3)
			{
LOG("PROCESSING ATTACH %d", _phost->address);
				_phost->valid = 1;
				_phost->busy  = 1;
			}

			break;
		}
	}

	if(_phost != NULL && _phost->valid)
	{
		HID_MOUSE_Info_TypeDef *minfo;
		minfo = USBH_HID_GetMouseInfo(_phost);
		if(minfo != NULL)
		{
LOG("BUTTON %d", minfo->buttons[0]);
		}
		else
		{
			HID_KEYBD_Info_TypeDef *kinfo;
			kinfo = USBH_HID_GetKeybdInfo(_phost);
			if(kinfo != NULL)
			{
LOG("KEYB %d", kinfo->keys[0]);
			}
		}
	}

}

void USBH_UserProcess (USBH_HandleTypeDef *pHost, uint8_t vId)
{
	switch (vId)
	{
		case HOST_USER_SELECT_CONFIGURATION:
			break;

		case HOST_USER_CLASS_SELECTED:
			break;

		case HOST_USER_CLASS_ACTIVE:
			break;

		case HOST_USER_CONNECTION:
			break;

		case HOST_USER_DISCONNECTION:
			break;

		case HOST_USER_UNRECOVERED_ERROR:
			USBH_ErrLog("HOST_USER_UNRECOVERED_ERROR %d", hUSBHost[0].RequestState);
			NVIC_SystemReset();
			break;

		default:
			break;
	}
}

void Error_Handler(void)
{
	
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}
