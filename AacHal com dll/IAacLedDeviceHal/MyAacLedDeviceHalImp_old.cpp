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
#include <wbemidl.h>
#include <comutil.h>
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "comsuppw.lib") 


const wchar_t* targetManufacturer = L"Team Group";
const wchar_t* targetPartNumber = L"UD5-7200";
#define dbg_printf(fmt, ...) 
//#define dbg_printf printf
#define ddr_i2c_address 0x70 //7BIT i2c
#define mem_slot 4

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


volatile char i2c_addrs[mem_slot] = { 0 };
volatile char wmi_addrs[mem_slot] = { 0 };
volatile char i2c_mem_slot_cnt;
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

//light register define micro
#define MR36 0x24	//LLSI Enable
#define MR37 0x25	//LLSI Transfer Mode and Frequency Selection
#define MR38 0x26	     //LLSI Build - in Mode LED Function Selection(for Build - in Mode only)
#define MR40 0x28  	//LLSI Pixel Count Selection 1
#define MR41 0x29	//LLSI Data Byte Selection 0 (for Sync.Mode only)
#define MR42 0x2a	//LLSI Data Byte Selection 1 (for Sync.Mode only)
#define MR43 0x2b	//LLSI Data(for Sync.Mode only)
#define MR39 0x27	//LLSI Pixel Count Selection 0
#define MR44 0x2c	//Color R(for Build - in Mode only)
#define MR45 0x2d	//Color G(for Build - in Mode only)
#define MR46 0x2e	//Color B(for Build - in Mode only)
#define MR47 0x2f	//Block Write(for Sync.Mode only)
#define MR55 0x37	//LED Speed(for Build - in Mode only)
#define MR56 0x38	//LED Brightness(for Build - in Mode only)
#define MR62 62



/*
This function is designed to install a driver by attempting to create a 
service using the CreateServiceA function. If the service already exists, 
it returns success. Otherwise, it tries to create the service and outputs 
an error message if the creation fails. Finally, it closes the service 
handle and returns an appropriate boolean value, indicating whether the 
installation was successful or not.
*/

BOOL InstallDriver(
	SC_HANDLE SchSCManager, // Handle to the Service Control Manager
	LPCSTR DriverName,      // Name of the driver
	LPCSTR ServiceExe       // Path to the driver's executable
)
{
	SC_HANDLE schService; // Handle to the service
	DWORD err; // Variable to store error codes

	// Try to create a service
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

		// If the service already exists, return success
		if (err == ERROR_SERVICE_EXISTS)
		{
			return TRUE;
		}
		else
		{
			// If creating the service fails, output an error message
			dbg_printf("CreateService failed! Error = %d \n", err);
			OUTINFO_1_PARAM("CreateService failed! Error = %d \n", err);
			return FALSE;
		}
	}

	// Close the service handle
	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return TRUE; // Successfully installed the driver, return true
}
/*
This function is used to remove a driver by opening the corresponding 
service using the OpenServiceA function and then attempting to delete 
the service using the DeleteService function. It outputs error messages
if any of these steps fail and returns a boolean value indicating whether
 the removal was successful or not.
*/
BOOL RemoveDriver(
	SC_HANDLE SchSCManager, // Handle to the Service Control Manager
	LPCSTR DriverName       // Name of the driver to remove
)
{
	SC_HANDLE schService;   // Handle to the service
	BOOLEAN rCode;          // Return code indicating success or failure

	// Open the service using the given Service Control Manager handle
	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)
	{
		// If opening the service fails, output an error message
		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}

	// Attempt to delete the service
	if (DeleteService(schService))
	{
		rCode = TRUE; // Deletion succeeded
	}
	else
	{
		// If deleting the service fails, output an error message
		dbg_printf("DeleteService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("DeleteService failed! Error = %d \n", GetLastError());
		rCode = FALSE; // Deletion failed
	}

	// Close the service handle
	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return rCode; // Return whether the removal was successful or not
}
/*
This function is used to start a driver by opening the corresponding service
using the OpenServiceA function and then attempting to start the service 
using the StartServiceA function. It outputs error messages if any of these
steps fail and returns a boolean value indicating whether the service was 
started successfully or not. If the service is already running, it treats 
it as a success.
*/

BOOL StartDriver(
	SC_HANDLE SchSCManager, // Handle to the Service Control Manager
	LPCSTR DriverName       // Name of the driver to start
)
{
	SC_HANDLE schService; // Handle to the service
	DWORD err;            // Variable to store error codes

	// Open the service using the given Service Control Manager handle
	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)
	{
		// If opening the service fails, output an error message
		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}

	// Attempt to start the service
	if (!StartServiceA(schService, 0, NULL))
	{
		err = GetLastError();

		// If the service is already running, treat it as a success
		if (err == ERROR_SERVICE_ALREADY_RUNNING)
		{
			dbg_printf("StartService run \n", err);
			OUTINFO_1_PARAM("StartService run \n", err);
			return TRUE;
		}
		else
		{
			// If starting the service fails, output an error message
			dbg_printf("StartService failure! Error = %d \n", err);
			OUTINFO_1_PARAM("StartService failure! Error = %d \n", err);
			return FALSE;
		}
	}

	// Close the service handle
	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return TRUE; // Return whether the service was started successfully or not
}
/*
This function is used to stop a driver by opening the corresponding service
using the OpenServiceA function and then attempting to stop the service 
using the ControlService function with the SERVICE_CONTROL_STOP control 
code. It outputs error messages if any of these steps fail and returns 
a boolean value indicating whether the service was stopped successfully 
or not.
*/
BOOL StopDriver(
	SC_HANDLE SchSCManager,   // Handle to the Service Control Manager
	LPCSTR DriverName         // Name of the driver to stop
)
{
	BOOLEAN rCode = TRUE;     // Return code indicating success or failure
	SC_HANDLE schService;     // Handle to the service
	SERVICE_STATUS serviceStatus; // Structure to store the service status

	// Open the service using the given Service Control Manager handle
	schService = OpenServiceA(SchSCManager,
		DriverName,
		SERVICE_ALL_ACCESS
	);

	if (schService == NULL)
	{
		// If opening the service fails, output an error message
		dbg_printf("OpenService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("OpenService failed! Error = %d \n", GetLastError());
		return FALSE;
	}

	// Attempt to stop the service
	if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		rCode = TRUE; // Stopping the service succeeded
	}
	else
	{
		// If stopping the service fails, output an error message
		dbg_printf("ControlService failed! Error = %d \n", GetLastError());
		OUTINFO_1_PARAM("ControlService failed! Error = %d \n", GetLastError());
		rCode = FALSE; // Stopping the service failed
	}

	// Close the service handle
	if (schService)
	{
		CloseServiceHandle(schService);
	}

	return rCode; // Return whether the service was stopped successfully or not
}
/*
This function determines whether the operating system is running on a 64-bit
system. It does this by obtaining the system information using the 
GetNativeSystemInfo function and then checking the wProcessorArchitecture 
field of the SYSTEM_INFO structure. If the processor architecture is 
PROCESSOR_ARCHITECTURE_AMD64 or PROCESSOR_ARCHITECTURE_IA64, it returns 
TRUE, indicating a 64-bit system; otherwise, it returns FALSE for a 
non-64-bit system.

*/

