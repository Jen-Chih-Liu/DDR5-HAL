#include "MyAacLedDeviceHalImp.h"
#include <map>
#include <string>
#include "../xml/GsXml.h"
#include "config.h"
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>
#include <debugapi.h> //for DebugView, asus hal debug use
#include "atlbase.h"
#include "atlstr.h"
//#define dbg_printf(fmt, ...) 
#define dbg_printf printf

#if 1
#define OUTINFO_0_PARAM(fmt, ...) 
#define OUTINFO_1_PARAM(fmt, ...) 
#define OUTINFO_2_PARAM(fmt, ...) 
#define OUTINFO_3_PARAM(fmt, ...)
#endif 

#if 0
#define OUTINFO_0_PARAM(fmt) {CHAR sOut[256];CHAR sfmt[50];sprintf_s(sfmt,"%s%s","INFO--",fmt);sprintf_s(sOut,(sfmt));OutputDebugStringW(CA2W(sOut));}    
#define OUTINFO_1_PARAM(fmt, var) { CHAR sOut[256]; CHAR sfmt[50]; sprintf_s(sfmt, "%s%s", "INFO--", fmt); sprintf_s(sOut, (sfmt), var); OutputDebugStringW(CA2W(sOut)); }
#define OUTINFO_2_PARAM(fmt, var1, var2) { CHAR sOut[256]; CHAR sfmt[50]; sprintf_s(sfmt, "%s%s", "INFO--", fmt); sprintf_s(sOut, (sfmt), var1, var2); OutputDebugStringW(CA2W(sOut)); }
#define OUTINFO_3_PARAM(fmt, var1, var2, var3) { CHAR sOut[256]; CHAR sfmt[50]; sprintf_s(sfmt, "%s%s", "INFO--", fmt); sprintf_s(sOut, (sfmt), var1, var2, var3); OutputDebugStringW(CA2W(sOut)); }
#endif


#define DRIVER_NAME    "MyPortIO"
#define Device_NAME    "\\\\.\\MyPortIODev"

char OutputBuffer[100];
char InputBuffer[100];
volatile char i2c_addrs[4] = { 0 };
#define GPD_TYPE 40000

#define GPD_IOCTL_READ_PORT_BASE_INDEX  0x900
#define GPD_IOCTL_WRITE_PORT_BASE_INDEX  0x910


#define IOCTL_GPD_READ_PORT_UCHAR  CTL_CODE(GPD_TYPE, GPD_IOCTL_READ_PORT_BASE_INDEX + 0, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_GPD_READ_PORT_USHORT CTL_CODE(GPD_TYPE, GPD_IOCTL_READ_PORT_BASE_INDEX + 1, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_GPD_READ_PORT_ULONG  CTL_CODE(GPD_TYPE, GPD_IOCTL_READ_PORT_BASE_INDEX + 2, METHOD_BUFFERED, FILE_READ_ACCESS)


#define IOCTL_GPD_WRITE_PORT_UCHAR CTL_CODE(GPD_TYPE, GPD_IOCTL_WRITE_PORT_BASE_INDEX + 0, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GPD_WRITE_PORT_USHORT CTL_CODE(GPD_TYPE, GPD_IOCTL_WRITE_PORT_BASE_INDEX + 1, METHOD_BUFFERED, FILE_WRITE_ACCESS)
#define IOCTL_GPD_WRITE_PORT_ULONG CTL_CODE(GPD_TYPE, GPD_IOCTL_WRITE_PORT_BASE_INDEX + 2, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// MMIO
#define IOCTL_GPD_READ_MMIO   CTL_CODE(GPD_TYPE, 0x9A0, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_GPD_WRITE_MMIO  CTL_CODE(GPD_TYPE, 0x9A1, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Port
#define IOCTL_GPD_R1_READ_PORT  CTL_CODE(GPD_TYPE, 0x960, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_GPD_R1_WRITE_PORT CTL_CODE(GPD_TYPE, 0x961, METHOD_BUFFERED, FILE_WRITE_ACCESS)


		// SMBus I/O Register Address Map
#define Off_Smb_HstCnt  0x02
#define Off_Smb_HstSts  0x00
#define Off_Smb_HstCmd  0x03
#define Off_Smb_XmitSlva  0x04
#define Off_Smb_HstD0  0x05
#define Off_Smb_HstD1  0x06
#define Off_Smb_HostBlockDb  0x07
#define Off_Smb_Pec  0x08
#define Off_Smb_RcvSlva  0x09
#define Off_Smb_SlvData_0  0x0A
#define Off_Smb_SlvData_1  0x0B
#define Off_Smb_AuxSts  0x0C
#define Off_Smb_AuxCtl  0x0D
#define Off_Smb_SmlinkPinCtl  0x0E
#define Off_Smb_SmbusPinCtl  0x0F
#define Off_Smb_SlvSts  0x10
#define Off_Smb_SlvCmd  0x11
#define Off_Smb_NotifyDaddr  0x14
#define Off_Smb_NotifyDlow  0x15
#define Off_Smb_NotifyDhigh  0x17

unsigned int Smb_HstSts = 0x00;
unsigned int Smb_HstCnt = 0x00;
unsigned int Smb_HstCmd = 0x00;
unsigned int Smb_XmitSlva = 0x00;
unsigned int Smb_HstD0 = 0x00;
unsigned int Smb_HstD1 = 0x00;
unsigned int Smb_HostBlockDb = 0x00;
unsigned int Smb_Pec = 0x00;
unsigned int Smb_RcvSlva = 0x00;
unsigned int Smb_SlvData_0 = 0x00;
unsigned int Smb_SlvData_1 = 0x00;
unsigned int Smb_AuxSts = 0x00;
unsigned int Smb_AuxCtl = 0x00;
unsigned int Smb_SmlinkPinCtl = 0x00;
unsigned int Smb_SmbusPinCtl = 0x00;
unsigned int Smb_SlvSts = 0x00;
unsigned int Smb_SlvCmd = 0x00;
unsigned int Smb_NotifyDaddr = 0x00;
unsigned int Smb_NotifyDlow = 0x00;
unsigned int Smb_NotifyDhigh = 0x00;

BOOL InstallDriver(SC_HANDLE, LPCSTR, LPCSTR);
BOOL RemoveDriver(SC_HANDLE, LPCSTR);
BOOL StartDriver(SC_HANDLE, LPCSTR);
BOOL StopDriver(SC_HANDLE, LPCSTR);


BOOL InstallDriver(
	SC_HANDLE SchSCManager,
	LPCSTR DriverName,
	LPCSTR ServiceExe
)
{
	SC_HANDLE schService;
	DWORD err;


	schService = CreateServiceA(SchSCManager,
		DriverName,
		DriverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_NORMAL,
		ServiceExe,
		NULL, NULL, NULL, NULL, NULL
	);

	if (schService == NULL)
	{
		err = GetLastError();

		if (err == ERROR_SERVICE_EXISTS)
		{
			return TRUE;
		}
		else
		{

			dbg_printf("CreateService failed! Error = %d \n", err);
			OUTINFO_1_PARAM("CreateService failed! Error = %d \n", err);
			return FALSE;
		}
	}

	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return TRUE;
}

BOOL RemoveDriver(
	SC_HANDLE SchSCManager,
	LPCSTR DriverName
)
{
	SC_HANDLE schService;
	BOOLEAN rCode;


	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)
	{

		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}


	if (DeleteService(schService))
	{
		rCode = TRUE;
	}
	else
	{

		dbg_printf("DeleteService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("DeleteService failed!Error = %d \n", GetLastError());
		rCode = FALSE;
	}


	if (schService)
	{
		CloseServiceHandle(schService);
	}
	return rCode;
}


