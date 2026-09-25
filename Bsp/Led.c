#include "Led.h"

uint8_t Led_Status;

void Led_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void Led_Set(_Bool status)
{
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, status == LED_ON ? GPIO_PIN_RESET : GPIO_PIN_SET);
    Led_Status = status;
}