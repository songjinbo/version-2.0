
#ifndef _GET_IMAGE_H_
#define _GET_IMAGE_H_

#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include "afxmt.h"

#define MAXA 6400
#define PI 3.1415926

//存储位置和姿态信息
class Position
{
public:
	double x;
	double y;
	double z;
	double roll;
	double pitch;
	double yaw;
};

extern CEvent event_display_image;
extern CCriticalSection critical_display_image;

extern char path[MAX_PATH];
extern std::vector<cv::Mat> vec_depth;
extern std::vector<cv::Mat> vec_left;
extern std::vector<Position> vec_position;

extern cv::Mat depth_image;
extern cv::Mat left_image;
extern Position position;

//声明外部函数
UINT GetDepthAndLeft(LPVOID pParam);

#endif