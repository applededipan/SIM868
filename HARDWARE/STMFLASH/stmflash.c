#include "stmflash.h"
#include "delay.h"
#include "usart.h"
#include "sim868.h"

char datatemp[100];            //������ʱ����FLASH�ж���������
char TEXT_Buffer_1[100];        //����������
s32 signalQuality =98;     //�ź�ǿ��,�Ժ���Ҫɾ������д


//��ȡָ����ַ�İ���(16λ����)
//faddr:����ַ(�˵�ַ����Ϊ2�ı���!!)
//����ֵ:��Ӧ����.
u16 STMFLASH_ReadHalfWord(u32 faddr)
{
    return *(vu16*)faddr;
}
#if STM32_FLASH_WREN	//���ʹ����д   
//������д��
//WriteAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��  2���ֽڲ���
void STMFLASH_Write_NoCheck(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)
{
    u16 i;
    for(i=0; i<NumToWrite; i++)
    {
        FLASH_ProgramHalfWord(WriteAddr,pBuffer[i]);
        WriteAddr+=2;//��ַ����2.
    }
}
//��ָ����ַ��ʼд��ָ�����ȵ�����
//WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ2�ı���!!)
//pBuffer:����ָ��
//NumToWrite:����(16λ)��(����Ҫд���16λ���ݵĸ���.)
#if STM32_FLASH_SIZE<256
#define STM_SECTOR_SIZE 1024 //�ֽ�
#else
#define STM_SECTOR_SIZE	2048
#endif
u16 STMFLASH_BUF[STM_SECTOR_SIZE/2];//�����2K�ֽ�
void STMFLASH_Write(u32 WriteAddr,u16 *pBuffer,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ,��ʵ��ƫ�Ƶ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			  //������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ����д������ݳ��ȸ���ʣ��Ŀռ�����
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//��Ҫ����
        }
        if(i<secremain)//��Ҫ����
        {
            FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//�����������
            for(i=0; i<secremain; i++) //����
            {
                STMFLASH_BUF[i+secoff]=pBuffer[i];
            }
            STMFLASH_Write_NoCheck(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//д����������
        } else STMFLASH_Write_NoCheck(WriteAddr,pBuffer,secremain);//д�Ѿ������˵�,ֱ��д������ʣ������.
        if(NumToWrite==secremain)break;//д�������          ʵ�ʵ���������д������ѭ�������break
        else//д��δ����
        {
            secpos++;				//������ַ��1
            secoff=0;				//ƫ��λ��Ϊ0 	 ��Ϊ����һ���µ�ҳ������ָ���ƫ�Ƶĵ�ַ��0
            pBuffer+=secremain;  	//ָ��ƫ��
            WriteAddr+=secremain;	//д��ַƫ��
            NumToWrite-=secremain;	//�ֽ�(16λ)���ݼ�
            if(NumToWrite>(STM_SECTOR_SIZE/2))secremain=STM_SECTOR_SIZE/2;//��һ����������д����
            else secremain=NumToWrite;//��һ����������д����
        }
    };
    FLASH_Lock();//����
}
#endif

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToWrite:����(16λ)��
void STMFLASH_Read(u32 ReadAddr,u16 *pBuffer,u16 NumToRead)
{
    u16 i;
    for(i=0; i<NumToRead; i++)
    {
        pBuffer[i]=STMFLASH_ReadHalfWord(ReadAddr);//��ȡ2���ֽ�.
        ReadAddr+=2;//ƫ��2���ֽ�.
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//WriteAddr:��ʼ��ַ
//WriteData:Ҫд�������
void Test_Write(u32 WriteAddr,u16 WriteData)
{
    STMFLASH_Write(WriteAddr,&WriteData,1);//д��һ����
}

void saveData(void)
{
    CRC_ResetDR();//��λCRC
    sysData_Pack.data.CRCData = CRC_CalcBlockCRC((u32*)sysData_Pack.buf, sizeof(_sysData)/4 - 2);  //����CRC   ������һ�ε�CRCУ��
    sysData_Pack.data.isEffective = 1;//��־λ
    STMFLASH_Write(FLASH_Information,sysData_Pack.buf,sizeof(_sysData)/2);//������д��FLASH	 ����2��ԭ����STMFLASH_Write()Ҫ����16λ���ֽ�
}


void FLASH_initialize(void)  //������ʼ��FLASH������
{
    u32 CRC_DATA = 0;
    STMFLASH_Read(FLASH_Information,sysData_Pack.buf,sizeof(_sysData)/2);    //��FLASH�е����ݶ�ȡ����
    if(sysData_Pack.data.isEffective == 1)    //FLASH��������Ч
    {
        CRC_ResetDR();         //��λCRC���ݼĴ���
        //�����ĺ��� 1.ָ�����Ҫ��������ݵĻ�������ָ��.  2.Ҫ����Ļ������ĳ���
        CRC_DATA = CRC_CalcBlockCRC((u32*)sysData_Pack.buf, sizeof(_sysData)/4 - 2);   //CRC����У�� ÿһ������4���ֽڣ��õ�����,��ȥ����ġ��õ�У�����ݳ���
        if(CRC_DATA == sysData_Pack.data.CRCData)    //У��λ��Ч
        {
            sysData_Pack.data.bootTimes++;         //��¼���������Ĵ���
        }
        else
        {
            sysData_Pack.data.isEffective = 0;     //���FLASH��������Ч
        }
    }

    if(sysData_Pack.data.isEffective != 1)    //FLASH�е�������Ч,��ʼ������
    {
        sysData_Pack.data.bootTimes = 1;                               //��������
        sysData_Pack.data.SIM868_bootTimes = 0;             //�ϴ�������ʧ�ܴ���
		    sysData_Pack.data.failedTimes=0;                               //��һ�γɹ����ͺ�ʧ�ܴ���
        sysData_Pack.data.postErrorTimes = 0;                          //POSTʧ�ܴ���
        sysData_Pack.data.GPSErrorTimes = 0;                           //��ΪGPSδ��λʱ�䳬ʱ�����������
        sysData_Pack.data.isEffective = 1;                             //��¼FLASH�е������Ƿ���Ч
    }
    saveData();//������д��FLASH

    printf("�豸����������%d\r\n",sysData_Pack.data.bootTimes);
    printf("A7����������%d\r\n",sysData_Pack.data.SIM868_bootTimes);
    printf("POSTʧ�ܴ�����%d\r\n",sysData_Pack.data.postErrorTimes);
    printf("GPS��ʱδ��λ������%d\r\n\r\n",sysData_Pack.data.GPSErrorTimes);
}



/********************************************
FLASH�� 59K��63K���GPSδ�ϴ��ɹ�������
1K���6������
���Դ��30��δ�ɹ��ϴ���GPS����

��Ϊ��������
�ȴ�65k��ʼ�������
65-69k���δ�ϴ��ɹ���FLASH����
*********************************************/
void FLASH_GPS_Pack(u8 m)   //δ�ϴ��ɹ��Ĵ�����ݴ�ŵ�FLASH��
{ 
	  if(m>49)     //�������Ĵ洢��Χ,��ͷ�洢
		{
			 m=0;
		}
		
    if((m+1)*Pack_length<=Pack_length*10)    //59k���    ��10��   
    {                                      
        STMFLASH_Write(FLASH_SAVE_GPS+m*Pack_length,(u16*)TEXT_Buffer_1,sizeof(TEXT_Buffer_1)/2);    //д��GPS���ݵ�FLASH
    } 
    else if(9<m&&m<=19)     //60k              
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024+(m-10)*Pack_length,(u16*)TEXT_Buffer_1,sizeof(TEXT_Buffer_1)/2);
    }
    else if(19<m&&m<=29)    //61               
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024*2+(m-20)*Pack_length,(u16*)TEXT_Buffer_1,sizeof(TEXT_Buffer_1)/2);
    }
    else if(29<m&&m<=39)    //62           
    {
        STMFLASH_Write(FLASH_SAVE_GPS+1024*3+(m-30)*Pack_length,(u16*)TEXT_Buffer_1,sizeof(TEXT_Buffer_1)/2);
    }
