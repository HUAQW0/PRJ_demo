#ifndef _ONENET_H_
#define _ONENET_H_





_Bool OneNET_RegisterDevice(void);

_Bool OneNet_DevLink(void);

_Bool OneNet_SendData(void);

_Bool OneNET_Publish(const char *topic, const char *msg);

_Bool OneNET_Subscribe(void);

void OneNet_RevPro(unsigned char *cmd);

#endif