BOOL StartDriver(
	SC_HANDLE SchSCManager,
	LPCSTR DriverName
)
{
	SC_HANDLE schService;
	DWORD err;


	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);
	if (schService == NULL)
	{

		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}


	if (!StartServiceA(schService,
		0,
		NULL
	))
	{

		err = GetLastError();

		if (err == ERROR_SERVICE_ALREADY_RUNNING)
		{
			dbg_printf("StartService run \n", err);
			OUTINFO_1_PARAM("StartService run \n", err);
			return TRUE;
		}
		else
		{
			dbg_printf("StartService failure! Error = %d \n", err);
			OUTINFO_1_PARAM("StartService failure! Error = %d \n", err);
			return FALSE;
		}
	}


	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return TRUE;

}

BOOL StopDriver(
	SC_HANDLE SchSCManager,
	LPCSTR DriverName
)
{
	BOOLEAN rCode = TRUE;
	SC_HANDLE schService;
	SERVICE_STATUS serviceStatus;


	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)
	{

		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}


	if (ControlService(schService,
		SERVICE_CONTROL_STOP,
		&serviceStatus
	))
	{
		rCode = TRUE;
	}
	else
	{

		dbg_printf("ControlService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("ControlService failed! Error = %d \n", GetLastError());
		rCode = FALSE;
	}


	if (schService)
	{
		CloseServiceHandle(schService);
	}
	return rCode;

}


BOOL Is64bitSystem()
{
	SYSTEM_INFO si;
	GetNativeSystemInfo(&si);
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
		return TRUE;
	else
		return FALSE;
}

#if 0
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;
BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;
	//IsWow64Process is not available on all supported versions
	//of Windows. Use GetModuleHandle to get a handle to the
	//DLL that contains the function and GetProcAddress to get
	//a pointer to the function if available.
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			//handle error
		}
	}
	return bIsWow64;
}

#endif 




