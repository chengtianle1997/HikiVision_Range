#define  _CRT_SECURE_NO_WARNINGS
#include "iostream"
#include "MyCamera.h"
#include "stdio.h"
#include <process.h>
#include "afxwin.h"
#include  "cv.h"
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "RangeDetection.h"
#include "omp.h"
#include "Timer.h"
#include "thread"

using namespace cv;
using namespace std;

#define GET_IDENTIFIER //显示标示结果
#define SINGLE_CAMERA_TEST //单相机性能测试
//#define STIMULATE_MULTI_CAMERA_TEST // 多相机性能测试
#define DOUBLE_BUFFER_TEST //双缓冲测试
#define WORK_THREAD_TEST  //工作线程测试

bool g_bExit = false;
unsigned int g_nPayloadSize = 0;
stop_watch Twatch;
int FrameCount = 0;
int FrameCountInThread = 0;
void* handle = NULL;//相机句柄
int DoubleBufferStatus = 0; //the identifier for double buffer thread
MV_FRAME_OUT stOutBuffer1 = { 0 };
MV_FRAME_OUT stOutBuffer2 = { 0 };



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

	MV_FRAME_OUT stOutFrame1 = { 0 };
	memset(&stOutFrame1, 0, sizeof(MV_FRAME_OUT));

	//while(1){
			nRet = MV_CC_GetImageBuffer(pUser, &stOutFrame1, 1000);
			if (nRet == MV_OK)
			{
				printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame1.stFrameInfo.nWidth, stOutFrame1.stFrameInfo.nHeight, stOutFrame1.stFrameInfo.nFrameNum);
				//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame1.stFrameInfo.nWidth, stOutFrame1.stFrameInfo.nHeight),
					CV_8UC1,
					stOutFrame1.pBufAddr
				);
				////namedWindow("Camera 0", 0);
				////imshow("Camera 0", matImage);
				int Rows = matImage.rows;
				MPoint *point;
				point = new MPoint[Rows];
				double maxError = 0.10;
				double minError = 0.10;
				int xRange = 40;
				int threads = 8;
			    //获取高斯中心
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);	
				matImage.release();
			}
			else
			{
				printf("No data[0x%x]\n", nRet);
			}
			if (NULL != stOutFrame1.pBufAddr)
			{
				nRet = MV_CC_FreeImageBuffer(pUser, &stOutFrame1);
				if (nRet != MV_OK)
				{
					printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
				}
			}
	//}
	return 0;
}

//The Buffer Getting threads
//Put Buffer in buffer1
void ImageAcq1(){
	int nRet = MV_OK;
	memset(&stOutBuffer1, 0, sizeof(MV_FRAME_OUT));
	//while(1){
	nRet = MV_CC_GetImageBuffer(handle, &stOutBuffer1, 1000);
	if (nRet == MV_OK)
	{
		//printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
			//stOutBuffer1.stFrameInfo.nWidth, stOutBuffer1.stFrameInfo.nHeight, stOutBuffer1.stFrameInfo.nFrameNum);
	}
	else
	{
		printf("No data[0x%x]\n", nRet);
	}
	if (NULL != stOutBuffer1.pBufAddr)
	{
		nRet = MV_CC_FreeImageBuffer(handle, &stOutBuffer1);
		if (nRet != MV_OK)
		{
			printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
		}
	}
}
//Put Buffer in buffer1
void ImageAcq2() {
	int nRet = MV_OK;
	memset(&stOutBuffer2, 0, sizeof(MV_FRAME_OUT));
	//while(1){
	nRet = MV_CC_GetImageBuffer(handle, &stOutBuffer2, 1000);
	if (nRet == MV_OK)
	{
		//printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
			//stOutBuffer2.stFrameInfo.nWidth, stOutBuffer2.stFrameInfo.nHeight, stOutBuffer2.stFrameInfo.nFrameNum);
	}
	else
	{
		printf("No data[0x%x]\n", nRet);
	}
	if (NULL != stOutBuffer2.pBufAddr)
	{
		nRet = MV_CC_FreeImageBuffer(handle, &stOutBuffer2);
		if (nRet != MV_OK)
		{
			printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
		}
	}
}
//The Buffer Resolving threads
void ImageSolve1() {
	//生成cv::Mat
	Mat matImage(
		cvSize(stOutBuffer1.stFrameInfo.nWidth, stOutBuffer1.stFrameInfo.nHeight),
		CV_8UC1,
		stOutBuffer1.pBufAddr
	);
	////namedWindow("Camera 0", 0);
	////imshow("Camera 0", matImage);
	int Rows = matImage.rows;
	MPoint *point;
	point = new MPoint[Rows];
	double maxError = 0.05;
	double minError = 0.25;
	int xRange = 40;
	int threads = 8;
	//获取高斯中心
	getGaussCenter(matImage, point, maxError, minError, xRange, threads);
	matImage.release();
}
void ImageSolve2() {
	//生成cv::Mat
	Mat matImage(
		cvSize(stOutBuffer2.stFrameInfo.nWidth, stOutBuffer2.stFrameInfo.nHeight),
		CV_8UC1,
		stOutBuffer2.pBufAddr
	);
	////namedWindow("Camera 0", 0);
	////imshow("Camera 0", matImage);
	int Rows = matImage.rows;
	MPoint *point;
	point = new MPoint[Rows];
	double maxError = 0.05;
	double minError = 0.25;
	int xRange = 40;
	int threads = 8;
	//获取高斯中心
	getGaussCenter(matImage, point, maxError, minError, xRange, threads);
	matImage.release();
}

