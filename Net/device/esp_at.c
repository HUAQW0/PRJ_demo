/**
	************************************************************
	************************************************************
	************************************************************
	*	文件名： 	esp_at.c
	*
	*	作者： 		张继瑞
	*
	*	日期： 		2017-05-08
	*
	*	版本： 		V1.0
	*
	*	说明： 		ESP32-C3 ESP-AT固件驱动
	*
	*	修改记录：	
	************************************************************
	************************************************************
	************************************************************
**/

//单片机头文件
#include "main.h"

//网络设备驱动
#include "esp_at.h"

//硬件驱动
#include "delay.h"
#include "usart.h"

//C库
#include <string.h>
#include <stdio.h>


#define ESP_AT_WIFI_INFO		"AT+CWJAP=\"FAST_B5A6\",\"hua123456\"\r\n"
#define ESP_AT_RETRY_COUNT		3

unsigned char esp_at_buf[512];
uint8_t esp_at_rx_byte;
volatile unsigned short esp_at_cnt = 0;


//==========================================================
//	函数名称：	ESP_AT_Clear
//
//	函数功能：	清空缓存
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
void ESP_AT_Clear(void)
{
	uint32_t primask = __get_PRIMASK();

	__disable_irq();
	memset(esp_at_buf, 0, sizeof(esp_at_buf));
	esp_at_cnt = 0;
	__set_PRIMASK(primask);

}

//==========================================================
//	函数名称：	ESP_AT_WaitReceive
//
//	函数功能：	等待接收完成
//
//	入口参数：	无
//
//	返回参数：	REV_OK-接收完成		REV_WAIT-接收超时未完成
//
//	说明：		循环调用检测是否接收完成
//==========================================================
_Bool ESP_AT_WaitReceive(void)
{
	return (esp_at_cnt > 0) ? REV_OK : REV_WAIT;

}

//==========================================================
//	函数名称：	ESP_AT_SendCmd
//
//	函数功能：	发送命令
//
//	入口参数：	cmd：命令
//				res：需要检查的返回指令
//
//	返回参数：	0-成功	1-失败
//
//	说明：		
//==========================================================
_Bool ESP_AT_SendCmd(char *cmd, char *res)
{
	
	uint32_t start_tick = HAL_GetTick();
	uint32_t timeout = 2000;

	if((cmd == NULL) || (res == NULL))
		return 1;

	if((strstr(cmd, "CWJAP") != NULL) || (strstr(cmd, "CIPSTART") != NULL))
		timeout = 20000;

	printf("AT TX: %s", cmd);
	// Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
    HAL_UART_Transmit(&huart2, (uint8_t *)cmd, strlen((const char *)cmd), 500);

		
	while((uint32_t)(HAL_GetTick() - start_tick) < timeout)
	{
		if(ESP_AT_WaitReceive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp_at_buf, res) != NULL)		//如果检索到关键词
			{
				ESP_AT_Clear();									//清空缓存
				
				return 0;
			}

			if(strstr((const char *)esp_at_buf, "ERROR") != NULL)
			{
				printf("AT RX: %s\r\n", esp_at_buf);
				ESP_AT_Clear();
				return 1;
			}
		}
		
		delay_ms(10);
	}
	
	printf("AT timeout, expect: %s\r\n", res);
	printf("AT RX: %s\r\n", esp_at_buf);
	return 1;

}

static _Bool ESP_AT_SendCmdRetry(char *cmd, char *res)
{
	unsigned char retry;

	for(retry = 0; retry < ESP_AT_RETRY_COUNT; retry++)
	{
		if(ESP_AT_SendCmd(cmd, res) == 0)
			return 0;

		delay_ms(500);
	}

	return 1;
}

//==========================================================
//	函数名称：	ESP_AT_ConnectServer
//
//	函数功能：	建立到OneNET MQTT服务器的TCP连接
//
//	入口参数：	host：服务器域名
//				port：服务器端口
//
//	返回参数：	0-成功	1-失败
//
//	说明：		ESP-AT单连接模式，连接响应为CONNECT
//==========================================================
_Bool ESP_AT_ConnectServer(const char *host, unsigned short port)
{
	char cmd[128];

	if(host == NULL)
		return 1;

	snprintf(cmd, sizeof(cmd), "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",
				host, (unsigned int)port);

	return ESP_AT_SendCmd(cmd, "CONNECT");
}