BOOL Is64bitSystem()
{
	SYSTEM_INFO si; // Structure to hold system information
	GetNativeSystemInfo(&si); // Get native system information

	// Check if the system architecture is 64-bit (AMD64 or IA64)
	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
	{
		return TRUE; // The system is 64-bit
	}
	else
	{
		return FALSE; // The system is not 64-bit
	}
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


/*
This function is responsible for retrieving the path to a driver file 
based on the system's architecture (32-bit or 64-bit). It first attempts 
to get the module (DLL) handle corresponding to the appropriate architecture,
retrieves the path to that module, and then extracts the directory portion. 
If the first attempt fails, it tries the second option. Finally, 
it concatenates the driver file name based on the system architecture 
and returns the path in the DriverLocation variable. If any of the steps 
fail, it outputs error messages and returns FALSE.
*/
#pragma warning( disable : 4996 )
BOOL GetDriverPath(
	LPSTR DriverLocation
)
{
	HMODULE hModule;

	// Check if the system is 64-bit or 32-bit
	if (Is64bitSystem())
	{
		hModule = GetModuleHandle(_T("AacHal_x64.dll"));
	}
	else
	{
		hModule = GetModuleHandle(_T("AacHal_x86.dll"));
	}

	// Attempt to get the module (DLL) handle
	if (hModule)
	{
		// Get the path to the module (DLL) containing the driver
		GetModuleFileNameA(hModule, DriverLocation, MAX_PATH);

		// Extract the directory from the path
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
		// Output an error message if getting the module handle fails
		dbg_printf("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		dbg_printf("dll path false\n\r");
		OUTINFO_1_PARAM("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		OUTINFO_0_PARAM("dll path false\n\r");
	}

	// If the first attempt fails, try the second option
	hModule = GetModuleHandle(_T("AacHal_x86.dll"));
	if (hModule)
	{
		// Get the path to the module (DLL) containing the driver
		GetModuleFileNameA(hModule, DriverLocation, MAX_PATH);

		// Extract the directory from the path
		char *pEnd0 = strrchr(DriverLocation, '\\');
		char *pEnd1 = strrchr(DriverLocation, '/');
		if (pEnd0 < pEnd1)
			pEnd0 = pEnd1;

		if (pEnd0 != NULL)
			*pEnd0 = '\0';
	}
	else
	{
		// Output an error message if getting the module handle fails for the second time
		dbg_printf("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		dbg_printf("dll path false\n\r");
		OUTINFO_1_PARAM("GetCurrentDirectory failed! Error = %d \n\r", GetLastError());
		OUTINFO_0_PARAM("dll path false\n\r");
		return FALSE;
	}

EXIT1:

	// Concatenate the driver file name based on system architecture
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

	return TRUE; // Successfully obtained the driver file path
}

#define PCI_DataPort  0x00000CFC
#define PCI_ControlPort  0x00000CF8
HANDLE hDevice;
volatile unsigned int smbus_address = 0;
volatile unsigned int piix4_smba = 0x0B00;
volatile  int chipset = 0; //0 intel, 1 amd

/*
This function is used to perform a 4-byte write operation to a device. 
It sets the address and data to be written, and then it uses 
DeviceIoControl to perform the I/O control operation with the 
IOCTL_GPD_WRITE_PORT_ULONG control code. If the operation is successful, 
it returns TRUE, indicating a successful write. 
If the operation fails, it returns FALSE.
*/
BOOL IOWrite_4(unsigned int u32address, unsigned int u32Data)
{
	BOOL IoctlResult;
	unsigned int out_buf[2];  // Output buffer
	unsigned int in_buf[2];   // Input buffer
	DWORD dwOutBytes;

	out_buf[0] = u32address;   // Set the address to write to
	out_buf[1] = u32Data;      // Set the data to write

	// Perform an I/O control operation to write to the device
	IoctlResult = DeviceIoControl(
		hDevice,                       // Handle to the device
		IOCTL_GPD_WRITE_PORT_ULONG,    // Control code for write operation
		out_buf, sizeof(out_buf),      // Input data buffer
		NULL, 0,                       // Output data buffer
		&dwOutBytes,                   // Output data length
		NULL
	);

	if (!IoctlResult)
	{
		// If the I/O control operation fails, return FALSE
		return FALSE;
	}

	// The write operation was successful, return TRUE
	return TRUE;
}



/*
This function is used to perform a 4-byte read operation from a device. 
It sets the address to read from and then uses DeviceIoControl to perform
the I/O control operation with the IOCTL_GPD_READ_PORT_ULONG control code. 
If the operation is successful, it copies the read data from the input 
buffer to the pu32Data parameter and returns TRUE, indicating a successful
read. If the operation fails, it returns FALSE.
*/
BOOL IORead_4(unsigned int u32address, unsigned int *pu32Data)
{
	BOOL IoctlResult;
	DWORD out_buf[1];      // Output buffer
	DWORD in_buf[1];       // Input buffer
	DWORD dwOutBytes;

	out_buf[0] = u32address; // Set the address to read from

	// Perform an I/O control operation to read from the device
	IoctlResult = DeviceIoControl(
		hDevice,                    // Handle to the device
		IOCTL_GPD_READ_PORT_ULONG,  // Control code for read operation
		out_buf, sizeof(out_buf),   // Input data buffer
		in_buf, sizeof(in_buf),     // Output data buffer
		&dwOutBytes,                // Output data length
		NULL
	);

	if (!IoctlResult)
	{
		// If the I/O control operation fails, return FALSE
		return FALSE;
	}

	// Copy the read data from the input buffer to the output parameter
	*pu32Data = in_buf[0];

	// The read operation was successful, return TRUE
	return TRUE;
}

/*
This function is used to read a 32-bit configuration register value from
a PCI device. It assembles the configuration register address using the
input parameters and then writes this address to the PCI control port using
IOWrite_4. After that, it reads the data from the PCI data port using 
IORead_4. If either the write or read operation fails, it outputs an error
message and returns 0 to indicate failure. If both operations succeed, 
it returns 1 to indicate success, and the read data is stored in the 
pu32Data parameter.
*/
int PCI_Read(unsigned char bBus, unsigned char bDevice, unsigned char bFunction, unsigned char bRegIndex, unsigned int *pu32Data)
{
	// Create the configuration register address by combining various values
	unsigned int u32Config = (unsigned int)(0x80000000 | bRegIndex | ((unsigned int)bFunction << 8) | ((unsigned int)bDevice << 11) | ((unsigned int)bBus << 16));

	// Write the configuration address to the PCI control port
	if (IOWrite_4(PCI_ControlPort, u32Config) == FALSE)
	{
		// If writing to the control port fails, output an error message
		dbg_printf("pci read false 1\n\r");
		OUTINFO_0_PARAM("pci read false 1\n\r");
		return 0; // Return 0 to indicate failure
	}

	// Read the data from the PCI data port
	if (IORead_4(PCI_DataPort, &(*pu32Data)) == FALSE)
	{
		// If reading from the data port fails, output an error message
		dbg_printf("pci read false 2\n\r");
		OUTINFO_0_PARAM("pci read false 2\n\r");
		return FALSE; // Return 0 to indicate failure
	}

	// Return 1 to indicate success
	return TRUE;	
}
/*
This function is used to perform an I/O write operation to a device. 
It prepares an output buffer containing the address to write to, 
the length of data (assuming 1 byte), and the data itself. Then, 
it uses DeviceIoControl to perform the I/O control operation with 
the IOCTL_GPD_R1_WRITE_PORT control code. If the operation is successful,
it returns TRUE, indicating a successful write. If the operation fails,
it outputs an error message and returns FALSE.
*/

BOOL IOWrite(unsigned int u32address, unsigned char pu32Data)
{
	BOOL IoctlResult;
	unsigned int out_buf[3];  // Output buffer
	DWORD dwOutBytes;

	// Prepare the output buffer with address, length, and data
	out_buf[0] = u32address;  // Address to write to
	out_buf[1] = 1;           // Length (assuming 1 byte)
	out_buf[2] = pu32Data;    // Data to write

	// Perform an I/O control operation to write to the device
	IoctlResult = DeviceIoControl(
		hDevice,                    // Handle to the device
		IOCTL_GPD_R1_WRITE_PORT,    // Control code for write operation
		out_buf, 9,   // Input data buffer
		NULL, 0,                    // Output data buffer
		&dwOutBytes,                // Output data length
		NULL
	);

	if (!IoctlResult)
	{
		// If the I/O control operation fails, output an error message
		dbg_printf("IOWrite write false \n\r");
		OUTINFO_0_PARAM("IOWrite write false \n\r");
		return FALSE; // Return FALSE to indicate failure
	}

	// The write operation was successful, return TRUE
	return TRUE;
}
/*
This function is used to perform an I / O read operation from a device.
It prepares an output buffer containing the address to read from and the 
length of data(assuming 1 byte).Then, it uses DeviceIoControl to perform 
the I / O control operation with the IOCTL_GPD_R1_READ_PORT control code.
If the operation is successful, it copies the read data from the input 
buffer to the pu32Data parameter and returns TRUE, indicating a successful 
read.If the operation fails, it outputs an error message and returns FALSE.
*/
BOOL IORead(unsigned int u32address, unsigned char *pu32Data)
{
	BOOL IoctlResult;
	unsigned int out_buf[2];    // Output buffer
	unsigned char in_buf[1];    // Input buffer
	DWORD dwOutBytes;

	out_buf[0] = u32address;    // Address to read from
	out_buf[1] = 1;             // Length (assuming 1 byte)

	// Perform an I/O control operation to read from the device
	IoctlResult = DeviceIoControl(
		hDevice,                  // Handle to the device
		IOCTL_GPD_R1_READ_PORT,   // Control code for read operation
		out_buf, sizeof(out_buf), // Input data buffer
		in_buf, sizeof(in_buf),   // Output data buffer
		&dwOutBytes,              // Output data length
		NULL
	);

	if (!IoctlResult)
	{
		// If the I/O control operation fails, output an error message
		dbg_printf("IOWrite read false \n\r");
		OUTINFO_0_PARAM("IOWrite read false \n\r");
		return FALSE; // Return FALSE to indicate failure
	}

	// Copy the read data from the input buffer to the output parameter
	*pu32Data = in_buf[0];

	// The read operation was successful, return TRUE
	return TRUE;
}


#define  Intel_SMBusCtrl_Dev  0x1f
#define  Intel_SMBusCtrl_Fun   0x04
#define  Intel_SMBusCtrl_Idx_VidDid 0x0
#define Intel_SMBusCtrl_Idx_BaseAddr 0x20

BOOL SmbCtrl_Get_BaseAddress_Intel(volatile unsigned int *u32Addr)
{
	unsigned int  u32Data = 0x00;
	PCI_Read(0x00, Intel_SMBusCtrl_Dev, Intel_SMBusCtrl_Fun, Intel_SMBusCtrl_Idx_VidDid, &u32Data);
	dbg_printf("CPU_ID=0x%x", u32Data & 0XFFFF);
	OUTINFO_1_PARAM("CPU_ID=0x%x", u32Data & 0XFFFF);
	if ((u32Data & 0xffff) != 0x8086)
	{
		dbg_printf("AMD CPU");		
		chipset = 1;//amd chp
		return FALSE;
	}
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
		chipset = 0;//intel
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
//intel smbus read write 
unsigned char intel_read_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char  *dest)
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
			printf("i2c devices no ack\n\r");
			return 0xff; //false
		}
	}

	if ((temp & 0x1c) == 0)
	{//ok
		IORead((SMBHSTDAT0 + SMB_base), &temp);
		*dest = temp;
		return 0x00;//pass
	}
	return 0xff;//false
}

#define hst_cnt_start 0x40
#define smbcmd_bytedata 0x08
unsigned char intel_write_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char wdata)
{
	unsigned char temp;
	int timeout = 0;
	//if (slave_address == 0x61)
		//printf("address error\n\r");
	//IORead((SMB_base + SMBHSTSTS), &temp);
	//delay1us(5);
	//clear all status bits
	IOWrite((SMBHSTSTS + SMB_base), 0xff);	
	//delay1us(5);
	// write the offset
	IOWrite((SMBHSTCMD + SMB_base), offset);
	// write the slave address 
	IOWrite((SMBHSTADD + SMB_base), (slave_address << 1));
	//IORead((SMB_base + SMBHSTADD), &temp);
	//delay1us(5);
	// Step 4. Write  Count value to Host Data0 Register
	IOWrite((SMBHSTDAT0 + SMB_base), wdata); //test count

	//delay1us(5);
	//clear all status bits

	IOWrite((SMBHSTSTS + SMB_base), 0xff);
	//delay1us(5);

	IOWrite((SMBHSTCNT + SMB_base), hst_cnt_start | smbcmd_bytedata);
	//IORead((SMB_base + SMBHSTCNT), &temp);
	//if (temp != 0x0)
		//printf("SMBHSTCNT error\n\r");
	
	while (1)
	{
		//delay1us(150);
		IORead((SMB_base + SMBHSTSTS), &temp);
		if ((temp & 0x02) == 0x2)
		{
			break;
		}
		timeout++;

		if (timeout > MAX_TIMEOUT)
		{
			printf("i2c devices no ack\n\r");
			return 0xff; //false
		}
	}

//	delay1us(100);

	return 0x00;//pass
}

