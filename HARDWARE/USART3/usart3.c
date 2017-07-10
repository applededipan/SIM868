#include "usart3.h"
#include "usart.h"
#include "delay.h"
#include "timer.h"
#include "string.h"
#include "stdlib.h"
#include "conversion.h"
#include "stdio.h"
#include "stmflash.h"
#include "sim868.h"


Time_Pack u3_time_Pack;		   //ʱ��
u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];  //���ڽ������黺��
u16 receive_len=0;
vu16 USART1_RX_STA=0;
u32 startNum = 0;       //��¼��ǰ�����ַ����ڽ��ջ����еĿ�ʼλ��
u8  stringNum=0;				//���ν��յ����ַ�������
char *GNRMC;		        //������յ����Ƽ�GPS��λ����
char* string[SRTING_NUM];
char UTCTime[7];          //���UTCʱ������ ʱ����
char UTCTime_year[7];     //���������ʱ��
char Longitude_Str[11];	  //����dddmm.mmmm(�ȷ�)��ʽ
char Latitude_Str[10];	  //γ��ddmm.mmmm(�ȷ�)��ʽ
char La_Position[2];     //γ�ȷ�λ
char Lo_Position[2];     //���ȷ�λ
char Ground_speed[10];    //�����ٶ���ȡ
char Azimuth[10];        //��λ��
double Latitude_Temp = 0,Baidu_Latitude_Temp = 0;     //��¼ת���ľ�γ��
double Longitude_Temp = 0,Baidu_Longitude_Temp = 0;   //��¼ת���ľ�γ��
double Drift_speed=0;    //Ư���ٶ�
double BaiduLongitude_Range[GPS_array];      //����(�ٶ�����)
double BaiduLatitude_Range[GPS_array];	     //γ��(�ٶ�����)
double Speed_Range[GPS_array];    //�����ٶ���Ϣ
u8 GPS_array_Count=0;
u8 GPS_effective=0;               //GPS�����Ƿ���Чλ
u8 GPS_receive=0;                 //GPS�жϼ�ʱ��־
//static u8 count=0;                //GPS���մ���
char *GPSState="IMEI GPS  2";
_sysData_Pack sysData_Pack;	      //������Flash�е�������ݵ����ݽṹ
u8 Pack_length=0;                 //���ݴ������
double distance=0;                //���ζ�λ�ľ���
u16 GPS_unspecified_time=0;       //GPS����δ��λ��ʱ��
//��ʼ��IO ����3
//pclk1:PCLK1ʱ��Ƶ��(Mhz)
//bound:������
//��������GPS���� ֻʹ�ܽ��չܽ�
void USART3_Init(u32 bound)
{

    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	// GPIOBʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE); //����3ʱ��ʹ��

    USART_DeInit(USART3);  //��λ����3


    //USART3_RX	  PB11
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOB, &GPIO_InitStructure);  //��ʼ��PB11

    USART_InitStructure.USART_BaudRate = bound;//������һ������Ϊ9600;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
    USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
    USART_InitStructure.USART_Mode = USART_Mode_Rx;	   //ֻ����Ϊ��ģʽ

    USART_Init(USART3, &USART_InitStructure); //��ʼ������	3

    USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ���

    //ʹ�ܽ����ж�
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
    //����ERR�ж�
    USART_ITConfig(USART3, USART_IT_ERR, ENABLE);	    //USART_IT_ERR���������жϣ�֡������������),���ڴ������ݽ��յķ���
    //�رշ����ж�
    USART_ITConfig(USART3, USART_IT_TXE, DISABLE);

    //�����ж����ȼ�
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//��ռ���ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		//�����ȼ�3
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
    NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
}


//GPS���ݻ���������
void Receive_empty(void)
{
    u8 i;
    startNum = 0;
    stringNum = 0;
    receive_len = 0;
    for(i = 0; i < SRTING_NUM; i++)
    {
        string[i] = 0;
    }
}