//==========================================================
//	函数名称：	ESP_AT_SendData
//
//	函数功能：	发送数据
//
//	入口参数：	data：数据
//				len：长度
//
//	返回参数：	无
//
//	说明：		
//==========================================================
_Bool ESP_AT_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	uint32_t start_tick;
	
	ESP_AT_Clear();								//清空接收缓存
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//发送命令
	if(!ESP_AT_SendCmd(cmdBuf, ">"))				//收到‘>’时可以发送数据
	{
		// Usart_SendString(USART2, data, len);		//发送设备连接请求数据
		HAL_UART_Transmit(&huart2, (uint8_t *)data, len, 500);

		start_tick = HAL_GetTick();
		while((uint32_t)(HAL_GetTick() - start_tick) < 3000)
		{
			if(strstr((const char *)esp_at_buf, "SEND OK") != NULL)
			{
				ESP_AT_Clear();
				return 0;
			}

			if((strstr((const char *)esp_at_buf, "SEND FAIL") != NULL) ||
			   (strstr((const char *)esp_at_buf, "ERROR") != NULL))
			{
				printf("AT send failed: %s\r\n", esp_at_buf);
				ESP_AT_Clear();
				return 1;
			}

			delay_ms(10);
		}

		printf("AT send timeout: %s\r\n", esp_at_buf);
		ESP_AT_Clear();
		return 1;
	}

	return 1;
}

//==========================================================
//	函数名称：	ESP_AT_GetIPD
//
//	函数功能：	获取平台返回的数据
//
//	入口参数：	等待的时间(乘以10ms)
//
//	返回参数：	平台返回的原始数据
//
//	说明：		不同网络设备返回的格式不同，需要去调试
//				如ESP32-C3的返回格式为	"+IPD,x:yyy"	x代表数据长度，yyy是数据内容
//==========================================================
unsigned char *ESP_AT_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	char *ptrColon = NULL;
	char *ptrLen = NULL;
	unsigned short dataLen = 0;
	unsigned short recvLen = 0;
	
	do
	{
		if(ESP_AT_WaitReceive() == REV_OK)								//如果接收完成
		{
			ptrIPD = strstr((char *)esp_at_buf, "IPD,");				//搜索“IPD”头
			if(ptrIPD != NULL)
			{
				ptrColon = strchr(ptrIPD, ':');							//找到':'
				if(ptrColon != NULL)
				{
					//解析"+IPD,<len>:"中的数据长度
					dataLen = 0;
					for(ptrLen = ptrIPD + 4; (ptrLen < ptrColon) && (*ptrLen >= '0') && (*ptrLen <= '9'); ptrLen++)
						dataLen = (unsigned short)(dataLen * 10 + (unsigned short)(*ptrLen - '0'));
					
					recvLen = (unsigned short)(esp_at_cnt - (unsigned short)(ptrColon + 1 - (char *)esp_at_buf));
					
					//必须整包收齐才能返回，否则会把残缺的MQTT报文交给上层解析
					if((ptrLen > ptrIPD + 4) && (recvLen >= dataLen))
						return (unsigned char *)(ptrColon + 1);
					
					//缓存已写满仍未收全，说明数据已丢失，清空后重新同步
					if(esp_at_cnt >= (sizeof(esp_at_buf) - 1U))
					{
						ESP_AT_Clear();
						return NULL;
					}
				}
			}
		}
		
		delay_ms(5);													//延时等待
	} while(timeOut--);
	
	return NULL;														//超时还未找到，返回空指针

}

//==========================================================
//	函数名称：	ESP_AT_Init
//
//	函数功能：	初始化ESP32-C3
//
//	入口参数：	无
//
//	返回参数：	无
//
//	说明：		
//==========================================================
_Bool ESP_AT_Init(void)
{
	
	ESP_AT_Clear();
	HAL_UART_Receive_IT(&huart2, &esp_at_rx_byte, 1);
	
	printf("1. AT\r\n");
	// OLED_Clear(); OLED_ShowString(0,0,"1.AT...",8);
	if(ESP_AT_SendCmdRetry("AT\r\n", "OK"))
		return 1;

	printf("2. ATE0\r\n");
	if(ESP_AT_SendCmdRetry("ATE0\r\n", "OK"))
		return 1;
		
	printf("3. CWMODE\r\n");
	// OLED_ShowString(0,2,"2.CWMODE...",8);
	if(ESP_AT_SendCmdRetry("AT+CWMODE=1\r\n", "OK"))
		return 1;
		
	printf("4. CWJAP\r\n");
	if(ESP_AT_SendCmdRetry(ESP_AT_WIFI_INFO, "OK"))
		return 1;

	printf("5. CIPMUX\r\n");
	if(ESP_AT_SendCmdRetry("AT+CIPMUX=0\r\n", "OK"))
		return 1;

	printf("6. CIPDINFO\r\n");
	ESP_AT_SendCmd("AT+CIPDINFO=0\r\n", "OK");
		
	printf("7. ESP32-C3 Init OK\r\n");
	// OLED_Clear(); OLED_ShowString(0,0,"ESP32-C3 Init OK",16); delay_ms(500);
	return 0;

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == &huart2)
	{
		if(esp_at_cnt < (sizeof(esp_at_buf) - 1U))
		{
			esp_at_buf[esp_at_cnt++] = esp_at_rx_byte;
			esp_at_buf[esp_at_cnt] = '\0';
		}

		HAL_UART_Receive_IT(&huart2, &esp_at_rx_byte, 1);
	}
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart2)
	{
		__HAL_UART_CLEAR_OREFLAG(huart);
		HAL_UART_Receive_IT(&huart2, &esp_at_rx_byte, 1);
	}
}


