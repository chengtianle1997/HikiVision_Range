
#include "iostream"

#include <highgui.h>  

#include <stdio.h>  

#include <vector>  

#include <fstream>  

#include "opencv2/opencv.hpp"

#include "RangeDetection.h"

 using namespace std;

using namespace cv;

//注意参数是有符号短整型，该函数的作用是使i限定为[a,b]区间内  
int bound(short i, short a, short b);

CvScalar getInverseColor(CvScalar c);

//On_Mouse_Callback
void on_mouse(int event, int x, int y, int flags, void* ustc);

//Get Mouse Identify
int GetMouseIdentify(Mat matImage, MPoint *point);