//UTCʱ��ת����ʱ��
void UTC_Localtime(void)
{
    //UTCСʱ��8hתΪ����ʱ��
    u3_time_Pack.hour+=8;
    if(u3_time_Pack.hour>=24)  //������һ��
    {
        u3_time_Pack.hour-=24;
        u3_time_Pack.day+=1;  //������1
        switch(u3_time_Pack.month)
        {
        case 1:     //����
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            if(u3_time_Pack.day>31)
            {
                u3_time_Pack.day-=31;
                u3_time_Pack.month+=1;     //�·ݼ�1
                if(u3_time_Pack.month>12)
                {
                    u3_time_Pack.month-=12;
                    u3_time_Pack.year+=1;
                }

            }
            break;
        case 4:             //С��
        case 6:
        case 9:
        case 11:
            if(u3_time_Pack.day>30)
            {
                u3_time_Pack.day-=30;
                u3_time_Pack.month+=1;     //�·ݼ�1
                if(u3_time_Pack.month>12)
                {
                    u3_time_Pack.month-=12;
                    u3_time_Pack.year+=1;
                }

            }
            break;
        case 2:
            //���ж�ƽ�껹������
            if((u3_time_Pack.year%400==0)||(u3_time_Pack.year%4==0&&u3_time_Pack.year%100!=0)) //����
            {
                if(u3_time_Pack.day>29)
                {
                    u3_time_Pack.day-=29;
                    u3_time_Pack.month+=1;
                }
            }
            else   //ƽ��
            {
                if(u3_time_Pack.day>28)
                {
                    u3_time_Pack.day-=28;
                    u3_time_Pack.month+=1;
                }
            }
            break;
        }
    }

    if(u3_time_Pack.year<2017||	(u3_time_Pack.month>12&&u3_time_Pack.month<1)||
            (u3_time_Pack.day>31&&u3_time_Pack.day<1)||
            u3_time_Pack.hour>24||
            u3_time_Pack.minute>60||
            u3_time_Pack.second>60)          //ת����ʱ����ִ���
    {   //������0
        u3_time_Pack.year=0;
        u3_time_Pack.month=0;
        u3_time_Pack.day=0;
        u3_time_Pack.hour=0;
        u3_time_Pack.minute=0;
        u3_time_Pack.second=0;
    }

}