#if 0
void intel_block_read(unsigned int SMB_base, unsigned char slave_address, unsigned char index)
{
	unsigned char  data[32] = { 0 };
	// Initial SMBus
	unsigned char temp;
	int Fail_CNT;
	unsigned char SMB_STS;
	unsigned bSent, bSent2, LoopCnt;
	int LoopIndex;
	IOWrite(SMB_base + 0x0D, 0x00);
	Fail_CNT = 0;
	unsigned char  Blockcount = 0;
	do {
		IOWrite(SMB_base + SMBHSTSTS, 0xFF);
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		Fail_CNT++;
	} while (SMB_STS != 0x00 && Fail_CNT < 100);
	if (Fail_CNT > MAX_TIMEOUT) {
		printf("timeout\n\r");
		return;
	}
	// Step 2. Write the Slave Address in Address Register
	IOWrite(SMB_base + SMBHSTADD, (slave_address << 1 | 1));             // Read protocol
	// Step 3. Write Index in Command Register
	IOWrite(SMB_base + SMBHSTCMD, index);
	// Step 4. Set SMB_CMD for Block Command
	IOWrite(SMB_base + SMBHSTCNT, 0x54);

	// Step 5. Read the data bytes from Host Block Data Byte Register
	for (bSent = 0; bSent <= 0x20; bSent++) {                           // Max: 32 bytes.
		   // Wait until the previous byte is sent out
		for (LoopIndex = 0; LoopIndex < 1000; LoopIndex++) {
			IORead((SMB_base + SMBHSTSTS), &SMB_STS);
			if ((SMB_STS & 0x80))
			{
				break;
			}
																   // Delay 1 ms
		}
		IORead((SMB_base + SMBHSTSTS), &SMB_STS);
		if (SMB_STS == 0x00) {                // Byte Done bit is not set
			printf("SMBus Failed: Byte done for Block command = 0 !\n\r");
			return ;
		}

		if (SMB_STS & 0x1C) {
			if (SMB_STS & 0x04) {
				printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);
				
			}
			if (SMB_STS & 0x08) {
				printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);
				
			}
			if (SMB_STS & 0x10) {
			    printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);
				
			}
			IOWrite(SMB_base + SMBHSTSTS, 0x1C);                      // Clear error status and INUSE_STS/INTR bits.
			return ;
		}



		if (bSent == 0) {
			 IORead(SMB_base + SMBHSTDAT0,&Blockcount);    // Retrieve the block count value
		}
		IORead(SMB_base + SMBBLKDAT, &temp);                // Retrieve the next data byte.
		data[bSent] = temp;                // Retrieve the next data byte.
		IOWrite(SMB_base + SMBHSTSTS, 0x80);                      // Clear byte done bit to request another byte read.
		if ((bSent + 1) >= Blockcount) {
			break;  // Reach the block count number
		}
	}
	LoopCnt = 1000;
	for (LoopIndex = 0; LoopIndex < LoopCnt; LoopIndex++) {
		//       my_delay(1);                                                // delay one millisecond
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		if (SMB_STS & 0x01) continue;                               // HOST is still busy
		if (SMB_STS & 0x02) break;                                  // Termination of the SMBus command.
	}

	IOWrite(SMB_base + SMBHSTSTS, 0x42);                                 // Clear INUSE_STS and INTR bits.

	//========= Time-out or completion of the SMBus command. To check the SMBus host status bits.
	if (SMB_STS & 0x1C) {
		if (SMB_STS & 0x04) {
			printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);
			
		}
		if (SMB_STS & 0x08) {
			printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);
			
		}
		if (SMB_STS & 0x10) {
			printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);
			
		}
		IOWrite(SMB_base + SMBHSTSTS, 0x1C);                       // Clear error status and INUSE_STS/INTR bits.
		return ;
	}
	if ((SMB_STS & 0x01) == 0x01) {
		printf("SMBus Failed: HOST_BUSY ![%2X]", SMB_STS);
		IOWrite(SMB_base + SMBHSTSTS, 0x42);                             // Clear INUSE_STS and INTR bits.
		return ;
	}
}

