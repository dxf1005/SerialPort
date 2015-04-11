// Copyright (C) 1991 - 1999 Rational Software Corporation

#include "stdafx.h"
#include "SealDevice.h"
#include <time.h>

const char * LAST_MODIFY_TIME = "UpdateTime";


#define FRAME_MAX_LEN		255
#define FRAME_BEGIN_FLAG	0x02
#define FRAME_END_FLAG		0x03
#define BUFFER_MAX_LEN		1024

#define COM_TIME_OUT       60
#define RECV_PARA_FIRST_ADD 7
#pragma  pack(1)
//命令协议表
struct FRAME_HEADER
{
	BYTE	cbPrefix;		//起始符
	BYTE	cbFrameLen;		//长度	
	BYTE	cbCmdType;		//命令属性
	ULONG	ulCommand;		//命令码
	BYTE	cbReDataLen;	//返回参数长度
};

struct RE_FRAME_HEADER
{
	BYTE	cbPrefix;		//起始符
	BYTE	cbFrameLen;		//长度	
	BYTE	cbCmdType;		//命令属性
	ULONG	ulCommand;		//命令码
};
struct COMMAND_FRAME_HEADER
{
	FRAME_HEADER frame;
	BYTE    szReserved[4];  
    ULONG	ulTime;			//时间
};

//返回值协议表
struct RETURN_FRAME_HEADER
{
	RE_FRAME_HEADER frame;
	char    szParam[1];     //参数
};

struct FRAME_END
{
	BYTE	cbCheckXOR;		//异或校验
	BYTE	cbCheckSUM;		//和校验
	BYTE	cbSuffix;		//结束符
};
#pragma pack()
//设备命令码
#define DEVCMD_SHAKE_HANDS		0	//握手命令
#define DEVCMD_RESET_MACHINE	1	//用印机复位命令
#define DEVCMD_SAVE_DATA		2	//数据存储
#define DEVCMD_LOAD_DATA		3	//数据读取
#define DEVCMD_GERNERAL_CHECK	4	//常规检测
#define DEVCMD_SEAL_CHECK		5	//印章检测
#define DEVCMD_SWITCH_CHECK		6	//拨码开关检测
#define DEVCMD_DOOR_CTRL		7	//进纸门开关
#define DEVCMD_EXIT_CTRL		8	//安全门开关
#define DEVCMD_LIGHT_CTRL		9	//照明灯开关
#define DEVCMD_USB_CTRL			10	//USB开关
#define DEVCMD_PAGE_HANDLE_CTRL	11	//翻页板
#define DEVCMD_SEAL_MOVE		12	//印章平台移动
#define DEVCMD_SEAL_CTRL		13	//印章头动作
#define DEVCMD_SINGLE_SEAL		14	//盖章组合命令
#define DEVCMD_SEAL_RESET		15	//印章归位
#define DEVCMD_SELECT_SEAL		16	//设置印章号
#define DEVCMD_SELF_EXAMINE		17	//自检
#define DEVCMD_DIP_INKPAD		18	//蘸印泥
#define DEVCMD_SINGLE_NOINKPAD	19	//盖章组合命令2
#define DEVCMD_DIP_INKPAD_2		20	//蘸印泥2(不回归原点)
#define DEVCMD_SINGLE_3			21	//盖章组合命令3(从印泥盒出发)
#define DEVCMD_GET_CURRENT_SEAL			22	//获取当前印章
#define DEVCMD_SET_CURRENT_SEAL			23//设置当前印章
#define DEVCMD_SET_CURRENT_SEAL_PARAM	24//设置盖章参数
#define DEVCMD_ERROR_STATE				25//机器状态
#define DEVCMD_SEAL_STATE				26//印章安装状态
#define DEVCMD_SAFE_SEAL				27//安全用印


////存储区地址索引
#define ADDR_SEAL_COUNT_START	0		//盖章次数
#define ADDR_MODIFI_TIME        6		//参数调整时间
#define ADDR_AUTH_CODE			7		//Mac地址
#define ADDR_LOCK_FLAG			9		//锁定标志
#define ADD_PARAM_BEGIN			10		//参数起始地址
#define ADD_CAMM_ID				16		//相机ID
#define ADD_ZONE_ID				17		//区域ID
#define ADDR_MAX				18
//
////参数长度
#define LEN_MODIFI_TIME			8		///最后修改时间长度
#define LEN_AUTH_CODE			32		//认证码
#define LEN_MAC_ADDRESS			12		//MAC地址
#define LEN_CAMA_ID				8		//相机ID
#define LEN_PAGE				32		//单片机页大小
#define LEN_SEAlRANGE			8		//盖章范围（步数）起始地址
#define LEN_SEAlRANGE_FLAG		4		//盖章范围（步数）标志
#define LEN_REN_FRAM			10		//下位机返回命令帧长度


const char * SETTING_KEY="SOFTWARE\\ORIENT-IT";
const char * MACH_PARA_VALIE_NAME = "MachPara";

static struct FRAME_HEADER protocal_header[]=
{
	{ 0x2,	0x00,	0x00,	0x00000061 ,12},	//握手命令
	{ 0x2,	0x00,	0x00,	0x00000062 ,0 },	//用印机复位命令
	{ 0x2,	0x00,	0x01,	0x00000041 ,0 },	//数据存储
	{ 0x2,	0x00,	0x01,	0x00000042 ,-1},	//数据读取
	{ 0x2,	0x00,	0x02,	0x00000021 ,1 },	//常规检测
	{ 0x2,	0x00,	0x02,	0x00000022 ,1 },	//印章检测
	{ 0x2,	0x00,	0x02,	0x00000023 ,1 },	//拨码开关检测
	{ 0x2,	0x00,	0x03,	0x00000011 ,0 },	//进纸门开关
	{ 0x2,	0x00,	0x03,	0x00000012 ,0 },	//安全门开关
	{ 0x2,	0x00,	0x03,	0x00000013 ,0 },	//照明灯开关
	{ 0x2,	0x00,	0x03,	0x00000014 ,0 },	//USB开关
	{ 0x2,	0x00,	0x03,	0x00000016 ,0 },	//翻页板
	{ 0x2,	0x00,	0x03,	0x00000031 ,0 },	//印章平台移动
	{ 0x2,	0x00,	0x03,	0x00000032 ,0 },	//印章头动作
	{ 0x2,	0x00,	0x03,	0x00000033 ,0 },	//盖章组合命令
	{ 0x2,	0x00,	0x03,	0x00000034 ,0 },	//印章归位
	{ 0x2,	0x00,	0x03,	0x00000035 ,0 },	//设置印章号
	{ 0x2,	0x00,	0x00,   0x00000051 ,1 },	//自检
	{ 0x2,	0x00,	0x03,   0x00000036 ,0 },	//蘸印泥
	{ 0x2,	0x00,	0x03,   0x00000037 ,0 },	//盖章组合命令2
	{ 0x2,	0x00,	0x03,   0x00000038 ,0 },	//蘸印泥2(不回归原点)
	{ 0x2,	0x00,	0x03,   0x00000039 ,0 },	//盖章组合命令3(从印泥盒出发)
	{ 0x2,	0x00,	0x03,   0x0000003a ,0 },	//获取当前印章
	{ 0x2,	0x00,	0x03,   0x0000003b ,0 },	//设置当前印章
	{ 0x2,	0x00,	0x03,   0x0000003c ,0 },	//设置盖章参数
	{ 0x2,	0x00,	0x03,   0x0000003d ,0 },	//设置、获取机器状态
	{ 0x2,	0x00,	0x02,	0x00000051 ,1 },	//获取印章安装状态
	{ 0x2,	0x00,	0x03,	0x00000015 ,0 }		//安全用印
};

static unsigned long LoadDataAddress[]=
{
		0x00000030,			//盖章次数 印章 1
		0x00000034,			//		   印章 2
		0x00000038,			//		   印章 3
		0x0000003C,			//		   印章 4
		0x00000040,			//		   印章 5
		0x00000044,			//		   印章 6

		0x00000060,			//6 参数调整时间
		0x00000120,			//7 Mac地址1  
		0x00000140,			//8 Mac地址2
		0x00000160,			//9 锁定标志
		0x00000200,			//10 盖章参数1
		0x00000220,			//   盖章参数2
		0x00000240,			//   盖章参数3
		0x00000260,			//   盖章参数4
		0x00000280,			//   盖章参数5
		0x000002A0,			//   盖章参数6
		0x000002C0,			//16 相机ID
		0x000002E0,			//17 区域ID
		0x00000FFF
};

#define SEAL_DOWN_OK    0x01
#define SEAL_UP_OK      0x10
void BCD2ASCII(char *ascii_buf, const char *bcd_buf, long  bcd_len)
{
	long   i,j;
	unsigned char  ch;
	j = 0;
	for (i=0; i<bcd_len; i++) 
	{
		ch=(bcd_buf[i] >> 4) & 0x0f;
		if(ch>9)
			ascii_buf[j] = ch+'A'-10;
		else
			ascii_buf[j] = ch+'0';

		ch=bcd_buf[i] & 0x0f;
		if(ch>9)
			ascii_buf[j+1] = ch+'A'-10;
		else
			ascii_buf[j+1] = ch+'0';
		j = j +2;
	}
	ascii_buf[j] = '\0';
	return;
}