#pragma warning( disable : 4996 )
BOOL GetDriverPath(
	LPSTR DriverLocation
)
{
	HMODULE hModule;
	if (Is64bitSystem())
	{
		hModule = GetModuleHandle(_T("AacHal_x64.dll"));
	}
	else
	{
		hModule = GetModuleHandle(_T("AacHal_x86.dll"));
	}



#if 1

	if (hModule)
	{
		GetModuleFileNameA(hModule/*NULL*/, DriverLocation, MAX_PATH);

		char *pEnd0 = strrchr(DriverLocation, '\\');
		char *pEnd1 = strrchr(DriverLocation, '/');
		if (pEnd0 < pEnd1)
			pEnd0 = pEnd1;

		if (pEnd0 != NULL)
			*pEnd0 = '\0';
		goto EXIT1;

	}
	else
	{
		dbg_printf("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		dbg_printf("dll path false\n\r");
		OUTINFO_1_PARAM("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		OUTINFO_0_PARAM("dll path false\n\r");
	}

	hModule = GetModuleHandle(_T("AacHal_x86.dll"));
	if (hModule)
	{
		GetModuleFileNameA(hModule/*NULL*/, DriverLocation, MAX_PATH);

		char *pEnd0 = strrchr(DriverLocation, '\\');
		char *pEnd1 = strrchr(DriverLocation, '/');
		if (pEnd0 < pEnd1)
			pEnd0 = pEnd1;

		if (pEnd0 != NULL)
			*pEnd0 = '\0';

	}
	else
	{
		dbg_printf("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		dbg_printf("dll path false\n\r");
		OUTINFO_1_PARAM("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		OUTINFO_0_PARAM("dll path false\n\r");
		return FALSE;
	}


EXIT1:

#endif 
#if 0
	DWORD driverLocLen = 0;


	driverLocLen = GetCurrentDirectoryA(MAX_PATH,
		DriverLocation
	);

	if (!driverLocLen)
	{
		printf("GetCurrentDirectory failed! Error = %d \n", GetLastError());
		return FALSE;
	}
#endif
	strcat(DriverLocation, "\\");
	strcat(DriverLocation, DRIVER_NAME);
	if (Is64bitSystem())
	{
		strcat(DriverLocation, "_x64");
	}
	else
	{
		strcat(DriverLocation, "_x86");
	}
	strcat(DriverLocation, ".sys");

	return TRUE;
}

#define PCI_DataPort  0x00000CFC
#define PCI_ControlPort  0x00000CF8
HANDLE hDevice;
unsigned int smbus_address = 0;
BOOL IOWrite_4(unsigned int u32address, unsigned int u32Data)
{
	BOOL IoctlResult;
	unsigned int out_buf[2];            // 输出缓冲区            
	unsigned int in_buf[2];            // 输入缓冲区  
	DWORD dwOutBytes;
	out_buf[0] = u32address;
	out_buf[1] = u32Data;
	//printf("%d", sizeof(out_buf));
	IoctlResult = DeviceIoControl(
		hDevice,
		IOCTL_GPD_WRITE_PORT_ULONG,
		out_buf, sizeof(out_buf),        // 输入数据缓冲区
		NULL, 0,        // 输出数据缓冲区
		&dwOutBytes,             // 输出数据长度
		NULL
	);

	if (!IoctlResult)
	{

		return FALSE;
	}
	return TRUE;
}




BOOL IORead_4(unsigned int u32address, unsigned int *pu32Data)
{
	BOOL IoctlResult;
	DWORD out_buf[1];            // 输出缓冲区            
	DWORD in_buf[1];            // 输入缓冲区  
	DWORD dwOutBytes;
	out_buf[0] = u32address;
	//printf("%d\n\r", sizeof(out_buf));
	IoctlResult = DeviceIoControl(
		hDevice,
		IOCTL_GPD_READ_PORT_ULONG,
		out_buf, sizeof(out_buf),        // 输入数据缓冲区
		in_buf, sizeof(in_buf),        // 输出数据缓冲区
		&dwOutBytes,             // 输出数据长度
		NULL
	);

	if (!IoctlResult)
	{

		return FALSE;
	}
	*pu32Data = in_buf[0];
	return TRUE;
}

int PCI_Read(unsigned char bBus, unsigned char bDevice, unsigned char bFunction, unsigned char bRegIndex, unsigned int *pu32Data)
{

	unsigned int u32Config = (unsigned int)(0x80000000 | bRegIndex | ((unsigned int)bFunction << 8) | ((unsigned int)bDevice << 11) | ((unsigned int)bBus << 16));

	if (IOWrite_4(PCI_ControlPort, u32Config) == FALSE)
	{
		dbg_printf("pci read false 1\n\r");
		OUTINFO_0_PARAM("pci read false 1\n\r");
		return 0;
	}
	//*pu32Data = 0x55;

	if (IORead_4(PCI_DataPort, &(*pu32Data)) == FALSE)
	{
		dbg_printf("pci read false 2\n\r");
		OUTINFO_0_PARAM("pci read false 2\n\r");
		return 0;
	}
	return 1;
}


BOOL IOWrite(unsigned int u32address, unsigned char pu32Data)
{
	BOOL IoctlResult;
	unsigned int  out_buf[3];            // 输出缓冲区            
	DWORD dwOutBytes;
	out_buf[0] = u32address;
	out_buf[1] = 1;
	out_buf[2] = pu32Data;
	IoctlResult = DeviceIoControl(
		hDevice,
		IOCTL_GPD_R1_WRITE_PORT,
		out_buf, 9,        // 输入数据缓冲区
		NULL, 0,        // 输出数据缓冲区
		&dwOutBytes,             // 输出数据长度
		NULL
	);

	if (!IoctlResult)
	{
		dbg_printf("IOWrite write false \n\r");
		OUTINFO_0_PARAM("IOWrite write false \n\r");
		return FALSE;
	}
	return TRUE;
}

BOOL IORead(unsigned int u32address, unsigned char *pu32Data)
{
	BOOL IoctlResult;
	unsigned int out_buf[2];            // 输出缓冲区            
	unsigned char in_buf[1];            // 输入缓冲区  
	DWORD dwOutBytes;
	out_buf[0] = u32address;
	out_buf[1] = 1;
	//printf("%d\n\r", sizeof(out_buf));
	IoctlResult = DeviceIoControl(
		hDevice,
		IOCTL_GPD_R1_READ_PORT,
		out_buf, sizeof(out_buf),        // 输入数据缓冲区
		in_buf, sizeof(in_buf),        // 输出数据缓冲区
		&dwOutBytes,             // 输出数据长度
		NULL
	);

	if (!IoctlResult)
	{
		dbg_printf("IOWrite read false \n\r");
		OUTINFO_0_PARAM("IOWrite read false \n\r");
		return FALSE;
	}
	*pu32Data = in_buf[0];
	return TRUE;
}


#define  Intel_SMBusCtrl_Dev  0x1f
#define  Intel_SMBusCtrl_Fun   0x04
#define  Intel_SMBusCtrl_Idx_VidDid 0x0
#define Intel_SMBusCtrl_Idx_BaseAddr 0x20

BOOL SmbCtrl_Get_BaseAddress_Intel(unsigned int *u32Addr)
{
	unsigned int  u32Data = 0x00;
	PCI_Read(0x00, Intel_SMBusCtrl_Dev, Intel_SMBusCtrl_Fun, Intel_SMBusCtrl_Idx_VidDid, &u32Data);
	dbg_printf("CPU_ID=0x%x", u32Data & 0XFFFF);
	OUTINFO_1_PARAM("CPU_ID=0x%x", u32Data & 0XFFFF);
	for (int i = 0; i < 6; i++)
	{
		unsigned int  u32Addr_temp = 0x00;
		if (PCI_Read(0x00, Intel_SMBusCtrl_Dev, Intel_SMBusCtrl_Fun, Intel_SMBusCtrl_Idx_BaseAddr, &u32Addr_temp) == FALSE)
			return FALSE;
		if ((u32Addr_temp & 0x00000001) != 0x00000001)
			continue;
		u32Addr_temp &= 0xFFFFFFFE;
		*u32Addr = u32Addr_temp;
		// SMBusCtrl: Base Address: {0} u32Addr
#if 0
		Smb_HstSts = *u32Addr + Off_Smb_HstSts;
		Smb_HstCnt = *u32Addr + Off_Smb_HstCnt;
		Smb_HstCmd = *u32Addr + Off_Smb_HstCmd;
		Smb_XmitSlva = *u32Addr + Off_Smb_XmitSlva;
		Smb_HstD0 = *u32Addr + Off_Smb_HstD0;
		Smb_HstD1 = *u32Addr + Off_Smb_HstD1;
		Smb_HostBlockDb = *u32Addr + Off_Smb_HostBlockDb;
		Smb_Pec = *u32Addr + Off_Smb_Pec;
		Smb_RcvSlva = *u32Addr + Off_Smb_RcvSlva;
		Smb_SlvData_0 = *u32Addr + Off_Smb_SlvData_0;
		Smb_SlvData_1 = *u32Addr + Off_Smb_SlvData_1;
		Smb_AuxSts = *u32Addr + Off_Smb_AuxSts;
		Smb_AuxCtl = *u32Addr + Off_Smb_AuxCtl;
		Smb_SmlinkPinCtl = *u32Addr + Off_Smb_SmlinkPinCtl;
		Smb_SmbusPinCtl = *u32Addr + Off_Smb_SmbusPinCtl;
		Smb_SlvSts = *u32Addr + Off_Smb_SlvSts;
		Smb_SlvCmd = *u32Addr + Off_Smb_SlvCmd;
		Smb_NotifyDaddr = *u32Addr + Off_Smb_NotifyDaddr;
		Smb_NotifyDlow = *u32Addr + Off_Smb_NotifyDlow;
		Smb_NotifyDhigh = *u32Addr + Off_Smb_NotifyDhigh;
#endif
		return TRUE;
	}

	return FALSE;
}

#define MAX_TIMEOUT 1000

#define SMBHSTSTS	0x0    // smbus host status register    
#define SMBHSTCNT	0x2    // smbus host control register   
#define SMBHSTCMD	0x3    // smbus host command register   
#define SMBHSTADD	0x4    // smbus host address register   
#define SMBHSTDAT0	0x5    // smbus host data 0 register    
#define SMBHSTDAT1	0x6    // smbus host data 1 register    
#define SMBBLKDAT	0x7    // smbus block data register 

void delay1us(DWORD delay)
{
	LARGE_INTEGER start, end, Freq;

	QueryPerformanceFrequency(&Freq);
	//end.QuadPart = (delay*Freq.QuadPart)/1000;        //1ms
	end.QuadPart = (delay*Freq.QuadPart) / 1000000;        //1us
	QueryPerformanceCounter(&start);
	end.QuadPart = end.QuadPart + start.QuadPart;/*-System_test()*2;*/
	do
	{
		QueryPerformanceCounter(&Freq);
	} while (Freq.QuadPart <= end.QuadPart);
}

unsigned char read_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset)
{
	unsigned char temp;
	int timeout = 0;
	//clear all status bits
	IOWrite((SMBHSTSTS + SMB_base), 0x1E);

	// write the offset
	IOWrite((SMBHSTCMD + SMB_base), offset);

	// write the slave address 
	IOWrite((SMBHSTADD + SMB_base), (slave_address << 1 | 1));

	//Read byte protocol and Start
	IOWrite((SMBHSTCNT + SMB_base), 0x48);

	while (1)
	{
		delay1us(10);
		IORead((SMB_base + SMBHSTSTS), &temp);
		if ((temp & 0x42) == 0x42)
		{
			break;
		}
		timeout++;

		if (timeout > MAX_TIMEOUT)
		{
			return 0xff; //false
		}
	}

	if ((temp & 0x1c) == 0)
	{//ok
		IORead((SMBHSTDAT0 + SMB_base), &temp);
		return temp;
	}
	return 0xff;
}

#define hst_cnt_start 0x40
#define smbcmd_bytedata 0x08
unsigned char write_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char wdata)
{
	unsigned char temp;
	int timeout = 0;
	//if (slave_address == 0x61)
		//printf("address error\n\r");
	//IORead((SMB_base + SMBHSTSTS), &temp);
	//delay1us(5);
	//clear all status bits
	IOWrite((SMBHSTSTS + SMB_base), 0xff);	
	delay1us(5);
	IORead((SMB_base + SMBHSTSTS), &temp);
	if (temp != 0x00)
		printf("clear states error\n\r");
	delay1us(5);
	// write the offset
	IOWrite((SMBHSTCMD + SMB_base), offset);
	IORead((SMB_base + SMBHSTCMD), &temp);
	if (temp != offset)
		printf("SMBHSTCMD error\n\r");

	delay1us(5);
	// write the slave address 
	IOWrite((SMBHSTADD + SMB_base), (slave_address << 1));
	IOWrite((SMBHSTADD + SMB_base), (slave_address << 1));
	IORead((SMB_base + SMBHSTADD), &temp);
	if (temp != (slave_address << 1))
		printf("SMBHSTADD error\n\r");
	delay1us(5);
	// Step 4. Write  Count value to Host Data0 Register
	IOWrite((SMBHSTDAT0 + SMB_base), wdata); //test count
	IOWrite((SMBHSTDAT0 + SMB_base), wdata); //test count
	IORead((SMB_base + SMBHSTDAT0), &temp);
	if (temp != wdata)
		printf("SMBHSTDAT0 error\n\r");
	delay1us(5);
	//clear all status bits
	IOWrite((SMBHSTSTS + SMB_base), 0xff);
	IOWrite((SMBHSTSTS + SMB_base), 0xff);
	IORead((SMB_base + SMBHSTSTS), &temp);
	if (temp != 0x0)
		printf("SMBHSTSTS error\n\r");
	delay1us(5);

	IOWrite((SMBHSTCNT + SMB_base), hst_cnt_start | smbcmd_bytedata);
	IORead((SMB_base + SMBHSTCNT), &temp);
	//if (temp != 0x0)
		//printf("SMBHSTCNT error\n\r");

	delay1us(5);
	while (1)
	{
		delay1us(150);
		IORead((SMB_base + SMBHSTSTS), &temp);
		if ((temp & 0x02) == 0x2)
		{
			break;
		}
		timeout++;

		if (timeout > MAX_TIMEOUT)
		{
			return 0xff; //false
		}
	}

	delay1us(100);

	return 0x00;//pass
}


#if 1
void block_write(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char count, unsigned char* buffer)
{
	int Fail_CNT = 0;
	unsigned char SMB_STS;
	// Initial SMBus
	IOWrite((0x0D + SMB_base), 0x00);

	// Step 1. Make sure status bis is clear   
	do {
		delay1us(10);
		IOWrite((SMBHSTSTS + SMB_base), 0xFF);

		IORead(SMB_base + SMBHSTSTS, &SMB_STS);

		Fail_CNT++;
	} while (SMB_STS != 0x00 && Fail_CNT < 100);
	if (Fail_CNT > MAX_TIMEOUT) {
		printf("timeout\n\r");
		return;
	}

	// Step 2. Write the Slave Address in Address Register
	 // write the slave address 
	IOWrite((SMBHSTADD + SMB_base), (slave_address << 1 | 0));

	// Step 3. Write Index in Command Register
	// write the offset
	IOWrite((SMBHSTCMD + SMB_base), index);//INDEX


   // Step 4. Write Block Count value to Host Data0 Register
	IOWrite((SMBHSTDAT0 + SMB_base), count); //test count

	// Step 5. Write 1st byte to Host Block Data Block Register
	IOWrite((SMBBLKDAT + SMB_base), buffer[0]); //first byte

	// Step 6. Set SMB_CMD for Block Command
	IOWrite((SMBHSTCNT + SMB_base), 0x54);


	// Step 7. Write the remaining bytes to Host Block Data Byte Register
	for (int bSent = 0; bSent < 5; bSent++) {
		for (int LoopIndex = 0; LoopIndex < 1000; LoopIndex++) {
			delay1us(100);
			IORead(SMB_base + SMBHSTSTS, &SMB_STS);
			if (SMB_STS & 0x80) // Byte Done bit is set
				break;
		}
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);

		if (SMB_STS == 0x00) {                // Byte Done bit is not set		   
			return;
		}
		//========= Time-out or completion of the SMBus command. To check the SMBus host status bits.
		if (SMB_STS & 0x1c) {
			if (SMB_STS & 0x04) {
				printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]\n\r", SMB_STS);
			}
			if (SMB_STS & 0x08) {
				printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]\n\r", SMB_STS);

			}
			if (SMB_STS & 0x10) {
				printf("SMBus Failed: FAILED (failed bus transaction) ![0x%2X]\n\r", SMB_STS);
			}
			IOWrite((SMBHSTSTS + SMB_base), 0x1c);                     // Clear error status and INUSE_STS/INTR bits.
			return;
		}

		IOWrite((SMBBLKDAT + SMB_base), buffer[bSent + 1]);   // Place the next data byte.
		IOWrite((SMBHSTSTS + SMB_base), 0x80);              // Clear byte done bit to request another byte write.
	}

	// Wait until the previous byte is sent out
	//
	for (int LoopIndex = 0; LoopIndex < 1000; LoopIndex++) {
		delay1us(100);
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		if (SMB_STS & 0x80)
			break; // Byte Done bit is set
	}
	IORead(SMB_base + SMBHSTSTS, &SMB_STS);
	if (SMB_STS == 0x00) {                    // Byte Done bit is not set

		printf("SMBus Failed: Byte done for Block command = 0 !\n\r");
		return;
	}
	IOWrite((SMBHSTSTS + SMB_base), 0x80); // Clear byte done bit to request another byte write.

	int LoopCnt = 1000;
	for (int LoopIndex = 0; LoopIndex < LoopCnt; LoopIndex++) {
		delay1us(100);                                                  // delay one millisecond
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);


		if (SMB_STS & 0x01) continue;                               // HOST is still busy
		if (SMB_STS & 0x02) break;                                  // Termination of the SMBus command.
	}

	IOWrite((SMBHSTSTS + SMB_base), 0x42);                    // Clear INUSE_STS and INTR bits.

	//========= Time-out or completion of the SMBus command. To check the SMBus host status bits.
	if (SMB_STS & 0x1c) {
		if (SMB_STS & 0x04) {
			printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![0x%2X]", SMB_STS);

		}
		if (SMB_STS & 0x08) {
			printf("SMBus Failed: BUS_ERR (transaction collision) ![0x%2X]", SMB_STS);

		}
		if (SMB_STS & 0x10) {
			printf("SMBus Failed: FAILED (failed bus transaction) ![0x%2X]", SMB_STS);
		}
		IOWrite((SMBHSTSTS + SMB_base), 0x18);          // Clear error status and INUSE_STS/INTR bits.
		return;
	}
	if ((SMB_STS & 0x01) == 0x01) {
		IOWrite((SMBHSTSTS + SMB_base), 0x42);                       // Clear INUSE_STS and INTR bits.
		return;
	}

	return;
}
#endif