//    else if(39<m&&m<=49)    //63         
//    {
//        STMFLASH_Write(FLASH_SAVE_GPS+1024*4+(m-40)*Pack_length,(u16*)TEXT_Buffer_1,sizeof(TEXT_Buffer_1)/2);
//    }
		
		printf("δ�ϴ��ɹ���GPS�����ѱ���FLASH\r\n\r\n");
}

void FLASH_GPS_Read(s8 j)
{
    if((j+1)*Pack_length<=(Pack_length*10))  //��һ���ֽ���
    {
        STMFLASH_Read(FLASH_SAVE_GPS+j*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer_1)/2);
//        datatemp[Pack_length]='\0';
    
    }
    else if(9<j&&j<=19)   //2
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024+(j-10)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer_1)/2);
//        datatemp[Pack_length]='\0';
      
    }
    else if(19<j&&j<=29)    //3
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024*2+(j-20)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer_1)/2);
//        datatemp[Pack_length]='\0';
  
    }
    else if(29<j&&j<=39)  //4
    {
        STMFLASH_Read(FLASH_SAVE_GPS+1024*3+(j-30)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer_1)/2);
//        datatemp[Pack_length]='\0';
    
    }
//    else if(39<j&&j<=49)        //5�ֽ�
//    {
//        STMFLASH_Read(FLASH_SAVE_GPS+1024*4+(j-40)*Pack_length,(u16*)datatemp,sizeof(TEXT_Buffer_1)/2);
////        datatemp[Pack_length]='\0';    
//    }

}