void addlog(char *cLog,size_t len)
{
	struct tm when;
	time_t now;
	time( &now );
	when = *localtime( &now );

	FILE *pFile;
	char fpath[1024],fname[128],sztime[32];
	GetCurrentDirectory(800,fpath);

	int y=when.tm_year+1900;
	int m=when.tm_mon+1;
	int d=when.tm_mday;
	int h=when.tm_hour;
	int mi=when.tm_min;
	int s=when.tm_sec;

	memset(sztime,0,32);
	sprintf(sztime,"%04d-%02d-%02d %02d:%02d:%02d ",y,m,d,h,mi,s);
	//	sprintf(fname,"\\device_%04d%02d.txt",y,m);//log4c
	sprintf(fname,"\\log%04d%02d.txt",y,m);
	strcat(fpath,fname);

	pFile = fopen(fpath,"a");
	if(pFile==NULL)
	{
		return;
	}

	len=strlen(cLog);
	fwrite("\r\n",sizeof(char),2,pFile);

	if(len)
		fwrite(sztime,sizeof(char),strlen(sztime),pFile);

	fwrite(cLog,sizeof(char),strlen(cLog),pFile);
	fwrite("\r\n",sizeof(char),2,pFile);
	fclose(pFile);
}

void addBCDLog(void* Head,void*bcd,size_t nSize,bool bUseLog)
{
	if (!bUseLog) 
	{
		return ;
	}

	long nLen = 0;
	char ASSCBuf[1024];
	memset(ASSCBuf ,0,1024);
	if (Head == NULL)
	{
		nLen = 0;
	}
	else
	{
		nLen = strlen((char*)Head);
		strncpy(ASSCBuf,(char*) Head,nLen);
	}
	if (nSize + nLen > 1024)
	{
		BCD2ASCII((char *)ASSCBuf + nLen, (const char *)bcd, 1023 - nLen);
	}
	else
	{
		BCD2ASCII((char *)ASSCBuf + nLen, (const char *)bcd, nSize);
	}
	//addlog(ASSCBuf,strlen(ASSCBuf));
}
void enCIPHER(unsigned long * const v,	unsigned long * const k)
{

	unsigned long y = v[0], z = v[1], sum = 0, delta = 0x9E3779B9,
		a = k[0], b = k[1], c = k[2], d = k[3], n = 32;

	while ( n-- > 0 )
	{
		sum += delta;
		y += ( z << 4 ) + a ^ z + sum ^ ( z >> 5 ) + b;
		z += ( y << 4 ) + c ^ y + sum ^ ( y >> 5 ) + d;
	}
	v[0] = y;
	v[1] = z;
}

void deCIPHER(unsigned long * const v, const unsigned long * const k )
{
	unsigned long y = v[0], z = v[1], sum = 0xC6EF3720, delta = 0x9E3779B9,
		a = k[0], b = k[1], c = k[2], d = k[3], n = 32;
	while ( n-- > 0 )
	{
		z -= ( y << 4 ) + c ^ y + sum ^ ( y >> 5 ) + d;
		y -= ( z << 4 ) + a ^ z + sum ^ ( z >> 5 ) + b;
		sum -= delta;
	}
	v[0] = y;
	v[1] = z;
}

static void Encryption_Ex(char *strData,const char *strKey)
{
	// 获取密钥不需加密
	if(strlen(strData) == 0)
		return;


	UCHAR strKey2[16];
	memset(strKey2,0,sizeof(strKey2));
	memcpy(strKey2,strKey,8);
	memcpy(strKey2 + 8,strKey,8);

	int i = (int)strlen(strData);
	if((i % 8) == 0)
		i = i / 8;
	else
		i = (i / 8) + 1;

	for (int k = 0;k < i;k ++)
	{
		enCIPHER((unsigned long *const)(strData + k * 8),(unsigned long *const)strKey2);
	}

	return;
}

static void EncryptionSendBuffer(const char *szKey, char *pOutput)
{
	ASSERT(pOutput);
	ASSERT(szKey);

	DWORD dwTime = ::GetTickCount();

	memcpy(pOutput + 7,szKey + 4, 4);
	char  *pCmd;
	pCmd = &pOutput[3];
	unsigned long lKey2[4];
	unsigned short usKeyR[2];

	memcpy(usKeyR,szKey,4);

	lKey2[0] = (unsigned long)((dwTime & 0x0000ffff) * usKeyR[0]);
	lKey2[1] = (unsigned long)(((dwTime & 0xffff0000) >> 16) * usKeyR[1]);
	lKey2[2] = 0;
	lKey2[3] = 0;
	enCIPHER((unsigned long *const)pCmd,(unsigned long *const)lKey2);//加密命令码
	memcpy(pOutput + 11, &dwTime,4);
}


static void CounterLineBK(void* iNum)
{
	char temp[5];
	memcpy(temp,iNum,sizeof(long));

	temp[4] = temp[0];
	temp[0] = temp[3];
	temp[3] = temp[4];

	temp[4] = temp[1];
	temp[1] = temp[2];
	temp[2] = temp[4];
	memcpy(iNum,temp,sizeof(long));
}

static void encipherBK( 
		 unsigned long * const v, 
		 unsigned long * const k 
		 )
{

	CounterLineBK(&v[0]);
	CounterLineBK(&v[1]);
	CounterLineBK(&k[0]);
	CounterLineBK(&k[1]);
	CounterLineBK(&k[2]);
	CounterLineBK(&k[3]);

	unsigned long y = v[0], z = v[1], sum = 0, delta = 0x9E3779B9,
		a = k[0], b = k[1], c = k[2], d = k[3], n = 32;

	while ( n-- > 0 )
	{
		sum += delta;
		y += ( z << 4 ) + a ^ z + sum ^ ( z >> 5 ) + b;
		z += ( y << 4 ) + c ^ y + sum ^ ( y >> 5 ) + d;
	}
	v[0] = y;
	v[1] = z;

	CounterLineBK(&v[0]);
	CounterLineBK(&v[1]);
}

static void ConvertEncryptionBK(char *strKeyR0,char *Output,long *OutputLen)
{
	// 获取密钥不需加密
	if(Output[0] == 1)
		return;

	DWORD dwTime;
	DWORD dwKey2[4];
	DWORD dwData[2];

	USHORT usA0,usA1;
	USHORT usB0,usB1;

	UCHAR strCode[4];
	UCHAR strSwap[4];

	UCHAR strKey2[16];
	UCHAR strData[8];

	ZeroMemory(strCode,sizeof(strCode));
	ZeroMemory(strSwap,sizeof(strSwap));

	//产生a1
	strSwap[0] = strKeyR0[1];
	strSwap[1] = strKeyR0[0];
	memcpy(&usA1,strSwap,sizeof(USHORT));

	//产生a0
	strSwap[0] = strKeyR0[3];
	strSwap[1] = strKeyR0[2];
	memcpy(&usA0,strSwap,sizeof(USHORT));

	//产生b1,b0
	dwTime = ::GetTickCount();
	//dwTime = 0x12345678;
	memcpy(strSwap,&dwTime,sizeof(DWORD));
	memcpy(&usB0,&strSwap[0],sizeof(USHORT));
	memcpy(&usB1,&strSwap[2],sizeof(USHORT));

	//产生K2
	dwKey2[0] = usA1 * usB1; 
	dwKey2[1] = usA0 * usB0; 
	dwKey2[2] = 0;
	dwKey2[3] = 0;
	CounterLineBK(&dwKey2[0]);
	CounterLineBK(&dwKey2[1]);
	memcpy(strKey2,&dwKey2[0],16);

	memcpy(strCode,&Output[1],4); 
	memcpy(&dwData[0],strCode,4);

	memcpy(&dwData[1],&strKeyR0[4],4);
	memcpy(strData,&dwData[0],8);

	encipherBK((unsigned long *const)strData,(unsigned long *const)strKey2);

	char temp[128];
	ZeroMemory(temp,sizeof(temp));
	memcpy(temp,Output,*OutputLen);

	Output[0] = 2;						//加密
	Output[1] = (char)(*OutputLen + 12 -5);		//长度
	memcpy(&Output[2],strData,8);
	CounterLineBK(&dwTime);
	memcpy(&Output[10],&dwTime,4);
	memcpy(&Output[14],&temp[5],*OutputLen - 5);	//命令体不加密
	memcpy(temp,Output,*OutputLen + 9);
	*OutputLen += 9;

	return;
}

	//calculate xor check and sum check
static void CalculateXORAndSUM(unsigned char *buffer, long len)
{
	BYTE xor=0;
	BYTE sum=0;
	for (long i=1; i < len -3 ; i ++)
	{
		xor ^= (BYTE)buffer[i];
		sum += (BYTE)buffer[i];
	}
	buffer[len-3] = (char)xor;
	buffer[len-2] = (char)sum;
}

static bool CheckXORAndSUM(unsigned char *buffer, long len)
{
	BYTE xor=0;
	BYTE sum=0;
	for (long i=1; i < len-3; i ++)
	{
		xor ^= (BYTE)buffer[i];
		sum += (BYTE)buffer[i];
	}
	return ((buffer[len-3] == xor) && (buffer[len-2] == sum));
}