extern  ULONG g_Components;    // number of components;



#define MY_DEVICE_NAME  L"MyComponentName";
#define MY_DEVICE_COUNT   1 
#define MY_LED_COUNT      5

wchar_t* g_MyLedLoaction[MY_LED_COUNT] = {
	L"Loaction_1",
	L"Loaction_2",
	L"Loaction_3",
	L"Loaction_4",
	L"Loaction_5",
};


// ==== CAP ====

std::vector<DeviceEffect> g_deviceEffectInfo = {

	{ DeviceEffect(EFEFCT_0_NAME, EFEFCT_0_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_1_NAME, EFEFCT_1_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_FF_NAME, EFEFCT_FF_ID, false, false, false, false, 0, 0, 0, 0)}
	//{ DeviceEffect(EFEFCT_USER, L"User Effect 1", false, false, false, false, 0, 0, 0, 0) },
	//{ DeviceEffect(EFEFCT_USER + 1, L"User Effect 2", false, false, false, false, 0, 0, 0, 0) }
};



//------------------------------------------------------------------------------------
std::string Wcs2Str(const wchar_t* wcs)
{
	int wlen = wcslen(wcs);
	int len = WideCharToMultiByte(CP_ACP, 0, wcs, wlen, NULL, 0, NULL, NULL);
	char* str = new char[len + 1];
	str[len] = 0;
	WideCharToMultiByte(CP_ACP, 0, wcs, wlen, str, len + 1, NULL, NULL);
	std::string ret = str;
	delete[] str;
	return ret;
}


std::wstring Str2Wcs(const char* str)
{
	int len = strlen(str);
	int wlen = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
	wchar_t* wcs = new wchar_t[wlen + 1];
	wcs[wlen] = 0;
	MultiByteToWideChar(CP_ACP, 0, str, len, wcs, wlen + 1);
	std::wstring ret = wcs;
	delete[] wcs;
	return ret;
}

//-----------------------------------------------------------------------------

// MyAacLedDeviceHal
MyAacLedDeviceHal::~MyAacLedDeviceHal(void)
{
}



HRESULT MyAacLedDeviceHal::CreateLedDevice(std::vector<IAacLedDevice*>& devices)
{
	DeviceLightControl* DeviceLightCtrl = new DeviceLightControl;

	devices.clear();

	for (size_t i = 0; i < MY_DEVICE_COUNT; ++i)
	{

		MyAacLedDevice* device = new MyAacLedDevice();
		device->Init(DeviceLightCtrl, i);
		devices.push_back(device);
	}

	return S_OK;
}