//�Ƽ���λ��Ϣ��GPS���ݵĽ���
//$GNRMC,000021.262,V,,,,,0.00,0.00,060180,,,N*47
//��ʽ:$GNRMC,<1>,<2>,<3>,<4>,<5>,<6>,<7>,<8>,<9>,<10>,<11>,<12>*<13><CR><LF>
void analysisGPS(void)
{
    u32 i=0;           //forѭ������
    u32 commaNum = 0;  //���Ÿ���
    u32 lenth=0;       //���ݵĳ���
    char * start = 0;  //���ݵ�ͷ��ַ
    char * end = 0;    //����β��ַ
    char * str = 0;
	  char * str1=0;
    u8 checksum=0,recCheckSum=0;   //GPS����У��
//    static u8  first_time= 0;      //�Ƿ��һ�λ�ȡ����λ���� 0:�� 1:��
    char *pEnd;          //����ת�����ַ���ָ��
    commaNum = charNum(GNRMC,',');  //��У��GPS��������
    if(commaNum==12)   //��һ��������GPS����
    {
        //����У���
        //strstr()�������������Ӵ����ַ������״γ��ֵ�λ��
        start=strstr(GNRMC,"GNRMC");   //�õ���һ����ַ
        end=strstr(GNRMC,"*");
        lenth=(u32)(end-start);
        for(i=0; i<lenth; i++)
        {
            checksum=checksum^start[i];   //�����������õ�У���
        }
        //���ַ���������ת��Ϊ��������
        recCheckSum=strtol(end+1,&pEnd,16);  //$��*֮�������ַ�ASCII���У��ͣ����ֽ���������㣬�õ�У��ͺ���ת��16���Ƹ�ʽ��ASCII�ַ�����

        if(recCheckSum==checksum)
        {
//            printf("����У��ɹ�\r\n");
            str = ReturnCommaPosition(GNRMC,2);   //���Ҷ�λ״̬��A=��λ��V=δ��λ
            if(*(str+1)=='A')              //��Ч��λ
            {
							
							
							//strncpy()�Ὣ�ַ���srcǰn���ַ��������ַ���dest��
							
                GPS_unspecified_time=0;               //����δ��λ������0
                //���и����ݵ���ȡ
                str = ReturnCommaPosition(GNRMC,1);  //hhmmss��ʱ����) ʱ�����ȡ
                strncpy(UTCTime,str+1,6);         //��ǰ�������ݿ���������,strncpy������ĩβ���������ַ���������
                UTCTime[6]=(u8)'\0';    //�����ַ���β

                str=ReturnCommaPosition(GNRMC,3);   //γ�ȵ���ȡ
                strncpy(Latitude_Str,str+1,9);
                Latitude_Str[9]='\0';
                
							  str=ReturnCommaPosition(GNRMC,4);   //γ�ȵķ�λ��ȡ
                strncpy(La_Position,str+1,1);
                La_Position[1]='\0';
							
                str=ReturnCommaPosition(GNRMC,5);  //���ȵ���ȡ
                strncpy(Longitude_Str,str+1,10);
                Longitude_Str[10]='\0';

							  str=ReturnCommaPosition(GNRMC,6);  //���ȵķ�λ����ȡ
                strncpy(Lo_Position,str+1,1);
                Lo_Position[1]='\0';
								
                str=ReturnCommaPosition(GNRMC,7);  //�������ʵ���ȡ
								str1=ReturnCommaPosition(GNRMC,8);
                strncpy(Ground_speed,str+1,str1-str-1);
                Ground_speed[str1-str-1]='\0';
                 
								str=ReturnCommaPosition(GNRMC,8);   //��λ�ǵ���ȡ
								str1=ReturnCommaPosition(GNRMC,9);
                strncpy(Azimuth,str+1,str1-str-1);
                Azimuth[str1-str-1]='\0';
								
                str = ReturnCommaPosition(GNRMC,9); //UTC���� ddmmyy�������꣩��ʽ
                strncpy(UTCTime_year,str+1,6);
                UTCTime_year[6]='\0';

                //��������ת��
//                Drift_speed=atof(Ground_speed);  //���ַ���ת��Ϊdouble��ʽ���ٶ�
//                Drift_speed=Drift_speed*1.58;      //�������� ����ʱתǧ��ʱ
                //γ��ת��
//                Latitude_Temp=atof(Latitude_Str);    //���ַ���ת��Ϊdouble��ʽ�ľ���
//                Latitude_Temp=(int)Latitude_Temp/100+(Latitude_Temp-((int)Latitude_Temp/100)*100)/60; //ת��Ϊ��
//                //����ת��
//                Longitude_Temp=atof(Longitude_Str);
//                Longitude_Temp=(int)Longitude_Temp/100+(Longitude_Temp-((int)Longitude_Temp/100)*100)/60;

//                GPS_transformation(Latitude_Temp,Longitude_Temp,&Baidu_Latitude_Temp,&Baidu_Longitude_Temp);	//��ʵ����ת�ٶ�����

//                if((Baidu_Longitude_Temp < 72)||(Baidu_Longitude_Temp > 136)||
//                        (Baidu_Latitude_Temp < 18)||(Baidu_Latitude_Temp > 54))   //��������й���Χ
//                {
//                    printf("�������й��ķ�Χ");
//                    return;
//                }
//                printf("��ʵGPS����%f,%f\r\n",Latitude_Temp,Longitude_Temp);  //��ǰ��GPS����
//                printf("�ٶ�GPS����%f,%f\r\n",Baidu_Latitude_Temp,Baidu_Longitude_Temp);

                //10��GPS����������һ�����飬ÿ��һ�����������ֻ�
//                if(first_time++==0)    //�ǵ�һ��
//                {   //���ж�������
//                    for(i=0; i<GPS_array; i++)	 //������������
//                    {
//                        BaiduLatitude_Range[i] = Baidu_Latitude_Temp;
//                        BaiduLongitude_Range[i] = Baidu_Longitude_Temp;
//                        Speed_Range[i]=Drift_speed;   //�ٶ���Ϣ
//                    }
//                    GPS_array_Count=1;
//                }

//                else  //���ǵ�һ�ν��յ�����
//                {
//                    BaiduLatitude_Range[GPS_array_Count]=Baidu_Latitude_Temp;
//                    BaiduLongitude_Range[GPS_array_Count]=Baidu_Longitude_Temp;
//                    Speed_Range[GPS_array_Count]=Drift_speed;
//                    GPS_array_Count++;                  //���θı�����
//                    if(GPS_array_Count>=GPS_array)
//                    {
//                        GPS_array_Count=0;
//                    }
//                }

                //091202������ĸ�ʽ 083559.00ʱ����ĸ�ʽ   ʱ�����ȡ
//                u3_time_Pack.year=2000+(UTCTime_year[4]-'0')*10+(UTCTime_year[5]-'0');   //�����ȡ
//                u3_time_Pack.month=(UTCTime_year[2]-'0')*10+UTCTime_year[3]-'0';         //�µ���ȡ
//                u3_time_Pack.day=(UTCTime_year[0]-'0')*10+UTCTime_year[1]-'0';           //��
//                u3_time_Pack.hour=(UTCTime[0]-'0')*10+UTCTime[1]-'0';                    //ʱ
//                u3_time_Pack.minute=(UTCTime[2]-'0')*10+UTCTime[3]-'0';                  //��
//                u3_time_Pack.second=(UTCTime[4]-'0')*10+UTCTime[5]-'0';                  //��



//                UTC_Localtime();                  //UTCʱ��ת����ʱ��



                GPS_effective=1;                  //��ǻ�ȡ��GPS������Ч
                printf("A��λ��Ч\r\n");

            }
            else  //GPS��λ��Ϣ��Ч
            {
                GPS_effective=0;
                printf("GPSδ�ɹ���λ\r\n");
                GPS_unspecified_time++;                 //δ��λ����+1
            }
        }
        else  //����û��У��ɹ�
        {
            printf("����У���ʧ��");
            return;
        }

    }
    else
    {
        printf("û��12������\r\n");
    }
}