CDevice4xEx::CDevice4xEx()
{
	m_SealCount = MAX_SEAL;
	m_bIsBK = false;
	m_PacketNo = 0;
	m_ProgFun = NULL;
}
CDevice4xEx::~CDevice4xEx()
{
	if(!m_bIsBK)
	{
		UsbOff();
	}
}
//取密钥
long CDevice4xEx::HandShake(char* strKeyR1)
{
	char buffer[FRAME_MAX_LEN]="";
	long recvLen=12;

	m_ErrorCode = CommWithDevice(DEVCMD_SHAKE_HANDS, NULL, 0, buffer, &recvLen);
	if(m_ErrorCode == 0) 
	{
		memcpy(m_szKeyR1,buffer, 8);//获得R1和DeviceID
		char ch;
		for(int i = 0;i < 2;i ++)
		{
			ch = m_szKeyR1[i];
			m_szKeyR1[i] = m_szKeyR1[3-i];
			m_szKeyR1[3-i] = ch;

			ch = m_szKeyR1[i + 4];
			m_szKeyR1[i + 4] = m_szKeyR1[7 - i];
			m_szKeyR1[7 - i] = ch;
		}
		m_lDeviceID=*(long*)(buffer + 8);
		return 0;
	}
	return m_ErrorCode;
}

void CDevice4xEx::ReSetMachType(char*pBuf)
{
	if(pBuf == NULL)
	{
		return;
	}
	if (!memcmp(pBuf,"M\x00\x11",3) || !memcmp(pBuf,"m\x00\x11",3))
	{
		m_nDeviceMode = DEVICE_TYPE_S_30R;	
	}
	else if (!memcmp(pBuf,"S\x00\x11",3) || !memcmp(pBuf,"s\x00\x11",3))
	{
		m_nDeviceMode = DEVICE_TYPE_S_30R;	
	}
	if (!memcmp(pBuf,"M\x00\x11",3) || !memcmp(pBuf,"m\x00\x11",3))
	{
		m_nDeviceMode = DEVICE_TYPE_M_11;	
	}
	else if ( !memcmp(pBuf,"S\x00\x11",3) || !memcmp(pBuf,"s\x00\x11",3) ||
			  !memcmp(pBuf,"S\x00\x30",3) || !memcmp(pBuf,"s\x00\x30",3) )
	{
		m_nDeviceMode = DEVICE_TYPE_S_11;	
	}
	else if (!memcmp(pBuf,"M\x00\x30",3) || !memcmp(pBuf,"m\x00\x30",3))
	{
		m_nDeviceMode = DEVICE_TYPE_M_30;	
	}
	else if (!memcmp(pBuf,"M\x00\x40",3) || !memcmp(pBuf,"m\x00\x40",3))
	{
		m_nDeviceMode = DEVICE_TYPE_M_40;	
	}
	else if (!memcmp(pBuf,"M\x00\x20",3) || !memcmp(pBuf,"m\x00\x20",3))
	{
		m_nDeviceMode = DEVICE_TYPE_M_20;	
	}
	else if (!memcmp(pBuf,"S\x00\x31",3) || !memcmp(pBuf,"s\x00\x31",3))
	{
		m_nDeviceMode = DEVICE_TYPE_S_31;
	}
	

}

//取密钥
long CDevice4xEx::HandShakeM10(char* strKeyR1)
{
	char ch;
	int i;
	char buffer[FRAME_MAX_LEN]="";
	long recvLen=22;
	unsigned char nSeal;
	//char model[8];
	m_ErrorCode = CommWithDevice(DEVCMD_SHAKE_HANDS, NULL, 0, buffer, &recvLen);
	if(m_ErrorCode == 0) 
	{

		m_lDeviceID=*(long*)(buffer + 8);

		ch = buffer[12];
		buffer[12] = buffer[13];
		buffer[13]= ch;
		m_MachineVer = *(short*)(buffer + 12);

		if (m_MachineVer >= _DEVICE_VER_1020)
		{
			for (i = 0;i < 8;i ++)
			{
				buffer[i] ^= 0xAA;
			}
		}

		memcpy(m_szKeyR1,buffer, 8);//获得R1和DeviceID
		for(i = 0;i < 2;i ++)
		{
			ch = m_szKeyR1[i];
			m_szKeyR1[i] = m_szKeyR1[3-i];
			m_szKeyR1[3-i] = ch;

			ch = m_szKeyR1[i + 4];
			m_szKeyR1[i + 4] = m_szKeyR1[7 - i];
			m_szKeyR1[7 - i] = ch;
		}
		ReSetMachType((char*)buffer + 18);
		nSeal = *(unsigned char*)(buffer + 21);

		m_SealCount = nSeal;
	}
	return m_ErrorCode;
}
//////////////////////////////////////////////////////////////////////////
long CDevice4xEx::CommWithDevice(int cmd, const char *pIutput, long inLen, char *pOutput, long *outLen,bool bDouble)
{
	char buffer[FRAME_MAX_LEN]="";
	COMMAND_FRAME_HEADER *pHeader=(COMMAND_FRAME_HEADER*)buffer;
	DWORD timeout;
	if (!bDouble)
	{
		timeout = m_iSerialOverTime;
	}
	else
	{
		timeout =  2 * m_iSerialOverTime;
	}

	pHeader->frame=protocal_header[cmd];
	pHeader->frame.ulCommand=protocal_header[cmd].ulCommand;
	pHeader->frame.cbFrameLen= sizeof(COMMAND_FRAME_HEADER) - sizeof(BYTE);
	pHeader->frame.cbReDataLen = 0;
	if(inLen > 0 && pIutput)
	{
		memcpy(buffer + pHeader->frame.cbFrameLen, pIutput, inLen);
		pHeader->frame.cbFrameLen +=(BYTE)inLen;
	}
	memset(buffer + pHeader->frame.cbFrameLen, 0, 3);
	pHeader->frame.cbFrameLen +=3;
	buffer[pHeader->frame.cbFrameLen-1]=(BYTE)FRAME_END_FLAG;
	BYTE sendLen=pHeader->frame.cbFrameLen;
	pHeader->frame.cbFrameLen -=2;
	long readLen;

	addBCDLog("Send:",buffer,sendLen,GetLogUsed());

	if (protocal_header[cmd].cbCmdType != 0x00)
	{
		EncryptionSendBuffer(m_szKeyR0,  buffer);//加密 //命令属性是0x00时不加密
	}
	CalculateXORAndSUM((BYTE*)buffer, sendLen);
	if((m_ErrorCode = m_RawDevice.Open(m_strCommPort,timeout)) != 0)
	{
		return m_ErrorCode;
	}

	if(m_RawDevice.Send( buffer, sendLen)==sendLen)
	{
		//send ok
		memset(buffer,0,sizeof(buffer));
		if (outLen) 
		{
			readLen=m_RawDevice.Read( buffer, *outLen + LEN_REN_FRAM);
			//addBCDLog("Receve:",buffer,*outLen + LEN_REN_FRAM,1);
		}
		else
		{
			readLen=m_RawDevice.Read( buffer, protocal_header[cmd].cbReDataLen + LEN_REN_FRAM);
			//addBCDLog("Receve:",buffer,protocal_header[cmd].cbReDataLen + LEN_REN_FRAM,1);
		}
		if(readLen > 0)
		{
			RETURN_FRAME_HEADER	*pReturnFrame=(RETURN_FRAME_HEADER*)buffer;
			FRAME_END *pFrameEnd=(FRAME_END*)(buffer + readLen -3 );
			if(pReturnFrame->frame.cbPrefix !=FRAME_BEGIN_FLAG || 
				pFrameEnd->cbSuffix!=FRAME_END_FLAG)
			{
				m_RawDevice.Close();
				addBCDLog("Error Read:",buffer,readLen,1);
				return -1;  //format error
			}
			//if( (pReturnFrame->frame.ulCommand >> 8) != protocal_header[cmd].ulCommand)
			//{
			//	m_RawDevice.Close();
			//	addBCDLog("Error Read:",buffer,readLen,1);
			//	return (pReturnFrame->frame.ulCommand & 0xFFFF00FF);//return -2;  //return value is not matched
			//}
			if( (pReturnFrame->frame.ulCommand & 0xFFFF0000) !=0 )  
			{
				m_RawDevice.Close();
				addBCDLog("Error Read:",buffer,readLen,1);
				return pReturnFrame->frame.ulCommand;   //command execute error, just return error code
			}
			if(!CheckXORAndSUM((BYTE*)buffer, readLen))
			{
				m_RawDevice.Close();
				addBCDLog("Error Read:",buffer,readLen,1);
				return -3; //data error
			}
			if(pReturnFrame->frame.ulCommand)
				if(outLen)
				{
					long copyLen=readLen - RECV_PARA_FIRST_ADD -3;
					copyLen = copyLen < *outLen ? copyLen : *outLen;
					*outLen=copyLen;
					if(copyLen > 0 && pOutput)
					{
						memcpy(pOutput, buffer + RECV_PARA_FIRST_ADD, copyLen);
					}
				}
				m_RawDevice.Close();
				return 0;  //every thing is ok.
		}
		addBCDLog("Error Read is Null ",NULL,0,1);
	}
	m_RawDevice.Close();
	addBCDLog("Error Send is NULL:",NULL,0,1);
	return -4; //communication error
}


