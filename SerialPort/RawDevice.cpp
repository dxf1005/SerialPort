#include "StdAfx.h"
#include "rawdevice.h"
#include <time.h>

#define COMM_DCB_DEFAULT "9600,n,8,1"

CRawDevice::CRawDevice()
{
	m_hHandle=NULL;
	m_hWriteHandle=CreateEvent(NULL, TRUE, FALSE, NULL);
	m_hReadHandle=CreateEvent(NULL, TRUE, FALSE, NULL);
}
CRawDevice::~CRawDevice()
{
	Close();
	if(m_hReadHandle)
	{
		CloseHandle(m_hReadHandle);
	}
	if(m_hWriteHandle)
	{
		CloseHandle(m_hWriteHandle);
	}
}
BOOL CRawDevice::IsOpened() const
{
	return m_hHandle!=NULL;
}
LONG CRawDevice::Open(CONST CHAR * port, DWORD timeout,CONST CHAR *DCB_string)
{
	if(m_hHandle)
	{
		return RAW_DEVICE_AREADY_OPENED;
	}
	m_dwTimeout=(timeout==MAXWORD? MAXWORD: timeout *1000);
	if(DCB_string==NULL)
	{
        strcpy(m_szDCB, COMM_DCB_DEFAULT);
	}
	else if(DCB_string!=m_szDCB)
	{
		strncpy(m_szDCB, DCB_string, 1024);
		m_szDCB[1023]=0;
	}
	if(port==NULL)
	{
		return RAW_DEVICE_INVALID_PARAMETER;
	}
	else
	{
		if(m_szPortName!=port)
		{
            strncpy(m_szPortName, port, 1024);
			m_szPortName[1023]=0;
		}
	}


	DCB			 DCBNew;
	COMMTIMEOUTS  COMMTIMEOUTSOut;			//超时
	//打开端口
	m_hHandle=CreateFile(m_szPortName,GENERIC_WRITE|GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,0);
	if(m_hHandle==INVALID_HANDLE_VALUE)
	{
		return RAW_DEVICE_OPEN_FAILED;	    //串口无法打开
	}
	if(SetupComm(m_hHandle,1024,1024)==FALSE)//建立缓冲区
	{
		Close();
		return RAW_DEVICE_SETUP;
	}
	if(GetCommState(m_hHandle,&DCBNew)==FALSE)//读取DCB结构
	{
		Close();
		return RAW_DEVICE_GET_COMM_STATE;
	}

	//设置超时结构ms
	COMMTIMEOUTSOut.ReadIntervalTimeout=MAXWORD;
	COMMTIMEOUTSOut.ReadTotalTimeoutMultiplier=0;
	COMMTIMEOUTSOut.ReadTotalTimeoutConstant=m_dwTimeout;
	COMMTIMEOUTSOut.WriteTotalTimeoutMultiplier=0;
	COMMTIMEOUTSOut.WriteTotalTimeoutConstant=m_dwTimeout;

	if (SetCommTimeouts(m_hHandle,&COMMTIMEOUTSOut)==FALSE)
	{
		Close();
		return RAW_DEVICE_SET_COMM_TIMEOUT;		//设置超时失败
	}
	//初始化DCB结构
	if (BuildCommDCB(m_szDCB,&DCBNew)==FALSE)	
	{
		Close();
		return RAW_DEVICE_BUILD_COMM_DCB;		//配置通信设备失败
	}

	if(SetCommState(m_hHandle,&DCBNew)==FALSE)
	{
		Close();
		return RAW_DEVICE_SET_COMM_STATE;
	}
	if(PurgeComm(m_hHandle,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR)==FALSE) //清除输入/输出缓出
	{
		Close();
		return RAW_DEVICE_CLEAR_BUFFER;
	}
	return 0;
}
VOID CRawDevice::Close()
{
	if(m_hHandle)
	{
		CloseHandle(m_hHandle);
		m_hHandle=NULL;
	}
};

LONG CRawDevice::Send(CONST CHAR *data, LONG len)
{
	DWORD dwError;
	COMSTAT errorState;
	if(m_hHandle==NULL)
	{
		return RAW_DEVICE_NOT_OPENED;
	}
	OVERLAPPED oWrite;		//异步写
	DWORD dwWrote=0;
	memset(&oWrite,0,sizeof(OVERLAPPED));
	//监视写通信
	ResetEvent(m_hWriteHandle);
	oWrite.hEvent=m_hWriteHandle;

	if(ClearCommError(m_hHandle, &dwError, &errorState)==FALSE)
	{
		return RAW_DEVICE_CLEAR_ERROR;
	};
	if(PurgeComm(m_hHandle,PURGE_TXCLEAR|PURGE_RXCLEAR)==FALSE) //清除输入/输出缓出
	{
		return RAW_DEVICE_CLEAR_ERROR;
	}

	//发送数据
	DWORD total=0;
	if (!WriteFile(	m_hHandle, data, (DWORD)len,	&dwWrote,	&oWrite) )
	{
		dwError=GetLastError();
		if (dwError == ERROR_IO_PENDING)
		{
			if(GetOverlappedResult(m_hHandle, &oWrite, &dwWrote,TRUE))
			{
				return (long)dwWrote;
			}
		}
		return RAW_DEVICE_WRITE_ERROR;
	}
	return dwWrote;
}
LONG CRawDevice::Read(CHAR *data, LONG len)
{
	if(m_hHandle==NULL)
	{
		return RAW_DEVICE_NOT_OPENED;
	}
	OVERLAPPED oRead;						//异步读
	DWORD dwRead=0;
	DWORD   dwError;	
	memset(&oRead,0,sizeof(OVERLAPPED));
	ResetEvent(m_hReadHandle);
	oRead.hEvent=m_hReadHandle;
	char *p=data;
	COMSTAT errorState;
	if(ClearCommError(m_hHandle, &dwError, &errorState)==FALSE)
	{
		return RAW_DEVICE_CLEAR_ERROR;
	};

	if (!ReadFile(m_hHandle,p,(DWORD)len,&dwRead,&oRead) )
	{
		dwError=GetLastError();
		if ( dwError == ERROR_IO_PENDING)
		{
			if(GetOverlappedResult(m_hHandle,&oRead,&dwRead,TRUE))
			{
				dwError=GetLastError();
				return (long)dwRead;
			}

			if(dwError==WAIT_TIMEOUT)
			{
				return RAW_DEVICE_READ_TIMEOUT;
			}
		}
		return RAW_DEVICE_READ_ERROR;
	}
	return dwRead;
}

LONG CRawDevice::Exchange(CONST CHAR *input, LONG input_len, CHAR *output, LONG output_len, DWORD timeout, BOOL reconnectFlag)
{
	long retCode;
	if(reconnectFlag)
	{
		Close();
		retCode=Open(m_szPortName, timeout, m_szDCB);
		if(retCode)
		{
			return retCode;
		}
	}
	else
	{
		if(PurgeComm(m_hHandle,PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR)==FALSE) //清除输入/输出缓出
		{
			Close();
			return RAW_DEVICE_CLEAR_BUFFER;
		}
	}
	retCode=Send(input, input_len);
	if(retCode >=0)
	{
		if(retCode < input_len)
		{
			return RAW_DEVICE_WRITE_TIMEOUT;
		}
	}
	else
	{
		return retCode;
	}
	retCode=Read(output, output_len);
	if(retCode >=0)
	{
		if(retCode < output_len)
		{
			return RAW_DEVICE_READ_TIMEOUT;
		}
	}
	else
	{
		return retCode;
	}
	return 0;
}
