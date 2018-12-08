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

using namespace cv;
using namespace std;

//#define GET_IDENTIFIER //��ʾ��ʾ���
//#define SINGLE_CAMERA_TEST //��������ܲ���
#define STIMULATE_MULTI_CAMERA_TEST // ��������ܲ���
//#define WORK_THREAD_TEST  //�����߳�

bool g_bExit = false;
unsigned int g_nPayloadSize = 0;
stop_watch Twatch;
int FrameCount = 0;
int FrameCountInThread = 0;

//��ӡ����豸��Ϣ
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

//ͼ��ȡ�������߳�
static  unsigned int __stdcall WorkThread(void* pUser)
{
	int nRet = MV_OK;

	MV_FRAME_OUT stOutFrame1 = { 0 };
	memset(&stOutFrame1, 0, sizeof(MV_FRAME_OUT));
	//MV_FRAME_OUT stOutFrame2 = { 0 };
	//memset(&stOutFrame1, 0, sizeof(MV_FRAME_OUT));
	//int counter = 0;

	while(1){
		//if (counter % 2 == 0) {
			//��ȡͼ��buffer
			nRet = MV_CC_GetImageBuffer(pUser, &stOutFrame1, 1000);
			if (nRet == MV_OK)
			{
				printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
					stOutFrame1.stFrameInfo.nWidth, stOutFrame1.stFrameInfo.nHeight, stOutFrame1.stFrameInfo.nFrameNum);
				//����cv::Mat
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
				double maxError = 0.05;
				double minError = 0.25;
				int xRange = 40;
				int threads = 8;
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);

				#pragma omp parallel for  num_threads(threads)
							for (int i = 0; i < 8; i++) {
								//��ȡ��˹����
								getGaussCenter(matImage, point, maxError, minError, xRange, threads);
							}
							//cvWaitKey(1);
				
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
			//counter++;
		//}
		//if (counter % 2 == 1) {
		//	//��ȡͼ��buffer
		//	nRet = MV_CC_GetImageBuffer(pUser, &stOutFrame2, 1000);
		//	if (nRet == MV_OK)
		//	{
		//		printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
		//			stOutFrame2.stFrameInfo.nWidth, stOutFrame2.stFrameInfo.nHeight, stOutFrame2.stFrameInfo.nFrameNum);
		//		//����cv::Mat
		//		Mat matImage(
		//			cvSize(stOutFrame2.stFrameInfo.nWidth, stOutFrame2.stFrameInfo.nHeight),
		//			CV_8UC1,
		//			stOutFrame2.pBufAddr
		//		);
		//		//namedWindow("Camera 0", 0);
		//		//imshow("Camera 0", matImage);
		//		int Rows = matImage.rows;
		//		MPoint *point;
		//		point = new MPoint[Rows];
		//		double maxError = 0.05;
		//		double minError = 0.25;
		//		int xRange = 40;
		//		int threads = 8;
		//		getGaussCenter(matImage, point, maxError, minError, xRange, threads);
		//		//#pragma omp parallel for  
		//		//			for (int i = 0; i < 8; i++) {
		//		//				//��ȡ��˹����
		//		//				getGaussCenter(matImage, point, maxError, minError, xRange, threads);
		//		//			}
		//		//			//cvWaitKey(1);
		//		matImage.release();

		//	}
		//	else
		//	{
		//		printf("No data[0x%x]\n", nRet);
		//	}
		//	if (NULL != stOutFrame2.pBufAddr)
		//	{
		//		nRet = MV_CC_FreeImageBuffer(pUser, &stOutFrame2);
		//		if (nRet != MV_OK)
		//		{
		//			printf("Free Image Buffer fail! nRet [0x%x]\n", nRet);
		//		}
		//	}
		//	counter++;
		//}
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

	
	//��ȡ�豸ö���б�
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
			//�豸��Ϣ
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

	//ѡ��������� ��0��ʼ
	unsigned int devNum = 0;
	//ȷ��������ȷ
	if (devNum >= stDevList.nDeviceNum)
	{
		printf("Intput error!\n");
	}
	
	//ѡ���豸���������
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

	//���ô���ģʽΪoff
	nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
	if (MV_OK != nRet)
	{
		printf("Set Trigger Mode fail! nRet [0x%x]\n", nRet);
	}

	//��ȡ���ݰ���С
	MVCC_INTVALUE stParam;
	memset(&stParam, 0, sizeof(MVCC_INTVALUE));
	nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
	if (MV_OK != nRet)
	{
		printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
	}
	g_nPayloadSize = stParam.nCurValue;

	//��ʼȡ��
	nRet = MV_CC_StartGrabbing(handle);
	if (MV_OK != nRet)
	{
		printf("Start Grabbing fail! nRet [0x%x]\n", nRet);
	}

	MV_FRAME_OUT stOutFrame = { 0 };
	memset(&stOutFrame, 0, sizeof(MV_FRAME_OUT));
	
#ifdef GET_IDENTIFIER
	while (1)
	{
		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
		if (nRet == MV_OK)
		{
			/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
				stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

			//����cv::Mat
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
			//��ȡ��˹����
			getGaussCenter(matImage, point, maxError, minError, xRange, threads);

			double doorin = 0.5;
			int eHeight = 0;				
			//����double������ֵ����Ǻ���
			getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
			//cvNamedWindow("Camera 0", 0); 
			//imshow("Camera 0", matImage);
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
#endif
	
#ifdef SINGLE_CAMERA_TEST
	while (1)
	{
		
		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
		if (nRet == MV_OK)
		{
			/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
				stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/
			Twatch.start();
			//����cv::Mat
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
			//��ȡ��˹����
			getGaussCenter(matImage, point, maxError, minError, xRange, threads);

			//double doorin = 0.5;
			//int eHeight = 0;
			//����double������ֵ����Ǻ���
			//getErrorIdentifyDoubleW(matImage, point, doorin, eHeight);
			//cvNamedWindow("Camera 0", 0); 
			//imshow("Camera 0", matImage);
			//cvWaitKey(1);
			matImage.release();
			FrameCount++;
			Twatch.stop();
			if (Twatch.elapsed() > 1000000)
			{
				printf("��ǰ֡��Ϊ%dfps\n", FrameCount);
				FrameCount = 0;
				Twatch.restart();
			}
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
#endif

#ifdef STIMULATE_MULTI_CAMERA_TEST
	while (1)
	{
		
		nRet = MV_CC_GetImageBuffer(handle, &stOutFrame, 1000);
		if (nRet == MV_OK)
		{
			/*printf("Get One Frame: Width[%d], Height[%d], nFrameNum[%d]\n",
				stOutFrame.stFrameInfo.nWidth, stOutFrame.stFrameInfo.nHeight, stOutFrame.stFrameInfo.nFrameNum);*/

				//����cv::Mat
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
			Twatch.start();
#pragma omp parallel for num_threads(8)
			//ģ��8̨����Ĺ�����
			for (int i = 0; i < 4; i++) 
			{
				//��ȡ��˹����
				getGaussCenter(matImage, point, maxError, minError, xRange, threads);
				FrameCountInThread++;
			}
			//double doorin = 0.5;
			//int eHeight = 0;
			//����double������ֵ����Ǻ���
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
			printf("��ǰ����֡��Ϊ%dfps , ����֡��Ϊ%dfps \n", FrameCount,FrameCountInThread);
			FrameCount = 0;
			FrameCountInThread = 0;
			Twatch.restart();
		}
	}
#endif


#ifdef WORK_THREAD_TEST	
		unsigned int nThreadID0 = 0;
		void* hThreadHandle0 = (void*)_beginthreadex(NULL, 0, WorkThread, handle, 0, &nThreadID0);

		if (NULL == hThreadHandle0)
		{
			printf("Get ThreadHandle Error\n");
			//break;
		}
		
		Sleep(1000);
	//} while (1);
#endif
	
	return 0;
}