//数据存储
//@param address
//@param len
//@param data
//@return
//	-	正确： >0
//	-	错误： <0	
long CDevice4xEx::SaveData(unsigned long address, void *data, unsigned char len )
{
	char buffer[FRAME_MAX_LEN]="";
	*(ULONG*)buffer=address;
	buffer[4]=len;
	memcpy(buffer + 5, data, len);
	//Send buffer and read result;
	m_ErrorCode=CommWithDevice(DEVCMD_SAVE_DATA, buffer, 5 + len, NULL, NULL);
	if(m_ErrorCode)
	{
		return m_ErrorCode;
	}
	return 0;
}
//数据读取
//@param address
//@param data
//@param len
//@return
//	-	正确： 0
//	-	错误： <0
long CDevice4xEx::LoadData(unsigned long address, void *data, unsigned char *len)
{
	char buffer[FRAME_MAX_LEN]="";
	*(ULONG*)buffer=address;
	buffer[4]=*len;
	char outBuffer[FRAME_MAX_LEN];
	long outLen= *len;
	m_ErrorCode=CommWithDevice(DEVCMD_LOAD_DATA, buffer, 5, outBuffer, &outLen);
	if(m_ErrorCode)
	{
  		return m_ErrorCode;
	}
	memcpy(data,outBuffer,outLen);
	return m_ErrorCode;
}

long CDevice4xEx::LoadCameraID()
{
	char buffer[FRAME_MAX_LEN];
	unsigned char len = LEN_CAMA_ID;
	m_ErrorCode = LoadData(LoadDataAddress[ADD_CAMM_ID], buffer, &len);
	if(m_ErrorCode==0)
	{
		ASSERT(len >= LEN_CAMA_ID);
		memcpy(&m_CamerID,buffer,8); 
	}
	return m_ErrorCode;
}

//认证
long CDevice4xEx::CheckMacAddress(char* sAuthCode,long nType)
{
	char buffer[FRAME_MAX_LEN];
	unsigned char len = LEN_AUTH_CODE;
	long i = nType;
	char Code[33];
	if (sAuthCode == NULL)
	{
		return -1;
	}
	memset(Code,0,sizeof(33));
	strncpy(Code,sAuthCode,32);

	m_ErrorCode = LoadData(LoadDataAddress[ADDR_AUTH_CODE + i], buffer, &len);
	if(m_ErrorCode!=0)
	{
		return m_ErrorCode;
	}
	Encryption_Ex(Code,m_szKeyR3);
	if (memcmp(Code,buffer,32)) 
	{
		return -1;
	}
	return 0;
}

//获取存储的MAC地址
long CDevice4xEx::LoadMacAddress(char* sAuthCode,long nType)
{
	char buffer[FRAME_MAX_LEN];
	unsigned char len = LEN_MAC_ADDRESS;
	long i = nType;

	m_ErrorCode = LoadData(LoadDataAddress[ADDR_AUTH_CODE + i], buffer, &len);
	if(m_ErrorCode!=0)
	{
		return m_ErrorCode;
	}
	memcpy(sAuthCode,buffer,len);

	return 0;
}


