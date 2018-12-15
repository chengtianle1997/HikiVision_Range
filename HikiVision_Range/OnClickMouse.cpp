#define _CRT_SECURE_NO_WARNINGS

#include "OnClickMouse.h"


#define max(a,b)            (((a) > (b)) ? (a) : (b))  

#define min(a,b)            (((a) < (b)) ? (a) : (b))  



//注意参数是有符号短整型，该函数的作用是使i限定为[a,b]区间内  

int bound(short i, short a, short b)

{

	return min(max(i, min(a, b)), max(a, b));

}



CvScalar getInverseColor(CvScalar c)

{

	CvScalar s;

	for (int i = 0; i <= 2; ++i)

	{

		s.val[i] = 255 - c.val[i];

	}

	return s;

}



Mat src;

Mat dst;

Mat ori;

int n = 0;

vector<Point> points;



void on_mouse(int event, int x, int y, int flags, void* ustc)

{

	Point pt;

	Point tmp_pt = { -1,-1 };

	char temp[100];

	Size text_size;

	int baseline;



	Scalar clrPoint = Scalar(255, 0, 0, 0);

	Scalar clrText = Scalar(255, 255, 255, 0);

	MPoint *point = (MPoint*)ustc;

	//MPoint *point = { 0 };

	if (event == CV_EVENT_MOUSEMOVE)

	{

		dst.copyTo(src);

		//x = bound(x, 0, &src->width - 1);

		//y = bound(y, 0, src->height - 1);

		pt = Point(x, y);

		circle(src, pt, 2, clrPoint, 1, 8, 0);


		//sprintf(temp, "%d (%d,%d)", n + 1, x, y);

		if(y>1&&y<point[0].Rows)
		sprintf(temp, "Row:%d , center:%f , ErrorUp:%f , ErrorDown:%f", y, point[y].cx, point[y].cx - point[y-1].cx, point[y].cx - point[y + 1].cx);


		//getTextSize(temp, 2, &text_size, &baseline);

		//tmp_pt.x = bound(pt.x, 0, src->width - text_size.width);

		//tmp_pt.y = bound(pt.y, text_size.height + baseline, src->height - 1 - baseline);

		tmp_pt = Point(x+30, y);

		putText(src, temp, tmp_pt, 2, 1, clrText, 1, 8, 0);

		imshow("src", src);

	}

	else if (event == CV_EVENT_LBUTTONDOWN)

	{

		pt = cvPoint(x, y);

		points.push_back(pt); n++;

		circle(src, pt, 2, clrPoint, 1, 8, 0);

		//sprintf(temp, "%d (%d,%d)", n, x, y);

		if (y > 1 && y < point[0].Rows)
		sprintf(temp, "Row:%d , center:%f , ErrorUp:%f , ErrorDown:%f", y, point[y].cx, point[y].cx - point[y - 1].cx, point[y].cx - point[y + 1].cx);

		//cvGetTextSize(temp, &font, &text_size, &baseline);

		//tmp_pt.x = bound(pt.x, 0, src->width - text_size.width);

		//tmp_pt.y = bound(pt.y, text_size.height + baseline, src->height - 1 - baseline);

		tmp_pt = Point(x+30, y);

		putText(src, temp, tmp_pt, 2, 1, clrText, 1, 8, 0);

		src.copyTo(dst);

		imshow("src", src);

	}

	else if (event == CV_EVENT_RBUTTONDOWN)

	{
		ori.copyTo(src);

		ori.copyTo(dst);

		imshow("src", src);



		while (!points.empty()) {
			points.pop_back();
		}
		n = 0;

		//{

		//	

		//	//pt = points.back();



		//	/*circle(src, pt, 2, getInverseColor(clrPoint), CV_FILLED, CV_AA, 0);



		//	sprintf(temp, "%d (%d,%d)", n, pt.x, pt.y); --n;

		//	cvGetTextSize(temp, &font, &text_size, &baseline);

		//	tmp_pt.x = bound(pt.x, 0, src->width - text_size.width);

		//	tmp_pt.y = bound(pt.y, text_size.height + baseline, src->height - 1 - baseline);

		//	cvPutText(src, temp, tmp_pt, &font, getInverseColor(clrText));*/

		//	//src.copyTo(dst);

		//	imshow("src", src);

		//}

	}

}



int GetMouseIdentify(Mat matImage , MPoint *point)

{

	//src = imread("lena.jpg", 1);

	src = matImage.clone();

	dst = src.clone();

	ori = src.clone();

	cvNamedWindow("src", 0);

	cvSetMouseCallback("src", on_mouse, (void*)point);

	imshow("src", src);

	cvWaitKey(0);

	cvDestroyAllWindows();

	src.release();

	dst.release();



	ofstream file("sample.txt");

	if (!file)

	{

		cout << "open file error!";

		return 1;

	}

	vector<Point>::iterator it = points.begin();

	for (; it != points.end(); ++it)

	{

		file << it->x << ',' << it->y << endl;

	}

	file << endl;

	file.close();

	return 0;

}
