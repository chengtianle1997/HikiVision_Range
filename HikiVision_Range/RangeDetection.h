#pragma once
#include "iostream"
#include "RangeDetection.h"
#include "cv.h"

using namespace cv;

//ÿ�С��д��µ���Ҫ���ݵ��������Ϣ
typedef struct MPoint
{
	int x; //�ֹ�����
	int y; //��λ��
	int brightness; //���������
	double cx;  //��˹�������
	double cy;  //��˹�����λ��
	int bright;  //��˹���������ȣ�����������ȣ�
	int Pixnum;  //��˹�����
	int Rows;    //ͼ��������
} MPoint;

//��˹��
typedef struct GPoint {
	int x;
	int brightness;
} GPoint;

//��x��n�η�
double fx(int x, int n);
//X,Z��������� //X���� Z����  ���ݵ����  ����GPoint
int getXZmatrix(CvMat* X, CvMat* Z, int n, GPoint *gpoint);
//���ڸ�˹��ϵ������������߼���㷨  
void getGaussCenter(Mat matImage, MPoint *point, double maxError, double minError, int xRange, int threads);
//����double������ֵ����Ǻ���
void getErrorIdentifyDoubleW(Mat matImage, MPoint *point, double doorin, int eHeight);
//����������ֶ����
void getErrorManu(Mat matImage, MPoint *point);