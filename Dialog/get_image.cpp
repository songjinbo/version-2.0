/********************************************************************************
Author:Song Jinbo
Date:2016-7-19
Description:裁剪图片,用于验证guidance的精度和运动采集的数据
注意：每次采集的图片必须要经过这个函数的处理
这个函数同时对左相机图像和深度图像进行处理
能够处理多个文件夹
**********************************************************************************/
#include <opencv2/core/core.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <tchar.h>
#include <vector>
#include "get_image.h" 

using namespace cv;
using namespace std;

string itos(double i)
{
	stringstream ss;

	ss << i;

	return ss.str();
}

Mat depth_32S;//CV_16S显示成图片会有问题，需转化为8U

UINT GetDepthAndLeft(LPVOID pParam)
{
	//获取采集的帧数，并且动态分配数组
	char str_file_count[80];
	string command = "cmd.exe /c dir " + string(path) + "| findstr \"depth[0-9]*\.xml\" | find /v /c \"\">>" + string(path) + "file_count.txt";
	WinExec(command.data(), SW_HIDE);
	fstream file_read(string(path) + "file_count.txt", ios::in);
	file_read.getline(str_file_count, 80);
	int file_count = atoi(str_file_count);
	file_read.close();
	fstream file_clear(string(path) + "file_count.txt", ios::out); //清除里面的内容
	file_clear.close();

	if (!file_count) return 0; //代表没有文件
	int count = 0; //用来计数已经打开的文件数量


	fstream _file1, _file2;
	bool is_catch;
	//有可能陷入死循环，关键在于file_count的计数是否准确,如果file_count大于实际的帧数，那么就会陷入死循环
	for (int no = 1; count<file_count; no++)
	{
		string tmp1 = string(path) + "depth" + itos(no) + ".xml";
		string tmp2 = string(path) + "left" + itos(no) + ".xml";

		_file1.open(tmp1, ios::in);
		_file2.open(tmp2, ios::in);
		
		critical_display_image.Lock();
		if (!_file1 && !_file2) //没left和depth
		{
			_file1.close(); //不要忘记关闭，否则会出错误
			_file2.close(); //不要忘记关闭，否则会出错误
			critical_display_image.Unlock();
			continue;
		}
		else if (_file1 && _file2)//有left和depth
		{
			count++;
			_file1.close(); //不要忘记关闭，否则会出错误
			_file2.close(); //不要忘记关闭，否则会出错误
			FileStorage fs1_depth;
			FileStorage fs1_left;
			try
			{
				//可能会出现xml文件格式不正确的情况
				is_catch = 0;
				fs1_depth.open(string(path) + "depth" + itos(no) + ".xml", FileStorage::READ);
			}
			catch (std::exception const& e)
			{
				is_catch = 1;
				position.x = position.y = position.z = position.roll = position.pitch = position.yaw = 0;
				vec_position.push_back(position);
				depth_image = Mat(Scalar(0));
				vec_depth.push_back(depth_image);
			}
			if (!is_catch)
			{
				FileNode fs2_depth = fs1_depth["image"];
				fs2_depth["depth"] >> depth_image;
				depth_image.convertTo(depth_32S, CV_32SC1);
				depth_32S = depth_32S * 255 / MAXA;
				depth_32S.convertTo(depth_image, CV_8UC1);
				vec_depth.push_back(depth_image);

				fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
				position.x = double(int(position.x * 1000)) / 1000; position.y = double(int(position.y * 1000)) / 1000; position.z = double(int(position.z * 1000)) / 1000;//保留小数点后三位
				fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
				position.yaw = double(int(position.yaw * 180 / PI * 1000))/ 1000; //变成角度，保留小数点后三位
				position.roll = double(int(position.roll * 180 / PI * 1000)) / 1000;
				position.pitch = double(int(position.pitch * 180 / PI * 1000)) / 1000;
				vec_position.push_back(position);
			}
			fs1_depth.release();

			try
			{
				//可能会出现xml文件格式不正确的情况
				is_catch = 0;
				fs1_left.open(string(path) + "left" + itos(no) + ".xml", FileStorage::READ);
			} 
			catch (std::exception const& e)
			{
				is_catch = 1;
				left_image = Mat(Scalar(0));
				vec_left.push_back(left_image);
			}
			if (!is_catch)
			{
				//如果这个xml文件的格式正确，则进行下面的操作
				FileNode fs2_left = fs1_left["image"];
				fs2_left["left"] >> left_image;
				vec_left.push_back(left_image);
			}
			fs1_left.release();
		}
		else if (_file1 && !_file2)//有depth没left
		{
			count++;
			_file1.close(); //不要忘记关闭，否则会出错误
			_file2.close(); //不要忘记关闭，否则会出错误
			left_image = Mat(Scalar(0));
			vec_left.push_back(left_image);
			FileStorage fs1_depth;
			try
			{
				//读取原来的深度数据和左相机的数据
				is_catch = 0;
				fs1_depth.open(string(path) + "depth" + itos(no) + ".xml", FileStorage::READ);
			}
			catch (std::exception const& e)
			{
				is_catch = 1;
				position.x = position.y = position.z = position.roll = position.pitch = position.yaw = 0;
				vec_position.push_back(position);
				depth_image = Mat(Scalar(0));
				vec_depth.push_back(depth_image);
			}
			if (!is_catch)
			{
				FileNode fs2_depth = fs1_depth["image"];
				fs2_depth["depth"] >> depth_image;
				depth_image.convertTo(depth_32S, CV_32SC1);
				depth_32S = depth_32S * 255 / MAXA;
				depth_32S.convertTo(depth_image, CV_8UC1);
				vec_depth.push_back(depth_image);

				fs1_depth["gps"]["gps_x"] >> position.x; fs1_depth["gps"]["gps_y"] >> position.y; fs1_depth["gps"]["gps_z"] >> position.z;
				position.x = double(int(position.x * 1000)) / 1000; position.y = double(int(position.y * 1000)) / 1000; position.z = double(int(position.z * 1000)) / 1000;//保留小数点后三位
				fs1_depth["attitude"]["yaw"] >> position.yaw; fs1_depth["attitude"]["roll"] >> position.roll; fs1_depth["attitude"]["pitch"] >> position.pitch;
				position.yaw = double(int(position.yaw * 180 / PI * 1000)) / 1000; //变成角度，保留小数点后三位
				position.roll = double(int(position.roll * 180 / PI * 1000)) / 1000;
				position.pitch = double(int(position.pitch * 180 / PI * 1000)) / 1000;
				vec_position.push_back(position);
			}
			fs1_depth.release();
		}
		else
		{
			_file1.close(); //不要忘记关闭，否则会出错误
			_file2.close(); //不要忘记关闭，否则会出错误
			depth_image = Mat(Scalar(0));
			vec_depth.push_back(depth_image);
			FileStorage fs1_left;
			try
			{
				//读取原来的深度数据和左相机的数据
				is_catch = 0;
				fs1_left.open(string(path) + "left" + itos(no) + ".xml", FileStorage::READ);
			}
			catch (std::exception const& e)
			{
				is_catch = 1;
				position.x = position.y = position.z = position.roll = position.pitch = position.yaw = 0;
				vec_position.push_back(position);
				left_image = Mat(Scalar(0));
				vec_left.push_back(left_image);
			}
			if (!is_catch)
			{
				//如果这个xml文件的格式正确，则进行下面的操作
				FileNode fs2_left = fs1_left["image"];
				fs2_left["left"] >> left_image;
				vec_left.push_back(left_image);		

				fs1_left["gps"]["gps_x"] >> position.x; fs1_left["gps"]["gps_y"] >> position.y; fs1_left["gps"]["gps_z"] >> position.z;
				position.x = double(int(position.x * 1000)) / 1000; position.y = double(int(position.y * 1000)) / 1000; position.z = double(int(position.z * 1000)) / 1000;//保留小数点后三位
				fs1_left["attitude"]["yaw"] >> position.yaw; fs1_left["attitude"]["roll"] >> position.roll; fs1_left["attitude"]["pitch"] >> position.pitch;
				position.yaw = double(int(position.yaw * 180 / PI * 1000)) / 1000; //变成角度，保留小数点后三位
				position.roll = double(int(position.roll * 180 / PI * 1000)) / 1000;
				position.pitch = double(int(position.pitch * 180 / PI * 1000)) / 1000;
				vec_position.push_back(position);
			}
			fs1_left.release();
		}

		critical_display_image.Unlock();
		event_display_image.SetEvent();//触发display_image线程
		Sleep(100);
	}
	return 1;//代表返回正常值
}