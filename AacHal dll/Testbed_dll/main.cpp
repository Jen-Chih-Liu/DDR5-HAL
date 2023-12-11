#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include <time.h>


typedef  int(*pIAP_Init)(void);
typedef  int(*pIAP_Exit)(void);
typedef  void(*pIAP_ReadVersion)(void);
typedef  void(*pIAP_ReadBoot)(void);
typedef  void(*pIAP_ErasePage2KB)(unsigned int address);
//typedef  void(*pIAP_WriteWord)(unsigned int address, unsigned int flashdata);
typedef  void(*pIAP_Write_16Byte)(unsigned int address, unsigned char* flashdata_array);
//typedef  void(*pIAP_ReadWord)(unsigned int address, unsigned int *r_data);
typedef  void(*pIAP_Read_16Byte)(unsigned int address, unsigned char* r_data);



typedef  int(*pSetEffect)(ULONG effectId, ULONG *colors, ULONG numberOfColors);
typedef  int(*pInit)(void);
typedef  int(*pExit)(void);
typedef  int(*pInit)(void);
typedef  int(*pSetOff)(void);
typedef  int(*pSetEffect_RGB_SP)(ULONG effectId, int R, int G, int B, int SP);
typedef  int(*pSetEffect_block)(ULONG effectId, ULONG *colors, ULONG numberOfColors);
typedef  unsigned char(*pext_read_slave_data)(unsigned char slave_address, unsigned char offset, unsigned char *dest);
typedef  unsigned char(*pext_write_slave_data)(unsigned char slave_address, unsigned char offset, unsigned char wdata);
typedef  unsigned char(*pcheck_boot_ap)(void);
ULONG color[15];

