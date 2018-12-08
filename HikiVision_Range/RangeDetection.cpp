
#include "iostream"
#include "cv.h"
#include "math.h"
#include "RangeDetection.h"
#include "omp.h"
#include "opencv2/opencv.hpp"


using namespace std;
using namespace cv;

//求x的n次方
double fx(int x, int n) {
	int y;
	if (n == 0)
	{
		y = 1;
	}
	else if (n == 1) {
		y = x;
	}
	else if (n == 2) {
		y = x * x;
	}
	return y;
}

//X,Z矩阵的生成 //X矩阵 Z矩阵  数据点个数  输入GPoint
int getXZmatrix(CvMat* X, CvMat* Z, int n, GPoint *gpoint) {
	//n个数据点 以n行形式存入
	for (int i = 0; i < n; i++) {
		//顺序存入 1  x  x^2
		//double* xData = (double*)(X->data.ptr + i * X->step);
		//double* zData = (double*)(X->data.ptr + i * X->step);
		for (int j = 0; j < 3; j++) {
			cvmSet(X, i, j, fx(gpoint[i].x, j));
			//xData[j] = fx(gpoint[i].x, j);
			//cout << j << endl;
		}
		//y存入Z矩阵
		cvmSet(Z, i, 0, log(gpoint[i].brightness));
		//zData[0] = log(gpoint[i].brightness);
	}
	return 1;
}

