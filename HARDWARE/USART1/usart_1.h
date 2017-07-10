#ifndef __USART_1_H
#define __USART_1_H
#include "sys.h"
#include "stdio.h"

#define USART1_MAX_RECV_LEN		5000	//�����ջ����ֽ���
#define USART1_MAX_SEND_LEN		2000	//����ͻ����ֽ���
#define USART1_RX_EN 			1		//0,������;1,����.
#define SRTING_NUM	 			20		//һ�������յ��ַ�������

extern void USART1_Init(u32 bound); //����1��ʼ������
void u1_printf(char* fmt,...);    //����1��ӡ����
#endif



