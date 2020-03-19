/* 
* 수업명: 2019-2 컴퓨터 비전(001)
* 프로젝트명: Hand Gesture Detection
* 팀원: 이하람, 박동연
* 설명: OpenCV를 사용하여 웹캠으로 실시간 피부색 검출과 이를 사용한 핸드 제스쳐 인식이 가능하도록 한다
*/

#include "pch.h"
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/ximgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <cmath>
#include <conio.h>
#include <windows.h>

using namespace cv;
using namespace std;

void detectSkin(Mat& original, Mat& detect);
void drawHandGesture(Mat &original, Mat &mask);
Point getHandCenter(const Mat& mask, double& radius);
vector<Point> getFingerPoint(const Mat& mask, Point center, double radius, double scale);
double getAngle(Point &a1, Point &a2, Point &b1, Point &b2);
string getHandGesture(vector<Point> &points, vector<int> &angles);
int getVariance(vector<int> &v, int mean);
void cursorEvent(int fingerNum, int x, int y);

int main(int, char**) {
   Mat original, detect;
   VideoCapture cap; // Initialize VideoCapture

   int deviceID = 0; // 0 = open default camera
   int apiID = cv::CAP_ANY; // 0 = autodetect default API

   // open selected camera using selected API
   cap.open(deviceID + apiID);

   // Exception Handling
   if (!cap.isOpened()) {
      cerr << "ERROR! Unable to open camera\n";
      return -1;
   }

   // Grap and write LOOP
   cout << "Start grabbing - Press any key to terminate : " << endl;

   for (;;) {
      // wait for a new frame from camera and store it into 'original'
      cap.read(original);

      // Exception Handling
      if (original.empty()) {
         cerr << "ERROR! Blank frame grabbed\n";
         break;
      }

	  // Invert left and right
	  flip(original, original, 1);

	  // Skin detect and Noise reduction
      detectSkin(original, detect);

	  // Find out and Draw Hand Gesture
	  drawHandGesture(original, detect);

      // show live and wait for a key with timeout long enough to show images
	  namedWindow("Live");
	  //moveWindow("Live", 0, 0);
      imshow("Live", original);

      if (waitKey(5) >= 0)
		  break;
   }

   destroyAllWindows();
   return 0;

}

// Skin detect and Noise reduction
void detectSkin(Mat &original, Mat &detect) {
	detect = original.clone();

	// Skin Color : 128 <= Cr <= 170 && 73 <= Cb <= 158
	cvtColor(detect, detect, COLOR_RGB2YCrCb); // Color Model Conversion - YCrCb is faster than RGB
	inRange(detect, Scalar(0, 85, 135), Scalar(255, 135, 180), detect); // Color Binarization - Skin is White

	// Noise reduction - Median Filter
	medianBlur(detect, detect, 7);
	Mat element = getStructuringElement(MORPH_RECT, Size(7, 7));
	morphologyEx(detect, detect, MORPH_CLOSE, element);
	morphologyEx(detect, detect, MORPH_OPEN, element);
}

// Find out and draw Hand Gesture
void drawHandGesture(Mat &original, Mat &mask) {
	// 글씨 정보 변수(putText)
	int font = FONT_HERSHEY_COMPLEX;
	double fontScale = 1;
	Scalar color(0, 255, 255);
	int thickness = 1;

	// 손바닥 중심점 그리기
	double radius;
	Point center = getHandCenter(mask, radius);
	circle(original, center, 2, Scalar(0, 255, 0), -1);

	// 손바닥 영역(원) 그리기
	circle(original, center, (int)(radius*2.0), Scalar(255, 0, 0), 2);

	// 손가락 마디(선) 그리기
	vector<Point> points = getFingerPoint(mask, center, radius, 2.0);
	vector<int> angles;
	for (int i = 0;i < points.size();i++) {
		line(original, center, points[i], Scalar(0, 255, 0), 3);
		// 바로 다음 손가락과의 각도 출력하기
		int angle = (int)getAngle(center, points[i], center, points[(i + 1) % points.size()]);
		angles.push_back(angle);
		string text = to_string(angle);
		putText(original, text, points[i], font, fontScale, color, thickness);
	}

	// 커서 이벤트 맵핑
	//cursorEvent(points.size() - 1, center.x, center.y);

	// 제스처 출력
	string text = getHandGesture(points, angles);
	putText(original, text, center, font, fontScale, color, thickness);
}

// 손바닥의 중심점과 반지름 반환
Point getHandCenter(const Mat& mask, double& radius) {
	Mat dst; //거리 변환 행렬을 저장할 변수
	distanceTransform(mask, dst, DIST_L2, 5);  //결과는 CV_32SC1 타입

	//거리 변환 행렬에서 값(거리)이 가장 큰 픽셀의 좌표와, 값을 얻어온다.
	int maxIdx[2];    //좌표 값을 얻어올 배열(행, 열 순으로 저장됨)
	minMaxIdx(dst, NULL, &radius, NULL, maxIdx, mask);   //최소값은 사용 X

	return Point(maxIdx[1], maxIdx[0]);
}

