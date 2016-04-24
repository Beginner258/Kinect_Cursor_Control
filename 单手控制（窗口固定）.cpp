/*
	��ȡ����ֿ��ơ����˴���̶��˲������ڵ�λ�ã�����ʵʱ���´�С��λ��

��ע��	�������ƶ���ֵ����ȿռ�����أ�Ϊ1������ʱ�������̶ȿ��Խ��ܣ����Ǿ��Ȳ����ߣ���Ϊ0ʱ�������㹻�����Ƕ���̫����.
	Ҳ��ת��Ϊ�ò�ɫ�ռ�����������������һЩ����Ϊ��ɫ�ռ�ķֱ��ʽϸߣ�������ȿռ����ɫ�ռ��ת�����Ǻܾ�ȷ.
*/


#include <iostream>
#include <cstdio>
#include <Kinect.h>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\imgproc.hpp>
#include <math.h>

using	namespace	std;
using	namespace	cv;

const	double	OK_LEVEL = 0.15;	//�жϸ����Ƿ���һ��ƽ���ϵ��ݴ�ֵ,��λ��
const	int	HAND_UP = 150;		//�����Ͽ��ܴ���ָ������򣬵�λ����
const	int	HAND_LEFT_RIGHT = 100;	//�������ҿ��ܴ���ָ������򣬵�λ����
const	int	OK_MOUSE = 0;		//��꿪ʼ�ƶ�����ֵ��Խ��Խ�ȶ���ԽСԽ��ȷ
Vec3b	COLOR_TABLE[] = { Vec3b(255,0,0),Vec3b(0,255,0),Vec3b(0,0,255),Vec3b(255,255,255),Vec3b(0,0,0) };
enum { BLUE, GREEN, RED, WHITE, BLACK };