//MyAacLedDevice

MyAacLedDevice::MyAacLedDevice(void)
{
	InitMutex();

}


MyAacLedDevice::~MyAacLedDevice(void)
{

}




HRESULT STDMETHODCALLTYPE MyAacLedDevice::GetCapability(BSTR *capability)
{
	EnterMutex();





	std::wstring Capability_profile;

	gs::GsConfigFile XMLprofile;
	XMLprofile.Create();

	// root:
	gs::GsElementNode root = XMLprofile.Root();

	// version
	gs::GsElementNode element;
	element = root->AddElement("version");
	element->SetInt(1);

	// Type
	element = root->AddElement("type");
	element->SetInt(aura::DRAM_RGB_LIGHTING);

	// device :
	gs::GsElementNode device_node;
	std::wstring id = doGetDeviceId();
	device_node = root->AddElement(L"device");
	device_node->AddElementValue(L"name", id.c_str());

	//Index:
	gs::GsElementNode indexNode = device_node->AddElement(L"id");
	indexNode->SetInt(m_index);

	// layout
	gs::GsElementNode layoutNode;
	layoutNode = device_node->AddElement(L"layout");

	// ledcount :
	int led_count = doGetLedCount();
	gs::GsElementNode ledCountNode = layoutNode->AddElement(L"led_count");
	ledCountNode->SetInt(led_count);

	// size :
	gs::GsElementNode sizeNode;
	sizeNode = layoutNode->AddElement(L"size");
	gs::GsElementNode widthNode = sizeNode->AddElement(L"width");
	widthNode->SetInt(led_count);
	gs::GsElementNode heightNode = sizeNode->AddElement(L"height");
	heightNode->SetInt(1);

	// led_name :
	gs::GsElementNode ledListNode;
	ledListNode = layoutNode->AddElement(L"led_name");

	// ledLocationNode :
	for (int led = 0; led < led_count; led++) {

		gs::GsElementNode ledLocationNode;
		std::wstring location_str = doGetLedLocation(led);

		ledLocationNode = ledListNode->AddElementValue(L"led", location_str.c_str());
	}

	// <DeviceModeList>, under device node:
	std::vector<DeviceEffect> effect_list = g_deviceEffectInfo;
	gs::GsElementNode  deviceModeListNode;
	deviceModeListNode = device_node->AddElement(L"supported_effect");

	for (size_t i = 0; i < effect_list.size(); ++i)
	{
		gs::GsElementNode deviceModeNode = deviceModeListNode->AddElementWithoutKey(L"effect");

		gs::GsElementNode effectNode_Name;
		effectNode_Name = deviceModeNode->AddElementValue(L"name", effect_list[i].EffectName.c_str());

		gs::GsElementNode effectNode_Index;
		effectNode_Index = deviceModeNode->AddElementValue(L"id", std::to_wstring(effect_list[i].EffectId).c_str());

		gs::GsElementNode effectNode_Support_set_frame;
		effectNode_Index = deviceModeNode->AddElementValue(L"synchronizable", std::to_wstring(effect_list[i].SynchronizationSupported).c_str());

		gs::GsElementNode node;

		// customized color?
		node = deviceModeNode->AddElementValue(L"customized_color", std::to_wstring(effect_list[i].CustomizedColorSupported).c_str());

		// support speed ?
		node = deviceModeNode->AddElementValue(L"speed_supported", std::to_wstring(effect_list[i].DirectionSupported).c_str());
		if (effect_list[i].SpeedSupported)
		{
			gs::GsElementNode value;
			value = node->AddElement("levels");
			value->SetInt(effect_list[i].MaxSpeedLevels);
			value = node->AddElement("default");
			value->SetInt(effect_list[i].DefaultSpeedLevel);
		}

		// support direction?
		node = deviceModeNode->AddElementValue(L"direction_supported", std::to_wstring(effect_list[i].DirectionSupported).c_str());
		if (effect_list[i].DirectionSupported)
		{
			gs::GsElementNode value;
			value = node->AddElement("levels");
			value->SetInt(effect_list[i].MaxDirectionLevels);
			value = node->AddElement("default");
			value->SetInt(effect_list[i].DefaultDirectionLevel);
		}
	}

	// turn to string:
	XMLprofile.ToString(Capability_profile);
	if (Capability_profile.length() > 0)
	{
		// TODO: trun wstring profile to BSTR
		*capability = SysAllocString(Capability_profile.c_str());
	}

	LeaveMutex();
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyAacLedDevice::SetEffect(ULONG effectId, ULONG *colors, ULONG numberOfColors)
{
	HRESULT hr = S_OK;
	EnterMutex();
#if 0
	dbg_printf("initial\n\r");
	OUTINFO_0_PARAM("initial\n\r");
	DWORD errNum = 0;
	UCHAR  driverLocation[MAX_PATH];

	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!schSCManager)
	{

		dbg_printf("Open SC Manager failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("Open SC Manager failed! Error = %d \n", GetLastError());
		return S_FALSE;
	}
	dbg_printf("Open SC Manager\n\r");
	OUTINFO_0_PARAM("Open SC Manager\n\r");
	if (!GetDriverPath((LPSTR)driverLocation))
	{
		return S_FALSE;
	}
	dbg_printf("driverLocation:%s\n\r", driverLocation);
	OUTINFO_1_PARAM("driverLocation:%s\n\r", driverLocation);
	if (InstallDriver(schSCManager,
		DRIVER_NAME,
		(LPSTR)driverLocation
	))
	{

		if (!StartDriver(schSCManager, (LPSTR)DRIVER_NAME))
		{
			dbg_printf("Unable to start driver. \n");
			OUTINFO_0_PARAM("Unable to start driver. \n");
			RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME); //jc can start so to remove the driver
			return S_FALSE;
		}
		dbg_printf("instal and start driver\n\r");
		OUTINFO_0_PARAM("instal and start driver\n\r");
	}
	else
	{

		RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME);
		dbg_printf("Unable to install driver. \n");
		OUTINFO_0_PARAM("Unable to install driver. \n");
		return S_FALSE;
	}

	hDevice = CreateFileA(Device_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		dbg_printf("Error: CreatFile Failed : %d\n", GetLastError());
		OUTINFO_1_PARAM("Error: CreatFile Failed : %d\n", GetLastError());
		return S_FALSE;
	}
	//unsigned int smbus_address = 0;
	SmbCtrl_Get_BaseAddress_Intel(&smbus_address);
	dbg_printf("smbus address:0x%x\n\r", smbus_address);
	OUTINFO_1_PARAM("smbus address:0x%x\n\r", smbus_address);

	unsigned char temp = read_slave_data(smbus_address, 0x60, 0x01); //check ddr5 id

	dbg_printf("read 0x3a:0x%x\n\r", temp);
	OUTINFO_1_PARAM("read 0x3a:0x%x\n\r", temp);

	if (temp == 0xff)
		return S_FALSE;
