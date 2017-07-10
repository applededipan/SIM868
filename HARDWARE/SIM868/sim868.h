#ifndef __SIM868_H
#define __SIM868_H
#include "sys.h"
#include "usart_1.h"
#include "usart3.h"


#define Continue_Post_Errotimes_MAX    8  //����ʧ�ܴ���
#define Post_Errotimes_MAX             50   //һ�ο����������������
#define SendHeart    0
#define SendGPS      1
#define GPS_EN    PBout(13)
#define SIM868_EN PBout(14)
#define GPS_IMEI "006"

typedef struct
{
    vu16 USART1_RX_STA;			//���յ�������״̬//[15]:0,û�н��յ�����;1,���յ���һ������.//[14:0]:���յ������ݳ���
    u8 Error;								//����A6�Ƿ����0:δ���� 1:����
    u8 USART1_RX_BUF[USART1_MAX_RECV_LEN];//���ջ���,���USART2_MAX_RECV_LEN�ֽ�.
    u8 USART1_TX_BUF[USART1_MAX_SEND_LEN];//���ͻ���,���USART2_MAX_SEND_LEN�ֽ�
    u8 signalQuality;			  //GSM�ź�����
    char revision[50];			//ģ��̼��汾��Ϣ
    char IMEI[16];				  //ģ��IMEI��
    char Operator[15];			//��Ӫ��
} Data_Pack;


typedef struct
{
	char LBS_latStr[20];
	char LBS_lonStr[20];
	double  LBS_latNum;
	double  LBS_lonNum;
}LBS_Info;



extern u8 Heart_error;              //�����ϴ�ʧ��λ
extern char p1[300];
extern char POSTDATA[1000];
extern u8 Continue_Post_Errotimes ; //
extern u8 Post_Errotimes;       //SIM868һ�ο���������
extern Data_Pack u1_data_Pack;  //SIM868��ؽṹ��
extern u8 isSendDataError;       //���ʹ����־λ
extern u8 isSendData;           //ͨ�ű�־λ
extern u8 deviceState;          //SIM868����״̬��־
extern u8 GPRS_status;
extern u8 SendType;
extern u8 firstsend_flag;


extern s8 sendAT(char *sendStr,char *searchStr,u32 outTime);//����ATָ���
extern s8 Check_TCP(void);    //��鵱ǰ����״̬
extern s8 GPRS_Connect(void);   //GPRS����
extern s8 TCP_Connect(void);   //TCP����
//extern s8 HTTP_POST(char * GPS_Info ); //POST����
extern s8 extractParameter(char *sourceStr,char *searchStr,s32 *dataOne,s32 *dataTwo,s32 *dataThree);
extern s8 SIM868_GPRS_Test(void);  //���GPRS����״̬

extern u8 charNum(char *p ,char Char);


extern void cleanReceiveData(void);    //���������
extern void checkRevision(void);//��ѯģ��汾��Ϣ
extern void getIMEI(void); //��ȡIMEI
extern void SIM868_Init(void);   //��ʼ��SIM868
extern void SIM868_GPS_Init(void);  //��ʼ��SIM868
extern void submitInformation(char * GPS_Info);  //���Ͳ�У��
extern void SIM868_SendHeart(void);//����������
extern void Restart_SIM868(void);   //Ӳ������SIM868
extern void Check_AT(void);
extern void PWR_Init(void);

extern char * my_strstr(char *FirstAddr,char *searchStr);  //�����ַ�������
extern char * AnalyticalLBS(void);//��������
extern char * ReturnCommaPosition(char *p ,u8 num);
extern char * AnalyticalData(char *searchStr_ack);

#endif
