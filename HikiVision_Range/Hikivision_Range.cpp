#define  _CRT_SECURE_NO_WARNINGS
#include "iostream"
#include "MyCamera.h"
#include "stdio.h"
#include <process.h>
#include "Windows.h"
#include  "cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "RangeDetection.h"
#include "omp.h"

using namespace cv;
using namespace std;

bool g_bExit = false;
unsigned int g_nPayloadSize = 0;

//打印相机设备信息
bool  PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo) 
{
	if (pstMVDevInfo == NULL)
	{
		printf("The Pointer of pstMVDevInfo is NULL!\n");
		return false;
	}
	if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
	{
		printf("UserDefinedName: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
		printf("Serial Number: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chSerialNumber);
		printf("Device Number: %d\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.nDeviceNumber);
	}
	else
	{
		printf("Not Support!\n");
	}
	return true;
}

//图像取流工作线程
static  unsigned int __stdcall WorkThread(void* pUser)
{
	int nRet = MV_OK;

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));

	while (1)
	{
		nRet = MV_CC_GetImageBuffer(pUser, &stOutFrame, 1000);
		if (nRet == MV_OK)
		{
			printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
				stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);
			//生成cv::Mat
			Mat matImage(
				cvSize(stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight),
				CV_8UC1,
				stOutFrame.pBufAddr
			);
			/*long threadidx = GetCurrentThreadId();
			if (threadidx == 17456)
			{*/
			/*char tmp[20];
			sprintf(tmp, "%ld", GetCurrentThreadId());*/
 			imshow("Camera 0", matImage);
			cvWaitKey(1);
			//}
			matImage.release();

		}
		else
		{
			printf("No data[0x%x]\n", nRet);
		}
		if (NULL != stOutFrame.pBufAddr)
		{
			nRet = MV_CC_FreeImageBuffer(pUser, &stOutFrame);
			if (nRet != MV_OK)
			{
				printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
			}
		}
		/*if (g_bExit)
		{
			break;
		}*/
	}
	return 0;
}


int main() 
{

	int nRet = MV_OK;
	void* handle = NULL;
	
	
	//获取设备枚举列表
	MV_CC_DEVICE_INFO_LIST stDevList;
	memset(&stDevList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
	nRet = MV_CC_EnumDevices(MV_USB_DEVICE, &stDevList);

	if (MV_OK != nRet)
	{
		printf("Enum Devices fail! nRet [0x%x]\n", nRet);
	}

	if (stDevList.nDeviceNum > 0)
	{
		for (unsigned int i = 0; i < stDevList.nDeviceNum; i++)
		{
			printf("[device %d]:\n", i);
			//设备信息
			MV_CC_DEVICE_INFO* pDeviceInfo = stDevList.pDeviceInfo[i];
			if (NULL == pDeviceInfo)
			{
				break;
			}
			PrintDeviceInfo(pDeviceInfo);
		}
	}
	else
	{
		printf("Find No Devices!\n");
	}

	//选择的相机编号 从0开始
	unsigned int devNum = 0;
	//确认输入正确
	if (devNum >= stDevList.nDeviceNum)
	{
		printf("Intput error!\n");
	}
	
	//选择设备并创建句柄
	nRet = MV_CC_CreateHandle(&handle, stDevList.pDeviceInfo[devNum]);
	if (MV_OK != nRet)
	{
		printf("Create Handle fail! nRet [0x%x]\n", nRet);
	}
	nRet = MV_CC_OpenDevice(handle);
	if (MV_OK != nRet)
	{
		printf("Open Device fail! nRet [0x%x]\n", nRet);
	}
	else {
		printf("Device is ready!\n");
	}

	//设置触发模式为off
	nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
	if (MV_OK != nRet)
	{
		printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
	}

	//获取数据包大小
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
	if (MV_OK != nRet)
	{
		printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
	}
	g_nPayloadSize = stParam.nCurValue;

	//开始取流
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));
	

	while (1)
	{
		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
		if (nRet == MV_OK)
		{
			/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
				stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

			//生成cv::Mat
			Mat matImage(
				cvSize(2592, 2048),
				CV_8UC1,
				stOutFrame.pBufAddr
			);
			int Rows = matImage.rows;
			MPoint *point;
			point = new MPoint[Rows];
			double maxError = 0.05;
			double minError = 0.25;
			int xRange = 40;
			int threads = 8;
			//获取高斯中心
			getGaussCenter(matImage, point, maxError, minError, xRange, threads);

			//double doorin = 0.2;
			//int eHeight = 0;				
			////基于double的有阈值误差标记函数
			//getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
			cvNamedWindow("Camera 0", 0); 
			imshow("Camera 0", matImage);
			cvWaitKey(1);  
			matImage.release();
		}
		else
		{
			printf("No data[0x%x]\n", nRet);
		}
		if (NULL != stOutFrame.pBufAddr)
		{
			nRet = MV_CC_FreeImageBuffer(handle, &stOutFrame);
			if (nRet != MV_OK)
			{
				printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
			}
		}
	}

/*
	while (1) {
		unsigned int nThreadID = 0;
		void* hThreadHandle = (void*)_beginthreadex(NULL, 0, WorkThread, handle, 0, &nThreadID);
		if (NULL == hThreadHandle)
		{
			printf("Get ThreadHandle Error\n");
			break;
		}
	Sleep(1000);
	}*/
	return 0;
}