#endif
	DeviceEffect* effect = GetEffectInfo(effectId);
	if (effect != nullptr)
		hr = E_FAIL;

	int i2c_count = 0;
	for (i2c_count = 0; i2c_count < 4; i2c_count++)
	{
		if (i2c_addrs[i2c_count] != 0)
		{
			printf("write i2c addrss 0x%x", 0x60 + i2c_count);
			//sync mode
			if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x01) == 0xff)
			{
				printf("false\n\r");
			}
			//led pixels
			if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
			{
				printf("false\n\r");
			}
			if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
			{
				printf("false\n\r");
			}
			//dbg_printf("led numbers:0x%x\n\r", numberOfColors);
			//OUTINFO_1_PARAM("led numbers:0x%x\n\r", numberOfColors);
			int j = 0;
			for (int i = 0; i < (numberOfColors * 2); i = i + 2)
			{
				//dbg_printf("numbers:0x%x\n\r", colors[i]);
				//OUTINFO_1_PARAM("numbers:0x%x\n\r", colors[i]);
#if 0
				unsigned char temp;
				temp = write_slave_data(smbus_address, 0x3a, (i * 3) + 0, colors[i] & 0xff); //r
				if (temp != 0x0)
					OUTINFO_0_PARAM("error %d\n\r", i);
				temp = write_slave_data(smbus_address, 0x3a, (i * 3) + 1, (colors[i] >> 8) & 0xff); //g
				if (temp != 0x0)
					OUTINFO_0_PARAM("error %d\n\r", i);
				temp = write_slave_data(smbus_address, 0x3a, (i * 3) + 2, (colors[i] >> 16) & 0xff);//b
				if (temp != 0x0)
					OUTINFO_0_PARAM("error %d\n\r", i);
#endif

				//delay1us(5000);



				if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 0) != 0x0)
				{
					OUTINFO_0_PARAM("l error %d\n\r", i);
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
				{
					OUTINFO_0_PARAM("h error %d\n\r", i);
				}

				//r
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, colors[j] & 0xff) != 0)
				{
					OUTINFO_0_PARAM("color error %d\n\r", i);
				}


				if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 1) != 0x0)
				{
					OUTINFO_0_PARAM("l error %d\n\r", i);
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0)
				{
					OUTINFO_0_PARAM("h error %d\n\r", i);
				}

				//g
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, (colors[j] >> 8) & 0xff) != 0x0)
				{
					OUTINFO_0_PARAM("color error %d\n\r", i);
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 2) != 0x0)
				{
					OUTINFO_0_PARAM("l error %d\n\r", i);
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
				{
					OUTINFO_0_PARAM("h error %d\n\r", i);
				}
				//b
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, (colors[j] >> 16) & 0xff) != 0x0)
				{
					OUTINFO_0_PARAM("color error %d\n\r", i);
				}
				j++;
			}
			delay1us(2000);
			if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
			{
				printf("false\n\r");
			}
		}
	}
#if 0
	CloseHandle(hDevice);
	StopDriver(schSCManager,
		DRIVER_NAME
	);
	printf("stop driver\n\r");

	RemoveDriver(schSCManager,
		DRIVER_NAME
	);
	printf("remove driver\n\r");

	CloseServiceHandle(schSCManager);
#endif
	ULONG speed = 0;
	ULONG direction = 0;
	speed = effect->DefaultSpeedLevel;
	direction = effect->DefaultDirectionLevel;

	return DoSetEffectOptSpeed(effectId, colors, numberOfColors, speed, direction);

	LeaveMutex();
	return hr;
}


HRESULT MyAacLedDevice::SetEffectOptSpeed(ULONG effectId, ULONG *colors, ULONG numberOfColors, ULONG speed, ULONG direction)
{
	printf("SetEffectOptSpeed\n\r");
	printf("%d\n\r", effectId);
	DeviceEffect* effect = GetEffectInfo(effectId);
	if (effect == nullptr || (!effect->SpeedSupported && !effect->DirectionSupported))
		return E_FAIL;

	if (effect->SpeedSupported && speed >= effect->MaxSpeedLevels)
		return E_FAIL;

	if (effect->DirectionSupported && speed >= effect->MaxDirectionLevels)
		return E_FAIL;

	////TODO

	return DoSetEffectOptSpeed(effectId, colors, numberOfColors, speed, direction);
}

HRESULT STDMETHODCALLTYPE MyAacLedDevice::Synchronize(ULONG effectId, ULONGLONG tickcount)
{
	////TODO

	return E_FAIL;
}

HRESULT MyAacLedDevice::Init(DeviceLightControl* deviceControl, int index)
{
	m_deviceControl = deviceControl;
	m_index = index;
	//CreateDeviceCapability();
#if 1
	dbg_printf("initial\n\r");
	OUTINFO_0_PARAM("initial\n\r");
	DWORD errNum = 0;
	UCHAR  driverLocation[MAX_PATH];

	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!schSCManager)
	{

		dbg_printf("Open SC Manager failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("Open SC Manager failed! Error = %d \n", GetLastError());
		return S_FALSE;
	}
	dbg_printf("Open SC Manager\n\r");
	OUTINFO_0_PARAM("Open SC Manager\n\r");
	if (!GetDriverPath((LPSTR)driverLocation))
	{
		return S_FALSE;
	}
	dbg_printf("driverLocation:%s\n\r", driverLocation);
	OUTINFO_1_PARAM("driverLocation:%s\n\r", driverLocation);
	if (InstallDriver(schSCManager,
		DRIVER_NAME,
		(LPSTR)driverLocation
	))
	{

		if (!StartDriver(schSCManager, (LPSTR)DRIVER_NAME))
		{
			dbg_printf("Unable to start driver. \n");
			OUTINFO_0_PARAM("Unable to start driver. \n");
			RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME); //jc can start so to remove the driver
			return S_FALSE;
		}
		dbg_printf("instal and start driver\n\r");
		OUTINFO_0_PARAM("instal and start driver\n\r");
	}
	else
	{

		RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME);
		dbg_printf("Unable to install driver. \n");
		OUTINFO_0_PARAM("Unable to install driver. \n");
		return S_FALSE;
	}

	hDevice = CreateFileA(Device_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		dbg_printf("Error: CreatFile Failed : %d\n", GetLastError());
		OUTINFO_1_PARAM("Error: CreatFile Failed : %d\n", GetLastError());
		return S_FALSE;
	}
	//unsigned int smbus_address = 0;
	SmbCtrl_Get_BaseAddress_Intel(&smbus_address);
	dbg_printf("smbus address:0x%x\n\r", smbus_address);
	OUTINFO_1_PARAM("smbus address:0x%x\n\r", smbus_address);

	int loccunt = 0;
	for (loccunt = 0; loccunt < 4; loccunt++)
	{
		unsigned char temp = 0xff;
		temp = read_slave_data(smbus_address, 0x60 + loccunt, 0x01); //check ddr5 id
		dbg_printf("read 0x3a:0x%x\n\r", temp);
		OUTINFO_1_PARAM("read 0x3a:0x%x\n\r", temp);
		if (temp != 0xff)
		{
			i2c_addrs[loccunt] = 1; //devices detect
		}
		else {
			i2c_addrs[loccunt] = 0;
		}
	}

	if ((i2c_addrs[0] == 0) && (i2c_addrs[1] == 0) && (i2c_addrs[2] == 0) && (i2c_addrs[3] == 0))
	{
		CloseHandle(hDevice);

		StopDriver(schSCManager,
			DRIVER_NAME
		);
		printf("stop driver\n\r");

		RemoveDriver(schSCManager,
			DRIVER_NAME
		);
		printf("remove driver\n\r");

		CloseServiceHandle(schSCManager);
		printf("close Service\n\r");
		return S_FALSE;
	}
#endif

	return S_OK;
}

void MyAacLedDevice::LeaveMutex(void)
{
	ReleaseMutex(m_hHalMutex);
}

void MyAacLedDevice::EnterMutex(void)
{
	DWORD result;
	// result = WaitForSingleObject(m_hMutex, 1000);
	result = WaitForSingleObject(m_hHalMutex, MUTEX_WAITTINGTIME);

	if (result == WAIT_OBJECT_0)
	{
	}
	else
	{
	}
}

void MyAacLedDevice::InitMutex(void)
{
	SECURITY_DESCRIPTOR sid;

	::InitializeSecurityDescriptor(&sid, SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sid, TRUE, NULL, FALSE);
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof SECURITY_ATTRIBUTES;
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sid;

	m_hHalMutex = CreateMutex(
		&sa,              // security attributes
		FALSE,             // initially not owned
		MY_MUTEXT_NAME);             // SMBus Mutex name
}


