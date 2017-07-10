#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "delay.h"
#include "timer.h"
#include "usart.h"
#include "usart_1.h"
#include "stmflash.h"
#include "sim868.h"

u8 firstsend_flag=1;           //����Ƿ�Ϊ��һ�η���  ����Ϊ�ڴ�AGPS��ʱ�򣬻�������ӣ��ڷ���ָ���ʱ����Ҫ�ر��������
u8 Continue_Post_Errotimes=0;  //����ʧ�ܴ���
u8 Post_Errotimes=0;           //����SIM868����һ���ϴ�ʧ�ܴ���
u8 Heart_error=0;              //�����ϴ�ʧ��λ
u8 deviceState = 0;            //�豸��ǰ״̬ 0:���� 1:��ʼ��ATָ�� 2:���SIM�� 3:�����ź� 4:��ʼ��GPRS̬ 5:��������̬
Data_Pack u1_data_Pack;        //SIM868�����Ϣ
u8 GPRS_status=0;              //TCP�Ƿ�����״̬,���TCP����û�г�����ôΪ
LBS_Info LBS_PACK;


char p1[300];
char POSTDATA[1000]= "*HQ,8800000999,V1,145956.000,A,2927.335567,N,10631.592282,E,0.42,196.89,190517,FFFFFBFF#";
u8 SendType=0 ;

//��ʼ��SIM868����

void SIM868_Init()
{
    s8 ret;
	char *res=0;
    u32 i = 0;
    deviceState = 0;  //����̬
	firstsend_flag=1;
    cleanReceiveData_GPS();
	sysData_Pack.data.SIM868_bootTimes++;
    deviceState=1;    //��ʼ��ATָ��̬
    /*��ʼ��ATָ�*/
    i = 0;
    while((ret = sendAT("AT\r\n","OK",500)) != 0)
    {
        printf("���ڳ�ʼ��ATָ�```%d\r\n",i++);
        if(i >= 50)
        {
            printf("\r\n��ʼ��ATָ�ʧ�ܣ�����������Ƭ��\r\n");
			saveData();
            u1_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
            while(1);	//�ȴ����Ź���λ
        }
    }
    if(ret != 0) {
        printf("��ʼ��ATָ�ʧ��\r\n");
    }
    else
    {
        printf("��ʼ��ATָ��ɹ�\r\n");
        /*�رջ���*/
        ret=sendAT("ATE0","OK",5000);
        if(ret == 0) {
            printf("�رջ��Գɹ�\r\n");
        }
        else {
            printf("�رջ���ʧ��\r\n");
        }

        /*���ϱ���ϸ������Ϣ*/
        ret=sendAT("AT+CMEE=2","OK",5000);
        if(ret == 0) {
            printf("���ϱ���ϸ������Ϣ�ɹ�\r\n");
        }
        else {
            printf("���ϱ���ϸ������Ϣʧ��\r\n");
        }
        /*��ѯIMEI��*/
        getIMEI();
        deviceState = 2;//���SIM��̬
        /*��ѯ�Ƿ��⵽SIM��*/
        i = 0;
        while((ret = sendAT("AT+CPIN?","READY",3000)) != 0)//ֻ�в���SIM���Ž��н������Ĳ���
        {
            printf("���ڲ�ѯSIM��״̬```%d\r\n",i++);
            if(i >= 5)
            {
                printf("\r\n���SIM��ʧ�ܣ�����������Ƭ��\r\n");
								saveData();
                u1_data_Pack.Error = 1;//��ǳ�ʼ��SIM808�����ȴ����Ź���λ��Ƭ��
                while(1);	//�ȴ����Ź���λ
            }
        }
        if(ret == 0) {
            printf("�Ѳ���SIM��\r\n");
        }
        else {
            printf("δ����SIM��\r\n");
        }
		i=0;
		deviceState = 3;  //ע������̬
        /*��ѯ����ע�����*/
        while(1)
		{
			sendAT("AT+CREG?","+CREG:",3000);
			res=my_strstr((char *)u1_data_Pack.USART1_RX_BUF,(char * )"+CREG:");
			if(*(res+9) =='1'||*(res+9) =='5')//����"0��1"����"0,5"
			{
				printf("�ɹ�ע������\r\n");
				break;
			}
			i++;
			if(i>=40)				
			{
				printf("ע������ʧ��,׼������\r\n");
				saveData();
				u1_data_Pack.Error = 1;//��ǳ�ʼ��SIM808�����ȴ����Ź���λ��Ƭ��
                while(1);	//�ȴ����Ź���λ
			}
            printf("����ע������....%d\r\n",i);
			delay_ms(1500);			
        }
        deviceState = 4;//��ʼ��GPRS̬
		GPRS_Connect(); //��GPRS����
		SIM868_GPRS_Test();
        SIM868_GPS_Init();
        deviceState = 5;//��������̬
    }
}
//��ѯģ���Ƿ���GPRS����
//�����Ƿ���GPRS���� 0������ -1��δ����
s8 SIM868_GPRS_Test(void)
{
    s8 ret;
    char * str = 0;
    /*��ѯ�ź�����*/
    ret = sendAT("AT+CSQ","OK",5000);
    if(ret != 0)
    {
        printf("��ѯ�ź�����ʧ��\r\n");
        signalQuality=99;
    }
    else
    {
        if(extractParameter(my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"+CSQ:"),"+CSQ: ",&signalQuality,0,0) == 0)
        {
             printf("�ź�������%d\r\n",signalQuality);
        }
    }
    /*��ѯģ���Ƿ���GPRS����*/
    ret = sendAT("AT+CGATT?","+CGATT:",1000);
    if(ret != 0) {
        return -1;
    }
    str = my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"+CGATT:");
    if(str[8] == '1')//�Ѹ���GPSR����
    {
        printf("����GPRS����ɹ�\r\n");
		GPRS_status=1;
        return 0;
    }
    else//δ����GPSR���磬���Ը���GPRS����
    {
		GPRS_status=0;
		return  -1;
    }
}