//�˲������㷨��ȥ��һ�����ֵ����Сֵ��������ƽ��ֵ
double GPS_filter(double data[])
{
    u8 i=0;
    double Min=0,Max=0,Average=0,Sum=0;
    Min=Max=data[1];
    for(i=0; i<GPS_array; i++)
    {
        if(data[i]<Min)
        {
            Min=data[i];
        }
        if(data[i]>Max)
        {
            Max=data[i];
        }
        Sum+=data[i];
    }
    Average=(Sum-Max-Min)/(GPS_array-2);
    return Average;
}


void getFilterLoc(double * Lat_filter,double * Lon_filter ,double * Spend_filter)     //��ȡ��10��GPS���ݽ����˲�����
{
    *Lat_filter=GPS_filter(BaiduLatitude_Range);
    *Lon_filter=GPS_filter(BaiduLongitude_Range);
	  *Spend_filter=GPS_filter(Speed_Range);
}


//Drift_speed
void GPS_Packed_Data(void)   //GPS���ݴ���ϴ�
{

    sprintf(u3_time_Pack.Time,"%04d-%02d-%02d %02d:%02d:%02d",             //2016-01-14 05:24:19 ʱ�����ݷ�������
            u3_time_Pack.year,u3_time_Pack.month,u3_time_Pack.day,
            u3_time_Pack.hour,u3_time_Pack.minute,u3_time_Pack.second);


    Pack_length=sprintf(TEXT_Buffer_1,"Time=%s-Log=%s,%04d,%03d,%03d,%03d,%03f,sQ: %02d",   //һ�����ݴ��
                        u3_time_Pack.Time,
                        u1_data_Pack.IMEI,                          //ģ�����
                        sysData_Pack.data.bootTimes,      //������������
                        sysData_Pack.data.SIM868_bootTimes, //�ϴ���������ʧ�ܴ���
                        sysData_Pack.data.postErrorTimes,  //POSTʧ�ܴ���
                        sysData_Pack.data.GPSErrorTimes,
                        Spend_Now,
                        signalQuality
                       );
    printf("��������ϴ���С%d\r\n",Pack_length);
    GPS_PACK=1;           //��Ǵ�����
}