//��FLASHָ��λ�õĲ���
//ֻ��һҳһҳ�Ĳ���
//������� WriteAddr      ��Ҫ�����ĵ�ַ
//         NumToWrite     ��Ҫ�����İ���(16λ)��
void Erase_FLASH(u32 WriteAddr,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			//������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break;//��Ҫ����
        }
				
        if(i<secremain)//��Ҫ����
        {
            FLASH_ErasePage(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE);//�����������
        }
        break;
    };
    FLASH_Lock();//����
}


//���FLASH�������Ƿ�Ϊ��
//������� WriteAddr     ��Ҫ���ĵ�ַ
//         NumToWrite   ��Ҫ���İ���(16λ)��
void Testing_FLASH(u32 WriteAddr,u16 NumToWrite)
{
    u32 secpos;	   //������ַ
    u16 secoff;	   //������ƫ�Ƶ�ַ(16λ�ּ���)
    u16 secremain; //������ʣ���ַ(16λ�ּ���)
    u16 i;
    u32 offaddr;   //ȥ��0X08000000��ĵ�ַ,��ʵ��ƫ�Ƶ�ַ
    if(WriteAddr<STM32_FLASH_BASE||(WriteAddr>=(STM32_FLASH_BASE+1024*STM32_FLASH_SIZE)))return;//�Ƿ���ַ
    FLASH_Unlock();						//����
    offaddr=WriteAddr-STM32_FLASH_BASE;		//ʵ��ƫ�Ƶ�ַ.
    secpos=offaddr/STM_SECTOR_SIZE;			  //������ַ  0~127 for STM32F103RBT6
    secoff=(offaddr%STM_SECTOR_SIZE)/2;		//�������ڵ�ƫ��(2���ֽ�Ϊ������λ.)
    secremain=STM_SECTOR_SIZE/2-secoff;		//����ʣ��ռ��С
    if(NumToWrite<=secremain)secremain=NumToWrite;//�����ڸ�������Χ����д������ݳ��ȸ���ʣ��Ŀռ�����
    while(1)
    {
        STMFLASH_Read(secpos*STM_SECTOR_SIZE+STM32_FLASH_BASE,STMFLASH_BUF,STM_SECTOR_SIZE/2);//������������������
        for(i=0; i<secremain; i++) //У������
        {
            if(STMFLASH_BUF[secoff+i]!=0XFFFF)break; 
        }
        if(i<secremain)  //��Ϊ��
        {
					 printf("�����FLASH��Ϊ��\r\n"); break;
        }
        else
				{
					printf("�����FLASH��\r\n"); break;
				}			
    };
    FLASH_Lock();//����
}