//�ڴ��ڽ��յ��ַ���������
//����ֵ		�ɹ����ر������ַ������׵�ַ
//			ʧ�ܷ��� 0
char * my_strstr(char *FirstAddr,char *searchStr)
{

    char * ret = 0;
	u16 i;
	for(i=0;i<u1_data_Pack.USART1_RX_STA;i++)
	{
		if(u1_data_Pack.USART1_RX_BUF[i]==*(searchStr))
		{
			ret= strstr((char *)u1_data_Pack.USART1_RX_BUF+i,searchStr);
			if(ret!=0)
			  break;
		}
		if((i==USART1_MAX_RECV_LEN)||ret!=0)
			break;
	}

    return ret;
}

//���������
//��������
//����ֵ ��
void cleanReceiveData(void)
{
    u16 i;
    u1_data_Pack.USART1_RX_STA=0;			//���ռ���������
    for(i = 0; i < USART1_MAX_RECV_LEN; i++)
    {
        u1_data_Pack.USART1_RX_BUF[i] = 0;
    }
}



//����һ��ATָ�������Ƿ��յ�ָ����Ӧ��
//sendStr:���͵������ַ���,��sendStr<0XFF��ʱ��,��������(���緢��0X1A),���ڵ�ʱ�����ַ���.
//searchStr:�ڴ���Ӧ����,���Ϊ��,���ʾ����Ҫ�ȴ�Ӧ��
//outTime:�ȴ�ʱ��(��λ:1ms)
//����ֵ:0,���ͳɹ�(�õ����ڴ���Ӧ����)
//       -1,����ʧ��
s8 sendAT(char *sendStr,char *searchStr,u32 outTime)
{
	u16 i;
    s8 ret = 0;
    char * res =0;
//	outTime=outTime/10;
    cleanReceiveData();//���������
    if((u32)sendStr < 0xFF)
    {
        while((USART1->SR&0X40)==0);//�ȴ���һ�����ݷ������
        USART1->DR=(u32)sendStr;
    }
    else
    {
        u1_printf(sendStr);//����ATָ��
        u1_printf("\r\n");//���ͻس�����
    }
    if(searchStr && outTime)//��searchStr��outTime��Ϊ0ʱ�ŵȴ�Ӧ��
    {
        while((--outTime)&&(res == 0))//�ȴ�ָ����Ӧ���ʱʱ�䵽��
        {
			res = my_strstr((char *)u1_data_Pack.USART1_RX_BUF,searchStr);
			if(res!=0)
				break;
			if((i==USART1_MAX_RECV_LEN)||res!=0)
				break;
			delay_ms(1);
        }
        if(outTime == 0) {
            ret = -1;    //��ʱ
        }
        if(res != 0)//res��Ϊ0֤���յ�ָ��Ӧ��
        {
            ret = 0;
        }
    }
	delay_ms(50);
    return ret;
}


