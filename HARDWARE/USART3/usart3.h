#ifndef __USART3_H
#define __USART3_H
#include "sys.h"

#define USART3_MAX_RECV_LEN		5000	//�����ջ����ֽ���
#define USART3_RX_EN 			1					//0,������;1,����.
#define SRTING_NUM 20
#define GPS_array 10        //GPS���ݴ洢������ 

typedef struct
{
    u16 year;    //��
    u8  month;   //��
    u8  day;     //��
    u8  hour;    //ʱ
    u8 minute;   //��
    u8 second;	  //��
    char Time[21];     //��ǰʱ��
    char Time_Old[21]; //��ʷʱ��
} Time_Pack;

__packed typedef struct     //__packed�������ʾ 1�������������Զ���ѹ������ �����ж���
{   //2��ʹ��δ����ķ��ʶ�ȡ��д��ѹ�����͵Ķ���
    s32 bootTimes;						//��������
//    s32 submitInformationErrorTimes;//�ϴ�������ʧ�ܴ���
	s32 SIM868_bootTimes;
    u32 failedTimes;	           //��һ�γɹ�֮��ʧ�ܴ��������ڼ�⵽һ�γɹ�����֮�󣬽�֮ǰδ���͵����ݷ�����ȥ
    s32 postErrorTimes;				//POSTʧ�ܴ���
    s32 GPSErrorTimes;				//��ΪGPSδ��λʱ�䳬ʱ��ɵ�SIM868��������
    u32 CRCData;					    //CRCData֮ǰ���ݵ�CRCУ��
    s32 isEffective;				  //��¼FLASH�е������Ƿ���Ч
} _sysData;

typedef union       //������,���湲��һ�δ洢�ռ�  һ��buf�д洢�����ֽ� ÿ����buf�洢һ������
{
    u16 buf[sizeof(_sysData)/2];//��ȡ��д��FLASH������
    _sysData data;	//�Զ���buf[]�е����ݷ���Ϊ��Ҫ������    buf��ÿ������Ӧһ������������
} _sysData_Pack;

extern u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];   //�����ջ�����USART3_MAX_RECV_LEN�ֽ�
extern vu16 USART1_RX_STA;   						//��������״̬
extern char *GNRMC;		//������յ�������


extern char* string[SRTING_NUM];

extern u16 receive_len;			//���ռ�����
extern u32 startNum;        //��¼��ǰ�����ַ����ڽ��ջ����еĿ�ʼλ��
extern char* string[SRTING_NUM];
//extern char UTCTime[7];		       //UTCʱ�䣬hhmmss��ʽ
//extern char Longitude_Str[11];	 //����dddmm.mmmm(�ȷ�)��ʽ
extern Time_Pack u3_time_Pack;		//ʱ��
extern double Latitude_Temp,Baidu_Latitude_Temp;    //��¼ת���ľ�γ��
extern double Longitude_Temp,Baidu_Longitude_Temp;  //��¼ת���ľ�γ��
extern double Drift_speed;   //Ư���ٶ�
extern double distance;             //���������ϴ��ľ�����
extern double BaiduLongitude_Range[GPS_array];     //����(�ٶ�����)
extern double BaiduLatitude_Range[GPS_array];	     //γ��(�ٶ�����)
extern double Speed_Range[GPS_array]; 
extern u8 GPS_effective;                           //GPS�����Ƿ���Чλ
extern u8 Pack_length;              //���ݴ������
extern u16 GPS_unspecified_time;       //GPS����δ��λ��ʱ��
extern u8 GPS_receive;                    //����3�����ж϶�ʱ���
 
extern _sysData_Pack sysData_Pack;
extern void getFilterLoc(double * Lat_filter,double * Lon,double * Spend_filter);   //GPS�����˲�����
extern void GPS_Packed_Data(void);    //GPS���ݴ���ϴ�����
extern void Receive_empty(void);    //���GPS����
extern void analysisGPS(void);     //GPS���ݽ�������
extern void USART3_Init(u32 BaudRate); 				//����3��ʼ��
extern void cleanReceiveData_GPS(void);


extern char UTCTime[7];          //���UTCʱ������ ʱ����
extern char UTCTime_year[7];     //���������ʱ��
extern char Longitude_Str[11];	  //����dddmm.mmmm(�ȷ�)��ʽ
extern char Latitude_Str[10];	  //γ��ddmm.mmmm(�ȷ�)��ʽ
extern char La_Position[2];     //γ�ȷ�λ
extern char Lo_Position[2];     //���ȷ�λ
extern char Ground_speed[10];    //�����ٶ���ȡ
extern char Azimuth[10];        //��λ��
#endif



