#include "timer.h"
#include "sim868.h"
#include "stdio.h"
#include "stmflash.h"
#include "iwdg.h"
#include "conversion.h"
#include "sim868.h"
#include "led.h"


u32  MeanwhileHeart = MeanwhileHeart_MAX;    //��ѭ������
u32 GPS_Upload=0;    //GPS���ݴ����־
double 	LatNow = 0,LonNow = 0,LatOld=0, LonOld=0 ,Spend_Now;//��¼�ϴ�ʱ��γ�� ����  ���ٶ�
u32 GPS_invalidtimes=0;
u32 Upload_Time=0;      //�ϴ�������ʱ����
u32 Heartbeat_Upload=0;  //�����ϴ���־λ
u8 submit_info_flag=0;   //����GPS��־λ
u8 GPS_PACK=0;           //���δ����ɱ�־λ
u8 waitservice_flag=0;   //�ȴ���������ʱ��־
u8 LEDshine_flag=0;
u8 dabao=0;
//ͨ�ö�ʱ��3�жϳ�ʼ��
//����ʱ��ѡ��ΪAPB1��2������APB1Ϊ36M
//arr���Զ���װֵ��
//psc��ʱ��Ԥ��Ƶ��


void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��

    //��ʱ��TIM3��ʼ��
    TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ж�,��������ж�

    //�ж����ȼ�NVIC����
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //��ռ���ȼ�0��
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //�����ȼ�3��
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
    NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���

    TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx
}

//��ʱ��3�жϷ������
//ÿ100ms��һ���жϷ�����
void TIM3_IRQHandler(void)
{
    static u32 T=0;
    static u32 times=0;              //����������ʱ����
    static u8 tt=0;                  //�ܵĶ�ʱ����
    static u32 waitservicetime = 0;  //�������ȴ��ļ�ʱ
    if(TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)    //�Ǹ����ж�
    {
        TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //���TIMx�����жϱ�־

        T++;
        if(T%10==0)    //ÿ1s  ������һ
        {
            if(MeanwhileHeart!=0)   //������Ϊ0��ʱ���ټ�����
                MeanwhileHeart --;   //��ѭ������
        }
        if(u1_data_Pack.Error==0)   //û�г���
        {
            if(MeanwhileHeart!=0 )   //���Ź���λ
                IWDG_Feed(); //ι��
            else
                printf("�ﵽ����\r\n");
        }
        switch(deviceState)
        {
        case 0:LED1=1;break;
        case 1:
        case 2:
				case 3:
        case 4:if(T%5==0)
			     LED1=!LED1;
            break;
        case 5:    //��������״̬ gps��
        {
						LED1=0;
            times++; //���ڼ�ʱ��
            if(times%5== 0)  //ÿ0.5����һ�ε�
            {
                if(GPS_effective&GPRS_status)
                    LED0=!LED0;
                else if((GPS_effective==1)&&(GPRS_status!=1))   //ֻ��GPS����
                    LED0=1;
                else
                    LED0=0;                //GPS������
            }

            if(submit_info_flag==0)     //�Ż��������ϴ��ϴ���ʱ��ļ��
            {
                tt++;
                if(tt%150==0)   //ÿ30���ϴ�һ��
                {
                    submit_info_flag=1;
                }
            }
            if(times%10==0)       //ÿ��������1s
            {

                if(GPS_effective==1)    //GPS������Ч
                {
                    Upload_Time=0;      //�ϴ�������������
                    GPS_effective=0;    //GPS���ݱ��Ϊ��Ч
                    if(times%150==0)     //ÿ15s���һ���˲���Ϣ
                    {
                        //ֻ��ǰ�����εľ��볬��40m�ű�֤�������
                        getFilterLoc(&LatNow, &LonNow,&Spend_Now);   //�õ��˲���ľ�γ��    �������洢�����ĵ�ַ���룬����һ��ָ�����
                        printf("�˲���İٶȵ�ͼγ��%f,����%f\r\n",LatNow,LonNow);
                        distance=Cal_Distance(LatOld,LonOld,LatNow,LonNow)*1000;       //�������ζ�λ��   (��һ�ζ�λ�ľ�����ܴ�)
                        printf("\r\n�����ϴ��ľ���Ϊ%lfm\r\n",distance);
                        LatOld=LatNow;           //������һ�μ������
                        LonOld=LonNow;
                        dabao=1;

                    }
                    GPS_invalidtimes=0;   //GPSδ��λ��ʱ����0
                }
                else
                {
                    GPS_effective=0;
                }
                Upload_Time++;   //�ϴ�ʱ����

                if(Upload_Time>=Upload_Time_MAX)    //����2����û���ϴ�GPS����
                {
                    Heartbeat_Upload=1;   //�����ϴ���־
                }
            }

            if(waitservice_flag==0)// ��־λ��������Ϊ�ȴ������м�ʱ�ȴ�
            {
                waitservicetime++;
                if((waitservicetime%100)==0)
                {
                    waitservice_flag=1;
                    waitservicetime=0;
                }
            }
            break;
        }

        }
    }
}