//��ѯģ��汾��
void checkRevision(void)
{
    s8 ret;
    char * str;
    /*��ѯģ������汾*/
    ret = sendAT("AT+GMR","OK",5000);
    if(ret == 0) {
        printf("��ѯģ������汾�ɹ�\r\n");
    }
    else
    {
        printf("��ѯģ������汾ʧ��\r\n");
		saveData();
        u1_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
    str = my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"V");
    if(str != 0)
    {
        strncpy(u1_data_Pack.revision, str , 20);
        u1_data_Pack.revision[20] = '\0';//����ַ���������
        printf("ģ������汾: %s  \r\n",u1_data_Pack.revision);
    }
    else
    {
        printf("��ѯģ������汾ʧ��\r\n");
				saveData();
        u1_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
}

//��ѯIMEI
void getIMEI(void)
{
    s8 ret;
    char * str;
    /*��ѯIMEI��*/
    ret = sendAT("AT+GSN","OK",5000);
    if(ret == 0) {  }
    else
    {
        printf("��ѯIMEI��ʧ��\r\n");
				saveData();
        u1_data_Pack.Error = 1;//��ǳ�ʼ��Ai_A6�����ȴ����Ź���λ��Ƭ��
    }
    str = my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"OK");
    if(str != 0)
    {
        strncpy(u1_data_Pack.IMEI, str - 19, 15);
        u1_data_Pack.IMEI[15] = '\0';//����ַ���������
        printf("IMEI��: %s\r\n",u1_data_Pack.IMEI);
    }
}






//POST /webservice.asmx/GpsSubmit HTTP/1.1
//Host: gpsws.cqutlab.cn
//Content-Type: application/x-www-form-urlencoded
//Content-Length: length

//lat=string&lon=string&IMEI=string&log=string


u8 isSendData = 0;		//�Ƿ��ںͷ�����ͨ�ŵı�־λ
u8 isSendDataError = 0;	//�ϴ��������Ƿ�ʧ�� 0:Ϊ�ɹ� 1:ʧ��






//��ȡ�ַ����еĲ���(��������������������)
//str:Դ�ַ�����searchStr:����ǰ�������ַ���,data1:��ȡ���Ĳ���1 data2:��ȡ���Ĳ���2 data3:��ȡ���Ĳ���3
//�ɹ�����0 ʧ�ܷ���-1