long CDevice4xEx::LoadSealUsedCount()
{
	char buffer[FRAME_MAX_LEN];
	unsigned char len = 4;
	memset(&m_SealUsedCount,0,sizeof(long) * MAX_SEAL);
	for(int i=0; i< m_SealCount; i++)
	{
		m_ErrorCode = LoadData(LoadDataAddress[ADDR_SEAL_COUNT_START + i], buffer, &len);
		if(m_ErrorCode==0)
		{
			ASSERT(len >= sizeof(long));
			m_SealUsedCount[i]=*(long *) buffer;
		}
		else
		{
			return m_ErrorCode;
		}
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////

 long CDevice4xEx::SaveCoordConvertor(int nSeal, COORDINATE_CONVERTOR & convertor,BOOL bCopy)
{
//	char *p;
	long lSize = sizeof(COORDINATE_CONVERTOR);
	m_MachParameter.coordConvertor[nSeal] = convertor;
	m_CoordConverter[nSeal] = m_MachParameter.coordConvertor[nSeal];
	if(bCopy)
	{
		for(int i=0;i<6;i++)
		{
			m_MachParameter.coordConvertor[i] = convertor;
			m_CoordConverter[i] = convertor;
		}
	}
	//m_ErrorCode = SaveParaReg();
	m_ErrorCode = SaveParaEx();

	return m_ErrorCode;
}
 
long CDevice4xEx::LoadDataFromMach()
{
	char buffer[FRAME_MAX_LEN] = "";
	unsigned char lenRet = LEN_PAGE;
	int nOffset = 0;
	char p[2048] = "";;
	DWORD dwSize = sizeof(MACH_PARAMETER);

	while(nOffset < dwSize)
	{
		memset(buffer,0,sizeof(buffer));
		m_ErrorCode = LoadData(LoadDataAddress[ADD_PARAM_BEGIN] + nOffset, buffer, &lenRet);
		if(m_ErrorCode==0)
		{
			ASSERT(lenRet == LEN_PAGE);//sizeof(COORDINATE_CONVERTOR));
			memcpy(p + nOffset, buffer,lenRet);
			nOffset += lenRet;
		}
		else
		{
			return m_ErrorCode;
		}
	}
	memcpy(&m_MachParameter,p,sizeof(MACH_PARAMETER));

	return 0;
}

long CDevice4xEx::SaveAllParaToMach(void *value,DWORD dwSize)
{
	char buffer[FRAME_MAX_LEN] = "";
	unsigned char lenRet = LEN_PAGE;
	int nOffset = 0;
	char *p = (char *)value;
	m_PacketNo = 0;
	while(nOffset < dwSize)
	{
		m_ErrorCode = SaveData(LoadDataAddress[ADD_PARAM_BEGIN] + nOffset, p + nOffset, LEN_PAGE);
		if (m_ErrorCode != 0)
		{
			return m_ErrorCode;
		}
		nOffset += LEN_PAGE;
		m_PacketNo++;
	}
	m_PacketNo = 0;
	return m_ErrorCode;
}

//常规检测
//返回：正确：0
//	错误：	< 0

//	返回参数：一个字节数
//	0位：1：Y轴原点到位，0：Y轴原点未到位
//	1位：1：Y轴远点到位，0：Y轴远点未到位
//	2位：1：X轴原点到位，0：X轴原点未到位
//	3位：1：X轴远点到位，0：X轴远点未到位
//	4位：1：安全门关闭，0：安全门没有关闭
//	5位：1：进纸门关闭，0：进纸门没有关闭
//	6位：未使用
//	7位：1：usb开状态，0：usb关闭状态

long CDevice4xEx::GetGeneralDetect(unsigned char &state)
{
	
	long nRecvLen = 1;
	m_ErrorCode = CommWithDevice(DEVCMD_GERNERAL_CHECK, NULL, 0, (char*)&state, &nRecvLen); 
	if (0 != m_ErrorCode) 
	{
		return m_ErrorCode;
	}
	return m_ErrorCode;
}





long CDevice4xEx::IsSealReset()
{
    BYTE state;
	m_ErrorCode=GetGeneralDetect(state);
	if(m_ErrorCode==0)
	{
		return (state & (Y_AT_NEAR | X_AT_NEAR));
	}
	return m_ErrorCode;
}


long CDevice4xEx::IsDoorOpened()
{
	BYTE state;
	m_ErrorCode=GetGeneralDetect(state);
	if(m_ErrorCode==0)
	{
		return (state & DOOR_CLOSE) >> 5;
	}
	return m_ErrorCode;
}
long CDevice4xEx::IsExitOpened()
{
	BYTE state;
	m_ErrorCode=GetGeneralDetect(state);
	if(m_ErrorCode==0)
	{
		BYTE b1 = (state & EXIT_MACH) >> 7;
		BYTE b2 = (state & EXIT_CLOSE) >> 4;
		return b1&b2;
	}
	return m_ErrorCode;
}

long CDevice4xEx::IsExitMach()
{
	//EXIT_MACH

	BYTE state;
	m_ErrorCode=GetGeneralDetect(state);
	if(m_ErrorCode==0)
	{
		return (state & EXIT_MACH);
	}
	return m_ErrorCode;
}

long CDevice4xEx::IsSealDownOK()
{
	BYTE state;
	m_ErrorCode=GetSealState(state);
	if(m_ErrorCode==0)
	{
		return (state & SEAL_DOWN_OK);
	}
	return m_ErrorCode;
}

long CDevice4xEx::IsSealUpOK()
{
	BYTE state;
	m_ErrorCode=GetSealState(state);
	if(m_ErrorCode==0)
	{
		return (state & SEAL_UP_OK);
	}
	return m_ErrorCode;
}

long CDevice4xEx::IsUSBOn()
{
	BYTE state;
	m_ErrorCode=GetGeneralDetect(state);
	if(m_ErrorCode==0)
	{
		return (state & USB_ON);
	}
	return m_ErrorCode;
}

//////////////////////////////////////////////////////////////////////////
//操作
//////////////////////////////////////////////////////////////////////////

//连接仪器
//@param pParam 连接参数，根据型号不同可能有不同解释
//@return 
//	-  0 成功
//  - <0 失败  
long CDevice4xEx::Connect(void *pParam)
{

	m_ErrorCode = HandShakeM10(m_szKeyR1);	

	return m_ErrorCode;
}

long CDevice4xEx::SaveParaEx()
{
	memset(&m_MachParameter,0,sizeof(MACH_PARAMETER));
	memcpy(&m_MachParameter.coordConvertor,m_CoordConverter,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
	strcpy(m_MachParameter.szTimeStamp,m_strTimeStamp.GetBuffer(30));
	strcpy(m_MachParameter.szCameraID,m_strCameraID.GetBuffer(30));
	strcpy(m_MachParameter.szSealInfo,m_strSealInfo.GetBuffer(30));
	SaveAllParaToMach(&m_MachParameter,sizeof(MACH_PARAMETER));
	

	char szFile[1024];
	char szPath[1024];
	memset(szFile,0,1024);
	memset(szPath,0,1024);

	GetModuleFileName( GetModuleHandle("DfjySeal.dll"), szPath, MAX_PATH );
	GetLongPathName(szPath, szFile, MAX_PATH);
	PathRemoveFileSpec(szFile);

	strcat(szFile,"\\parambak.dat");

	CFile file;
	if(file.Open(szFile,CFile::modeCreate|CFile::modeWrite))
	{
		file.Write(&m_CoordConverter,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
		file.Close();
		return 0;
	}

	return -1;
}

long CDevice4xEx::LoadParaEx()
{
	int r = LoadDataFromMach();
	memcpy(&m_CoordConverter,&m_MachParameter.coordConvertor,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
	SetCameraID(m_MachParameter.szCameraID);
	SetTimeStamp(m_MachParameter.szTimeStamp);
	SetSealInfo(m_MachParameter.szSealInfo);

	CString msg;
	msg.Format("fx1=%f,fx2=%f,offsetX=%f,fy1=%f,fy2=%f,offsetY=%f",
		m_CoordConverter->dFactorX1,m_CoordConverter->dFactorX2,m_CoordConverter->dOffsetX,
		m_CoordConverter->dFactorY1,m_CoordConverter->dFactorY2,m_CoordConverter->dOffsetY);

	return r;

	char szFile[1024];
	char szPath[1024];
	memset(szFile,0,1024);
	memset(szPath,0,1024);

	GetModuleFileName( GetModuleHandle("DfjySeal.dll"), szPath, MAX_PATH );
	GetLongPathName(szPath, szFile, MAX_PATH);
	PathRemoveFileSpec(szFile);

	strcat(szFile,"\\param.dat");

	CFile file;
	if(file.Open(szFile,CFile::modeRead))
	{
		file.Read(&m_CoordConverter,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
		file.Close();
		return 0;
	}

	return -1;
}

long CDevice4xEx::ExportParam()
{
	CFileDialog dlg(FALSE);
	if(dlg.DoModal() != IDOK )
		return -1;

	CFile file;
	if(file.Open(dlg.GetPathName(),CFile::modeCreate|CFile::modeWrite))
	{
		file.Write(&m_CoordConverter,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
		file.Close();
		return 0;
	}

	return -1;
}

long CDevice4xEx::ImportParam()
{
	CFileDialog dlg(TRUE);
	if(dlg.DoModal() != IDOK )
		return -1;

	CFile file;
	if(file.Open(dlg.GetPathName(),CFile::modeRead))
	{
		file.Read(&m_CoordConverter,sizeof(COORDINATE_CONVERTOR)*MAX_SEAL);
		file.Close();
	}

	return SaveParaEx();
}

//断开连接
//@return 
// -  0 成功
// - <0 失败 
void CDevice4xEx::Disconnect()
{
//	m_RawDevice.Close();
}

//打开灯
//返回：正确：0 
//	错误：	0xFFFF1301：命令长度错误
//	0xFFFF1302：PC命令校验错
//	0xFFFF1303：上位机下发命令非法
//	0xFFFF1304：上位机下发旧命令
long CDevice4xEx::LightOff()
{
	char param='\xAA';
	m_ErrorCode=CommWithDevice(DEVCMD_LIGHT_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;
}
//关闭灯
//返回：正确：0 
//	错误：	0xFFFF1301：命令长度错误
//	0xFFFF1302：PC命令校验错
//	0xFFFF1303：上位机下发命令非法
//	0xFFFF1304：上位机下发旧命令
long  CDevice4xEx::LightOn()
{
	char param='\x55';
	m_ErrorCode=CommWithDevice(DEVCMD_LIGHT_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}

//USB接口断电
//返回：正确：0 
//	错误：	0xFFFF1401：命令长度错误
//	0xFFFF1402：PC命令校验错
//	0xFFFF1403：上位机下发命令非法
//	0xFFFF1404：上位机下发旧命令
long CDevice4xEx::UsbOff(void)
{
	if (GetDeviceMode() == DEVICE_TYPE_M_20
		|| GetDeviceMode() == DEVICE_TYPE_S_31
		|| GetDeviceMode() == DEVICE_TYPE_M_30
		|| GetDeviceMode() == DEVICE_TYPE_M_40)
	{
		return 0;
	}

	char param='\xAA';
	m_ErrorCode=CommWithDevice(DEVCMD_USB_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//USB接口加电
//返回：正确：0 
//	错误：	0xFFFF1401：命令长度错误
//	0xFFFF1402：PC命令校验错
//	0xFFFF1403：上位机下发命令非法
//	0xFFFF1404：上位机下发旧命令
long CDevice4xEx::UsbOn(void)
{
	if (GetDeviceMode() == DEVICE_TYPE_M_20
		|| GetDeviceMode() == DEVICE_TYPE_S_31
		|| GetDeviceMode() == DEVICE_TYPE_M_30
		|| GetDeviceMode() == DEVICE_TYPE_M_40)
	{
		return 0;
	}

	char param='\x55';
	m_ErrorCode=CommWithDevice(DEVCMD_USB_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;
}

//关闭进纸门
//返回：正确：0 
//错误：	0xFFFF1101：命令长度错误
//0xFFFF1102：PC命令校验错
//0xFFFF1103：上位机下发命令非法
//0xFFFF1104：上位机下发旧命令
long CDevice4xEx::LockDoor(void)
{
	char param='\xAA';
	m_ErrorCode=CommWithDevice(DEVCMD_DOOR_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//打开进纸门，并停止计时
//返回：正确：0 
//错误：	0xFFFF1101：命令长度错误
//0xFFFF1102：PC命令校验错
//0xFFFF1103：上位机下发命令非法
//0xFFFF1104：上位机下发旧命令
long CDevice4xEx::UnlockDoor(void)
{
	char param='\x55';
	m_ErrorCode=CommWithDevice(DEVCMD_DOOR_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//打开进纸门
//返回：正确：0 
//错误：	0xFFFF1101：命令长度错误
//0xFFFF1102：PC命令校验错
//0xFFFF1103：上位机下发命令非法
//0xFFFF1104：上位机下发旧命令
long CDevice4xEx::UnlockDoorEx(void)
{
	char param='\xFF';
	m_ErrorCode=CommWithDevice(DEVCMD_DOOR_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//关闭安全门
//返回：正确：0x00001200 
//	错误：	0xFFFF1201：命令长度错误
//	0xFFFF1202：PC命令校验错
//	0xFFFF1203：上位机下发命令非法
//	0xFFFF1204：上位机下发旧命令
long CDevice4xEx::LockExit()
{
	char param='\xAA';
	m_ErrorCode=CommWithDevice(DEVCMD_EXIT_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//打开安全门
//返回：正确：0x00001200 
//	错误：	0xFFFF1201：命令长度错误
//	0xFFFF1202：PC命令校验错
//	0xFFFF1203：上位机下发命令非法
//	0xFFFF1204：上位机下发旧命令
long CDevice4xEx::UnlockExit()
{
	char param='\x55';
	m_ErrorCode=CommWithDevice(DEVCMD_EXIT_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;

}
//抬起印章头
//返回：正确：0x00003200 
//	错误：	0xFFFF3201：命令长度错误
//	0xFFFF3202：PC命令校验错
//	0xFFFF3203：上位机下发命令非法
//	0xFFFF3204：上位机下发旧命令
//	0xFFFF320A：盖章下压未离开高点
//	0xFFFF320B：盖章下压未到低点
//	0xFFFF320C：盖章抬起未离开最低点
//	0xFFFF320D：盖章抬起未到高点
long CDevice4xEx::TakeUpSeal(void)
{
	char param='\xAA';
	m_ErrorCode=CommWithDevice(DEVCMD_SEAL_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;
}

//压下印章头
//返回：正确：0x00003200 
//	错误：	0xFFFF3201：命令长度错误
//	0xFFFF3202：PC命令校验错
//	0xFFFF3203：上位机下发命令非法
//	0xFFFF3204：上位机下发旧命令
//	0xFFFF320A：盖章下压未离开高点
//	0xFFFF320B：盖章下压未到低点
//	0xFFFF320C：盖章抬起未离开最低点
//	0xFFFF320D：盖章抬起未到高点
long CDevice4xEx::PutDownSeal(void)
{
	char param='\x55';
	m_ErrorCode=CommWithDevice(DEVCMD_SEAL_CTRL, &param, 1, NULL, NULL);
	return m_ErrorCode;
}

//印章归位
//返回：正确：0 
//错误：
//0xFFFF3401：命令长度错误
//0xFFFF3402：PC命令校验错
//0xFFFF3403：上位机下发命令非法
//0xFFFF3404：上位机下发旧命令
//0xFFFF3409：印章平台返回原点失败
//0xFFFF340C：盖章抬起未离开最低点
//0xFFFF340D：盖章抬起未到高点
//0xFFFF340E：回原点后印章头未在高点
long CDevice4xEx::ResetSeal()
{
	m_ErrorCode=CommWithDevice(DEVCMD_SEAL_RESET, NULL, 0, NULL, NULL);
	return m_ErrorCode;
}


//印章检测
//返回：正确：0x00002200 + 参数
//	错误：	0xFFFF2201：命令长度错误
//	0xFFFF2202：PC命令校验错
//	0xFFFF2203：上位机下发命令非法
//	0xFFFF2204：上位机下发旧命令
//	返回参数：一个字节数
//	0位：1：下压到位，0：下压未到位
//	4位：1：抬起到位，0：抬起未到位
long CDevice4xEx::GetSealState(unsigned char &state)
{
	char buffer[1];
	long recvLen=1;
    m_ErrorCode=CommWithDevice(DEVCMD_SEAL_CHECK, NULL, NULL, buffer, &recvLen);
	if(m_ErrorCode==0)
	{
		state=buffer[0];
	}
	return m_ErrorCode;
}

//选择印章头
//@param nSeal 印章编号
//返回：
//正确：0 
//	错误：	0xFFFF3501：命令长度错误
//	0xFFFF3502：PC命令校验错
//	0xFFFF3503：上位机下发命令非法
//	0xFFFF3504：上位机下发旧命令
long CDevice4xEx::SelectSeal(int nSeal)
{
	char n=nSeal;
	m_ErrorCode=CommWithDevice(DEVCMD_SELECT_SEAL, &n, 1, NULL, NULL,true);
	return m_ErrorCode;
}
long CDevice4xEx::SelectSealM10(int nSeal,int nAngle)
{
	char n=nSeal;
	char Data[2];
	Data[0] = n;
	n = nAngle;
	Data[1] = n;

	if (GetDeviceMode() == DEVICE_TYPE_S_30)
	{
		return 0;
	}
	m_ErrorCode=CommWithDevice(DEVCMD_SELECT_SEAL, Data, 2, NULL, NULL,true);
	return m_ErrorCode;
}

///用印机复位命令
//返回：
//正确：
//		0
//错误：
//		0xFFFF6201：命令长度错误
//		0xFFFF6202：PC命令校验错
//		0xFFFF6203：上位机下发命令非法
//		0xFFFF6204：上位机下发旧命令
long CDevice4xEx::ResetMachine()
{
	m_ErrorCode=CommWithDevice(DEVCMD_RESET_MACHINE, NULL, 0, NULL, NULL);
	return m_ErrorCode;
}

//印章平台移动

//X：X轴步数
//Y：Y轴步数高
//DIR：位移方向：
//0位：0：x反向位移
//0位：1：x正向位移
//4位：0：y反向位移
//4位：1：y正向位移
//返回：正确：0x00003100 
//错误：	0xFFFF3101：命令长度错误
//0xFFFF3102：PC命令校验错
//0xFFFF3103：上位机下发命令非法
//0xFFFF3104：上位机下发旧命令
//0xFFFF3108：印章平台移动，未离开原点
//0xFFFF3109：印章平台返回原点失败
long CDevice4xEx::Move(unsigned short X,unsigned short Y, unsigned char direction)
{
	char buffer[FRAME_MAX_LEN];
	memcpy(buffer, &X, 2);
	memcpy(buffer + 2, &Y, 2);
	buffer[4]=direction;
	m_ErrorCode=CommWithDevice(DEVCMD_SEAL_MOVE, buffer, 5, NULL, NULL,true);
	return m_ErrorCode;
}

//盖章组合命令
//X：X轴步数
//Y：Y轴步数
//SHAKE：摇动印章（现在为0，不需要摇动印章）
//返回：正确：0 
//错误：	0xFFFF3301：命令长度错误
//0xFFFF3302：PC命令校验错
//0xFFFF3303：上位机下发命令非法
//0xFFFF3304：上位机下发旧命令
//0xFFFF3306：蘸印泥下行检测超时
//0xFFFF3307：蘸印泥抬起超时
//0xFFFF3308：印章平台移动，未离开原点
//0xFFFF3309：印章平台返回原点失败
//0xFFFF330A：盖章下压未离开高点
//0xFFFF330B：盖章下压未到低点
//0xFFFF330C：盖章抬起未离开最低点
//0xFFFF330D：盖章抬起未到高点
//0xFFFF330E：回原点后印章头未在高点
//0xFFFF3310：盖章前进纸门异常打开
//0xFFFF3311：盖章后进纸门异常打开
long CDevice4xEx::SingleSeal(unsigned short X, unsigned short Y, BYTE bShake)
{
	//2013-2-20 addby kongbin,suppurt M30
	short sX = X;
	if(m_nDeviceMode == DEVICE_TYPE_M_30)
		sX = X - M30_X_OFFSET;

	char buffer[FRAME_MAX_LEN];
	memcpy(buffer, &sX, 2);
	memcpy(buffer + 2, &Y, 2);
	buffer[4]= bShake;
	m_ErrorCode=CommWithDevice(DEVCMD_SINGLE_SEAL, buffer, 5, NULL, NULL,true);
	return m_ErrorCode;
}


//盖章组合命令2
//X：X轴步数
//Y：Y轴步数
//SHAKE：摇动印章（现在为0，不需要摇动印章）
//返回：正确：0 
//错误：	0xFFFF3301：命令长度错误
//0xFFFF3302：PC命令校验错
//0xFFFF3303：上位机下发命令非法
//0xFFFF3304：上位机下发旧命令
//0xFFFF3306：蘸印泥下行检测超时
//0xFFFF3307：蘸印泥抬起超时
//0xFFFF3308：印章平台移动，未离开原点
//0xFFFF3309：印章平台返回原点失败
//0xFFFF330A：盖章下压未离开高点
//0xFFFF330B：盖章下压未到低点
//0xFFFF330C：盖章抬起未离开最低点
//0xFFFF330D：盖章抬起未到高点
//0xFFFF330E：回原点后印章头未在高点
//0xFFFF3310：盖章前进纸门异常打开
//0xFFFF3311：盖章后进纸门异常打开
//0xFFFF3712: 远点到位
long CDevice4xEx::SingleSealNoInkpad(unsigned short X, unsigned short Y, BYTE bShake)
{
	//2013-2-20 addby kongbin,suppurt M30
	short sX = X;
	if(m_nDeviceMode == DEVICE_TYPE_M_30)
		sX = X - M30_X_OFFSET;

	char buffer[FRAME_MAX_LEN];
	memcpy(buffer, &sX, 2);
	memcpy(buffer + 2, &Y, 2);
	buffer[4]= bShake;
	m_ErrorCode=CommWithDevice(DEVCMD_SINGLE_NOINKPAD, buffer, 5, NULL, NULL,true);
	return m_ErrorCode;
}

//安全用印命令
//X：X轴步数
//Y：Y轴步数
//SHAKE：摇动印章（现在为0，不需要摇动印章）
//返回：正确：0 
long CDevice4xEx::SafeSeal(unsigned short X, unsigned short Y, BYTE bShake, BOOL bInkpad)
{
	//2013-2-20 addby kongbin,suppurt M30
	short sX = X;
	if(m_nDeviceMode == DEVICE_TYPE_M_30)
		sX = X - M30_X_OFFSET;

	char buffer[FRAME_MAX_LEN];
	memcpy(buffer, &sX, 2);
	memcpy(buffer + 2, &Y, 2);
	buffer[4]= bShake;
	buffer[5]= bInkpad;
	m_ErrorCode=CommWithDevice(DEVCMD_SAFE_SEAL, buffer, 6, NULL, NULL,true);
	return m_ErrorCode;
}


long CDevice4xEx::SelfExamine()
{
	m_ErrorCode=CommWithDevice(DEVCMD_SELF_EXAMINE, NULL, 0, NULL, NULL);
	return m_ErrorCode;
}

//翻页板抬起（未实现）
//返回：正确：0x00001600 
//错误：	0xFFFF1601：命令长度错误
//0xFFFF1602：PC命令校验错
//0xFFFF1603：上位机下发命令非法
//0xFFFF1604：上位机下发旧命令
//0xFFFF1612：抬起、放下 翻页板没到位
long CDevice4xEx::TackUpPageHandle()
{
	char param[]="\0x55";
	m_ErrorCode=CommWithDevice(DEVCMD_PAGE_HANDLE_CTRL, param, 1, NULL, NULL);
	return m_ErrorCode;
}
//放下翻页板
//返回：正确：0x00001600 
//错误：	0xFFFF1601：命令长度错误
//0xFFFF1602：PC命令校验错
//0xFFFF1603：上位机下发命令非法
//0xFFFF1604：上位机下发旧命令
//0xFFFF1612：抬起、放下 翻页板没到位
long CDevice4xEx::PutDownPageHandle()
{
	char param[]="\0xAA";
	m_ErrorCode=CommWithDevice(DEVCMD_PAGE_HANDLE_CTRL, param, 1, NULL, NULL);
	return m_ErrorCode;
}
//
long CDevice4xEx::RotateSeal(unsigned short angle)
{
	return -100;  //not implemente
}

//拨码开关检测
//返回：正确：0x00002300 + 参数
//	错误：	0xFFFF2301：命令长度错误
//	0xFFFF2302：PC命令校验错
//	0xFFFF2303：上位机下发命令非法
//	0xFFFF2304：上位机下发旧命令
//	返回参数：一个字节数        (未实现)
long CDevice4xEx::GetSwitchState(unsigned char &state)
{
	long len=1;
	m_ErrorCode=CommWithDevice(DEVCMD_SWITCH_CHECK, NULL, 0, (char*)&state, &len);
	return m_ErrorCode;
}


//位置
long CDevice4xEx::SavePosition(int i,COORDINATE_CONVERTOR &coordConverter)
{
	m_CoordConverter[i].dFactorX1 = coordConverter.dFactorX1;
	m_CoordConverter[i].dFactorX2 = coordConverter.dFactorX2;
	m_CoordConverter[i].dFactorY1 = coordConverter.dFactorY1;
	m_CoordConverter[i].dFactorY2 = coordConverter.dFactorY2;
	m_CoordConverter[i].dOffsetX = coordConverter.dOffsetX;
	m_CoordConverter[i].dOffsetY = coordConverter.dOffsetY;
	return 0;//SaveCoordConvertor(0,coordConverter);;
}



//获取同步盖章总数 4 Bytes
long CDevice4xEx::GetSynSealUsedCount(unsigned long * pSynData,unsigned long *pSealData2 ,unsigned char ucSealIndex)
{
/*	char buffer[FRAME_MAX_LEN];
	unsigned char len = 4;
	m_ErrorCode = LoadData(LoadDataAddress[ADDR_SYN_SEAL_COUNT_START + ucSealIndex], buffer, &len);
	if(m_ErrorCode==0)
	{
		ASSERT(len >= sizeof(long));
	}
	else
	{
		return m_ErrorCode;
	}
	memcpy(pSynData,buffer,sizeof(long));

	len = 4;
	m_ErrorCode = LoadData(LoadDataAddress[ADDR_SEAL_COUNT_START + ucSealIndex], buffer, &len);
	if(m_ErrorCode==0)
	{
		ASSERT(len >= sizeof(long));
		m_SealUsedCount[ucSealIndex]=*(long *) buffer;
	}
	else
	{
		return m_ErrorCode;
	}
	memcpy(pSealData2,buffer,sizeof(long));
	*/
	return 0;
}

//设置同步盖章总数 4 Bytes
long CDevice4xEx::SetSynSealUsedCount(unsigned char ucSealIndex)
{
/*	char buffer[FRAME_MAX_LEN];
	unsigned char len = 4;

	m_ErrorCode = LoadData(LoadDataAddress[ADDR_SEAL_COUNT_START + ucSealIndex], buffer, &len);
	if(m_ErrorCode==0)
	{
		ASSERT(len >= sizeof(long));
		m_SealUsedCount[ucSealIndex]=*(long *) buffer;
	}
	else
	{
		return m_ErrorCode;
	}

	m_ErrorCode = SaveData(LoadDataAddress[ADDR_SYN_SEAL_COUNT_START + ucSealIndex], buffer, 4);
	if(m_ErrorCode != 0)
	{
		return m_ErrorCode;
	}
*/
	return 0;
}

long CDevice4xEx::SaveMacAddress(char *strAuthCode,BYTE btType)
{
	m_ErrorCode = SaveData(LoadDataAddress[ADDR_AUTH_CODE + btType], strAuthCode, 12);	
	return m_ErrorCode;
}

long CDevice4xEx::SaveCameraID(unsigned __int64 nllCamID)
{	
	return m_ErrorCode = SaveData(LoadDataAddress[ADD_CAMM_ID], &nllCamID, LEN_CAMA_ID);
}


long CDevice4xEx::LoadLastTimeMach(unsigned __int64 & llTime)
{
	char buffer[FRAME_MAX_LEN] = "";
	unsigned char lenRet = LEN_MODIFI_TIME;

	m_ErrorCode = LoadData(LoadDataAddress[ADDR_MODIFI_TIME], buffer, &lenRet);
	if(m_ErrorCode==0)
	{
		ASSERT(lenRet >= lenRet);//sizeof(COORDINATE_CONVERTOR));
		memcpy(&llTime, buffer,lenRet);
	}
	else
	{
		return m_ErrorCode;
	}	
	return 0;
}
long CDevice4xEx::SaveLastTimeMach(__time64_t ltime)
{
	//	__time64_t ltime;
	unsigned char lenRet = sizeof(__time64_t);

	//	_time64( &ltime );

	m_ErrorCode = SaveData(LoadDataAddress[ADDR_MODIFI_TIME],&ltime, lenRet);
	if(m_ErrorCode==0)
	{
		ASSERT(lenRet == sizeof(__time64_t));//sizeof(COORDINATE_CONVERTOR));
	}
	else
	{
		return m_ErrorCode;
	}	
	return 0;
}

long CDevice4xEx::LoadLastTimeReg(unsigned __int64 & llTime)
{
	//	char readValue[2048];
	HKEY hKey = NULL;
	long nRet;
	DWORD type = REG_BINARY;
	DWORD dwBufLen = sizeof(unsigned __int64);


	nRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,SETTING_KEY , 0, KEY_QUERY_VALUE, &hKey );
	if(nRet == ERROR_SUCCESS)
	{
		nRet = RegQueryValueEx( hKey,LAST_MODIFY_TIME, NULL, &type, (LPBYTE) &llTime, &dwBufLen);
		if (nRet == ERROR_SUCCESS)
		{
			if(sizeof(unsigned __int64) != dwBufLen)
			{
				RegCloseKey( hKey );
				return 0;
			}
		}
		else
		{
			return 0;
		}
	}
	if(hKey)
	{
		RegCloseKey( hKey );
	}

	return 0;
}

long CDevice4xEx::SaveLastTimeReg(__time64_t ltime)
{
	HKEY hKey = NULL;
	long nRet;
	//	__time64_t ltime;
	unsigned char dwSize = sizeof(__time64_t);
	DWORD  dwDisposition;

	PSECURITY_DESCRIPTOR pSD = NULL;
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	//	_time64( &ltime );

	nRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,SETTING_KEY,0, KEY_SET_VALUE, &hKey );
	if (nRet != ERROR_SUCCESS) 
	{
		if(hKey)
		{
			RegCloseKey(hKey);
		}
		nRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SETTING_KEY, 0, "", 0, KEY_READ | KEY_WRITE, &sa, &hKey, &dwDisposition); 
		if (nRet == ERROR_SUCCESS)
		{
			nRet = RegSetValueEx(hKey, LAST_MODIFY_TIME,0,REG_BINARY,(LPBYTE) &ltime,dwSize);
		}
	}
	else
	{
		nRet = RegSetValueEx(hKey,LAST_MODIFY_TIME,0,REG_BINARY,(LPBYTE) &ltime,dwSize);
	}

	if (hKey)
	{
		RegCloseKey( hKey );
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
long CDevice4xEx::CommWithDeviceBK(const char *CmdCode,const long lCmdLen,const char *RetCmdCode,const long lRetCmdLen,
								   const char *pIutput, long inLen, char *pOutput, long *outLen)
{
	char buffer[FRAME_MAX_LEN]="";
	long sendLen = lCmdLen + inLen;
	long readLen = 0;

	memcpy(buffer,CmdCode,lCmdLen);
	if (inLen > 0 )
	{
		memcpy(buffer + lCmdLen,pIutput,inLen);
	}
	if (buffer[2] != 0x45) 
	{
		ConvertEncryptionBK(m_szKeyR0,  buffer,&sendLen);//加密 //命令属性是0x00时不加密
		sendLen = 17 + inLen;
	}

	if(m_ErrorCode = m_RawDevice.Open(m_strCommPort,60,"2400,n,8,1") != 0)
	{
		return m_ErrorCode;
	}
	if(m_RawDevice.Send( buffer, sendLen)==sendLen)
	{
		//send ok
		memset(buffer,0,sizeof(buffer));
		if (outLen) 
		{
			readLen=m_RawDevice.Read( buffer,*outLen + 4);
		}
	
		if(readLen > 0)
		{
			if(!memcmp(buffer,RetCmdCode,lRetCmdLen) && readLen != *outLen + 4)
			{
				return -4;
			}

			if (*outLen > 0)
			{
				*outLen = readLen - 4;
				memcpy(pOutput,buffer + 4,*outLen);
			}
			m_RawDevice.Close();
			return 0;
		}
	}
	m_RawDevice.Close();

	return -4; //communication error
}

//备板握手
long CDevice4xEx::HandleBK()
{
	char CmdCode[16] = "\x02\x04\x45\x45\x45\x45";
	long lCmdLen = 6;
	char RetCmdCode[8] = "\x45\x45\x45";
	long lRetCmdLen = 3;
	char *pIutput = NULL;
	long inLen = 0;
	char Output[256];
	long outLen = 4;

	m_ErrorCode = CommWithDeviceBK((const char *)CmdCode,lCmdLen,(const char *)RetCmdCode,lRetCmdLen,
		NULL,0,(char *)Output,&outLen);
	if (m_ErrorCode == 0)
	{
		m_lDeviceID = *(long*)Output;
	}
	return m_ErrorCode;
}
//关闭备板安全门
//返回：正确：0 
long CDevice4xEx::LockBKExit()
{	
	char CmdCode[FRAME_MAX_LEN] = "";
	long lCmdLen = 6;

	char RetCmdCode[8] = "\x11\x11\x11\x11";
	long lRetCmdLen = 4;
	long outLen = 0;

	memcpy(CmdCode,"\x02\x00\x00\x01\x11\xaa",6);

	m_ErrorCode = CommWithDeviceBK((const char *)CmdCode,lCmdLen,(const char *)RetCmdCode,lRetCmdLen,
		NULL,0,NULL,&outLen);

	return m_ErrorCode;
}
//打开备板安全门
//返回：正确：0 
long CDevice4xEx::UnlockBKExit()
{
	char CmdCode[FRAME_MAX_LEN] = "";
	long lCmdLen = 6;

	char RetCmdCode[8] = "\x11\x11\x11\x11";
	long lRetCmdLen = 4;
	long outLen = 0;

	memcpy(CmdCode,"\x02\x00\x00\x01\x11\xaa",6);

	m_ErrorCode = CommWithDeviceBK((const char *)CmdCode,lCmdLen,(const char *)RetCmdCode,lRetCmdLen,
		NULL,0,NULL,&outLen);
	Sleep(9000);
	return m_ErrorCode;
}


//写备板数据
//返回：正确：0 
long CDevice4xEx::WriteDataBK(char *strData)
{
	char CmdCode[FRAME_MAX_LEN] = "";
	long lCmdLen = 8;

	char RetCmdCode[8] = "\x93\x93\x93";
	long lRetCmdLen = 3;
	char Iutput[33];
	long inLen = 32;
	char Output[256];
	long outLen = 0;

	memset(Iutput,0,sizeof(Iutput));
	strncpy(Iutput,strData,32);
	Encryption_Ex(Iutput,m_szKeyR3);

	memcpy(CmdCode,"\x02\x00\x00\x01\x93\x00\x20\x20",8);

	m_ErrorCode = CommWithDeviceBK((const char *)CmdCode,lCmdLen,(const char *)RetCmdCode,lRetCmdLen,
		Iutput,inLen,(char *)Output,&outLen);

	return m_ErrorCode;

}

//读备板数据
//返回：正确：0 
long CDevice4xEx::ReadDataBK(char * strData)
{
	char CmdCode[16] = "\x02\x00\x00\x01\x94\x00\x20\x20";

	long lCmdLen = 8;
	char RetCmdCode[8] = "\x94\x94\x94";
	long lRetCmdLen = 3;
	char *pIutput = NULL;
	long inLen = 0;
	char Output[256];
	long outLen = 32;

	m_ErrorCode = CommWithDeviceBK((const char *)CmdCode,lCmdLen,(const char *)RetCmdCode,lRetCmdLen,
		NULL,0,(char *)Output,&outLen);
	memcpy(strData,Output,outLen);

	return m_ErrorCode;
}

long CDevice4xEx::CheckAuthCodeBK(char *strData)
{
	char strEn[33];
	char strEnOld[33];

	memset(strEnOld,0,sizeof(strEnOld));
	strncpy(strEnOld,strData,32);
	Encryption_Ex(strEnOld,m_szKeyR3);

	memset(strEn,0,sizeof(strEn));
	m_ErrorCode = ReadDataBK(strEn);
	if (m_ErrorCode != 0)
	{
		return m_ErrorCode;
	}

	if(memcmp(strEn,strEnOld,32))
	{
		return -1;
	}
	return 0;
}

//二进制
//类型
long CDevice4xEx::CreateKeyInitValue_B(void* value,DWORD dwSize)
{
	DWORD  dwDisposition;
	char  szSetValue[2048];
	PSECURITY_DESCRIPTOR pSD = NULL;
	SECURITY_ATTRIBUTES sa;
	long nRet ;
	HKEY hkSub = NULL;

	ASSERT(dwSize <= 2048);

	sa.nLength = sizeof (SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = pSD;
	sa.bInheritHandle = FALSE;

	memcpy(szSetValue,value,dwSize);


	nRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE, SETTING_KEY, 0, "", 0, KEY_READ | KEY_WRITE, &sa, &hkSub, &dwDisposition); 
	if (nRet == ERROR_SUCCESS)
	{
		nRet = RegSetValueEx(hkSub, MACH_PARA_VALIE_NAME,0,REG_BINARY,(LPBYTE) szSetValue,dwSize);
	}

	if (hkSub) 
	{
		RegCloseKey(hkSub);
	}

	return nRet;
}

long CDevice4xEx::LoadDataFromReg_B(void *value,DWORD *dwSize)
{
	char readValue[2048];
	HKEY hKey = NULL;
	long nRet;
	DWORD type = REG_BINARY;
	DWORD dwBufLen = 2048;


	nRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,SETTING_KEY , 0, KEY_QUERY_VALUE, &hKey );
	if(nRet == ERROR_SUCCESS)
	{
		nRet = RegQueryValueEx( hKey,MACH_PARA_VALIE_NAME, NULL, &type, (LPBYTE) readValue, &dwBufLen);
		if (nRet == ERROR_SUCCESS)
		{
			if(dwBufLen == 0)
			{
				if(hKey)
				{
					RegCloseKey( hKey );
				}

				return -1;
			}
			if(*dwSize != dwBufLen)
			{
				return -1;
			}
			memcpy(value,readValue,dwBufLen);
		}
	}
	if(hKey)
	{
		RegCloseKey( hKey );
	}

	return nRet;
}

long CDevice4xEx::SaveDataFromReg_B(void* value,DWORD dwSize)
{
	HKEY hKey = NULL;
	long nRet;

	ASSERT(dwSize<= 2048 || value != NULL);

	nRet = RegOpenKeyEx( HKEY_LOCAL_MACHINE,SETTING_KEY,0, KEY_SET_VALUE, &hKey );
	if (nRet != ERROR_SUCCESS) 
	{
		if(hKey)
		{
			RegCloseKey(hKey);
		}
		nRet = CreateKeyInitValue_B(value,dwSize); 
		return nRet;
	}
	nRet = RegSetValueEx(hKey,MACH_PARA_VALIE_NAME,0,REG_BINARY,(LPBYTE) value,dwSize);
	if (hKey)
	{
		RegCloseKey( hKey );
	}

	return nRet;
}

long CDevice4xEx::LoadParaReg()
{
	char buffer[2048];
	DWORD dwSize = sizeof(MACH_PARAMETER);
	if ((m_ErrorCode = LoadDataFromReg_B(buffer,&dwSize)) == 0)
	{
		memcpy(&m_MachParameter,buffer,sizeof(MACH_PARAMETER));
	}
	return m_ErrorCode;
}

long CDevice4xEx::SaveParaReg()
{
	char  value[2048];
	DWORD dwSize = sizeof(MACH_PARAMETER);
	long nRet = 0;
	memcpy(value,&m_MachParameter,sizeof(MACH_PARAMETER));
//	if (m_nDataReg == 0)
	{
		nRet = SaveDataFromReg_B(value,dwSize);
		if (nRet == 0)
		{
			memcpy(&m_MachParameter,value,dwSize);
		}
	}
	return nRet;
}


long CDevice4xEx::SavePara(void * value,DWORD dwSize)
{
	long nRet = 0;

	nRet = SaveDataFromReg_B(value,dwSize);
	if (nRet == 0)
	{
		memcpy(&m_MachParameter,value,dwSize);
	}
	else
	{
		return nRet;
	}
	return nRet;
}

byte CDevice4xEx::GetSealType(int index)
{
	return m_SealType[index];
}

byte CDevice4xEx::SetSealType(int index,byte type)
{
	m_SealType[index] = type;
	return 0;
}


long CDevice4xEx::GetCurrentSeal(BYTE &btSeal,BYTE &btAngle)
{
	char buf[2];
	long nRecvLen = 2;
	memset(buf,0,2);

	m_ErrorCode = CommWithDevice(DEVCMD_GET_CURRENT_SEAL, NULL, 0, (char*)buf, &nRecvLen); 
	if (0 != m_ErrorCode) 
	{
		return m_ErrorCode;
	}

	btSeal = buf[0];
	btAngle = buf[1];

	return 0;
}

long CDevice4xEx::SetCurrentSeal(BYTE btSeal,BYTE btAngle)
{
	if(m_MachineVer < _DEVICE_VER_1020)
	{
		return 0;
	}

	char param[2];
	param[0] = btSeal;
	param[1] = btAngle;
	m_ErrorCode=CommWithDevice(DEVCMD_SET_CURRENT_SEAL, param, 2, NULL, NULL);
	return m_ErrorCode;
}

long CDevice4xEx::SetSealParam(BYTE len,char *param)
{
	//if((m_MachineVer < _DEVICE_VER_1020) || (GetSealCount() == 1))
	//{
	//	return 0;
	//}

	char buf[32];
	memset(buf,0,32);
	buf[0] = len;
	memcpy(buf + 1,param,len);
	m_ErrorCode=CommWithDevice(DEVCMD_SET_CURRENT_SEAL_PARAM, buf, len + 1, NULL, NULL);
	return m_ErrorCode;
}

// isSetGet : true 设置， false 获取
long CDevice4xEx::ErrorState(bool isSetGet,int len,char* strParam)
{
	if(m_MachineVer < _DEVICE_VER_1020)
	{
		return 0;
	}

	unsigned char buf[32];
	long nRecvLen = 1;

	memset(buf,0,32);
	if (isSetGet)
	{
		buf[0] = 2;
		memcpy(buf + 1,strParam,len);
		m_ErrorCode=CommWithDevice(DEVCMD_ERROR_STATE, (char *)buf, len + 1, NULL, NULL);
	}
	else
	{
		buf[0] = 1;
		m_ErrorCode=CommWithDevice(DEVCMD_ERROR_STATE, (char *)buf, 1, strParam, &nRecvLen);

	}
	return m_ErrorCode;
}

long CDevice4xEx::GetSealInstallState(char *param)
{
	char state[2];
	memset(state,0,2);
	long len=1;
	m_ErrorCode=CommWithDevice(DEVCMD_SEAL_STATE, NULL, 0, state, &len);
	memcpy(param,state,1);
	return m_ErrorCode;
}

long CDevice4xEx::DipInkpad()
{
	//AfxMessageBox("蘸印泥");
	m_ErrorCode=CommWithDevice(DEVCMD_DIP_INKPAD, NULL,0, NULL, NULL);
	return m_ErrorCode;
}
