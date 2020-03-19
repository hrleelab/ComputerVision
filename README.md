# ComputerVision
Computer Vision Projects

## Hand Gesture Recognition
* 수업명: 2019-2 컴퓨터 비전(001)
* 프로젝트명: Hand Gesture Recognition Using OpenCV
* 팀원: 이하람, 박동연
* 설명: 웹캠을 이용해 받아온 실시간 프레임에서 피부색, 손가락 개수, 손가락 사이의 각도 등을 이용하여 핸드 제스쳐 인식이 가능하도록 한다.
#### 구조 및 순서
1. 웹캠에서 프레임 받아오기
```c++
...
VideoCapture cap;
int deviceID = 0; // 0 = open default camera
int apiID = cv::CAP_ANY; // 0 = autodetect default API
cap.open(deviceID + apiID); // open selected camera using selected API
if (!cap.isOpened()) { return -1; } // Exception Handling
for (;;) {
      cap.read(frame); // wait for a new frame from camera and store it into 'frame'
      ...
}
```
2. 피부색 검출
* *detectSkin(Mat& original, Mat& detect)*
  * 컬러 채널 변환 : RGB -> YCrCb
  * 피부색 범위에 따라 이진화
  * 노이즈 제거 : Median Blur
  * 작은 구멍 막기 : Morphology Closing
  * 외곽선 부드럽게 보정 : Morphology Opening
3. 손바닥 중심점 좌표 구하기
* *getHandCenter(const Mat& mask, double& radius)*
  * 거리 변환 행렬 구하기 : *cv::distanceTransform(~)*
  * 객체 안의 한 점에서 다른 점까지의 거리가 가장 큰 점(손바닥 중심점)과 그 거리(손바닥 반지름)를 구하기 : *cv::minMaxIdx(~)*
4. 손가락 개수 구하기(손목 포함)
* *getFingerPoint(const Mat& mask, Point center, double radius, double scale)*
  * 손바닥 중심에서 반지름보다 큰 원을 그리기 : *cv::circle(~)*
  * 원의 외곽선만 추출하기 : *cv::findContours(~)*
  * 원의 외곽선과 손 객체가 만나는 지점을 구하기 : 0->255로 바뀌는 지점 찾기
5. 손가락 사이의 각도 구하기
* *getAngle(Point& a1, Point& a2, Point& b1, Point& b2)*
  * 원의 외각선과 손 객체가 만나는 지점에서 각각 손바닥 중심에 이은 직선들을 구하고, *getAngle* 함수를 호출하여 이웃한 직선들의 각 구하기
6. 손가락 개수와 손가락 사이의 각도들의 분산을 이용한 핸드 제스처 판단
* *getHandGesture(vector<Point>& points, vector<int>& angles)*
  * 1차 : 손가락 개수에 따라 나눈다.
  * 2차 : 손가락 사이 각도나 이 각도들의 분산에 따라 핸드 제스쳐를 판단한다.