s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree)
{
    char *str;
    char *commaOneAddr;//��һ�����ŵĵ�ַ
    char *commaTwoAddr;//�ڶ������ŵĵ�ַ
    char dataStrOne[10], dataStrTwo[10], dataStrThree[10];
    u32 commaNum = 0,lenthOne = 0,lenthTwo = 0,lenthThree = 0,lenthEigen = 0;

    str = strstr(sourceStr,searchStr);
    if(str == 0) {
        return -1;
    }
    else
    {
        commaNum = charNum(sourceStr,',');//�����ж��ٶ���
        if(commaNum == 1)//����������
        {
            lenthEigen = strlen(searchStr);//�õ������ַ����ĳ���
            commaOneAddr = ReturnCommaPosition(str,1);//�õ���һ������λ��
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//�õ���һ�������ĳ���
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//����ַ���������
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //���ַ���ת��Ϊ����
            }

            lenthTwo = strlen(commaOneAddr + 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrTwo[lenthTwo] = '\0';//����ַ���������
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //���ַ���ת��Ϊ����
            }
            return 0;
        }
        if(commaNum >= 2)//���������ϲ���
        {
            lenthEigen = strlen(searchStr);//�õ������ַ����ĳ���
            commaOneAddr = ReturnCommaPosition(str,1);//�õ���һ������λ��
            lenthOne = (u32)(commaOneAddr - str) - lenthEigen;//�õ���һ�������ĳ���
            strncpy(dataStrOne, sourceStr + lenthEigen, lenthOne);
            dataStrOne[lenthOne] = '\0';//����ַ���������
            if(dataOne != 0) {
                *dataOne = atoi(dataStrOne);    //���ַ���ת��Ϊ����
            }

            commaTwoAddr = ReturnCommaPosition(str,2);//�õ��ڶ�������λ��
            lenthTwo = (u32)(commaTwoAddr - commaOneAddr - 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrTwo, commaOneAddr + 1, lenthTwo);
            dataStrOne[lenthTwo] = '\0';//����ַ���������
            if(dataTwo != 0) {
                *dataTwo = atoi(dataStrTwo);    //���ַ���ת��Ϊ����
            }

            lenthThree = strlen(commaTwoAddr + 1);//�õ��ڶ��������ĳ���
            strncpy(dataStrThree, commaTwoAddr + 1, lenthThree);
            dataStrThree[lenthTwo] = '\0';//����ַ���������
            if(dataThree != 0) {
                *dataThree = atoi(dataStrThree);    //���ַ���ת��Ϊ����
            }
            return 0;
        }
    }
    return -1;
}


//�������ƣ����ض���λ��
//���������p	������ַ����׵�ַ
//		  num	�ڼ�������
//���ز�������num�����ŵ�ַ
//˵    ������65535���ַ���û�ж��žͷ����ַ����׵�ַ
char * ReturnCommaPosition(char *p ,u8 num)
{
    u8 i=0;			//��i������
    u16 k=0;		//��ַ����ֵ
    while(k<65535)
    {
        if( *(p+k) ==',')
        {
            i++;
            if(i==num)
            {
                return p+k;
            }
        }
        k++;
    }
    return p;
}
//�������ƣ������ַ������ж��ٸ�ָ���ַ�
//���������p	������ַ����׵�ַ
//		  	Char	���������ַ�
//���ز������ַ������ж��ٸ�ָ���ַ�

u8 charNum(char *p ,char Char)
{
    u8 i=0;			//��i������
    u16 k=0;		//��ַ����ֵ
    while(1)
    {
        if( *(p+k) == Char)
        {
            i++;
        }
        else if(*(p+k) == '\0')
        {
            return i;
        }
        k++;
    }
}

void PWR_Init(void)
{
	  GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
		PBout(15)=1;                              //Ĭ��Ϊ�ߣ�ʵ��Ϊ��


    GPIO_InitStructure.GPIO_Pin=GPIO_Pin_7;   //��������
    GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);	
		PBin(7)=0;
}
//Ӳ������SIM868
//PB14 --EN 
void Restart_SIM868(void)
{
  s8 ret;
	u8 i;
	delay_ms(250);
	while(1)
	{
		PBout(15)=0;//��Ƭ������ PWR�ű�����
		delay_ms(500);
		PBout(15)=1;//
		delay_ms(1500);
		PBout(15)=0;
		delay_ms(1500);
		i=0;
		while((ret = sendAT("AT\r\n","OK",500)) != 0)
    {
        printf("���ڿ�������```%d\r\n",i++);
			if(i>=10)
				break;
    }
		if(i<10)
			break;
  }
	
	
	
	
}



//��ʼ��GPS
//��GPS_EN ����
void SIM868_GPS_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode=GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
  GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	
	PAout(15)=1; //��GPS_EN����2������
	delay_ms(1500);
}