void intel_block_read_word(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned int  *dest)
{
	unsigned char  data[32] = { 0 };
	// Initial SMBus
	unsigned char temp;
	int Fail_CNT;
	unsigned char SMB_STS;
	unsigned bSent, bSent2, LoopCnt;
	int LoopIndex;
	IOWrite(SMB_base + 0x0D, 0x00);
	Fail_CNT = 0;
	unsigned char  Blockcount = 0;
	do {
		IOWrite(SMB_base + SMBHSTSTS, 0xFF);
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		Fail_CNT++;
	} while (SMB_STS != 0x00 && Fail_CNT < 100);
	if (Fail_CNT > MAX_TIMEOUT) {
		printf("timeout\n\r");
		return;
	}
	// Step 2. Write the Slave Address in Address Register
	IOWrite(SMB_base + SMBHSTADD, (slave_address << 1 | 1));             // Read protocol
	// Step 3. Write Index in Command Register
	IOWrite(SMB_base + SMBHSTCMD, index);
	// Step 4. Set SMB_CMD for Block Command
	IOWrite(SMB_base + SMBHSTCNT, 0x54);

	// Step 5. Read the data bytes from Host Block Data Byte Register
	for (bSent = 0; bSent <= 0x20; bSent++) {                           // Max: 32 bytes.
		   // Wait until the previous byte is sent out
		for (LoopIndex = 0; LoopIndex < 1000; LoopIndex++) {
			IORead((SMB_base + SMBHSTSTS), &SMB_STS);
			if ((SMB_STS & 0x80))
			{
				break;
			}
			// Delay 1 ms
		}
		IORead((SMB_base + SMBHSTSTS), &SMB_STS);
		if (SMB_STS == 0x00) {                // Byte Done bit is not set
			printf("SMBus Failed: Byte done for Block command = 0 !\n\r");
			return;
		}

		if (SMB_STS & 0x1C) {
			if (SMB_STS & 0x04) {
				printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);

			}
			if (SMB_STS & 0x08) {
				printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);

			}
			if (SMB_STS & 0x10) {
				printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);

			}
			IOWrite(SMB_base + SMBHSTSTS, 0x1C);                      // Clear error status and INUSE_STS/INTR bits.
			return;
		}



		if (bSent == 0) {
			IORead(SMB_base + SMBHSTDAT0, &Blockcount);    // Retrieve the block count value
		}
		IORead(SMB_base + SMBBLKDAT, &temp);                // Retrieve the next data byte.
		data[bSent] = temp;                // Retrieve the next data byte.
		IOWrite(SMB_base + SMBHSTSTS, 0x80);                      // Clear byte done bit to request another byte read.
		if ((bSent + 1) >= Blockcount) {
			//if ((bSent + 1) >= 10) {
			break;  // Reach the block count number
		}
	}
	LoopCnt = 1000;
	for (LoopIndex = 0; LoopIndex < LoopCnt; LoopIndex++) {
		//       my_delay(1);                                                // delay one millisecond
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		if (SMB_STS & 0x01) continue;                               // HOST is still busy
		if (SMB_STS & 0x02) break;                                  // Termination of the SMBus command.
	}

	IOWrite(SMB_base + SMBHSTSTS, 0x42);                                 // Clear INUSE_STS and INTR bits.

	//========= Time-out or completion of the SMBus command. To check the SMBus host status bits.
	if (SMB_STS & 0x1C) {
		if (SMB_STS & 0x04) {
			printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);

		}
		if (SMB_STS & 0x08) {
			printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);

		}
		if (SMB_STS & 0x10) {
			printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);

		}
		IOWrite(SMB_base + SMBHSTSTS, 0x1C);                       // Clear error status and INUSE_STS/INTR bits.
		return;
	}
	if ((SMB_STS & 0x01) == 0x01) {
		printf("SMBus Failed: HOST_BUSY ![%2X]", SMB_STS);
		IOWrite(SMB_base + SMBHSTSTS, 0x42);                             // Clear INUSE_STS and INTR bits.
		return;
	}
	*dest = data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24);
	//printf("Blockcount:0x%x\n\r", Blockcount);
	//printf("\n\r");
	//int i;
	//for (i = 0; i < 32; i++)
		//printf("0x%x ", data[i]);
	//printf("\n\r");
}
#endif


void intel_block_read_multi_byte(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char* dest)
{
	unsigned char  data[32] = { 0 };
	// Initial SMBus
	unsigned char temp;
	int Fail_CNT;
	unsigned char SMB_STS;
	unsigned bSent, bSent2, LoopCnt;
	int LoopIndex;
	IOWrite(SMB_base + 0x0D, 0x00);
	Fail_CNT = 0;
	unsigned char  Blockcount = 0;
	do {
		IOWrite(SMB_base + SMBHSTSTS, 0xFF);
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		Fail_CNT++;
	} while (SMB_STS != 0x00 && Fail_CNT < 100);
	if (Fail_CNT > MAX_TIMEOUT) {
		dbg_printf("timeout\n\r");
		return;
	}
	// Step 2. Write the Slave Address in Address Register
	IOWrite(SMB_base + SMBHSTADD, (slave_address << 1 | 1));             // Read protocol
	// Step 3. Write Index in Command Register
	IOWrite(SMB_base + SMBHSTCMD, index);
	// Step 4. Set SMB_CMD for Block Command
	IOWrite(SMB_base + SMBHSTCNT, 0x54);

	// Step 5. Read the data bytes from Host Block Data Byte Register
	for (bSent = 0; bSent <= 0x20; bSent++) {                           // Max: 32 bytes.
		   // Wait until the previous byte is sent out
		for (LoopIndex = 0; LoopIndex < 1000; LoopIndex++) {
			IORead((SMB_base + SMBHSTSTS), &SMB_STS);
			if ((SMB_STS & 0x80))
			{
				break;
			}
			// Delay 1 ms
		}
		IORead((SMB_base + SMBHSTSTS), &SMB_STS);
		if (SMB_STS == 0x00) {                // Byte Done bit is not set
			dbg_printf("SMBus Failed: Byte done for Block command = 0 !\n\r");
			return;
		}

		if (SMB_STS & 0x1C) {
			if (SMB_STS & 0x04) {
				dbg_printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);

			}
			if (SMB_STS & 0x08) {
				dbg_printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);

			}
			if (SMB_STS & 0x10) {
				dbg_printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);

			}
			IOWrite(SMB_base + SMBHSTSTS, 0x1C);                      // Clear error status and INUSE_STS/INTR bits.
			return;
		}



		if (bSent == 0) {
			IORead(SMB_base + SMBHSTDAT0, &Blockcount);    // Retrieve the block count value
		}
		IORead(SMB_base + SMBBLKDAT, &temp);                // Retrieve the next data byte.
		data[bSent] = temp;                // Retrieve the next data byte.
		IOWrite(SMB_base + SMBHSTSTS, 0x80);                      // Clear byte done bit to request another byte read.
		if ((bSent + 1) >= Blockcount) {
			//if ((bSent + 1) >= 10) {
			break;  // Reach the block count number
		}
	}
	for (int LoopIndex = 0; LoopIndex < 10; LoopIndex++) {
		delay1us(10);                                               // delay one millisecond
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		if (SMB_STS & 0x01) continue;                               // HOST is still busy
		if (SMB_STS & 0x02) break;                                  // Termination of the SMBus command.
	}

	IOWrite(SMB_base + SMBHSTSTS, 0x42);                                 // Clear INUSE_STS and INTR bits.
