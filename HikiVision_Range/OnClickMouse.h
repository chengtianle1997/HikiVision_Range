
#include "iostream"

#include <highgui.h>  

#include <stdio.h>  

#include <vector>  

#include <fstream>  

#include "opencv2/opencv.hpp"

#include "RangeDetection.h"

 using namespace std;

using namespace cv;

//ע��������з��Ŷ����ͣ��ú�����������ʹi�޶�Ϊ[a,b]������  
int bound(short i, short a, short b);

CvScalar getInverseColor(CvScalar c);

//On_Mouse_Callback
void on_mouse(int event, int x, int y, int flags, void* ustc);

//Get Mouse Identify
int GetMouseIdentify(Mat matImage, MPoint *point);
