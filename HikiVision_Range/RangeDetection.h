#pragma once
#include "iostream"
#include "RangeDetection.h"
#include "cv.h"

using namespace cv;

//ÿ�С��д��µ���Ҫ���ݵ��������Ϣ
typedef struct MPoint
{
	int x;
	int y;
	double cx;
	double cy;
	int bright;
	int Pixnum;
	int xstart;
	int xstop;
	int errorup;
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