#if 0
	//========= Time-out or completion of the SMBus command. To check the SMBus host status bits.
	if (SMB_STS & 0x1C) {
		if (SMB_STS & 0x04) {
			printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]", SMB_STS);

		}
		if (SMB_STS & 0x08) {
			printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]", SMB_STS);

		}
		if (SMB_STS & 0x10) {
			printf("SMBus Failed: FAILED (failed bus transaction) ![%2X]", SMB_STS);

		}
		IOWrite(SMB_base + SMBHSTSTS, 0x1C);                       // Clear error status and INUSE_STS/INTR bits.
		return;
	}
	if ((SMB_STS & 0x01) == 0x01) {
		printf("SMBus Failed: HOST_BUSY ![%2X]", SMB_STS);
		IOWrite(SMB_base + SMBHSTSTS, 0x42);                             // Clear INUSE_STS and INTR bits.
		return;
	}
#endif
	for (int i = 0; i < 16; i++)
		dest[i] = data[3 + i];
	//*dest = data[3] | (data[4] << 8) | (data[5] << 16) | (data[6] << 24);
	//printf("Blockcount:0x%x\n\r", Blockcount);
	//printf("\n\r");
	//int i;
	//for (i = 0; i < 32; i++)
		//printf("0x%x ", data[i]);
	//printf("\n\r");
}
#if 1
void intel_block_write(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char count, unsigned char* buffer)
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
		dbg_printf("timeout\n\r");
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
	for (int bSent = 0; bSent < count; bSent++) {
		for (int LoopIndex = 0; LoopIndex < 100; LoopIndex++) {
			delay1us(10);
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
				dbg_printf("SMBus Failed: DEV_ERR (Cmd/cycle/time-out) ![%2X]\n\r", SMB_STS);
			}
			if (SMB_STS & 0x08) {
				dbg_printf("SMBus Failed: BUS_ERR (transaction collision) ![%2X]\n\r", SMB_STS);

			}
			if (SMB_STS & 0x10) {
				dbg_printf("SMBus Failed: FAILED (failed bus transaction) ![0x%2X]\n\r", SMB_STS);
			}
			IOWrite((SMBHSTSTS + SMB_base), 0x1c);                     // Clear error status and INUSE_STS/INTR bits.
			return;
		}

		IOWrite((SMBBLKDAT + SMB_base), buffer[bSent + 1]);   // Place the next data byte.
		IOWrite((SMBHSTSTS + SMB_base), 0x80);              // Clear byte done bit to request another byte write.
	}

	// Wait until the previous byte is sent out
	//
	for (int LoopIndex = 0; LoopIndex < 10; LoopIndex++) {
		delay1us(10);
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);
		if (SMB_STS & 0x80)
			break; // Byte Done bit is set
	}
#if 0
	IORead(SMB_base + SMBHSTSTS, &SMB_STS);
	if (SMB_STS == 0x00) {                    // Byte Done bit is not set

		printf("SMBus Failed: Byte done for Block command = 0 !\n\r");
		return;
	}
	IOWrite((SMBHSTSTS + SMB_base), 0x80); // Clear byte done bit to request another byte write.

	int LoopCnt = 10;
	for (int LoopIndex = 0; LoopIndex < LoopCnt; LoopIndex++) {
		delay1us(10);                                                  // delay one millisecond
		IORead(SMB_base + SMBHSTSTS, &SMB_STS);


		if (SMB_STS & 0x01) continue;                               // HOST is still busy
		if (SMB_STS & 0x02) break;                                  // Termination of the SMBus command.
	}
#endif
	IOWrite((SMBHSTSTS + SMB_base), 0x42);                    // Clear INUSE_STS and INTR bits.
#if 0
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
#endif
	return;
}
#endif

// amd read write block

//PIIX4 SMBus address offsets
#define AM5_SMBHSTSTS (0)
#define AM5_SMBHSLVSTS (1)
#define AM5_SMBHSTCNT (2)
#define AM5_SMBHSTCMD (3)
#define AM5_SMBHSTADD (4)
#define AM5_SMBHSTDAT0 (5)
#define AM5_SMBHSTDAT1 (6)
#define AM5_SMBBLKDAT (7)
#define AM5_SMBSLVCNT (8)
#define AM5_SMBSHDWCMD (9)
#define AM5_SMBSLVEVT (0xA)
#define AM5_SMBSLVDAT (0xC)

int piix4_transaction()
{
	int result = 0;
	unsigned char temp;
	int timeout = 0;
	delay1us(100);
	/* Make sure the SMBus host is ready to start transmitting */
	IORead(AM5_SMBHSTSTS + piix4_smba, &temp);	
	if (temp != 0x00)
	{
		//delay1us(100);
		IOWrite(AM5_SMBHSTSTS + piix4_smba, temp);

		IORead(AM5_SMBHSTSTS + piix4_smba, &temp);

		if (temp != 0x00)
		{
			return -EBUSY;
		}
	}

	/* start the transaction by setting bit 6 */
	IORead(AM5_SMBHSTCNT + piix4_smba, &temp);
	IOWrite(AM5_SMBHSTCNT + piix4_smba, temp | 0x040);

	/* We will always wait for a fraction of a second! (See PIIX4 docs errata) */
	temp = 0;

	while ((++timeout < MAX_TIMEOUT) && temp <= 1)
	{
		delay1us(10);
		IORead(AM5_SMBHSTSTS + piix4_smba, &temp);
		timeout++;
	}

	/* If the SMBus is still busy, we give up */
	if (timeout == MAX_TIMEOUT)
	{
		dbg_printf("time out\n\r");
		result = -ETIMEDOUT;
	}

	if (temp & 0x10)
	{
		dbg_printf("time eio 0x10\n\r");
		result = -EIO;
	}

	if (temp & 0x08)
	{
		dbg_printf("time eio 0x08\n\r");
		result = -EIO;
	}

	if (temp & 0x04)
	{
		dbg_printf("time ENXIO 0x04\n\r");
		result = -ENXIO;
	}
	delay1us(200);
	IORead(AM5_SMBHSTSTS + piix4_smba, &temp);
	if (temp != 0x00)
	{
		IOWrite(AM5_SMBHSTSTS + piix4_smba, temp);
	}

	return result;
}
#define PIIX4_QUICK             0x00
#define PIIX4_BYTE              0x04
#define PIIX4_BYTE_DATA         0x08
#define PIIX4_WORD_DATA         0x0C
#define PIIX4_BLOCK_DATA        0x14