int main() 
{

	int nRet = MV_OK;
	//void* handle = NULL;

	
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


	//输入相机编号
	printf("Please Intput camera index:");
	unsigned int devNum = 0;
	scanf("%d", &devNum);
	//确认输入正确
	if (devNum >= stDevList.nDeviceNum)
	{
		printf("Intput error!\n");
	}

	//input the test mode
	printf("\n0:Show the Identify Result\n");
	printf("1:Single camera Test for threads(with acquisition)\n");
	printf("2:Show the Identify Result by manu\n");
	//printf("2:Multi-Stimulate camera Test for threads\n");
	//printf("3:WorkThread Test\n");
	printf("4:Double Buffer Test for single camera\n");
	printf("5:Single camera Test for threads(without acquisition)\n");
	printf("6:Encode as mjpeg\n");
	unsigned int TestNum = 0;
	printf("Please Input test mode:");
	scanf("%d", &TestNum);
	
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

	//Set the properties of the camera
	//设置曝光时间
	float newExposureTime = 30000;
	nRet = MV_CC_SetFloatValue(handle, "ExposureTime", newExposureTime);
	if (MV_OK != nRet)
	{
		printf("Set ExposureTime fail! nRet [0x%x]\n", nRet);
	}
	//设置帧率
	float newAcquisitionFrameRate = 60.0;
	nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", newAcquisitionFrameRate);
	if (MV_OK != nRet)
	{
		printf("Set AcquisitionFrameRate fail! nRet [0x%x]\n", nRet);
	}
	//设置增益
	float newGain = 15;
	nRet = MV_CC_SetFloatValue(handle, "Gain", newGain);
	if (MV_OK != nRet)
	{
		printf("Set Gain fail! nRet [0x%x]\n", nRet);
	}


	//开始取流
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));

	switch(TestNum)
	{
	case 0: {
#ifdef GET_IDENTIFIER
		double maxError = 0.05;
		double minError = 0.10;
		int xRange = 20;
		int threads = 8;
		int DefaultSetting = 0;
		double doorin = 0.38;
		printf("Would you like to change the default settings? 0-No 1-Yes");
		scanf("%d", &DefaultSetting);
		if (DefaultSetting) {
			printf("Please input the Top error:");
			scanf("%lf", &maxError);
			printf("Please input the Bottom error:");
			scanf("%lf", &minError);
			printf("Please input the xRange:");
			scanf("%d", &xRange);
			printf("Please input the identify error:");
			scanf("%lf", &doorin);
		}

		while (1)
		{
			nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
			if (nRet == MV_OK)
			{
				/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

					//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame.stFrameInfo.nWidth,stOutFrame.stFrameInfo.nHeight),
					CV_8UC1,
					stOutFrame.pBufAddr
				);
				int Rows = matImage.rows;
				MPoint *point;
				point = new MPoint[Rows];

				//获取高斯中心
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);

				int eHeight = 0;
				//基于double的有阈值误差标记函数
				getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
				//cvNamedWindow("Camera 0", 0); 
				//imshow("Camera 0", matImage);
				cvWaitKey(1);
				matImage.release();
				delete point;
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
	}
#endif
	break;


	case 1: {
#ifdef SINGLE_CAMERA_TEST
		int threads = 0;
		printf("Please input the thread num:");
		scanf("%d",&threads);
		while (1)
		{
			Twatch.start();
			nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
			if (nRet == MV_OK)
			{
				/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

					//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight),
					CV_8UC1,
					stOutFrame.pBufAddr
				);
				int Rows = matImage.rows;
				MPoint *point;
				point = new MPoint[Rows];
				double maxError = 0.05;
				double minError = 0.25;
				int xRange = 40;

				//获取高斯中心
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);

				//double doorin = 0.5;
				//int eHeight = 0;
				//基于double的有阈值误差标记函数
				//getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
				//cvNamedWindow("Camera 0", 0); 
				//imshow("Camera 0", matImage);
				//cvWaitKey(1);
				matImage.release();
				FrameCount++;

				delete point;
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


			Twatch.stop();
			if (Twatch.elapsed() > 1000000)
			{
				printf("当前帧率为%dfps\n", FrameCount);
				FrameCount = 0;
				Twatch.restart();
			}
		}
#endif
	}
	break;


	case 2: {
		while (1)
		{

			nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
			if (nRet == MV_OK)
			{
				/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

					//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight),
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
				//Calculate
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);
				//put the rows of mat in point[0]
				point[0].Rows = Rows;
				//cvNamedWindow("src", 0);
				getErrorManu(matImage, point);
				matImage.release();
				delete point;
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

#ifdef STIMULATE_MULTI_CAMERA_TEST
		while (1)
		{

			nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
			if (nRet == MV_OK)
			{
				/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

					//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight),
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
				Twatch.start();
#pragma omp parallel for num_threads(8)
				//模拟8台相机的工作流
				for (int i = 0; i < 4; i++)
				{
					//获取高斯中心
					getGaussCenter(matImage, point, maxError, minError, xRange, threads);
					FrameCountInThread++;
				}
				//double doorin = 0.5;
				//int eHeight = 0;
				//基于double的有阈值误差标记函数
				//getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
				//cvNamedWindow("Camera 0", 0); 
				//imshow("Camera 0", matImage);
				//cvWaitKey(1);
				matImage.release();
				delete point;
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
			FrameCount++;
			Twatch.stop();
			if (Twatch.elapsed() > 1000000)
			{
				printf("当前捕获帧率为%dfps , 处理帧率为%dfps \n", FrameCount, FrameCountInThread);
				FrameCount = 0;
				FrameCountInThread = 0;
				Twatch.restart();
			}
		}
#endif
	}
	break;


	case 3: {
#ifdef WORK_THREAD_TEST	

		unsigned int nThreadID0 = 0;
		void* hThreadHandle0 = (void*)_beginthreadex(NULL, 0, WorkThread, handle, 0, &nThreadID0);

		if (NULL == hThreadHandle0)
		{
			printf("Get ThreadHandle Error\n");
		}
		Sleep(30);

#endif
	}
	break;


	case 4: {
#ifdef DOUBLE_BUFFER_TEST
		//threadA: buffer acquisition  threadB:buffer resolve
		thread ThA1(ImageAcq1);
		ThA1.join();
		int Fps =0;
		while (1)
		{
			Twatch.start();
			//the ThA running once, and then circulate the ThA and ThB
			thread ThA2(ImageAcq2);
			thread ThB1(ImageSolve1);
			ThB1.detach();
			ThA2.join();
			thread ThA1(ImageAcq1);
			thread ThB2(ImageAcq2);
			ThB2.detach();
			ThA1.join();
			Twatch.stop();
			Fps++;
			if (Twatch.elapsed() > 1000000) {
				printf("The Frame Rate is %d fps\n", Fps);
				Fps = 0;
				Twatch.restart();
			}
		}
#endif
	}
		break;
	case 5: {
		int threads = 0;
		printf("Please input the thread num:");
		scanf("%d", &threads);
		while (1)
		{
			
			nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
			if (nRet == MV_OK)
			{
				/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/
				Twatch.start();
					//生成cv::Mat
				Mat matImage(
					cvSize(stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight),
					CV_8UC1,
					stOutFrame.pBufAddr
				);
				int Rows = matImage.rows;
				MPoint *point;
				point = new MPoint[Rows];
				double maxError = 0.05;
				double minError = 0.25;
				int xRange = 40;

				//获取高斯中心
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);

				//double doorin = 0.5;
				//int eHeight = 0;
				//基于double的有阈值误差标记函数
				//getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
				//cvNamedWindow("Camera 0", 0); 
				//imshow("Camera 0", matImage);
				//cvWaitKey(1);
				matImage.release();
				FrameCount++;
				Twatch.stop();
				if (Twatch.elapsed() > 1000000)
				{
					printf("当前帧率为%dfps\n", FrameCount);
					FrameCount = 0;
					Twatch.restart();
				}
				delete point;
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
	}
	}
	
	return 0;
}