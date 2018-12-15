#pragma once
#include "iostream"
#include "RangeDetection.h"
#include "cv.h"

using namespace cv;

//每列、行存下的重要数据点和其他信息
typedef struct MPoint
{
	int x; //粗估中心
	int y; //行位置
	int brightness; //行最大亮度
	double cx;  //高斯拟合中心
	double cy;  //高斯拟合行位置
	int bright;  //高斯拟合最大亮度（虚拟最大亮度）
	int Pixnum;  //高斯点个数
	int Rows;    //图像总行数
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
//误差鼠标跟踪手动标记
void getErrorManu(Mat matImage, MPoint *point);