//GPRS����
s8 GPRS_Connect(void)
{
	u8 i=0;
    s8 ret;
    ret = sendAT("AT+CIPCLOSE","OK",1000);	//�ر�����
    if(ret == 0) {
        printf("�ر����ӳɹ�\r\n");
    }
    else {
        printf("�ر�����ʧ��\r\n");
    }

    ret = sendAT("AT+CIPSHUT","OK",1000);		//�ر��ƶ�����
    if(ret == 0) {
        printf("�ر��ƶ������ɹ�\r\n");
    }
    else {
        printf("�ر��ƶ�����ʧ��\r\n");
    }

    ret = sendAT("AT+CGCLASS=\"B\"","OK",1000);		//����GPRS�ƶ�̨���ΪB,֧�ְ����������ݽ���
    if(ret == 0) {
        printf("����GPRS�ƶ�̨����ΪB�ɹ�\r\n");
    }
    else {
        printf("����GPRS�ƶ�̨����ΪBʧ��\r\n");
    }
	i=0;
    while((ret=sendAT("AT+CGATT=1","OK",5000))!=0)//����GPRSҵ��
	{
		i++;
		printf("����GPRSҵ��ʧ��\r\n");
		if(i>=3)
		{
			break;
		}			
	}
    if(ret == 0) 
	{
        printf("����GPRSҵ��ɹ�\r\n");
    }

    ret = sendAT("AT+CIPCSGP=1,\"IP\",\"CMNET\"","OK",5000);	//����PDP����
    if(ret == 0) {
        printf("����PDP�����ɹ�\r\n");
    }
    else {
        printf("����PDP����ʧ��\r\n");
    }

    ret = sendAT("AT+CIPHEAD=1 ","OK",5000);			//����PDP����ȷ�����Ժ�Ϳ���������
    if(ret == 0) {
        printf("����PDP�ɹ�\r\n");
        GPRS_status=1;
    }
    else {
        printf("����PDPʧ��\r\n");
        GPRS_status=0;
			return -1;
    }
		return 0;
}

//�������ͺ���
void SIM868_SendHeart(void)
{
    char Heart_pack[100]= {0};
    sprintf(Heart_pack,"IMEI :%s ,%04d,%03d,%03d,%03d,sQ: %02d",   //һ�����ݴ��                
                            u1_data_Pack.IMEI,                          //ģ�����
                            sysData_Pack.data.bootTimes,      //������������
                            sysData_Pack.data.SIM868_bootTimes, //�ϴ���������ʧ�ܴ���
                            sysData_Pack.data.postErrorTimes,  //POSTʧ�ܴ���
                            sysData_Pack.data.GPSErrorTimes,
                            signalQuality);
	submitInformation(Heart_pack);

}





//�ϴ�GPS��Ϣ��������
void submitInformation(char * GPS_Info)
{
    s8 ret;
    isSendData = 1;		//�ںͷ�����ͨ�ŵı�־λ
    isSendDataError=0;   //����λ����
		SIM868_GPRS_Test();
		Check_TCP();
	
    if(sendAT("AT+CIPSEND",">",5000)==0)		//��������
    {
        u1_printf("%s\r\n",GPS_Info);
        delay_ms(10);
        if(sendAT((char*)0X1A,"SEND OK",5000)==0)
        {
            cleanReceiveData();//���������
            printf("POST�ɹ�\r\n");
            ret= 0;
        }
        else
        {
            isSendDataError = 1;//���ñ�־λΪʧ��
            printf("POSTʧ��\r\n");
            ret= -1;
            sysData_Pack.data.postErrorTimes++;
        }

    }
    else
    {
        sendAT((char*)0X1B,0,0);	//ESC,ȡ������
        isSendDataError = 1;//���ñ�־λΪʧ��
        printf("POSTʧ�ܣ�ȡ������");
        ret= -1;
    }
		

    isSendData = 0;//ת����־λΪδ���������ͨ��
}



