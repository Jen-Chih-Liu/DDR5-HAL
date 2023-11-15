#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__


//Ex. 0x726897aa, 0xec1a, 0x4f9e, {0x81, 0xf6, 0xb3, 0x5b, 0x9a, 0x26, 0xc1, 0xe}
#define MY_HAL_CLSID  {0x49bdad71, 0x2ce2, 0x450d, { 0xbc, 0xbc, 0x90, 0x44, 0x19, 0xc0, 0x7f, 0x5b }}

                      

//Ex. ASUS_Display
#define MY_HAL_FRIEND_NAME  L"MyFriendlyName"                  

//Ex. ASUS_Display.HAL
#define MY_HAL_VER_ID       L"MyComponentName.Hal"			   

//Ex. ASUS_Display.HAL.1	
#define MY_HAL_PROG_ID		L"MyComponentName.Hal.1"		   


// Ex. L"Global\\ASUS_HAL_MUTEX"
#define MY_MUTEXT_NAME		L"Global\\Access_SMBUS.HTP.Method"

#define MUTEX_WAITTINGTIME 3000




#endif //__HAL_CONFIG_H__