unsigned char amd_read_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char  *dest)
{
	unsigned char temp=0x00;
	unsigned char value = 0x00;
	int size;
	int timeout = 0;

	IOWrite(AM5_SMBHSTADD + piix4_smba, (slave_address << 1) | 1);
	IOWrite(AM5_SMBHSTCMD + piix4_smba, offset);
	size = PIIX4_BYTE_DATA;
	IOWrite(AM5_SMBHSTCNT + piix4_smba, (size & 0x1C));
	if (piix4_transaction() != 0x0)
	{
		temp = 0xff;
		return temp;
	}
	IORead(AM5_SMBHSTDAT0 + piix4_smba, &value);
	*dest = value;
	return temp;//false
}
unsigned char amd_write_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char wdata)
{
	unsigned char temp = 0x00;
	int size=0;
	IOWrite(AM5_SMBHSTADD + piix4_smba, (slave_address << 1) | 0);
	IOWrite(AM5_SMBHSTCMD + piix4_smba, offset);
	IOWrite(AM5_SMBHSTDAT0 + piix4_smba, wdata);
	size = PIIX4_BYTE_DATA;
	IOWrite(AM5_SMBHSTCNT + piix4_smba, (size & 0x1C));
	if (piix4_transaction() != 0x0)
	{
		temp = 0xff;
		return temp; //false
	}
	return temp; //pass
}

void amd_block_write(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char count, unsigned char* buffer)
{
	unsigned char temp = 0x00;
	int size = 0;
	IOWrite(AM5_SMBHSTADD + piix4_smba, (slave_address << 1) | 0);
	IOWrite(AM5_SMBHSTCMD + piix4_smba, index);
	size = PIIX4_BLOCK_DATA;
	IOWrite(AM5_SMBHSTDAT0 + piix4_smba, count);
	IORead(AM5_SMBHSTCNT + piix4_smba, &temp);//dummy read

	for (int i = 0; i < count; i++)
	{
		IOWrite(AM5_SMBBLKDAT + piix4_smba, buffer[i]);
	}

	IOWrite(AM5_SMBHSTCNT + piix4_smba, (size & 0x1C));
	piix4_transaction();
}

void amd_block_read_multi_byte(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char* dest)
{
	unsigned char  data[32] = { 0 };
	unsigned char length;
	unsigned char temp = 0x00;
	int size = 0;
	IOWrite(AM5_SMBHSTADD + piix4_smba, (slave_address << 1) | 1);
	IOWrite(AM5_SMBHSTCMD + piix4_smba, index);
	size = PIIX4_BLOCK_DATA;
	IOWrite(AM5_SMBHSTCNT + piix4_smba, (size & 0x1C));
	piix4_transaction();
	IORead(AM5_SMBHSTDAT0 + piix4_smba, &length); //length
	if (length == 0)
	{
		dbg_printf("lenth is 0\n\r");
		return ;
	}
	IORead(AM5_SMBHSTCNT + piix4_smba, &temp); //dummy read
	for (int i = 0; i < length; i++)
	{
		IORead(AM5_SMBBLKDAT + piix4_smba, &temp);
		//intf("0x%x \n\r", temp);
		data[i] = temp;
	}
	
	for (int i = 0; i < 16; i++)
		dest[i] = data[3 + i];
}
//common read write interface
unsigned char read_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char  *dest)
{
	unsigned char temp=0x00;
	if (chipset == 0) 
	{
		temp =intel_read_slave_data(SMB_base, slave_address, offset, dest);		
	}
	else 
	{
		temp=amd_read_slave_data(SMB_base, slave_address, offset, dest);
	}
	return temp;//false 

}
unsigned char write_slave_data(unsigned int SMB_base, unsigned char slave_address, unsigned char offset, unsigned char wdata)
{
	unsigned char temp = 0x00;
	if (chipset == 0)
	{
		temp = intel_write_slave_data(SMB_base, slave_address, offset, wdata);
	}
	else
	{
	temp=amd_write_slave_data(SMB_base, slave_address, offset, wdata);
	}
	return temp; //false
}

void block_write(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char count, unsigned char* buffer)
{
	if (chipset == 0)
	{
		intel_block_write(SMB_base, slave_address, index, count, buffer);
	}
	else {
		amd_block_write(SMB_base, slave_address, index, count, buffer);
	}
}

void block_read_multi_byte(unsigned int SMB_base, unsigned char slave_address, unsigned char index, unsigned char* dest)
{
	if (chipset == 0)
	{
		intel_block_read_multi_byte(SMB_base, slave_address, index, dest);
	}
	else {
		amd_block_read_multi_byte(SMB_base, slave_address, index, dest);
	}
}

extern  ULONG g_Components;    // number of components;

#define MY_DEVICE_NAME  L"MyComponentName";
#define MY_DEVICE_COUNT   1 

//updated for t-force
#define MY_LED_COUNT      8

wchar_t* g_MyLedLoaction[MY_LED_COUNT] = {
	L"Loaction_1",
	L"Loaction_2",
	L"Loaction_3",
	L"Loaction_4",
	L"Loaction_5",
	L"Loaction_6",
	L"Loaction_7",
	L"Loaction_8",
};


// ==== CAP ====

std::vector<DeviceEffect> g_deviceEffectInfo = {

	{ DeviceEffect(EFEFCT_0_NAME, EFEFCT_0_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_1_NAME, EFEFCT_1_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_2_NAME, EFEFCT_2_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_3_NAME, EFEFCT_3_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_4_NAME, EFEFCT_4_ID, true, false, false, false, 0, 0, 0, 0) },
	{ DeviceEffect(EFEFCT_5_NAME, EFEFCT_5_ID, true, false, false, false, 0, 0, 0, 0) },
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
	if (EnterMutex() == 1)
		return S_FALSE;

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

int SetOff_internal(void);
HRESULT STDMETHODCALLTYPE MyAacLedDevice::SetEffect(ULONG effectId, ULONG *colors, ULONG numberOfColors)
{
	OUTINFO_0_PARAM("seteffect\n\r");
	OUTINFO_1_PARAM("effectId %d\n\r", effectId);	
	HRESULT hr = S_OK;
	if (EnterMutex() == 1)
		return S_FALSE;

	DeviceEffect* effect = GetEffectInfo(effectId);
	if (effect == nullptr)
	{
		OUTINFO_0_PARAM("error nullptr\n\r");
		hr = E_FAIL;
		return hr;
	}
	int i2c_count = 0;
	//clear led light sync
	OUTINFO_1_PARAM("i2c_mem_slot_cnt %d\n\r", i2c_mem_slot_cnt);
	if (i2c_mem_slot_cnt > 1)
	{
		for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				//clear firts memslot, mr62 clear
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR62, 0x0) == 0xff)
				{
					dbg_printf("false\n\r");
					
				}
				i2c_count = mem_slot + 1;
				break;
			}
		}
	}
#if 0
	unsigned char buf[33] = { 0 };
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = 0x04;
	int p = 3;
	for (int z = 0; z < 4; z++)
	{
		buf[p] = colors[z] & 0xff;
		p++;
		buf[p] = (colors[z] >> 8) & 0xff;
		p++;
		buf[p] = (colors[z] >> 16) & 0xff;
		p++;
	}
#endif
	if ((effectId == 0) || (effectId == 1))
	{		
		i2c_count = 0;
		for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{

#if 0
				//sync mode
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR37, 0x01) == 0xff)
				{
					dbg_printf("false\n\r");
				}
				block_write(smbus_address, ddr_i2c_address + i2c_count, 0x2f, 15, buf);
#endif

				//led pixels
#if 1
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR37, 0x01) == 0xff)
				{
					dbg_printf("false\n\r");
				}
			    //if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR40, 0x00) == 0xff)
				//{
					//dbg_printf("false\n\r");
				//}
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR39, MY_LED_COUNT) == 0xff)
				{
					dbg_printf("false\n\r");
				}