void Send_LBS()
{
	s8 ret; 
	ret=sendAT("AT+CLBSCFG=0,3","www",3000);
	if(ret!=0)
		printf("��ѯLBSĬ�ϵ�ַʧ��\r\n");
	else
		printf("��ѯLBSĬ�ϵ�ַ�ɹ�\r\n");
	ret=sendAT("AT+CLBS=1,1","+CLBS:",5000);
	if(ret!=0)
	{
		printf("��ȡ��γ��ʧ��\r\n");
	}
}


//����LBS�ľ�γ�ȣ���ȡ����
char * AnalyticalLBS(void)
{
	u16 i;
	u16 Strlenth;
	char *CommaADDR1;
	char *CommaADDR2;
	char *CommaADDR3;
	
	CommaADDR1=ReturnCommaPosition((char *)u1_data_Pack.USART1_RX_BUF,1);
	CommaADDR2=ReturnCommaPosition((char *)u1_data_Pack.USART1_RX_BUF,2);
	CommaADDR3=ReturnCommaPosition((char *)u1_data_Pack.USART1_RX_BUF,3);
	//��ȡ����
	Strlenth=CommaADDR2-CommaADDR1;
	for(i=0;i<Strlenth-1;i++)
	{
		LBS_PACK.LBS_lonStr[i]=CommaADDR1[i+1];
	}
	LBS_PACK.LBS_lonStr[i]='\0';
	LBS_PACK.LBS_lonNum=atof(LBS_PACK.LBS_lonStr);
	
	//��ȡγ��
	Strlenth=CommaADDR3-CommaADDR2;
	for(i=0;i<Strlenth-1;i++)
	{
		LBS_PACK.LBS_latStr[i]=CommaADDR2[i+1];
	}
	LBS_PACK.LBS_latStr[i]='\0';
	LBS_PACK.LBS_latNum=atof(LBS_PACK.LBS_latStr);
	return 0;
}
//��������
//+HTTPACTION: 1,200,94
//+HTTPREAD: 94
//<?xml version="1.0" encoding="utf-8"?>
//<string xmlns="http://tempuri.org/">Submit OK</string>
char * AnalyticalData(char *searchStr_ack)
{
    u16 i;
	char *res=0;
	u16 outTime=200;
	if(searchStr_ack && outTime)//��searchStr��outTime��Ϊ0ʱ�ŵȴ�Ӧ��
    {
        while((--outTime)&&(res == 0))//�ȴ�ָ����Ӧ���ʱʱ�䵽��
        {
			for(i=0;i<u1_data_Pack.USART1_RX_STA;i++)
			{
				if(u1_data_Pack.USART1_RX_BUF[i]==*(searchStr_ack))
				{
					res = strstr((char *)u1_data_Pack.USART1_RX_BUF+i,searchStr_ack);
					if(res!=0)
					{
						delay_ms(50);
						break;
					}
				}
				if((i==USART1_MAX_RECV_LEN)||res!=0)
					break;
			}
            delay_ms(100);
        }
    }
	return res;
}
//2017/5/12 :

void Check_AT(void)
{
	u8 i=0;
     while(sendAT("AT\r\n","OK",500) != 0)
    {
        printf("AT���ģ���Ƿ�����```%d\r\n",i++);
        if(i >= 10)
        {
            printf("\r\nģ������Ӧ��׼������\r\n");
						saveData();
            u1_data_Pack.Error = 1;//��Ǽ��AT�����ȴ����Ź���λ��Ƭ��
            while(1);	//�ȴ����Ź���λ
        }
    }
	
}



const char ADDR[] = "\"58.17.235.65\",\"7700\"";
s8 TCP_Connect(void)
{
    s8 ret;
    char p[100];
    sprintf((char*)p,"AT+CIPSTART=\"TCP\",%s",ADDR);
    ret = sendAT(p,"CONNECT OK",5000);	//����TCP����
    if(ret == 0)
    {
        printf("����TCP�����ӳɹ�\r\n");
    }
    else
    {
        printf("����TCP������ʧ��\r\n");
				
    }
    return ret;
}
//��鵱ǰ����״̬�����ݵ�ǰ״̬���в���
//CONNECK OK:TCP������
//IP CLOSE :TCP���ӱ��ر�
//GPRSACT:GPRS�����ӣ�TCPδ����
//INITIAL:GPRSδ����
//����״̬��������һ��
s8 Check_TCP(void)
{
    s8 ret;
    if((ret=sendAT("AT+CIPSTATUS","CONNECT OK",3000))==0)
    {
        printf("��ǰ����״̬:CONNECT OK\r\n");
    }
    else
    {
			if(TCP_Connect()!=0)//�������ʧ�ܾ���������GPRS���磬�����ؽ���������
			{
				GPRS_Connect();
				TCP_Connect();
			}
		}

    return ret;
}









