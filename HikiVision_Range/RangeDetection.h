#pragma once
#include "iostream"
#include "RangeDetection.h"
#include "cv.h"

using namespace cv;

//每列、行存下的重要数据点和其他信息
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

//高斯点
typedef struct GPoint {
	int x;
	int brightness;
} GPoint;

//求x的n次方
double fx(int x, int n);
//X,Z矩阵的生成 //X矩阵 Z矩阵  数据点个数  输入GPoint
int getXZmatrix(CvMat* X, CvMat* Z, int n, GPoint *gpoint);
//基于高斯拟合的亚像素中心线检测算法  
void getGaussCenter(Mat matImage, MPoint *point, double maxError, double minError, int xRange, int threads);
//基于double的有阈值误差标记函数
void getErrorIdentifyDoubleW(Mat matImage, MPoint *point, double doorin, int eHeight);