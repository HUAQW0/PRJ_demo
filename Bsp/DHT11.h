#ifndef DHT11_H
#define DHT11_H

#include "main.h"

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

//PA0
#define DHT11_IO_IN()  {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=8;}
#define DHT11_IO_OUT() {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=3;} 


//IO操作函数   
#define	DHT11_DQ_OUT(X)  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_0, X)
#define	DHT11_DQ_IN  HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)




uint8_t DHT11_Init(void);//初始化DHT11
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);//读取数据
uint8_t DHT11_Read_Byte(void);//读取一个字节
uint8_t DHT11_Read_Bit(void);//读取一位
uint8_t DHT11_Check(void);//检测DHT11
void DHT11_Rst(void);//复位DHT11   

#endif