/*********************************************��ʷ��������,���������******************************/

#if 0
#define

//�˺������ڷ��Ͳ���У�����������������Ϣ
//��������Ϊ���ȷ����������ݣ�����ɹ��������ŷ���flash����δ�ɹ������ݣ�
//���ʧ�ܣ��ͽ�failedTimes++,failedTimes���Ϊ30��
void SIM868_SendPost(char * TEXT_Buffer )  //���Ͳ�У��
{
    u8 yl;
    s8 ret=0;
    Upload_Time=0;     			//�����ϴ�������ʱ����0
    SIM868_GPRS_Test();    			//GPRS���Ժ���
	if(firstsend_flag==1)
	{
		firstsend_flag=0;
		GPRS_Connect();
	}
    HTTP_POST(TEXT_Buffer);      //POST�������
    if(ret ==0)
        printf("���ͳɹ�\r\n");
    else
        printf("����ʧ��\r\n");

    if(!(isSendDataError || isSendData) )
    {
        waitservice_flag=0;          //�ȴ���־���㣬��ʼ�ȴ�������Ӧ��
        while(!(waitservice_flag ||(u1_data_Pack.USART1_RX_STA&(1<<15))));//�ȴ���������Ӧ,�������

        if(AnalyticalData()!=0)
        {
            isSendDataError = 0;//���ñ�־λΪ�ɹ�
            printf("�յ���ȷӦ��\r\n");
            Continue_Post_Errotimes=0 ;  //����ʧ�ܴ�������
        }
        else
        {
            isSendDataError = 1;                 
            printf("δ�յ���ȷӦ��\r\n");
        }
    }
    ret = sendAT("AT+CIPCLOSE","OK",1000);	//�ر�����
    if(ret == 0) {
        printf("�ر����ӳɹ�\r\n");
    }
    else {
        printf("�ر�����ʧ��\r\n");
    }


}


char SubWebService[]= {"WebService.asmx/GpsSubmit"};
char keystr[20]= {"2-409"};
//POST��ʽ�ύһ������
//����ֵ 0 �ɹ� ��-1ʧ��
//��ڲ���  GPS_Info ���͵��ַ���  SendType �������� ����/GPS����

s8 HTTP_POST(char * GPS_Info)
{
    u8 i=0;
    s8 ret;

    u16 length;
    isSendData = 1;		//�ںͷ�����ͨ�ŵı�־λ
    isSendDataError=0;   //����λ����
    while((ret=Check_TCP())!=0)
    {
        i++;
//        printf("TCP���ӳ����쳣...%d\r\n",i);
        if(i>=5)
        {
			i=0;
            printf("TCP���Ӷ��ʧ��...׼������\r\n");
			saveData();
            u1_data_Pack.Error=1;//
        }
        delay_ms(50);
    }
    if(SendType)
        sprintf(p1,"lat=%.6lf&lon=%0.6lf&IMEI=%s&log=%s",LatNow,LonNow,(u8 *) GPS_IMEI,GPS_Info);// �޸�ʱ��2017/3/24 17:21
    else
        sprintf(p1,"lat=%s&lon=%s&IMEI=%s&log=%s","heart","heart",(u8 *) GPS_IMEI,GPS_Info);
    printf("%s\r\n",p1);
    length=strlen(p1);
    printf("%d\r\n",length);
    sprintf(POSTDATA,"POST /%s HTTP/1.1\r\nHost: %s\r\n\
Content-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\n\r\n%s\r\n\r\n",
            SubWebService,ipaddr,length,p1	);

    if(sendAT("AT+CIPSEND",">",5000)==0)		//��������
    {
        printf("CIPSEND DATA:%s\r\n",p1);	 			//�������ݴ�ӡ������

        u1_printf("%s\r\n",POSTDATA);
        delay_ms(10);
        if(sendAT((char*)0X1A,"OK",5000)==0)
        {
            cleanReceiveData();//���������
            printf("POST�ɹ�\r\n");
            ret= 0;
        }
        else
        {
            isSendDataError = 1;//���ñ�־λΪʧ��
            printf("POSTʧ��\r\n");
            ret= -1;
            sysData_Pack.data.postErrorTimes++;
        }

    }
    else
    {
        sendAT((char*)0X1B,0,0);	//ESC,ȡ������
        isSendDataError = 1;//���ñ�־λΪʧ��
        printf("POSTʧ�ܣ�ȡ������");
        ret= -1;
    }

    isSendData = 0;//ת����־λΪδ���������ͨ��
    return ret;
}


