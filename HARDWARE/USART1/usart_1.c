#include "delay.h"
#include "usart_1.h"
#include "stdarg.h"	 	  	 
#include "string.h"	 
#include "timer.h"
#include "usart3.h"
#include "sim868.h"

void USART1_Init(u32 bound)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	// GPIOAʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE); //����2ʱ��ʹ��

	USART_DeInit(USART1);  //��λ����2
	//USART1_TX   PA9
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��

	//USART1_RX	  PA10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��

	USART_InitStructure.USART_BaudRate = bound;//������һ������Ϊ115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	USART_Init(USART1, &USART_InitStructure); //��ʼ������1


	USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 

	//ʹ�ܽ����ж�
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); 
	//����ERR�ж�
	USART_ITConfig(USART1, USART_IT_ERR, ENABLE);	
	//�رշ����ж�
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE); 

	//�����ж����ȼ�
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	
	
//	u1_data_Pack.USART1_RX_STA=0;		//����
}

//����1�жϷ�����
//�����յ������ݴ�����ջ����У������ַ�������
//�����ж��Ƿ��յ�GPS����
void USART1_IRQHandler(void)
{
	u8 res;	      
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)//���յ�����
	{
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);		
		res =USART_ReceiveData(USART1);		 
		if(u1_data_Pack.USART1_RX_STA<USART1_MAX_RECV_LEN)	//�����Խ�������
		{			//���������
			   u1_data_Pack.USART1_RX_BUF[u1_data_Pack.USART1_RX_STA++]=res;	//��¼���յ���ֵ	 
		}
	} 	
	
	//���-������������Ҫ�ȶ�SR,�ٶ�DR�Ĵ��� �������������жϵ�����
    if(USART_GetFlagStatus(USART1,USART_FLAG_ORE) == SET)
    {
        USART_ClearFlag(USART1,USART_FLAG_ORE);	//��SR
        USART_ReceiveData(USART1);				//��DR
    }
}  
//����1,printf ����
//ȷ��һ�η������ݲ�����USART1_MAX_SEND_LEN�ֽ�
void u1_printf(char* fmt,...)
{  
	u16 i,j; 
	va_list ap; 
	va_start(ap,fmt);
	vsprintf((char*)u1_data_Pack.USART1_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)u1_data_Pack.USART1_TX_BUF);		//�˴η������ݵĳ���
	for(j=0;j<i;j++)							//ѭ����������
	{
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)==RESET); //ѭ������,ֱ���������   
		USART_SendData(USART1,u1_data_Pack.USART1_TX_BUF[j]); 
	} 
}
