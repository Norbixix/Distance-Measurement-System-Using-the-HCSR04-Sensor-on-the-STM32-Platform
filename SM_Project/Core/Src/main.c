/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "usart.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hcsr04.h"
#include "SSD1306_OLED.h"
#include "GFX_BW.h"
#include "fonts/fonts.h"
#include <stdio.h>
#include "servo.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define HCSR04_TRIGGER_TIMER		htim3
#define HCSR04_ECHO_TIMER			htim3
#define HCSR04_TRIGGER_CHANNEL		TIM_CHANNEL_3
#define HCSR04_ECHO_START_CHANNEL	TIM_CHANNEL_1
#define HCSR04_ECHO_STOP_CHANNEL	TIM_CHANNEL_2

#define ADC_CHANNELS_USED				2
#define ADC_CHANNELS_SAMPLES			10
#define ADC_MIN							0
#define ADC_MAX							4095
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// Creation of rangefinder and servo "objects"
HCSR04_t HCSR04;
servo_t SERVO;

// Variable for storing the distance from HCSR04
uint16_t Distance_u16;

// Variable storing the corresponding LED duty cycle
uint16_t LedBrightness;

// 10 measurements for each channel
uint16_t AdcValue[ADC_CHANNELS_SAMPLES][ADC_CHANNELS_USED]; // index = 0 for CH1, index = 1 for CH2

// Variables for storing the average value from 10 ADC measurements
uint16_t MeanX, MeanY;

uint32_t SoftTimerHCSR04;
uint32_t SoftTimerOLED;

char MessageDist[32];
char MessageX[32];
char MessageY[32];
char MessageAngle[32];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Function for mapping LED brightness depending on distance
// The closer the object, the brighter the LED
float map_distance_led_PWM(float d_cm)
{
    const float d_min = 2.0f;
    const float d_max = 50.0f;

    if (d_cm <= d_min) return 999;
    if (d_cm >= d_max) return 0;

    float t = (d_max - d_cm) / (d_max - d_min);
    return (t * 999.0f);
}

// Function for averaging ADC values (for channels 0 and 1)
void Average_ADC_Values(uint16_t AdcValue[10][2], uint16_t *MeanX, uint16_t *MeanY)
{
    uint32_t MeanXtmp = 0;
    uint32_t MeanYtmp = 0;

    // Summing values from 10 measurements (in this case 10 samples)
    for (uint8_t i = 0; i < 10; i++)
    {
        MeanXtmp += AdcValue[i][0];  // Reading for X channel (e.g. joystick)
        MeanYtmp += AdcValue[i][1];  // Reading for Y channel (e.g. joystick)
    }

    // Calculating averages
    *MeanX = MeanXtmp / 10;
    *MeanY = MeanYtmp / 10;
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_LPUART1_UART_Init();
  MX_TIM3_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

  // Starts ADC conversion using DMA; results from ADC channels will be automatically stored in the AdcValue array (joystick reading)
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)AdcValue, (ADC_CHANNELS_SAMPLES*ADC_CHANNELS_USED));

  // Starts PWM signal generation on timer 2 channel 1 (used to control LED brightness)
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

  // Initialization of the rangefinder, servo and OLED
  HCSR04_Init(&HCSR04, &HCSR04_TRIGGER_TIMER, HCSR04_TRIGGER_CHANNEL, &HCSR04_ECHO_TIMER, HCSR04_ECHO_START_CHANNEL, HCSR04_ECHO_STOP_CHANNEL);
  Servo_Init(&SERVO, &htim4, TIM_CHANNEL_2);
  SSD1306_Init(&hi2c1);

  // Assign font
  GFX_SetFont(font_8x5);
  GFX_SetFontSize(1);

  // Set all pixels to black (off) in the buffer
  // Clear the buffer
  SSD1306_Clear(BLACK);

  // Send the buffer to the display
  SSD1306_Display();

  SoftTimerHCSR04 = HAL_GetTick();
  SoftTimerOLED = HAL_GetTick();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  	// Calculating the average value for X and Y joystick axes
  	Average_ADC_Values(AdcValue, &MeanX, &MeanY);

  	// Mapping the Y axis to an angle range of 0–180°
  	uint16_t AngleY = map(MeanY, ADC_MIN, ADC_MAX, ANGLE_MIN, ANGLE_MAX);

  	// Changing the servo position according to the calculated angle from the Y axis
  	Servo_SetAngle(&SERVO, AngleY);

  	// Checking if 50 ms have passed since the last HCSR04 distance measurement
  	if((HAL_GetTick() - SoftTimerHCSR04) > 50){
  		// Updating the timer for the next HCSR04 measurement
  		SoftTimerHCSR04 = HAL_GetTick();

  		// Calculating the distance
  		HCSR04_CalculateResultInteger(&HCSR04, &Distance_u16);

  		// Mapping distance to PWM value for the LED (the closer the object, the higher the brightness)
  		LedBrightness = map_distance_led_PWM(Distance_u16);

  		// Setting PWM duty cycle on timer 2 channel 1, which controls LED brightness
  		__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, LedBrightness);
  	}

  	// Checking if 100 ms have passed since the last OLED display update
  	if((HAL_GetTick() - SoftTimerOLED) > 100){
  		// Updating the timer for the next OLED drawing cycle
  		SoftTimerOLED = HAL_GetTick();

  		// Setting all pixels to black,
  		// clearing the buffer before displaying new data
  		SSD1306_Clear(BLACK);


  		sprintf(MessageDist, "Distance: %u cm", Distance_u16);
  		// Displaying the distance measured by the HCSR04
  		GFX_DrawString(0, 0, MessageDist, WHITE, 0);


  		sprintf(MessageX, "X: %u", MeanX);
  		// Displaying the X value of the joystick
  		GFX_DrawString(0, 10, MessageX, WHITE, 0);


  		sprintf(MessageY, "Y: %u", MeanY);
  		// Displaying the Y value of the joystick
  		GFX_DrawString(0, 20, MessageY, WHITE, 0);

  		// Function for displaying the angle with the degree symbol (°)
  		DrawAngleWithDegree(AngleY);

  		// Sending data from the buffer to the display
  		SSD1306_Display();
  	}

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    /*
     * Check if interrupt comes from the timer used
     * for HC-SR04 echo measurement
     */
	if(htim == HCSR04.htim_echo)
	{
        /*
         * Call the sensor interrupt handler
         * which calculates echo pulse duration
         */
		HCSR04_InterruptHandler(&HCSR04);
	}
}

void SetChannel(uint32_t Channel){
	/** Configure Regular Channel
	*/
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = Channel;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_2CYCLES_5;
	sConfig.SingleDiff = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset = 0;
	if(HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
	Error_Handler();
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