//HTTP/1.1 200 OK
//Content-Type: text/xml; charset=utf-8
//Content-Length: length

//<?xml version="1.0" encoding="utf-8"?>
//<string xmlns="http://tempuri.org/">string</string>
//�������ز���
char * AnalyticalData(void)
{
    u16 i;
    char * retHead = 0;
    char * retTail = 0;
    if(my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"HTTP/1.1 200 OK")!=0)
    {
        retTail=my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"</string>");
        if(retTail!=0)
        {
            for(i=0; i<USART1_MAX_RECV_LEN; i++)
            {
                if( *(retTail-i)=='>' )
                {
                    retHead=retTail-i+1;
                    * retTail=0;
                    delay_ms(20);
//                    LCD_SString(10,100,300,200,12,(u8 *)retHead);		//��ʾһ���ַ���,12/16����
                    return retHead;
                }
            }
        }


    }
    return 0;
}




char ipaddr[]={"gpsws.cqutlab.cn"};	//IP��ַ
char port[]={"80"};				//�˿ں�

s8 TCP_Connect(void)
{
    s8 ret;
    char p[100];
    sprintf((char*)p,"AT+CIPSTART=\"TCP\",\"%s\",%s",ipaddr,port);
    ret = sendAT(p,"OK",5000);	//����TCP����
    if(ret == 0)
    {
        printf("����TCP�����ӳɹ�\r\n");
    }
    else
    {
        printf("����TCP������ʧ��\r\n");
    }
    return ret;
}
//��鵱ǰ����״̬�����ݵ�ǰ״̬���в���
//CONNECK OK:TCP������
//IP CLOSE :TCP���ӱ��ر�
//GPRSACT:GPRS�����ӣ�TCPδ����
//INITIAL:GPRSδ����
//����״̬��������һ��
s8 Check_TCP(void)
{
    s8 ret;
    u8 state=0;

    if((ret=sendAT("AT+CIPSTATUS","0,IP CLOSE",3000))==0)
    {
        printf("��ǰ����״̬:IP CLOSE\r\n");
        state=5;
    }
    else
    {
        if(my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"0,CONNECT OK")!=0)
        {
            printf("��ǰ����״̬:CONNECT OK\r\n");
            state=4;
        }
        else if(my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"0,IP GPRSACT")!=0)
        {
            printf("��ǰ����״̬:GPRSACT\r\n");
            state=3;
        }
        else if(my_strstr((char *)u1_data_Pack.USART1_RX_BUF,"0,IP INITIAL")!=0)
        {
            printf("��ǰ����״̬:INITIAL\r\n");
            state=2;
        }
        else
        {
            printf("��ǰ����״̬:δ֪\r\n");
            state=1;
        }
    }
    switch(state)
    {
    case 1:
    case 2:
        GPRS_Connect();
    case 3:case 5:
        ret=TCP_Connect();break;
	case 4:ret=0;break;
	
        
    }
    return ret;
}
#endif