bool	depth_rage_check(int, int, int, int);
bool	level_check(const CameraSpacePoint &, const CameraSpacePoint &);
bool	distance_check(const CameraSpacePoint &, const CameraSpacePoint &);
bool	check_new_point(DepthSpacePoint &, DepthSpacePoint &, int, int);
void	draw_line(Mat &, const DepthSpacePoint &, DepthSpacePoint &);
void	draw_body(Mat &, BYTE *, int, int);
void	draw_Hand(Mat &, const DepthSpacePoint &);
void	draw_circle(Mat &, int, int);
int	main(void)
{
	IKinectSensor	* mySensor = nullptr;
	GetDefaultKinectSensor(&mySensor);
	mySensor->Open();

	IFrameDescription	* myDescription = nullptr;

	int	depthHeight = 0, depthWidth = 0;
	IDepthFrameSource	* myDepthSource = nullptr;
	IDepthFrameReader	* myDepthReader = nullptr;
	IDepthFrame		* myDepthFrame = nullptr;
	mySensor->get_DepthFrameSource(&myDepthSource);
	myDepthSource->get_FrameDescription(&myDescription);
	myDescription->get_Height(&depthHeight);
	myDescription->get_Width(&depthWidth);
	myDepthSource->OpenReader(&myDepthReader);			//����ΪDepth֡��׼����ֱ�ӿ���Reader


	IBodyIndexFrameSource	* myBodyIndexSource = nullptr;
	IBodyIndexFrameReader	* myBodyIndexReader = nullptr;
	IBodyIndexFrame		* myBodyIndexFrame = nullptr;
	mySensor->get_BodyIndexFrameSource(&myBodyIndexSource);
	myBodyIndexSource->OpenReader(&myBodyIndexReader);		//����ΪBodyIndex֡��׼��,ֱ�ӿ���Reader


	IBodyFrameSource	* myBodySource = nullptr;
	IBodyFrameReader	* myBodyReader = nullptr;
	IBodyFrame		* myBodyFrame = nullptr;
	mySensor->get_BodyFrameSource(&myBodySource);
	myBodySource->OpenReader(&myBodyReader);			//����ΪBody֡��׼����ֱ�ӿ���Reader

	ICoordinateMapper	* myMapper = nullptr;
	mySensor->get_CoordinateMapper(&myMapper);			//Maper��׼��

	DepthSpacePoint		front = { 0,0 };				//������¼��һ������λ��
	DepthSpacePoint		depthUpLeft = { 1,1 };			//�������ڵ����ϽǺ����½ǣ�Ҫע��������X��X��Y��Y�Ĳ����Ϊ���������Բ��ܶ�Ϊ0
	DepthSpacePoint		depthDownRight = { 0,0 };
	bool	gotEdge = false;
	while (1)
	{
		while (myDepthReader->AcquireLatestFrame(&myDepthFrame) != S_OK);
		UINT	depthBufferSize = 0;
		UINT16	* depthBuffer = nullptr;;
		myDepthFrame->AccessUnderlyingBuffer(&depthBufferSize, &depthBuffer);		//��ȡDepth����

		while (myBodyIndexReader->AcquireLatestFrame(&myBodyIndexFrame) != S_OK);	//��ȡBodyIndex����
		UINT	bodyIndexBufferSize = 0;
		BYTE	* bodyIndexBuffer = nullptr;
		myBodyIndexFrame->AccessUnderlyingBuffer(&bodyIndexBufferSize, &bodyIndexBuffer);
		Mat	img(depthHeight, depthWidth, CV_8UC3);
		draw_body(img, bodyIndexBuffer, depthHeight, depthWidth);

		while (myBodyReader->AcquireLatestFrame(&myBodyFrame) != S_OK);			//��ȡBody����
		int	bodyBufferSize = 0;
		myBodySource->get_BodyCount(&bodyBufferSize);
		IBody	** bodyArray = new IBody *[bodyBufferSize];
		for (int i = 0; i < bodyBufferSize; i++)
			bodyArray[i] = nullptr;
		myBodyFrame->GetAndRefreshBodyData(bodyBufferSize, bodyArray);

		for (int i = 0; i < bodyBufferSize; i++)					//����6����
		{
			BOOLEAN		result = false;
			if (bodyArray[i]->get_IsTracked(&result) == S_OK && result)		//���ؽڵ��������ʽ��ʼ����
			{
				Joint	jointArray[JointType_Count];
				bodyArray[i]->GetJoints(JointType_Count, jointArray);

				//����Ϊȷ���������ڵ����ϽǺ����½ǣ������ֵĴ���λ�ò�ͬ����Head�ؽڵ�Neck�ؽڵľ�����Ϊ��λ����.����õ�������ݣ�����Ҫת��
				if (!gotEdge)
					if (jointArray[JointType_Neck].TrackingState == TrackingState_Tracked && jointArray[JointType_Head].TrackingState == TrackingState_Tracked)
					{
						CameraSpacePoint	cameraNeck = jointArray[JointType_Neck].Position;
						CameraSpacePoint	cameraHead = jointArray[JointType_Head].Position;
						double	unite = sqrt(pow(cameraNeck.X - cameraHead.X, 2) + pow(cameraNeck.Y - cameraHead.Y, 2) + pow(cameraNeck.Z - cameraHead.Z, 2));
						CameraSpacePoint	cameraUpLeft = { cameraNeck.X + unite * 0.5,cameraNeck.Y + unite * 3,cameraNeck.Z };
						CameraSpacePoint	cameraDownRight = { cameraNeck.X + unite * 4,cameraNeck.Y + unite,cameraNeck.Z };
						myMapper->MapCameraPointToDepthSpace(cameraUpLeft, &depthUpLeft);
						myMapper->MapCameraPointToDepthSpace(cameraDownRight, &depthDownRight);
						gotEdge = true;
					}



				//ָ��ʶ��,��¼����Ǹ���ָ��ָ��
				DepthSpacePoint		highestPoint = { depthWidth - 1,depthHeight - 1 };
				if (jointArray[JointType_HandRight].TrackingState == TrackingState_Tracked)
				{
					CameraSpacePoint	cameraHandRight = jointArray[JointType_HandRight].Position;
					DepthSpacePoint		depthHandRight;
					myMapper->MapCameraPointToDepthSpace(cameraHandRight, &depthHandRight);

					//��ʼ����������������
					for (int i = depthHandRight.Y; i > depthHandRight.Y - HAND_UP; i--)
						for (int j = depthHandRight.X - HAND_LEFT_RIGHT; j < depthHandRight.X + 100; j++)	//ȷ��Ҫ���ķ�Χ
						{
							if (!depth_rage_check(j, i, depthWidth, depthHeight))				//�кϷ�
								continue;
							int	index = i * depthWidth + j;
							CameraSpacePoint	cameraTemp;
							DepthSpacePoint		depthTemp = { j,i };
							myMapper->MapDepthPointToCameraSpace(depthTemp, depthBuffer[index], &cameraTemp);

							if (bodyIndexBuffer[index] > 5 || (bodyIndexBuffer[index] <= 5 && !level_check(cameraHandRight, cameraTemp)))	//���������Ƿ���������(ָ���Ϸ�һ��)�������������嵫�Ǻ��ֲ���ͬһƽ�棨�أ�
							{
								bool	flag = true;
								for (int k = 1; k <= 5; k++)	//��ʱ��˵���������5�����ض��������壬�Һ�������ͬһƽ��,�Ҿ������
								{
									int	index_check = (i + k) * depthWidth + j;
									depthTemp.X = j;
									depthTemp.Y = i + k;
									myMapper->MapDepthPointToCameraSpace(depthTemp, depthBuffer[index_check], &cameraTemp);
									if (bodyIndexBuffer[index_check] <= 5 && level_check(cameraHandRight, cameraTemp) && distance_check(cameraHandRight, cameraTemp))
										continue;
									else
									{
										flag = false;
										break;
									}
								}
								if (flag && i < highestPoint.Y)
								{
									highestPoint.X = j;
									highestPoint.Y = i;
								}
							}
						}
				}


				int	windowWidth = (int)depthDownRight.X - (int)depthUpLeft.X;	//����������ڵĳߴ�
				int	windowHeight = (int)depthDownRight.Y - (int)depthUpLeft.Y;
				draw_line(img, depthUpLeft, depthDownRight);
				if (check_new_point(front, highestPoint, depthHeight, depthWidth))
				{
					draw_circle(img, highestPoint.X, highestPoint.Y);		//������Ե��ֹͣ��������
					if (highestPoint.X < depthUpLeft.X)
						highestPoint.X = depthUpLeft.X;
					if (highestPoint.X > depthDownRight.X)
						highestPoint.X = depthDownRight.X;
					if (highestPoint.Y > depthDownRight.Y)
						highestPoint.Y = depthDownRight.Y;
					if (highestPoint.Y < depthUpLeft.Y)
						highestPoint.Y = depthUpLeft.Y;
					int	mouseX = fabs(highestPoint.X - depthUpLeft.X);
					int	mouseY = fabs(highestPoint.Y - depthUpLeft.Y);
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 65535 * mouseX / windowWidth, 65535 * mouseY / windowHeight, 0, 0);	//���㹫ʽ��С���ڵĵ�/С���ڳߴ� = �󴰿ڵĵ�/��
					front = highestPoint;
				}
				else							//�������һ֡����ƶ��ķ���С����ֵ���򱣳���һ֡��״̬
				{
					draw_circle(img, front.X, front.Y);
					int	mouseX = fabs(front.X - depthUpLeft.X);
					int	mouseY = fabs(front.Y - depthUpLeft.Y);
					mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, 65535 * mouseX / windowWidth, 65535 * mouseY / windowHeight, 0, 0);
				}
			}
		}

		imshow("TEST", img);
		if (waitKey(30) == VK_ESCAPE)
			break;

		myDepthFrame->Release();
		myBodyIndexFrame->Release();
		myBodyFrame->Release();
		delete[] bodyArray;
	}
	myBodySource->Release();
	myBodyIndexSource->Release();
	myDepthSource->Release();
	myBodyReader->Release();
	myBodyIndexReader->Release();
	myDepthReader->Release();
	myDescription->Release();
	myMapper->Release();
	mySensor->Close();
	mySensor->Release();


	return	0;
}