#endif
				//dbg_printf("led numbers:0x%x\n\r", numberOfColors);
				//OUTINFO_1_PARAM("led numbers:0x%x\n\r", numberOfColors);
				int j = 0;
				for (int i = 0; i < (numberOfColors); i = i + 1)
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR41, (i * 3) + 0) != 0x0)
					{
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}
#if 0
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR42, 0x0) != 0x0)
					{
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}
#endif
					//r
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR43, colors[j] & 0xff) != 0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}


					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR41, (i * 3) + 1) != 0x0)
					{
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}
#if 0
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR42, 0x0) != 0)
					{
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}
#endif
					//g
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR43, (colors[j] >> 8) & 0xff) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}

					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR41, (i * 3) + 2) != 0x0)
					{
						OUTINFO_0_PARAM("l error %d\n\r", i);
					}
#if 0
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR42, 0x0) != 0x0)
					{
						OUTINFO_0_PARAM("h error %d\n\r", i);
					}
#endif
					//b
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR43, (colors[j] >> 16) & 0xff) != 0x0)
					{
						OUTINFO_0_PARAM("color error %d\n\r", i);
					}
					j++;
					
				}
				//delay1us(500);
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR36, 0x01) == 0xff)
				{
					dbg_printf("false\n\r");
				}

			}
		}

		LeaveMutex();
		return hr;
	}

	if ((effectId != 0) && (effectId != 1))
	{
		SetOff_internal();
		OUTINFO_0_PARAM("seteffect 2-5 \n\r");
		int i2c_count = 0;
		for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
		{
			if (i2c_addrs[i2c_count] != 0)
			{
				printf("write i2c addrss 0x%x", ddr_i2c_address + i2c_count);
				//build in mode
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, 0x25, 0x02) == 0xff)
				{
					dbg_printf("false\n\r");
				}

				//led pixels
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, 0x28, 0x00) == 0xff)
				{
					dbg_printf("false\n\r");
				}

				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, 0x27, MY_LED_COUNT) == 0xff)
				{
					dbg_printf("false\n\r");
				}
				//function
				if (effectId == 2)//FUNC_Breathing
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR38, 0x2) == 0xff)
					{
						dbg_printf("false\n\r");
					}
				}
				if (effectId == 3)//FUNC_Strobe
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR38, 0x3) == 0xff)
					{
						dbg_printf("false\n\r");
					}
				}
				if (effectId == 4)//FUNC_Cycling
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR38, 0x4) == 0xff)
					{
						dbg_printf("false\n\r");
					}
				}
#if 0
				if (effectId == 5)//FUNC_Random
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
#endif
				if (effectId == 5)//FUNC_Rainbow
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR38, 14) == 0xff)
					{
						printf("false\n\r");
					}
				}
				//r
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR44, (colors[0] & 0xff)) == 0xff)
				{
					printf("false\n\r");
				}

				//g
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR45, (colors[0] >> 8) & 0xff) == 0xff)
				{
					printf("false\n\r");
				}

				//b
				if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR46, (colors[0] >> 16) & 0xff) == 0xff)
				{
					printf("false\n\r");
				}

			}
		}
		
			for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
			{
				if (i2c_addrs[i2c_count] != 0)
				{
					if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, 0x24, 0x01) == 0xff)
					{
						printf("false\n\r");
					}
				}
			}
			if (i2c_mem_slot_cnt > 1)
			{
				for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
				{
					if (i2c_addrs[i2c_count] != 0)
					{
						//set mr62
						if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR62, 0x1) == 0xff)
						{
							dbg_printf("false\n\r");
						
						}
						i2c_count = mem_slot + 1;
						break;
					}
				}
			}
	LeaveMutex();
	return hr;
	}


	//ULONG speed = 0;
	//ULONG direction = 0;
	//speed = effect->DefaultSpeedLevel;
	//direction = effect->DefaultDirectionLevel;

	//return DoSetEffectOptSpeed(effectId, colors, numberOfColors, speed, direction);

	LeaveMutex();
	return hr;
}


HRESULT MyAacLedDevice::SetEffectOptSpeed(ULONG effectId, ULONG *colors, ULONG numberOfColors, ULONG speed, ULONG direction)
{
	dbg_printf("SetEffectOptSpeed\n\r");
	dbg_printf("%d\n\r", effectId);
		OUTINFO_0_PARAM("SetEffectOptSpeed\n\r");
		OUTINFO_1_PARAM("effectId\n\r", effectId);
		OUTINFO_1_PARAM("numberOfColors\n\r", numberOfColors);
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




void wmi_check_dram_com(void) {
	HRESULT hres;

	// Connect to WMI through the IWbemLocator interface
	IWbemLocator* pLoc = nullptr;
	hres = CoCreateInstance(
		CLSID_WbemLocator,
		0,
		CLSCTX_INPROC_SERVER,
		IID_IWbemLocator,
		(LPVOID*)&pLoc
	);

	if (FAILED(hres)) {
		//wprintf(L"Failed to create IWbemLocator object. Error code = 0x%x\n", hres);
		OUTINFO_1_PARAM("Failed to create IWbemLocator object. Error code = 0x%x\n", hres);
		CoUninitialize();
		return ;
	}

	IWbemServices* pSvc = nullptr;
	hres = pLoc->ConnectServer(
		_bstr_t(L"ROOT\\CIMV2"),
		NULL,
		NULL,
		0,
		NULL,
		0,
		0,
		&pSvc
	);

	if (FAILED(hres)) {
	//	wprintf(L"Could not connect. Error code = 0x%x\n", hres);
		OUTINFO_1_PARAM("Could not connect. Error code = 0x%x\n", hres);
		pLoc->Release();
		CoUninitialize();
		return ;
	}

	// Set security levels on the proxy
	hres = CoSetProxyBlanket(
		pSvc,
		RPC_C_AUTHN_WINNT,
		RPC_C_AUTHZ_NONE,
		NULL,
		RPC_C_AUTHN_LEVEL_CALL,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL,
		EOAC_NONE
	);

	if (FAILED(hres)) {
		//wprintf(L"Could not set proxy blanket. Error code = 0x%x\n", hres);
		OUTINFO_1_PARAM("Could not set proxy blanket. Error code = 0x%x\n", hres);
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return ;
	}

	// Use the IWbemServices pointer to make requests of WMI
	IEnumWbemClassObject* pEnumerator = nullptr;
	hres = pSvc->ExecQuery(
		bstr_t("WQL"),
		bstr_t("SELECT Manufacturer, PartNumber, DeviceLocator FROM Win32_PhysicalMemory"),
		WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
		NULL,
		&pEnumerator
	);

	if (FAILED(hres)) {
		//wprintf(L"Query for physical memory failed. Error code = 0x%x\n", hres);
		OUTINFO_1_PARAM("Query for physical memory failed. Error code = 0x%x\n", hres);
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return ;
	}

	IWbemClassObject* pclsObj = nullptr;
	ULONG uReturn = 0;
	
	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

		if (0 == uReturn) {
			break;
		}

		VARIANT vtManufacturer, vtPartNumber, vtSlot;
		hr = pclsObj->Get(L"Manufacturer", 0, &vtManufacturer, 0, 0);
		hr = pclsObj->Get(L"PartNumber", 0, &vtPartNumber, 0, 0);
		hr = pclsObj->Get(L"DeviceLocator", 0, &vtSlot, 0, 0);
#if 0
		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			wprintf(L"Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
		}
#endif

		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			if (wcsstr(vtManufacturer.bstrVal, targetManufacturer) != nullptr && wcsstr(vtPartNumber.bstrVal, targetPartNumber) != nullptr) {
				//wprintf(L"Partial match found! Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
				wmi_addrs[0] = 1;
				wmi_addrs[1] = 1;
				wmi_addrs[2] = 1;
				wmi_addrs[3] = 1;
				OUTINFO_0_PARAM("wdm detect\n\r");
			}
		}


#if 0
		const wchar_t* targetslot0 = L"0-DIMM0";
		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			if (wcsstr(vtManufacturer.bstrVal, targetManufacturer) != nullptr && wcsstr(vtPartNumber.bstrVal, targetPartNumber) != nullptr&&
				wcsstr(vtSlot.bstrVal, targetslot0) != nullptr) {
				//wprintf(L"Partial match found! Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
				wmi_addrs[0] = 1;
				OUTINFO_0_PARAM(L"wdm 0\n\r");
			}
		}

		const wchar_t* targetslot1 = L"0-DIMM1";
		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			if (wcsstr(vtManufacturer.bstrVal, targetManufacturer) != nullptr && wcsstr(vtPartNumber.bstrVal, targetPartNumber) != nullptr&&
				wcsstr(vtSlot.bstrVal, targetslot1) != nullptr) {
				//wprintf(L"Partial match found! Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
				wmi_addrs[1] = 1;
				OUTINFO_0_PARAM("wdm 1\n\r");
			}
		}
		const wchar_t* targetslot2 = L"1-DIMM0";
		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			if (wcsstr(vtManufacturer.bstrVal, targetManufacturer) != nullptr && wcsstr(vtPartNumber.bstrVal, targetPartNumber) != nullptr&&
				wcsstr(vtSlot.bstrVal, targetslot2) != nullptr) {
				//wprintf(L"Partial match found! Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
				wmi_addrs[2] = 1;
				OUTINFO_0_PARAM("wdm 2\n\r");
			}
		}
		const wchar_t* targetslot3 = L"1-DIMM1";
		if (SUCCEEDED(hr) && vtManufacturer.vt == VT_BSTR && vtPartNumber.vt == VT_BSTR && vtSlot.vt == VT_BSTR) {
			if (wcsstr(vtManufacturer.bstrVal, targetManufacturer) != nullptr && wcsstr(vtPartNumber.bstrVal, targetPartNumber) != nullptr&&
				wcsstr(vtSlot.bstrVal, targetslot3) != nullptr) {
				//wprintf(L"Partial match found! Manufacturer: %s, Part Number: %s, Slot: %s\n", vtManufacturer.bstrVal, vtPartNumber.bstrVal, vtSlot.bstrVal);
				wmi_addrs[3] = 1;
				OUTINFO_0_PARAM("wdm 3\n\r");
			}
		}
		
