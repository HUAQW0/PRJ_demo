#ifndef _ESP_AT_H_
#define _ESP_AT_H_





#define REV_OK		0	//接收完成标志
#define REV_WAIT	1	//接收未完成标志


_Bool ESP_AT_Init(void);

void ESP_AT_Clear(void);

_Bool ESP_AT_SendCmd(char *cmd, char *res);

_Bool ESP_AT_ConnectServer(const char *host, unsigned short port);

_Bool ESP_AT_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP_AT_GetIPD(unsigned short timeOut);


#endif