void USART3_IRQHandler(void)
{
    u32 i;//forѭ������
    if(USART_GetITStatus(USART3, USART_IT_RXNE) == SET)//���յ�����
    {
//        USART_ClearITPendingBit(USART3,USART_IT_RXNE);       //����ж�Ԥ����λ
        USART3_RX_BUF[receive_len++] = USART_ReceiveData(USART3);//��ȡ���յ�������

        if((USART3_RX_BUF[receive_len - 2] == '\r')||
                (USART3_RX_BUF[receive_len - 1] == '\n'))//����յ��س����У�������ǰ�ַ�������
        {
            GPS_receive=1;        //���������ݲ���;


            if(receive_len - startNum > 2)      //����ַ�������С�ڵ���2��֤��Ϊ���ַ��������ý���
            {
                string[stringNum] = (char *)USART3_RX_BUF + startNum;//�����ַ����׵�ַ
                USART3_RX_BUF[receive_len - 2] = (u8)'\0'; //�����ַ���������
                if((string[stringNum][0] == '$')&&
                        (string[stringNum][1] == 'G')&&
                        (string[stringNum][2] == 'N')&&
                        (string[stringNum][3] == 'R')&&
                        (string[stringNum][4] == 'M')&&
                        (string[stringNum][5] == 'C'))	      //�ж��Ƿ���յ��Ƽ�GPS��λ��Ϣ
                {

                    GNRMC = string[stringNum];
                    if(GNRMC != 0)
                    {
                        analysisGPS();//����GPS����
                    }
                }
                stringNum++;//�ַ�����������1        	//һ�������յ��ַ�������20

            }
            startNum = receive_len;
        }
        if((receive_len >= USART3_MAX_RECV_LEN)||(stringNum >= SRTING_NUM))   //�����˽��յķ�Χ
        {
            startNum = 0;
            stringNum = 0;
            receive_len = 0;
            for(i = 0; i < SRTING_NUM; i++)
            {
                string[i] = 0;
            }
        }
        USART_ClearITPendingBit(USART3,USART_IT_RXNE);       //����ж�Ԥ����λ
    }
    //���-������������Ҫ�ȶ�SR,�ٶ�DR�Ĵ��� �������������жϵ�����
    if(USART_GetFlagStatus(USART3,USART_FLAG_ORE) == SET)    //USART_FLAG_ORE��������־
    {
        USART_ClearFlag(USART3,USART_FLAG_ORE);	  //��SR
        USART_ReceiveData(USART3);				        //��DR
    }
}



//���GPS������
//��������
//����ֵ ��
void cleanReceiveData_GPS(void)
{

    u32 i;
    startNum = 0;
    stringNum = 0;             //���ν��յ����ַ���������0
    receive_len = 0;           //���ռ�����

    for(i = 0; i < USART3_MAX_RECV_LEN; i++)
    {
        USART3_RX_BUF[i]=0;
    }
    for(i = 0; i < SRTING_NUM; i++)
    {
        string[i] = 0;
    }
}