#endif	

		VariantClear(&vtManufacturer);
		VariantClear(&vtPartNumber);
		VariantClear(&vtSlot);

		pclsObj->Release();
	}

	pSvc->Release();
	pLoc->Release();
	CoUninitialize();	
}



HRESULT MyAacLedDevice::Init(DeviceLightControl* deviceControl, int index)
{
	SC_HANDLE schSCManager;
	m_deviceControl = deviceControl;
	m_index = index;
	//init i2c mem decect array
	for (int i = 0; i < mem_slot; i++)
	{
		i2c_addrs[i] = 0;
		wmi_addrs[i] = 0;
	}
	wmi_check_dram_com();
	if ((wmi_addrs[0] == 0) && (wmi_addrs[1] == 0) && (wmi_addrs[2] == 0) && (wmi_addrs[3] == 0))
	{
		return S_FALSE;
	}

	//CreateDeviceCapability();
#if 1
	dbg_printf("initial	wmi\n\r");
	OUTINFO_0_PARAM("initial wim\n\r");
	DWORD errNum = 0;
	UCHAR  driverLocation[MAX_PATH];

	//if (schSCManager == NULL)
	//{
		dbg_printf("SCH IS NULL\n\r");
		OUTINFO_0_PARAM("SCH IS NULL\n\r");
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
	//}
	 	
	i2c_mem_slot_cnt = 0;
	//unsigned int smbus_address = 0;
	SmbCtrl_Get_BaseAddress_Intel(&smbus_address);
	if (EnterMutex() == 1)
		return S_FALSE;
	int loccunt = 0;
	for (loccunt = 0; loccunt < mem_slot; loccunt++)
	{		
		if (wmi_addrs[loccunt] == 1)
		{
			unsigned char nuvoton_id = 0xff;
			unsigned char xor = 0xff;
			read_slave_data(smbus_address, (ddr_i2c_address + loccunt), 0x03, &nuvoton_id); //check ddr5 id
			read_slave_data(smbus_address, (ddr_i2c_address + loccunt), 0x03, &nuvoton_id); //check ddr5 id
			OUTINFO_1_PARAM("ddr_i2c_address + loccunt : 0x%x\n", ddr_i2c_address + loccunt);
			OUTINFO_1_PARAM("nuvoton id : 0x%x\n", nuvoton_id);
			write_slave_data(smbus_address, ddr_i2c_address + loccunt, 0x05, 0x5a);
			read_slave_data(smbus_address, ddr_i2c_address + loccunt, 0x05, &xor); //check ddr5 id
			if ((nuvoton_id == 0xda) && (xor == 0xa5))
			{
				i2c_addrs[loccunt] = 1; //devices detect
				i2c_mem_slot_cnt = i2c_mem_slot_cnt + 1;
				OUTINFO_1_PARAM("capture i2c address: 0x%x\n", ddr_i2c_address + loccunt);
			}
			else {
				i2c_addrs[loccunt] = 0;
			}
		}
	}

	if ((i2c_addrs[0] == 0) && (i2c_addrs[1] == 0) && (i2c_addrs[2] == 0) && (i2c_addrs[3] == 0))
	{
		CloseHandle(hDevice);

		StopDriver(schSCManager,
			DRIVER_NAME
		);
		dbg_printf("stop driver\n\r");

		RemoveDriver(schSCManager,
			DRIVER_NAME
		);
		dbg_printf("remove driver\n\r");

		CloseServiceHandle(schSCManager);
		dbg_printf("close Service\n\r");
		return S_FALSE;
	}
#endif
	LeaveMutex();
	return S_OK;
}

void MyAacLedDevice::LeaveMutex(void)
{
	ReleaseMutex(m_hHalMutex);
}

int MyAacLedDevice::EnterMutex(void)
{
	DWORD result;
	// result = WaitForSingleObject(m_hMutex, 1000);
	result = WaitForSingleObject(m_hHalMutex, MUTEX_WAITTINGTIME);

	if (result == WAIT_OBJECT_0)
	{
		return 0;//pass 
	}
	else
	{
		return 1; //false
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


void  MyAacLedDevice::EnterSmbusMutex(void)
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
	dbg_printf("DoSetEffectOptSpeed\n\r");
	dbg_printf("effectId %d\n\r", effectId);
	dbg_printf("%d\n\r", numberOfColors);
	dbg_printf("%d\n\r", speed);
	dbg_printf("%d\n\r", direction);
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
int SetOff_internal(void)
{


	int i2c_count = 0;

	i2c_count = 0;
	for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
	{
		if (i2c_addrs[i2c_count] != 0)
		{
			dbg_printf("write i2c addrss 0x%x", ddr_i2c_address + i2c_count);
			//build in mode
			if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR37, 0x02) == 0xff)
			{
				dbg_printf("false\n\r");
			}

			//led pixels
			if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR40, 0x00) == 0xff)
			{
				dbg_printf("false\n\r");
			}

			if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR39, MY_LED_COUNT) == 0xff)
			{
				dbg_printf("false\n\r");
			}
			//function
			//all led off
			if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR38, 0) == 0xff)
			{
				dbg_printf("false\n\r");
			}

		}
	}
	for (i2c_count = 0; i2c_count < mem_slot; i2c_count++)
	{

		if (i2c_addrs[i2c_count] != 0)
		{
			if (write_slave_data(smbus_address, ddr_i2c_address + i2c_count, MR36, 0x01) == 0xff)
			{
				dbg_printf("false\n\r");
			}
		}

	}
	return 0;
}