void	draw_body(Mat & img, BYTE * buffer, int height, int width)
{
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			int	index = buffer[i * width + j];
			if (index <= 5)
				img.at<Vec3b>(i, j) = COLOR_TABLE[WHITE];
			else
				img.at<Vec3b>(i, j) = COLOR_TABLE[BLACK];
		}
}

void	draw_circle(Mat & img, int x, int y)
{
	Point	center = { x,y };
	circle(img, center, 5, COLOR_TABLE[GREEN], -1, 0, 0);
}

void	draw_Hand(Mat & img, const DepthSpacePoint & hand)
{
	Point	center = { (int)hand.X,(int)hand.Y };
	circle(img, center, 5, COLOR_TABLE[BLUE], -1, 0, 0);
}

void	draw_line(Mat & img, const DepthSpacePoint & UpLeft, DepthSpacePoint & DownRight)
{
	Point	a = { (int)UpLeft.X,(int)DownRight.Y };
	circle(img, a, 5, COLOR_TABLE[RED], -1, 0, 0);
	Point	b = { (int)UpLeft.X,(int)UpLeft.Y };
	circle(img, b, 5, COLOR_TABLE[GREEN], -1, 0, 0);
	Point	c = { (int)DownRight.X,(int)UpLeft.Y };
	circle(img, c, 5, COLOR_TABLE[BLUE], -1, 0, 0);
	Point	d = { (int)DownRight.X,(int)DownRight.Y };
	circle(img, d, 5, COLOR_TABLE[WHITE], -1, 0, 0);
	line(img, a, b, COLOR_TABLE[RED], 1, 8, 0);
	line(img, b, c, COLOR_TABLE[RED], 1, 8, 0);
	line(img, c, d, COLOR_TABLE[RED], 1, 8, 0);
	line(img, a, d, COLOR_TABLE[RED], 1, 8, 0);
}

bool	level_check(const CameraSpacePoint & hand, const CameraSpacePoint & temp)
{
	if (fabs(temp.Z - hand.Z) <= OK_LEVEL)
		return	true;
	return	false;
}

bool	distance_check(const CameraSpacePoint & hand, const CameraSpacePoint & temp)
{
	double	ans = sqrt(pow(hand.X - temp.X, 2) + pow(hand.Y - temp.Y, 2) + pow(hand.Z - temp.Z, 2));
	if (ans <= 0.2 && ans >= 0.06)
		return	true;
	return	false;
}

bool	depth_rage_check(int x, int y, int depthWidth, int depthHeight)
{
	if (x >= 0 && x < depthWidth && y >= 0 && y < depthHeight)
		return	true;
	return	false;
}

bool	check_new_point(DepthSpacePoint & front, DepthSpacePoint & now, int height, int width)
{
	if (now.X == width - 1 && now.Y == height - 1 && (front.X || front.Y))
		return	false;
	else	if (fabs(now.X - front.X) <= OK_MOUSE && fabs(now.Y - front.Y) <= OK_MOUSE)
		return	false;
	return	true;
}