//基于高斯拟合的亚像素中心线检测算法  
void getGaussCenter(Mat matImage, MPoint *point, double maxError, double minError, int xRange,int threads) {
	Mat cloneImage = matImage.clone();
	//Mat OrgnImage = matImage.clone();
	//先运用canny检测得到初步中心线
	//int g_nCannyLowThreshold = 80;//canny检测低阈值
	//int minCanny = 200;//canny平均点筛选
	//Mat tmpImage, dstImage;
	//blur(cloneImage, tmpImage, Size(3, 3));
	//Canny(tmpImage, dstImage, g_nCannyLowThreshold, g_nCannyLowThreshold * 3);
	//namedWindow("canny function");
	//imshow("canny function", dstImage);
	int Rows = cloneImage.rows;
	int Cols = cloneImage.cols*cloneImage.channels();
	int *brightness;
	//int threads = 2;//调用线程数
	brightness = new int[Rows];
	memset(brightness, 0, Rows);
	//getPeaker1(matImage, point);
#pragma omp parallel for num_threads(threads)
	for (int i = 0; i < Rows; i++)
	{
		uchar* data = matImage.ptr<uchar>(i);
		int MaxPixel = data[0];
		int MaxX(0);
		for (int j = 1; j < Cols; j++)
		{
			if (data[j] > MaxPixel) {
				MaxPixel = data[j];
				MaxX = j;
			}
		}
		point[i].y = i;
		point[i].x = MaxX;
		//point[i].bright = MaxPixel;
		brightness[i] = MaxPixel;
	}
	/*for (int i = 0; i < Rows; i++)
	{
		brightness[i] = point[i].bright;
	}*/
	//int x[100];
	//int px = 0; 


	//int sum = 0;
	//int average = 0;
	////getPeaker(matImage, brightness);
	//for (int j = 0; j < Rows; j++) {
	//	uchar* data = dstImage.ptr<uchar>(j);
	//	uchar* odata = matImage.ptr<uchar>(j);
	//	for (int i = 0; i < Cols; i++) {
	//		int PixelDataof = data[i];
	//		if (PixelDataof > minCanny) {//修改canny检测后的边缘阈值
	//			x[px] = i;
	//			px++;
	//			sum = sum + i;
	//			if (px > 100) {
	//				cout << "there are too many canny points" << endl;
	//			}
	//		}
	//	}
	//	//逐行计算平均点
	//	if (px) {
	//		average = sum * 1.0 / px;
	//	}
	//	point[j].x = average;
	//	point[j].y = j;
	//	brightness[j] = odata[average];
	//	
	//	average = 0;
	//	sum = 0;
	//	px = 0;
	//	memset(x, 0, px);
	//	//cout << "(" << point[j].cx << "," << point[j].cy << ")" << endl;
	//}

	//读取point中的值
//	int Cols = cloneImage.cols;//x
//	int Rows = cloneImage.rows;//y




	//逐行存储所有点的x坐标和亮度值以便分析 在此只存入高斯点
#pragma omp parallel for num_threads(threads)
	for (int i = 0; i < Rows; i++) {
		int PixelData;
		int Pixnum = 0;
		GPoint *gpoint;
		gpoint = new GPoint[Rows];
		Pixnum = 0;
		//高斯点选取 
		//watch.restart();
		uchar* data = matImage.ptr<uchar>(i);
		for (int j = point[i].x - xRange; j <= point[i].x + xRange; j++) {
			PixelData = data[j];
			//cout << PixelData << endl;
			//minerror和maxerror条件筛选高斯点  //后期在此处考虑xRange
			//cout << "condition1" << (PixelData > minError*brightness[i]) << endl;
			//cout << "condition2" << (PixelData < ((1 - maxError)*brightness[i]))<<endl;
			//cout << "condition3" << (abs(j - point[i].x) < xRange) << endl;

			if (PixelData > minError*brightness[i] && PixelData < ((1 - maxError)*brightness[i])) {
				gpoint[Pixnum].x = j;
				gpoint[Pixnum].brightness = PixelData;
				Pixnum++;
			}
			/*if ((j - point[i].x) > xRange)
				break;*/
		}
		//watch.stop();
		//cout << "高斯点选取耗时:" << watch.elapsed() << endl;
		if (Pixnum >= 3) {
			int n = Pixnum;
			CvMat* X = cvCreateMat(n, 3, CV_64FC1);
			CvMat* Z = cvCreateMat(n, 1, CV_64FC1);
			//CvMat* XT = cvCreateMat(3, n, CV_64FC1);
			CvMat* B = cvCreateMat(3, 1, CV_64FC1);
			CvMat* SA = cvCreateMat(3, 3, CV_64FC1);
			CvMat* SAN = cvCreateMat(3, 3, CV_64FC1);
			CvMat* SC = cvCreateMat(3, n, CV_64FC1);
			//获取矩阵
			//watch.restart();
			getXZmatrix(X, Z, n, gpoint);
			//watch.stop();
			//cout << "矩阵获取耗时:" << watch.elapsed() << endl;
			//	/*for (int i = 0; i < n; i++) {
			//		for (int j = 0; j < 3; j++) {
			//			cout << cvmGet(X, i, j) << "\t";
			//		}
			//		cout << endl;
			//	}*/
			//	
			//	for (int i = 0; i < 3; i++) {
			//		for (int j = 0; j < n; j++) {
			//			cout << cvmGet(XT, i, j) << "\t";
			//		}
			//		cout << endl;
			//	}*/
			//乘法1
			//watch.restart();
			cvGEMM(X, X, 1, NULL, 0, SA, CV_GEMM_A_T);
			//watch.stop();
			//cout << "乘法1耗时:" << watch.elapsed() << endl;
			//for (int i = 0; i < 3; i++) {
			//		for (int j = 0; j < 3; j++) {
			//			cout << cvmGet(SA, i, j) << "\t";
			//		}
			//		cout << endl;
			//	}*/
			//矩阵求逆
			//watch.restart();
			cvInvert(SA, SAN, CV_LU);  //高斯消去法
			//watch.stop();
			//cout << "矩阵求逆耗时:" << watch.elapsed() << endl;
			/*for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {

					cout << cvmGet(SAN, i, j) << "\t";
				}
				cout << endl;
			}*/
			//矩阵乘法2
			//watch.restart();
			cvGEMM(SAN, X, 1, NULL, 0, SC, CV_GEMM_B_T);
			//watch.stop();
			//cout << "矩阵乘法2耗时:" << watch.elapsed() << endl;
			//	/*for (int i = 0; i < 3; i++) {
			//		for (int j = 0; j < n; j++) {
			//			cout << cvmGet(SC, i, j) << "\t";
			//		}
			//		cout << endl;
			//	}*/
			//	/*for (int i = 0; i < n; i++) {
			//		for (int j = 0; j < 1; j++) {
			//			cout << cvmGet(Z, i, j) << "\t";
			//		}
			//		cout << endl;
			//	}*/
			//矩阵乘法3
			//watch.restart();
			cvGEMM(SC, Z, 1, NULL, 0, B, 0);
			//watch.stop();
			//cout << "矩阵乘法3耗时:" << watch.elapsed() << endl;
			//	/*for (int i = 0; i < 3; i++) {
			//		cout << cvmGet(B, i, 0)<<"\t";
			//	}
			//	cout << endl;*/
			//结果计算
			//watch.restart();
			point[i].cx = (-cvmGet(B, 1, 0))*1.0 / (2 * cvmGet(B, 2, 0));
			point[i].bright = exp(cvmGet(B, 0, 0) - cvmGet(B, 1, 0)*cvmGet(B, 1, 0) / (4 * cvmGet(B, 2, 0)));
			//watch.stop();
			//cout << "结果计算耗时:" << watch.elapsed() << endl;
			cvReleaseMat(&X);
			cvReleaseMat(&Z);
			cvReleaseMat(&B);
			cvReleaseMat(&SA);
			cvReleaseMat(&SAN);
			cvReleaseMat(&SC);
		}
		else {
			point[i].cx = 0;
			point[i].bright = 0;
		}
		point[i].cy = i;
		printf("(%lf , %lf): %d)\n", point[i].cx, point[i].cy,point[i].bright);
		delete[]gpoint;
	}

	//基于double的有阈值误差标记函数
	//getErrorIdentifyDoubleW(cloneImage, point, 0.15,0);


}

//基于double的有阈值误差标记函数
void getErrorIdentifyDoubleW(Mat matImage, MPoint *point, double doorin, int eHeight) {
	int Rows = matImage.rows;//y
	//int Cols = matImage.cols;
	int Cols = matImage.cols*matImage.channels();//x
	//int div = 64;
	double error;
	for (int j = 0; j < Rows; j++) {
		//point[j].errorup = point[j].cx - point[j - 1].cx;
		if (abs(point[j].cx - point[j - 1].cx) > doorin) {
			line(matImage, Point((point[j].cx - 30), point[j].cy), Point((point[j].cx + 30), point[j].cy), Scalar(255, 100, 100), 2, 8, 0);
			line(matImage, Point(point[j].cx, point[j].cy - 30), Point(point[j].cx, point[j].cy + 30), Scalar(255, 100, 100), 2, 8, 0);
			error = point[j].cx - point[j - 1].cx;
			ostringstream oss;
			oss << error;
			string texterror = oss.str();
			putText(matImage, texterror, Point(point[j].cx + 40, point[j].cy), 2, 0.5, Scalar(255, 100, 100), 1, 8, 0);
		}
	}
	namedWindow("error identification");
	imshow("error identification", matImage);
}