void MyAacLedDevice::EnterSmbusMutex(void)
{
	DWORD result;
	result = WaitForSingleObject(m_hSmbusMutex, MUTEX_WAITTINGTIME);

	if (result == WAIT_OBJECT_0)
	{
	}
	else
	{
	}
}


void MyAacLedDevice::LeaveSmbusMutex(void)
{
	ReleaseMutex(m_hSmbusMutex);
}


void MyAacLedDevice::InitSmbusMutex(void)
{
	SECURITY_DESCRIPTOR sid;

	::InitializeSecurityDescriptor(&sid, SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sid, TRUE, NULL, FALSE);
	SECURITY_ATTRIBUTES sa;

	sa.nLength = sizeof SECURITY_ATTRIBUTES;
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = &sid;

	m_hSmbusMutex = CreateMutexW(
		&sa,              // security attributes
		FALSE,             // initially not owned
		L"Global\\Access_SMBUS.HTP.Method");             // SMBus Mutex name
}


DeviceEffect* MyAacLedDevice::GetEffectInfo(ULONG effectId)
{
	size_t i = 0;

	for (auto e : g_deviceEffectInfo)
	{
		if (e.EffectId == effectId)
			return &g_deviceEffectInfo[i];
	}

	return nullptr;
}


std::wstring MyAacLedDevice::doGetDeviceId()
{
	////TODO
	return MY_DEVICE_NAME;
}


size_t   MyAacLedDevice::doGetLedCount()
{
	////TODO
	return MY_LED_COUNT;
}


std::wstring MyAacLedDevice::doGetLedLocation(int led)
{
	////TODO
	return g_MyLedLoaction[led];
}


HRESULT MyAacLedDevice::DoSetEffectOptSpeed(ULONG effectId, ULONG *colors, ULONG numberOfColors, ULONG speed, ULONG direction)
{
	////TODO
	printf("DoSetEffectOptSpeed\n\r");
	printf("effectId %d\n\r", effectId);
	printf("%d\n\r", numberOfColors);
	printf("%d\n\r", speed);
	printf("%d\n\r", direction);
	return S_OK;
}

DeviceEffect::DeviceEffect() {

}

DeviceEffect::DeviceEffect(std::wstring effectName, size_t effectId, bool customizedColorSupported, bool synchronizationSupported, bool speedSupported, bool directionSupported, size_t maxSpeedLevels, size_t defaultSpeedLevel, size_t maxDirectionLevels, size_t defaultDirectionLevel)
{

	//m_isEcControlMode = isEcControlMode;
	SynchronizationSupported = synchronizationSupported;
	EffectName = effectName;
	EffectId = effectId;
	CustomizedColorSupported = customizedColorSupported;
	SpeedSupported = speedSupported;
	DirectionSupported = directionSupported;
	MaxSpeedLevels = maxSpeedLevels;
	DefaultSpeedLevel = defaultSpeedLevel;
	MaxDirectionLevels = maxDirectionLevels;
	DefaultDirectionLevel = defaultDirectionLevel;
}



extern "C" _declspec(dllexport) int SetEffect(ULONG effectId, ULONG *colors, ULONG numberOfColors)
{

	printf("seteffect\n\r");
	printf("effectid%d\n\r", effectId);
	printf("numberOfColors%d\n\r", numberOfColors);
	//for (int i = 0; i < 5; i++)
	//{
	//	printf("0x%x\n\r", colors[i]);
	//}
	if (effectId == 0)
	{
		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", 0x60 + i2c_count);
				//sync mode
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
				//led pixels
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
				{
					printf("false\n\r");
				}
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
				{
					printf("false\n\r");
				}
				//dbg_printf("led numbers:0x%x\n\r", numberOfColors);
				//OUTINFO_1_PARAM("led numbers:0x%x\n\r", numberOfColors);
				int j = 0;
				for (int i = 0; i < (numberOfColors * 2); i = i + 2)
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 0) != 0x0)
					{
						printf("l error %d\n\r", i);
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
					{
						printf("h error %d\n\r", i);
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}

					//printf("r:0x%x\n\r", colors[j] & 0xff);
					//r
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, colors[j] & 0xff) != 0)
					{
						printf("color error %d\n\r", i);
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}


					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 1) != 0x0)
					{
						printf("l error %d\n\r", i);
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0)
					{
						printf("h error %d\n\r", i);
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}

					//g
					//printf("g:0x%x\n\r", colors[j]>>8 & 0xff);
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, (colors[j] >> 8) & 0xff) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 2) != 0x0)
					{
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
					{
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}
					//b
					//printf("b:0x%x\n\r", (colors[j] >> 16) & 0xff);
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, (colors[j] >> 16) & 0xff) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}
					j++;
				}
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
#if 0
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
#endif
			}
		}
	}

	if (effectId != 0)
	{
		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", 0x60 + i2c_count);
				//build in mode
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x02) == 0xff)
				{
					printf("false\n\r");
				}

				//led pixels
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
				{
					printf("false\n\r");
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
				{
					printf("false\n\r");
				}
				//function
				if (effectId == 1)//FUNC_Breathing
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x2) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 2)//FUNC_Strobe
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x3) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 3)//FUNC_Cycling
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x4) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 4)//FUNC_Random
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x5) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 5)//FUNC_Music
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x6) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 6)//FUNC_Wave
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x7) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 7)//FUNC_Spring
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x8) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 8)//FUNC_Water
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 13) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 9)//FUNC_Rainbow
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 14) == 0xff)
					{
						printf("false\n\r");
					}
				}
				//r
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 44, 0xff) == 0xff)
				{
					printf("false\n\r");
				}

				//g
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 45, 0x0) == 0xff)
				{
					printf("false\n\r");
				}

				//b
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 46, 0x0) == 0xff)
				{
					printf("false\n\r");
				}

			}
		}
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			for (i2c_count = 0; i2c_count < 4; i2c_count++)
			{
				if (i2c_addrs[i2c_count] != 0)
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
					{
						printf("false\n\r");
					}
				}
			}
		}
	}

	return 0;
}



extern "C" _declspec(dllexport) int SetEffect_RGB_SP(ULONG effectId, int R, int G, int B, int SP)
{
	ULONG numberOfColors = 5; 
	if (effectId == 0)  //static
	{
		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", 0x60 + i2c_count);
				//sync mode
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
				//led pixels
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
				{
					printf("false\n\r");
				}
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
				{
					printf("false\n\r");
				}
				//dbg_printf("led numbers:0x%x\n\r", numberOfColors);
				//OUTINFO_1_PARAM("led numbers:0x%x\n\r", numberOfColors);
				int j = 0;
				for (int i = 0; i < (numberOfColors * 2); i = i + 2)
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 0) != 0x0)
					{
						printf("l error %d\n\r", i);
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
					{
						printf("h error %d\n\r", i);
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}

					//printf("r:0x%x\n\r", colors[j] & 0xff);
					//r
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, R ) != 0)
					{
						printf("color error %d\n\r", i);
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}


					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 1) != 0x0)
					{
						printf("l error %d\n\r", i);
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0)
					{
						printf("h error %d\n\r", i);
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}

					//g
					//printf("g:0x%x\n\r", colors[j]>>8 & 0xff);
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, G) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 41, (i * 3) + 2) != 0x0)
					{
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, 0x60 + i2c_count, 42, 0x0) != 0x0)
					{
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}
					//b
					//printf("b:0x%x\n\r", (colors[j] >> 16) & 0xff);
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 43, B) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}
					j++;
				}
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
#if 0
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x01) == 0xff)
				{
					printf("false\n\r");
				}