/*
//function
effectId = 0//FUNC_STATIC
effectId = 1//FUNC_Breathing
effectId = 2//FUNC_Strobe
effectId = 3//FUNC_Cycling
effectId = 4//FUNC_Random
effectId = 5//FUNC_Music 
effectId = 6//FUNC_Wave
effectId = 7//FUNC_Spring
effectId = 8//FUNC_Water
effectId = 9//FUNC_Rainbow
*/
void WordsCpy(void *dest, void *src, unsigned int size)
{
	unsigned char *pu8Src, *pu8Dest;
	unsigned int i;

	pu8Dest = (unsigned char *)dest;
	pu8Src = (unsigned char *)src;

	for (i = 0; i < size; i++)
		pu8Dest[i] = pu8Src[i];
}
#define aprom_size 64*1024
void main()
{

	char cmd;
	clock_t start_time, end_time;
	
	HINSTANCE hDll = LoadLibrary(L"AacHal_x64.dll");
	if (!hDll)
	{
		return;
	}
	pIAP_Init fIAP_Init = (pIAP_Init)GetProcAddress(hDll, "IAP_Init");
	pIAP_Exit fIAP_Exit = (pIAP_Exit)GetProcAddress(hDll, "IAP_Exit");
	pIAP_ReadVersion fIAP_ReadVersion = (pIAP_ReadVersion)GetProcAddress(hDll, "IAP_ReadVersion");
	pIAP_ReadBoot fIAP_ReadBoot = (pIAP_ReadBoot)GetProcAddress(hDll, "IAP_ReadBoot");
	pIAP_ErasePage2KB fIAP_ErasePage2KB = (pIAP_ErasePage2KB)GetProcAddress(hDll, "IAP_ErasePage2KB");
	//pIAP_WriteWord fIAP_WriteWord = (pIAP_WriteWord)GetProcAddress(hDll, "IAP_WriteWord");
	//pIAP_ReadWord fIAP_ReadWord = (pIAP_ReadWord)GetProcAddress(hDll, "IAP_ReadWord");
	pIAP_Read_16Byte fIAP_Read_16Byte = (pIAP_Read_16Byte)GetProcAddress(hDll, "IAP_Read_16Byte");
	pIAP_Write_16Byte fIAP_Write_16Byte = (pIAP_Write_16Byte)GetProcAddress(hDll, "IAP_Write_16Byte");

	pSetEffect fSetEffect = (pSetEffect)GetProcAddress(hDll, "SetEffect");
	pSetEffect_block fSetEffect_block = (pSetEffect_block)GetProcAddress(hDll, "SetEffect_block");
	pInit fInit = (pInit)GetProcAddress(hDll, "Init");

	pExit fExit = (pExit)GetProcAddress(hDll, "Exit");
	pSetOff fSetOff = (pSetOff)GetProcAddress(hDll, "SetOff");
	pSetEffect_RGB_SP fSetEffect_RGB_SP = (pSetEffect_RGB_SP)GetProcAddress(hDll, "SetEffect_RGB_SP");
	pext_read_slave_data fext_read_slave_data = (pext_read_slave_data)GetProcAddress(hDll, "ext_read_slave_data");
	pext_write_slave_data fext_write_slave_data = (pext_write_slave_data)GetProcAddress(hDll, "ext_write_slave_data");
	pcheck_boot_ap fcheck_boot_ap = (pcheck_boot_ap)GetProcAddress(hDll, "check_boot_ap");
#if 0
	if (!fIAP_Init)
	{
		printf("error \n\r");
	}

	else
	{
		if (fIAP_Init() == 1)
			printf("intial false... \n\r");
			return;
	}
#endif
	// Get the current time and date
	time_t buildTime;
	time(&buildTime);

	// Convert build time to a string
	char buildTimeStr[100];
	strftime(buildTimeStr, sizeof(buildTimeStr), "%Y-%m-%d %H:%M:%S", localtime(&buildTime));

	// Print build time and date
	printf("Build Time: %s\n", buildTimeStr);
	printf("Build Date: %s\n", __DATE__);
	
	fInit();
	if (fcheck_boot_ap() == 0xff)
	{
		printf("check boot ldrom (run ""read ldrom verson"") or devices no found\n\r");
	}
	while (1)
	{
		printf("\n\r");
		printf("e, exit\n\r");
		printf("..............flash test command...........\n\r");
		printf("a, flash auto test\n\r");
		printf("p, page erase flash\n\r");
		printf("v, read ldrom verson\n\r");
		printf("b, read boot\n\r");
		printf("r, read flash\n\r");
		printf("w, write flash\n\r");		
		printf("..............i2c test command...........\n\r");
		printf("g, write i2c memory\n\r");
		printf("h, read i2c memory\n\r");
		printf("..............light effect command...........\n\r");
		printf("0, led off \n\r");
		printf("1, (red)   breathing\n\r");
		printf("2, (green) strobe\n\r");
		printf("3, (blue)  cycling\n\r");
		printf("4, random\n\r");
		printf("5, wave\n\r");
		printf("6, spring\n\r");
		printf("7, breathing rainbow\n\r");
		printf("8, strobe rainbow\n\r");
		printf("9, water\n\r");
		printf("A, rainbow\n\r");
		printf("B, double strobe\n\r");
		printf("\n\r");
		cmd = getchar();

		if (cmd == 'g')
		{			
			getchar();
			char v[32];
			unsigned char i2c_addr;
			printf("i2c write, please input i2c address, input HEX or DEC\n\r");
			fgets(v, sizeof(v), stdin);

			if (strncmp(v, "0x", 2) == 0) {
				i2c_addr = strtol(v, NULL, 16);	 //hex input			
			}
	
			else {
				i2c_addr = atoi(v);				//dec input
			}

			printf("i2c address=0x%x\n\r", i2c_addr);

			printf("please input i2c offset, input HEX or DEC\n\r");
			unsigned char offset;
			
			fgets(v, sizeof(v), stdin);
			if (strncmp(v, "0x", 2) == 0) {
				offset = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				offset = atoi(v);				//dec input
			}
			printf("i2c offset=0x%x\n\r", offset);

			printf("please input i2c data, input HEX or DEC\n\r");
			unsigned char data;
			fgets(v, sizeof(v), stdin);
			if (strncmp(v, "0x", 2) == 0) {
				data = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				data = atoi(v);				//dec input
			}
			printf("i2c data=0x%x\n\r", data);
			fext_write_slave_data(i2c_addr, offset, data);

		}
		if (cmd == 'h')
		{		
			getchar();
			char v[32];
			printf("i2c read, please input i2c address, input HEX or DEC\n\r");
			fgets(v, sizeof(v), stdin);
			unsigned char i2c_addr;
			
			if (strncmp(v, "0x", 2) == 0) {
				i2c_addr = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				i2c_addr = atoi(v);				//dec input
			}
			printf("i2c address=0x%x\n\r", i2c_addr);

			printf("please input i2c offset, input HEX or DEC\n\r");
			unsigned char offset;
			fgets(v, sizeof(v), stdin);
			if (strncmp(v, "0x", 2) == 0) {
				offset = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				offset = atoi(v);				//dec input
			}
			printf("i2c off=0x%x\n\r", offset);

			unsigned char rdata;
			fext_read_slave_data(i2c_addr, offset, &rdata);
			printf("return data:0x%x\n\r", rdata);

		}

			if (cmd == '0')
			{
				fSetOff();
				printf("select 0, LED off\n\r");
			}
			if (cmd == '1')
			{				
				fSetEffect_RGB_SP(1, 255, 0, 0, 50);
				printf("select1, breathing\n\r");
			}
			if (cmd == '2')
			{
				fSetEffect_RGB_SP(2, 0, 255, 0, 50);
				printf("select 2, (green) strobe\n\r");
			}
			if (cmd == '3')
			{
				fSetEffect_RGB_SP(3, 0, 0, 255, 50);
				printf("select 3,  (blue)  cyclin\n\r");
			}
			if (cmd == '4')
			{
				fSetEffect_RGB_SP(4, 0, 0, 255, 50);
				printf("select 4, random\n\r");
			}
			if (cmd == '5')
			{
				
				fSetEffect_RGB_SP(5, 0, 0, 255, 50);
				printf("select 5, wave\n\r");
			}

			if (cmd == '6')
			{
			
				fSetEffect_RGB_SP(6, 255, 0, 0, 50);
				printf("select 6, spring\n\r");
			}
			if (cmd == '7')
			{
				
				fSetEffect_RGB_SP(7, 255, 0, 0, 50);
				printf("select 7, breathing rainbow\n\r");
			}
			if (cmd == '8')
			{				
				fSetEffect_RGB_SP(8, 255, 0, 0, 50);
				printf("select 8, strobe rainbow\n\r");
			}
			if (cmd == '9')
			{
				fSetEffect_RGB_SP(9, 255, 0, 0, 50);
				printf("select 9, water\n\r");
			}
			if (cmd == 'A')
			{
				fSetEffect_RGB_SP(10, 255, 0, 0, 50);
				printf("select A, rainbow\n\r");
			}
			if (cmd == 'B')
			{
				fSetEffect_RGB_SP(11, 255, 0, 0, 50);
				printf("select B, dobule strobe\n\r");
			}
		if (cmd == 'e')
		{
			printf("exit shell\n\r");
			break;
		}
		if (cmd == 'v')
		{
			getchar();
			printf("IAP_ReadVersion\n\r");
			fIAP_ReadVersion();
		}
		if (cmd == 'b')
		{
			getchar();
			printf("IAP_ReadBoot\n\r");
			fIAP_ReadBoot();
		}
		if (cmd == 'p')
		{
			getchar();
			char v[32];
			printf("flash page erase test, please input page address, input HEX or DEC\n\r");
			fgets(v, sizeof(v), stdin);
			unsigned int flash_addr;
			if (strncmp(v, "0x", 2) == 0) {
				flash_addr = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				flash_addr = atoi(v);				//dec input
			}
			printf("flash_addr=0x%x\n\r", flash_addr);

			fIAP_ErasePage2KB(flash_addr);
			Sleep(80);
		}
		if (cmd == 'r')
		{
			getchar();
			char v[32];
			printf("flash read, please input page address, input HEX or DEC\n\r");
			fgets(v, sizeof(v), stdin);
			unsigned int flash_addr;

			if (strncmp(v, "0x", 2) == 0) {
				flash_addr = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				flash_addr = atoi(v);				//dec input
			}
			printf("flash_addr=0x%x\n\r", flash_addr);
			unsigned char __declspec(align(32)) read_temp[16] = {0};
			fIAP_Read_16Byte(flash_addr, read_temp);
			printf("flash context: ");
			for (int i = 0; i < 16; i++)
			{
				printf("0x%x ", read_temp[i]);
			}
		}
		if (cmd == 'w')
		{
			getchar();
			char v[32];
			printf("flash write, please input page address, input HEX or DEC\n\r");
			fgets(v, sizeof(v), stdin);
			unsigned int flash_addr;
			if (strncmp(v, "0x", 2) == 0) {
				flash_addr = strtol(v, NULL, 16);	 //hex input			
			}

			else {
				flash_addr = atoi(v);				//dec input
			}

			printf("flash address:0x%x\n\r", flash_addr);
			printf("please input 16 byte data, input HEX or DEC\n\r");
			unsigned char __declspec(align(32)) temp[16];

			for (int i = 0; i < 16; i++)
			{
				fgets(v, sizeof(v), stdin);
				if (strncmp(v, "0x", 2) == 0) {
					temp[i] = strtol(v, NULL, 16);	 //hex input			
				}

				else {
					temp[i] = atoi(v);				//dec input
				}
				
			}
			for (int j = 0; j < 16; j++) {
				printf("0x%x ", temp[j]);
			}
			fIAP_Write_16Byte(flash_addr, temp);
			printf("\n\r");
		}


		if (cmd == 'a')
		{
			getchar();
			unsigned char flash_i2c_address;
			flash_i2c_address = fcheck_boot_ap();
			printf("flash_i2c_address 0x%x\n\r");
			if (flash_i2c_address != 0xff)
			{
				printf("boot in aprom, will boot to ldrom \n\r");
				fext_write_slave_data(flash_i2c_address, 8, 0x4c);
				fext_write_slave_data(flash_i2c_address, 8, 0x4a);
		    }
			Sleep(2);
			fIAP_ReadVersion(); //the verion will show 0xb0
			fIAP_ReadVersion();
			flash_i2c_address = fcheck_boot_ap();
			printf("flash_i2c_address 0x%x\n\r");
			if (flash_i2c_address == 0xff)
			{
				printf("Can not found nuvoton devices\n\r");
				goto ExitA;
			}
		    fIAP_ReadBoot();
#if 1
			printf("DDR5_APROM.bin IAP test, erase, programming ,verify\n\r");

			FILE *fp;
			unsigned int file_size = 0;
			//unsigned char FwFileData[128 * 1024];
			unsigned char flash_array[aprom_size] = {0x00};
			for (unsigned int i = 0; i < aprom_size; i++)
			{
				flash_array[i] = 0xff;
			}
			if ((fp = fopen(".\\DDR5_APROM.bin", "rb")) == NULL)
			{
				printf("APROM FILE OPEN FALSE\n\r");
				goto EXIT_Flashtest;
			}
			if (fp != NULL)
			{


				while (!feof(fp)) {
					fread(&flash_array[file_size], sizeof(char), 1, fp);
					file_size++;
				}
			}
		
			UINT32 file_totallen = file_size - 1;

			printf("APROM file size=%d \n\r", file_totallen);

			fclose(fp);
	
			

			float total_time = 0;
			start_time = clock(); /* mircosecond */


			for (unsigned int i = 0; i < aprom_size; i = i + 2048)
			{
				printf("erase address 2k:0x%x\n\r", i);
				fIAP_ErasePage2KB(i);
				Sleep(80);
			}
#if 0
			for (unsigned int i = 0; i < aprom_size; i++)
			{
				flash_array[i] = 0xff;
			}
			for (unsigned int i = 0; i < aprom_size; i = i + 16)
			{
				printf("erase verify address:0x%x \n\r", i);
				unsigned char __declspec(align(32)) read_temp[16];
				unsigned char __declspec(align(32)) temp[16];
				WordsCpy(&temp, &flash_array[i], 16);
				fIAP_Read_16Byte(i, read_temp);
				for (unsigned int j = 0; j < 16; j++)
				{
					if (temp[j] != read_temp[j])
					{

						printf("false");
						break;
					}
				}

			}


			for (unsigned int i = 0; i < aprom_size; i++)
			{
				flash_array[i] = i & 0xff;
			}
#endif
			for (unsigned int i = 0; i < aprom_size; i = i + 16)
			{
				printf("programming address:0x%x \n\r", i);
				unsigned char __declspec(align(32)) temp[16];
				WordsCpy(&temp, &flash_array[i], 16);
				fIAP_Write_16Byte(i, temp);
			}

			for (unsigned int i = 0; i < aprom_size; i = i + 16)
			{
				printf("verify address:0x%x \n\r", i);
				unsigned char __declspec(align(32)) read_temp[16];
				unsigned char __declspec(align(32)) temp[16];
				WordsCpy(&temp, &flash_array[i], 16);
				fIAP_Read_16Byte(i, read_temp);
				for (unsigned int j = 0; j < 16; j++)
				{
					if (temp[j] != read_temp[j])
					{

						printf("false");
						break;
					}
				}

			}
			end_time = clock();
			/* CLOCKS_PER_SEC is defined at time.h */
			total_time = (float)(end_time - start_time) / CLOCKS_PER_SEC;

			printf("Time : %f sec \n", total_time);
		    EXIT_Flashtest:
			printf("\n\r");
			//boot to aprom
			fext_write_slave_data(flash_i2c_address, 0xe2, 1); //ldrom jumper to aprom
			Sleep(2);
			unsigned char nuvoton_id = 0xff;
			unsigned char xor = 0xff;
			fext_read_slave_data(flash_i2c_address, 0x03, &nuvoton_id); //check ddr5 id
			fext_read_slave_data(flash_i2c_address, 0x03, &nuvoton_id); //check ddr5 id
			
			printf("nuvoton id : 0x%x\n", nuvoton_id);
			fext_write_slave_data(flash_i2c_address, 0x05, 0x5a);
			fext_read_slave_data(flash_i2c_address, 0x03, &xor);//check ddr5 id
			if ((nuvoton_id == 0xda) && (xor == 0xa5))
			{
				printf("boot aprom pass");
			}
			else
			{
				printf("boot aprom false");
			}
#endif     

		ExitA:
			printf("\n\r");
		}
	}
	fExit();
	FreeLibrary(hDll);
	
	//the light test
#if 0
	char cmd;
	HINSTANCE hDll = LoadLibrary(L"AacHal_x64.dll");
	if (!hDll)
	{
		return;
	}
	pSetEffect fSetEffect = (pSetEffect)GetProcAddress(hDll, "SetEffect");
	pSetEffect_block fSetEffect_block = (pSetEffect_block)GetProcAddress(hDll, "SetEffect_block");
	pInit fInit = (pInit)GetProcAddress(hDll, "Init");

	pExit fExit = (pExit)GetProcAddress(hDll, "Exit");
	pSetOff fSetOff = (pSetOff)GetProcAddress(hDll, "SetOff");
	pSetEffect_RGB_SP fSetEffect_RGB_SP = (pSetEffect_RGB_SP)GetProcAddress(hDll, "SetEffect_RGB_SP");
	pext_read_slave_data fext_read_slave_data = (pext_read_slave_data)GetProcAddress(hDll, "ext_read_slave_data");
	pext_write_slave_data fext_write_slave_data = (pext_write_slave_data)GetProcAddress(hDll, "ext_write_slave_data");
	if (!fInit)
	{
		printf("error \n\r");
	}
	else
	{
		if (fInit() == 1)
			return;
	}

	if (!fSetOff)
	{
		printf("fSetOff error \n\r");
	}

	if (!fSetEffect_RGB_SP)
	{
		printf("fSetEffect_RGB_SP error \n\r");
	}
	if (!fSetEffect_block)
	{
		printf("fSetEffect_block error \n\r");
	}
	if (!fSetEffect)
	{
		printf("error \n\r");
	}
		while (1)
		{
			printf("\n\r");
			printf("e, exit\n\r");
			printf("a, auto test light mode\n\r");
			printf("w, write i2c memory\n\r");
			printf("r, read i2c memory\n\r");
			printf("\n\r");
			cmd = getchar();
			if (cmd == 'e')
			{
				printf("exit shell\n\r");
				break;
			}
			if (cmd == 'w')
			{
				printf("i2c write, please input i2c address\n\r");
				unsigned char i2c_addr;
				scanf("%d", &i2c_addr);
				printf("please input i2c offset\n\r");
				unsigned char offset;
				scanf("%d", &offset);
				printf("please input i2c data\n\r");
				unsigned char data;
				scanf("%d", &data);
				fext_write_slave_data(i2c_addr, offset, data);
		
			}
			if (cmd == 'r')
			{
				printf("i2c read, please input i2c address\n\r");
				unsigned char i2c_addr;
				scanf("%d", &i2c_addr);
				printf("please input i2c offset\n\r");
				unsigned char offset;
				scanf("%d", &offset);				
				unsigned char rdata;
				fext_read_slave_data(i2c_addr, offset,&rdata);
				printf("return data:%d\n\r", rdata);
			
			}

			if (cmd == 'a')
			{
		    printf("auto test light mode\n\r");					
			fSetEffect_block(0, &color[0], 5);
			printf("test block write\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect(1, &color[0], 5);
			printf("test effect 1\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetOff();
			printf("test led all off\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(0, 255, 255, 255, 50);
			printf("test rgb all\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");

			fSetEffect_RGB_SP(1, 255, 0, 0, 50);
			printf("test r led and effect 1\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(1, 0, 255, 0, 10);
			printf("test g led and effect 1\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(1, 0, 0, 255, 100);
			printf("test b led and effect 1\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(2, 255, 0, 0, 50);
			printf("test b led and effect 2\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(3, 0, 255, 0, 10);
			printf("test g led and effect 3\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");
			fSetEffect_RGB_SP(4, 0, 0, 255, 100);
			printf("test b led and effect 4\n\r");
			printf("please input any key for next test\n\r");
			system("PAUSE");

#if 1 //test static
			for (int k = 0; k < 1; k++)
			{
				for (int i = 0; i < 5; i++)
				{
					color[i] = 0x0000ff;
				}
				fSetEffect(0, &color[0], 5);
				// printf("pass \n\r");
				printf("test static\n\r");
				printf("please input any key for next test\n\r");
				system("PAUSE");
				for (int i = 0; i < 5; i++)
				{
					color[i] = 0x00ff00;
				}
				fSetEffect(0, &color[0], 5);
				printf("test static\n\r");
				printf("please input any key for next test\n\r");
				system("PAUSE");
				for (int i = 0; i < 5; i++)
				{
					color[i] = 0xff0000;
				}
				fSetEffect(0, &color[0], 5);
				printf("test static\n\r");
				printf("please input any key for next test\n\r");
				system("PAUSE");


				for (int i = 0; i < 5; i++)
				{
					color[i] = 0x00000000;
				}
				fSetEffect(0, &color[0], 5);
				printf("test all off\n\r");
				printf("please input any key for next test\n\r");
				system("PAUSE");
			   }
			}
#endif
		}
	


	printf("end test\n\r");
	

 if (!fExit)
 {
	 printf("error \n\r");
 }
 else
 {
	 fExit();
 }


    FreeLibrary(hDll);
#endif 


}