// 손목을 제거하지 않은 상태에서 손가락 개수 세기
vector<Point> getFingerPoint(const Mat& mask, Point center, double radius, double scale = 2.0) {
	vector<Point> points;

	//손가락 개수를 세기 위한 원 그리기
	Mat cImg(mask.size(), CV_8U, Scalar(0));
	circle(cImg, center, radius*scale, Scalar(255));

	//원의 외곽선을 저장할 벡터
	vector<vector<Point>> contours;
	findContours(cImg, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

	if (contours.size() == 0)   //외곽선이 없을 때 == 손 검출 X
		return points;

	//외곽선을 따라 돌며 mask의 값이 0에서 1로 바뀌는 지점 확인
	for (int i = contours[0].size() - 1; i > 0; i--) {
		Point p1 = contours[0][i - 1];
		Point p2 = contours[0][i];
		if (mask.at<uchar>(p1.y, p1.x) == 0 && mask.at<uchar>(p2.y, p2.x) > 1) {
			points.push_back(p2);
		}
	}

	return points;
}

// 점 a1,a2를 잇는 직선과 점 b1,b2를 잇는 직선 사이의 각 반환
double getAngle(Point &a1, Point &a2, Point &b1, Point &b2) {
	double dx1 = a1.x - a2.x;
	double dy1 = a1.y - a2.y;
	double dx2 = b1.x - b2.x;
	double dy2 = b1.y - b2.y;

	double rad = atan2(dy1*dx2 - dx1 * dy2, dx1*dx2 + dy1 * dy2);
	const double pi = acos(-1);
	double degree = (rad * 180) / pi;
	return abs(degree);
}

// 벡터 안 요소들의 분산을 반환
int getVariance(vector<int> &v, int mean) {
	int result = 0;
	int size = v.size();
	for (int i = 0;i < size;i++) {
		int diff = mean - v[i];
		result += (diff*diff);
	}
	result /= size;
	return result;
}

// 손가락 개수와 손가락 사이 각을 이용한 제스쳐 인식
string getHandGesture(vector<Point> &points, vector<int> &angles) {
	string result = "";
	int variance = 0;
	int fingerCount = points.size() - 1;
	switch (fingerCount) {
	case 0:
		result = "Rock"; break; // 주먹
	case 1:
		if (angles[0] >= 170)
			result = "Thumb up"; // 따봉
		else if (angles[0] >= 160)
			result = "Oath"; // 선서
		else if (angles[0] >= 150)
			result = "One"; // 손가락 하나
		else
			result = "Promise"; // 약속
		break;
	case 2:
		variance = getVariance(angles, 120);
		cout << "분산 = " << variance << endl;
		if (variance > 3000)
			result = "Korean Heart"; // 손가락 하트
		else if (variance > 2300)
			result = "Two"; // 손가락 둘
		else if (variance > 1600)
			result = "L"; // L 모양
		else if (variance > 500)
			result = "Rock Spirit"; // 락스피릿
		else
			result = "Call"; // 전화
		break;
	case 3:
		variance = getVariance(angles, 120);
		cout << "분산 = " << variance << endl;
		if (variance > 5200)
			result = "Okay";
		else
			result = "Three";
		break;
	case 4:
		result = "Four"; break;
	case 5:
		result = "Five"; break;
	default:
		result = "None"; break;
	}
	return result;
}

// 손바닥 좌표와 손가락 개수를 기반으로 한 커서 이동 및 클릭
void cursorEvent(int fingerNum, int handX, int handY) {
	int windowX, windowY;
	windowX = handX * 3;
	windowY = handY * 2.8;
	//windowX = handX;
	//windowY = handY;

	cout << "손바닥 중심점 값 : (" << handX << ", " << handY << ")" << endl;
	cout << "디스플레이 커서 값 : (" << windowX << ", " << windowY << ")" << endl;

	// 커서이동
	SetCursorPos(windowX, windowY);

	if (fingerNum == 0) {
		//주먹을 쥐면 왼쪽 마우스 클릭
		mouse_event(MOUSEEVENTF_LEFTDOWN, windowX, windowY, 0, 0);
		mouse_event(MOUSEEVENTF_LEFTUP, windowX, windowY, 0, 0);
	}
	else {
		//다른 제스쳐일 때는 바로 왼쪽 마우스를 올린 상태로 변경
		mouse_event(MOUSEEVENTF_LEFTUP, windowX, windowY, 0, 0);
	}
}