#endif
			}
		}
	}

	if (effectId != 0)
	{
		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", 0x60 + i2c_count);
				//build in mode
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x02) == 0xff)
				{
					printf("false\n\r");
				}

				//led pixels
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
				{
					printf("false\n\r");
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
				{
					printf("false\n\r");
				}
				//function
				if (effectId == 1)//FUNC_Breathing
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x2) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 2)//FUNC_Strobe
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x3) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 3)//FUNC_Cycling
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x4) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 4)//FUNC_Random
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x5) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 5)//FUNC_Music
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x6) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 6)//FUNC_Wave
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x7) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 7)//FUNC_Spring
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0x8) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 8)//FUNC_Water
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 13) == 0xff)
					{
						printf("false\n\r");
					}
				}
				if (effectId == 9)//FUNC_Rainbow
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 14) == 0xff)
					{
						printf("false\n\r");
					}
				}
				//r
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 44, R) == 0xff)
				{
					printf("false\n\r");
				}

				//g
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 45, G) == 0xff)
				{
					printf("false\n\r");
				}

				//b
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 46, B) == 0xff)
				{
					printf("false\n\r");
				}

				//speed
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 55, SP) == 0xff)
				{
					printf("false\n\r");
				}

			}
		}
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			for (i2c_count = 0; i2c_count < 4; i2c_count++)
			{
				if (i2c_addrs[i2c_count] != 0)
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
					{
						printf("false\n\r");
					}
				}
			}
		}
	}

	return 0;
}



extern "C" _declspec(dllexport) int SetOff(void)
{


		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", 0x60 + i2c_count);
				//build in mode
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x25, 0x02) == 0xff)
				{
					printf("false\n\r");
				}

				//led pixels
				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x28, 0x00) == 0xff)
				{
					printf("false\n\r");
				}

				if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x27, 0x0a) == 0xff)
				{
					printf("false\n\r");
				}
				//function
	            //all led off
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 38, 0) == 0xff)
					{
						printf("false\n\r");
					}
			
			}
		}
		for (i2c_count = 0; i2c_count < 4; i2c_count++)
		{
			for (i2c_count = 0; i2c_count < 4; i2c_count++)
			{
				if (i2c_addrs[i2c_count] != 0)
				{
					if (write_slave_data(smbus_address, 0x60 + i2c_count, 0x24, 0x01) == 0xff)
					{
						printf("false\n\r");
					}
				}
			}
		}


	return 0;
}


extern "C" _declspec(dllexport) int Init(void)
{
	unsigned char retry_install = 0;
	printf("init\n\r");
#if 1
	dbg_printf("initial\n\r");
	OUTINFO_0_PARAM("initial\n\r");
	DWORD errNum = 0;
	UCHAR  driverLocation[MAX_PATH];

	SC_HANDLE schSCManager;
TEST:
	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!schSCManager)
	{

		dbg_printf("Open SC Manager failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("Open SC Manager failed! Error = %d \n", GetLastError());
		return S_FALSE;
	}
	dbg_printf("Open SC Manager\n\r");
	OUTINFO_0_PARAM("Open SC Manager\n\r");
	if (!GetDriverPath((LPSTR)driverLocation))
	{
		return S_FALSE;
	}
	dbg_printf("driverLocation:%s\n\r", driverLocation);
	OUTINFO_1_PARAM("driverLocation:%s\n\r", driverLocation);
	if (InstallDriver(schSCManager,
		DRIVER_NAME,
		(LPSTR)driverLocation
	))
	{

		if (!StartDriver(schSCManager, (LPSTR)DRIVER_NAME))
		{
			dbg_printf("Unable to start driver. \n");
			OUTINFO_0_PARAM("Unable to start driver. \n");
			RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME); //jc can start so to remove the driver
			return S_FALSE;
		}
		dbg_printf("instal and start driver\n\r");
		OUTINFO_0_PARAM("instal and start driver\n\r");
	}
	else
	{

		RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME);
		dbg_printf("Unable to install driver. \n");
		OUTINFO_0_PARAM("Unable to install driver. \n");
		if (retry_install == 3)
		{
			return S_FALSE;
		}
		else {
			retry_install++;
			goto TEST;
		}

	}

	hDevice = CreateFileA(Device_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		dbg_printf("Error: CreatFile Failed : %d\n", GetLastError());
		OUTINFO_1_PARAM("Error: CreatFile Failed : %d\n", GetLastError());
		return S_FALSE;
	}
	//unsigned int smbus_address = 0;
	SmbCtrl_Get_BaseAddress_Intel(&smbus_address);
	dbg_printf("smbus address:0x%x\n\r", smbus_address);
	OUTINFO_1_PARAM("smbus address:0x%x\n\r", smbus_address);

	int loccunt = 0;
	for (loccunt = 0; loccunt < 4; loccunt++)
	{
		unsigned char temp = 0xff;
		temp = read_slave_data(smbus_address, 0x60 + loccunt, 0x01); //check ddr5 id
		dbg_printf("read 0x3a:0x%x\n\r", temp);
		OUTINFO_1_PARAM("read 0x3a:0x%x\n\r", temp);
		if (temp != 0xff)
		{
			i2c_addrs[loccunt] = 1; //devices detect
		}
		else {
			i2c_addrs[loccunt] = 0;
		}
	}

	if ((i2c_addrs[0] == 0) && (i2c_addrs[1] == 0) && (i2c_addrs[2] == 0) && (i2c_addrs[3] == 0))
		return S_FALSE;
#endif

	return 0;
}


extern "C" _declspec(dllexport) int Exit(void)
{

	printf("Exit\n\r");

	DWORD errNum = 0;
	UCHAR  driverLocation[MAX_PATH];

	SC_HANDLE schSCManager;
	schSCManager = OpenSCManager(NULL,
		NULL,
		SC_MANAGER_ALL_ACCESS
	);
	if (!schSCManager)
	{

		dbg_printf("Open SC Manager failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("Open SC Manager failed! Error = %d \n", GetLastError());
		return S_FALSE;
	}
	dbg_printf("Open SC Manager\n\r");
	OUTINFO_0_PARAM("Open SC Manager\n\r");
	if (!GetDriverPath((LPSTR)driverLocation))
	{
		return S_FALSE;
	}
	dbg_printf("driverLocation:%s\n\r", driverLocation);
	OUTINFO_1_PARAM("driverLocation:%s\n\r", driverLocation);
#if 0
	if (InstallDriver(schSCManager,
		DRIVER_NAME,
		(LPSTR)driverLocation
	))
	{

		if (!StartDriver(schSCManager, (LPSTR)DRIVER_NAME))
		{
			dbg_printf("Unable to start driver. \n");
			OUTINFO_0_PARAM("Unable to start driver. \n");
			RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME); //jc can start so to remove the driver
			return S_FALSE;
		}
		dbg_printf("instal and start driver\n\r");
		OUTINFO_0_PARAM("instal and start driver\n\r");
	}
	else
	{

		RemoveDriver(schSCManager, (LPSTR)DRIVER_NAME);
		dbg_printf("Unable to install driver. \n");
		OUTINFO_0_PARAM("Unable to install driver. \n");
		return S_FALSE;
	}
#endif
	hDevice = CreateFileA(Device_NAME,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		dbg_printf("Error: CreatFile Failed : %d\n", GetLastError());
		OUTINFO_1_PARAM("Error: CreatFile Failed : %d\n", GetLastError());
		return S_FALSE;
	}
		CloseHandle(hDevice);

		StopDriver(schSCManager,
			DRIVER_NAME
		);
		printf("stop driver\n\r");

		RemoveDriver(schSCManager,
			DRIVER_NAME
		);
		printf("remove driver\n\r");

		CloseServiceHandle(schSCManager);
		printf("close Service\n\r");
		return S_FALSE;
	
	return 0;
}