
#include "iostream"
#include <stdio.h>
#include <Windows.h>
#include <process.h>
#include <conio.h>
#include "MyCamera.h"

#if 0
bool g_bExit = false;
unsigned int g_nPayloadSize = 0;

// ch:�ȴ��������� | en:Wait for key press
void WaitForKeyPress(void)
{
	while (!_kbhit())
	{
		Sleep(10);
	}
	_getch();
}

//��ӡ����豸��Ϣ
bool fPrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
	if (NULL == pstMVDevInfo)
	{
		printf("The Pointer of pstMVDevInfo is NULL!\n");
		return false;
	}
	if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
	{
		int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
		int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
		int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
		int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

		// ch:��ӡ��ǰ���ip���û��Զ������� | en:print current ip and user defined name
		printf("CurrentIp: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
		printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
	}
	else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
	{
		printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
		printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
		printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
	}
	else
	{
		printf("Not support.\n");
	}

	return true;
}



int fmain() {
	int nRet = MV_OK;
	void* handle = NULL;

	do
	{
		// ch:ö���豸 | en:Enum device
		MV_CC_DEVICE_INFO_LIST stDeviceList;
		memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
		//nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
		nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDeviceList);
		if (MV_OK != nRet)
		{
			printf("Enum Devices fail! nRet [0x%x]\n", nRet);
			break;
		}

		if (stDeviceList.nDeviceNum > 0)
		{
			for (unsigned int i = 0; i < stDeviceList.nDeviceNum; i++)
			{
				printf("[device %d]:\n", i);
				MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
				if (NULL == pDeviceInfo)
				{
					break;
				}
				fPrintDeviceInfo(pDeviceInfo);
			}
		}
		else
		{
			printf("Find No Devices!\n");
			break;
		}

		//printf("Please Intput camera index:");
		unsigned int nIndex = 0;   //������ѡ��  ��ͨ�����к��ж���Ӧ���
		//scanf("%d", &nIndex);


		if (nIndex >= stDeviceList.nDeviceNum)
		{
			printf("Intput error!\n");
			break;
		}

		// ch:ѡ���豸��������� | en:Select device and create handle
		nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
		if (MV_OK != nRet)
		{
			printf("Create Handle fail! nRet [0x%x]\n", nRet);
			break;
		}

		// ch:���豸 | en:Open device
		nRet = MV_CC_OpenDevice(handle);
		if (MV_OK != nRet)
		{
			printf("Open Device fail! nRet [0x%x]\n", nRet);
			break;
		}
		else {
			printf("Device is ready!\n");
		}

		// ch:���ô���ģʽΪoff | en:Set trigger mode as off
		nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
		if (MV_OK != nRet)
		{
			printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
			break;
		}

		// ch:��ȡ���ݰ���С | en:Get payload size
		MVCC_INTVALUE stParam;
		memset(&stParam, 0, sizeof(MVCC_INTVALUE));
		nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
		if (MV_OK != nRet)
		{
			printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
			break;
		}
		g_nPayloadSize = stParam.nCurValue;

		// ch:��ʼȡ�� | en:Start grab image
		nRet = MV_CC_StartGrabbing(handle);
		if (MV_OK != nRet)
		{
			printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
			break;
		}

		unsigned int nThreadID = 0;
		//void* hThreadHandle = (void*)_beginthreadex(NULL, 0, WorkThread, handle, 0, &nThreadID);
		if (NULL == hThreadHandle)
		{
			break;
		}

		printf("Press a key to stop grabbing.\n");
		WaitForKeyPress();

		g_bExit = true;
		Sleep(1000);

		// ch:ֹͣȡ�� | en:Stop grab image
		nRet = MV_CC_StopGrabbing(handle);
		if (MV_OK != nRet)
		{
			printf("Stop Grabbing fail! nRet [0x%x]\n", nRet);
			break;
		}

		// ch:�ر��豸 | Close device
		nRet = MV_CC_CloseDevice(handle);
		if (MV_OK != nRet)
		{
			printf("ClosDevice fail! nRet [0x%x]\n", nRet);
			break;
		}

		// ch:���پ�� | Destroy handle
		nRet = MV_CC_DestroyHandle(handle);
		if (MV_OK != nRet)
		{
			printf("Destroy Handle fail! nRet [0x%x]\n", nRet);
			break;
		}

	} while (0);
	return 0;
}
#endif