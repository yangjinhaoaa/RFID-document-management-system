/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usart.h"
#include "oled.h"
#include "stdio.h"
#include "gpio.h"
#include "EEPROM.h"
#include "ds3231.h"
#include "rc522.h"
#include "adc.h"


uint8_t		eep00;
uint8_t		docid[8];
uint8_t   docidraw[14];
uint8_t		docloca[2];
uint8_t		docnum;//for eeprom
uint8_t		timestr[15];
uint8_t   timeflag=0;
//uint8_t 	k;
uint8_t 	is;
uint8_t   ik;
uint8_t   newflag;
uint8_t   init2flag;
int 		is1;
int     adcval;
uint8_t 	screenstr[4];
uint8_t rxcmd;
uint8_t rfidkeya[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
//uint8_t 	j;
// uint8_t 	b;
// uint8_t 	q;
// uint8_t 	en;
// uint8_t 	ok;
uint8_t 	comand;
uint8_t		text1[63] = "STM32F103 Mifare RC522 RFID Card reader 13.56 MHz for KEIL HAL\r";
uint8_t		text2[9] = "Card ID: ";
uint8_t		end[1] = "\r";
uint8_t		txBuffer[18] = "Card ID: 00000000\r";
uint8_t 	retstr[10];
uint8_t 	rxBuffer[8];
uint8_t		lastID[4];

uint8_t		CardID[MFRC522_MAX_LEN];												// MFRC522_MAX_LEN = 16
uint8_t		LastCardID[4] = {0};
uint8_t		memID[8] = "9C55A1B5";
uint8_t		str[MFRC522_MAX_LEN];																						// MFRC522_MAX_LEN = 16
TIME ntime;

uint8_t   savedid[100][4];

//eeprom 存储相关
// #define DEV_ADDR 0xa0
// uint8_t dataw1[] = "hello world from EEPROM";
// uint8_t dataw2[] = "This is the second string from EEPROM";
// float dataw3 = 1234.5678;

  uint8_t datar1[100];
   uint8_t datar2[100];
// float datar3;


void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */
 
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_ADC1_Init();

  OLED_Init();
	startscreen();
	ledb(1);
	ledg(1);

	ledr(3);

	
	MFRC522_Init();
	HAL_UART_Receive_IT(&huart1, &rxcmd, 1);
	uint8_t beforeid[4];
  
  
  /*以下是擦除eeprom操作 仅使用一次即可*/
  // EEPROM_rangeerase(0,61);
  // EEPROM_Write(0, 0, &docnum, 1);
  // printf("eeprom erease!\n");
	// // for (is1 = 0; is1 < 512; is1++)
	// // {
	// // 	EEPROM_PageErase(is1);
	// // }

  //读取文档数量
  EEPROM_Read(0, 0, &eep00, 1);
  printf("eeprom已保存文档数：%d\n", eep00);

  //读取0-20 的eeprom内容 并展示
  printf("读取EEPROM\n");
  EEPROM_outrangeprint(0, 20);

  //读取RFID识别号放入savedid数组
  for (is = 0; is < 60; is++)
  {
    EEPROM_Read(is + 1, 0, (void *)(savedid + is), 4);
  }

  //展示存储的rfid识别号
  for (is = 0; is < 60; is++)
  {
    for (ik = 0; ik < 4; ik++)
    {
      printf("savedid%d行%d列 %d\n", is, ik, savedid[is][ik]);
    }
  }

  //获取当前时间 并显示
  ntime = Get_Time();
  sprintf(timestr, "%02d:%02d:%02d\r\n", ntime.hour, ntime.minutes, ntime.seconds);
  printf("当前时间%s", timestr);
  sprintf(timestr, "20%02d-%02d-%02d\r\n", ntime.year, ntime.month, ntime.dayofmonth);
  printf("当前日期%s", timestr);

  //开机时测量电压ADC
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, 50);
  adcval = HAL_ADC_GetValue(&hadc1);
  printf("adcval = %d\n", adcval);
  if (adcval < 2000)
  {
    OLED_ShowCHinese(112, 0, 31);
  }
  HAL_Delay(500);

  while (1)
  {
    //检查串口中断是否发送过0x02，如果有，擦除eeprom操作
    if (init2flag == 1)
    {
      EEPROM_rangeerase(1, 60);
      init2flag = 0;
      EEPROM_Write(0, 0, &init2flag, 1);
      
      printf("擦除完成 请重启系统\n");
    }

    //每20次检查一次电量
    timeflag++;
    if (timeflag == 20)
    {
      timeflag = 0;
      //电量
      HAL_ADC_Start(&hadc1);
      HAL_ADC_PollForConversion(&hadc1, 50);
      adcval = HAL_ADC_GetValue(&hadc1);
      //printf("adcval = %d\n", adcval);
      if (adcval < 1900)
      {
        OLED_ShowCHinese(112, 0, 31);
      }
    }

    if (!MFRC522_Request(PICC_REQIDL, CardID))
    {

      if (!MFRC522_Anticoll(CardID))
      {
        //判断是否和上个id一样？
        if ((beforeid[0] == CardID[0]) && (beforeid[1] == CardID[1]) && (beforeid[2] == CardID[2]) && (beforeid[3] == CardID[3]))
        {

          //showframework();
          ledb(1);
          // show ok picture
          OLED_ShowCHinese(112, 2, 11);
          printf("samecard\n");
        }
        else
        {
          EEPROM_Read(0, 0, &docnum, 1);
          //读取存储的savedid
          for (is = 0; is < docnum; is++)
          {
            EEPROM_Read(is + 1, 0, (void *)(savedid + is), 4);
          }
          // 与savedid 比较
          newflag = 0;
          for (is = 0; is < docnum; is++)
          {
            if ((savedid[is][0] == CardID[0]) && (savedid[is][1] == CardID[1]) && (savedid[is][2] == CardID[2]) && (savedid[is][3] == CardID[3]))
            {
              newflag =is;
            }
          }
          if (newflag >0)
          {
            printf("文档已经在库中了 \n");
            //读取位置信息
            docloca[0] = (newflag / 16)+1;
            docloca[1] = newflag % 16;
            printf("文档在%d行%d列 \n", docloca[0], docloca[1]);
            sprintf(screenstr, "H%dL%d", docloca[0], docloca[1]);
            OLED_ShowString(32, 3, screenstr, 4);
          
            //读取RFID标识号 文档号
            sprintf(screenstr, "%X%X%X%X", CardID[0], CardID[1], CardID[2], CardID[3]);
            printf("%X%X%X%X\n", CardID[0], CardID[1], CardID[2], CardID[3]);
            OLED_ShowString(40, 0, screenstr, 4);

            EEPROM_Read(newflag+1,5,docidraw,14);
            sprintf(docid, "%c%c%c%c%c%c%c%c%c%c%c%c%c%c", docidraw[0],docidraw[1],docidraw[2],docidraw[3],docidraw[4],docidraw[5],docidraw[6],docidraw[7],docidraw[8],docidraw[9],docidraw[10],docidraw[11],docidraw[12],docidraw[13],docidraw[14]);
            printf("文档号：%s\n", docid);
            printf("存入文档柜时间:20%c%c年%c%c月%c%c日%c%c时%c%c分%c%c秒 \n",docidraw[0],docidraw[1],docidraw[2],docidraw[3],docidraw[4],docidraw[5],docidraw[6],docidraw[7],docidraw[8],docidraw[9],docidraw[10],docidraw[11],docidraw[12]);
            OLED_ShowString(0, 7, docid, 14);
          }
          else
          {
            //printf("文档未出现 \n");
            printf("发现新文档，已入柜 \n");
            printf("已存储%d份文档 \n", docnum);
            sprintf(screenstr, "%X%X%X%X", CardID[0], CardID[1], CardID[2], CardID[3]);
            printf("RFID识别号 %X%X%X%X\n", CardID[0], CardID[1], CardID[2], CardID[3]);
            OLED_ShowString(40, 0, screenstr, 4);
            //show add file picture
            OLED_ShowCHinese(112, 2, 8);
            //读到新卡后的处理过程
            //生成卡号
            ntime = Get_Time();

            docidraw[0]=ntime.year;
            docidraw[1]=ntime.month;
            docidraw[2]=ntime.dayofmonth;
            docidraw[3]=ntime.hour;
            docidraw[4]=ntime.minutes;
            docidraw[5]=ntime.seconds;
            docidraw[6]=CardID[3];
            sprintf(docid, "%02d%02d%02d%02d%02d%02d%X", ntime.year, ntime.month, ntime.dayofmonth, ntime.hour, ntime.minutes, ntime.seconds, CardID[3]);
            printf("文档号：%s\n", docid);
            printf("进入文档柜时间:%d年%d月%d日%d时%d分%d秒 \n",ntime.year, ntime.month, ntime.dayofmonth, ntime.hour, ntime.minutes, ntime.seconds);
            OLED_ShowString(0, 7, docid, 7);
            //docnum
            docnum += 1;
            printf("已保存文档数：%d\n", docnum);
            //生成位置编号
            docloca[0] = (docnum / 16)+1;
            docloca[1] = docnum % 16;
            printf("文档在%d行%d列 \n", docloca[0], docloca[1]);
            sprintf(screenstr, "H%dL%d", docloca[0], docloca[1]);
            OLED_ShowString(32, 3, screenstr, 4);

            //存入eeprom
            //存文档数
            EEPROM_Write(0, 0, &docnum, 1);
            //存RFID卡号
            EEPROM_Write(docnum, 0, CardID, 4);
            //存文档号
            EEPROM_Write(docnum, 5, docid, 14);
            ledg(4);
          }
        }
        //将本次id赋值到上次id
        for (is = 0; is < 4; is++)
        {
          beforeid[is] = CardID[is];
        }
      }
    }

    HAL_Delay(10);
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 判断是哪个串口触发的中断 */
    if(huart ->Instance == USART1)
    {
        //show all
        printf("%x\n",rxcmd);
        if(rxcmd==1){
          EEPROM_outrangeprint(0,30);
        }
        if (rxcmd==2)
        {
          //EEPROM_rangeerase(1,60);
         // EEPROM_Write(0, 0, &docnum, 1);
         init2flag=1;
          printf("eeprom erease!\n");
        }
        
        
        //重新使能串口接收中断
        HAL_UART_Receive_IT(huart, &rxcmd, 1);
    }
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    init2flag=1;
    printf("init %d\n",init2flag);

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
