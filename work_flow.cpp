// obj_tracking.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>  
#include "highgui/highgui.hpp" 
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>


using namespace cv;
using namespace std;


// Convert to string
#define SSTR( x ) static_cast< std::ostringstream & >( \
( std::ostringstream() << std::dec << x ) ).str()


void OptimizeSeam(Mat& img1, Mat& trans, Mat& dst);
void get_four_points(string path, vector<Point2f> &four_points);
void warp_and_stitch(Mat &img_dst, Mat &img_left, Mat &img_right, Mat &H_left, Mat &H_right);

int main(int argc, char **argv)
{
	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);

	cameraMatrix.at<double>(0, 0) = 489.250107 * 2;
	cameraMatrix.at<double>(0, 2) = 326.599152 * 2;
	cameraMatrix.at<double>(1, 1) = 468.630164 * 2;
	cameraMatrix.at<double>(1, 2) = 185.832187 * 2;

	Mat distCoeffs = Mat::zeros(5, 1, CV_64F);
	distCoeffs.at<double>(0, 0) = -0.430856;
	distCoeffs.at<double>(1, 0) = 0.183290;
	distCoeffs.at<double>(2, 0) = 0;
	distCoeffs.at<double>(3, 0) = 0;
	distCoeffs.at<double>(4, 0) = 0;
	// List of tracker types in OpenCV 3.2
	// NOTE : GOTURN implementation is buggy and does not work.
	string trackerTypes[6] = { "BOOSTING", "MIL", "KCF", "TLD","MEDIANFLOW", "GOTURN" };
	// vector <string> trackerTypes(types, std::end(types));

	// Create a tracker
	string trackerType = trackerTypes[2];

	Ptr<Tracker> tracker;

#if (CV_MINOR_VERSION < 3)
	{
		tracker = Tracker::create(trackerType);
	}
#else
	{
		if (trackerType == "BOOSTING")
			tracker = TrackerBoosting::create();
		if (trackerType == "MIL")
			tracker = TrackerMIL::create();
		if (trackerType == "KCF")
			tracker = TrackerKCF::create();
		if (trackerType == "TLD")
			tracker = TrackerTLD::create();
		if (trackerType == "MEDIANFLOW")
			tracker = TrackerMedianFlow::create();
		if (trackerType == "GOTURN")
			tracker = TrackerGOTURN::create();
	}
#endif
	// Read videos
	VideoCapture video1("C:/Users/dell/Desktop/素材/video1.avi"); //left
	VideoCapture video2("C:/Users/dell/Desktop/素材/video2.avi"); //right

	// Exit if videos is not opened
	if (!video1.isOpened())
	{
		cout << "Could not read video1 file" << endl;
		return 1;

	}
	if (!video2.isOpened())
	{
		cout << "Could not read video2 file" << endl;
		return 1;

	}

	// Read first frame
	Mat frame1, frame2;
	bool ok1 = video1.read(frame1);
	bool ok2 = video2.read(frame2);

	//Undistort images
	Mat frameCalibration1, map1_1, map2_1;
	Mat frameCalibration2, map1_2, map2_2;

	initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, frame1.size(), 1, frame1.size(), 0), frame1.size(), CV_16SC2, map1_1, map2_1);
	initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), getOptimalNewCameraMatrix(cameraMatrix, distCoeffs, frame2.size(), 1, frame2.size(), 0), frame2.size(), CV_16SC2, map1_2, map2_2);
	remap(frame1, frameCalibration1, map1_1, map2_1, INTER_LINEAR);
	remap(frame2, frameCalibration2, map1_2, map2_2, INTER_LINEAR);


	// Destination image. The aspect ratio of the book is 3/4
	Size size(500, 400);
	Mat im_dst = Mat::zeros(size, CV_8UC3);


	// Create a vector of destination points.
	vector<Point2f> pts_dst;


	int gap_right = size.width * 0.266;
	int gap_left = size.width * 0.332;
	//cout << size.width << ' ' <<gap_left << ' ' << gap_right << endl;
	pts_dst.push_back(Point2f(size.width-gap_right, 0));
	pts_dst.push_back(Point2f(size.width-gap_right, size.height-1));
	pts_dst.push_back(Point2f(gap_left, size.height - 1));
	pts_dst.push_back(Point2f(gap_left, 0));


	
	// get four points and Calculate the homography 
	vector<Point2f> pts1_src, pts2_src;  
	get_four_points("E:/cplusplus_project/Perspective Correction/Perspective Correction/img1_four_points.txt", pts1_src);
	get_four_points("E:/cplusplus_project/Perspective Correction/Perspective Correction/img2_four_points.txt", pts2_src);
	Mat h1 = findHomography(pts1_src, pts_dst);
	Mat h2 = findHomography(pts2_src, pts_dst);
	waitKey();

	// Warp source image to destination
	warp_and_stitch(im_dst, frameCalibration2, frameCalibration1, h2, h1);

	// Define initial boundibg box
	Rect2d bbox(287, 23, 86, 320);

	// Uncomment the line below to select a different bounding box
	bbox = selectROI(im_dst, false);

	// Display bounding box.
	rectangle(im_dst, bbox, Scalar(255, 0, 0), 2, 1);
	imshow("Tracking", im_dst);

	tracker->init(im_dst, bbox);
/**/


/**/
	while (video1.read(frame1) && video2.read(frame2) )
	{
		remap(frame1, frameCalibration1, map1_1, map2_1, INTER_LINEAR);
		remap(frame2, frameCalibration2, map1_2, map2_2, INTER_LINEAR);
		warp_and_stitch(im_dst, frameCalibration2, frameCalibration1, h2, h1);
		// Start timer
		double timer = (double)getTickCount();

		//if tracking failed
		//rebuilt bbox

		// Update the tracking result
		bool ok = tracker->update(im_dst, bbox);

		// Calculate im_dsts per second (FPS)
		float fps = getTickFrequency() / ((double)getTickCount() - timer);

		if (ok)
		{
			// Tracking success : Draw the tracked object
			rectangle(im_dst, bbox, Scalar(255, 0, 0), 2, 1);
			cout << "position:" << bbox.tl() << endl;
		}
		else
		{
			// Tracking failure detected.
			putText(im_dst, "Tracking failure detected", Point(100, 80), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(0, 0, 255), 2);
		}

		// Display tracker type on im_dst
		putText(im_dst, trackerType + " Tracker", Point(100, 20), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

		// Display FPS on im_dst
		putText(im_dst, "FPS : " + SSTR(int(fps)), Point(100, 50), FONT_HERSHEY_SIMPLEX, 0.75, Scalar(50, 170, 50), 2);

		// Display im_dst.
		imshow("Tracking", im_dst);

		// Exit if ESC pressed.
		int k = waitKey(1);
		if (k == 27)
		{
			break;
		}

	}
	


}





void get_four_points(string path, vector<Point2f> &four_points) {
	fstream f;
	f.open(path);
	Point2f temp;
	for (int j = 0; j < 4; j++) {
		f >> temp.x >> temp.y;
		four_points.push_back(temp);
	}
	//cout << four_points << endl;
	f.close();
}


void warp_and_stitch(Mat &img_dst, Mat &img_left, Mat &img_right, Mat &H_left, Mat &H_right) {
	Mat img_left_correct, img_right_correct;


	warpPerspective(img_left, img_left_correct, H_left, img_dst.size());
	warpPerspective(img_right, img_right_correct, H_right, img_dst.size());

	
	img_right_correct.copyTo(img_dst(Rect(0, 0, img_right_correct.cols, img_right_correct.rows)));
	img_left_correct.copyTo(img_dst(Rect(0, 0, img_left_correct.cols, img_left_correct.rows)));
	OptimizeSeam(img_left_correct, img_right_correct, img_dst);
	//imshow("left", img_left_correct);
	//imshow("right", img_right_correct);
	//imshow("img_dst", img_dst);

}

//优化两图的连接处，使得拼接自然
void OptimizeSeam(Mat& img1, Mat& trans, Mat& dst)
{

	int start = dst.cols * 0.6;//开始位置，即重叠区域的左边界  

	double processWidth = img1.cols - start;//重叠区域的宽度  
	int rows = dst.rows;
	int cols = img1.cols; //注意，是列数*通道数
	double alpha = 1;//img1中像素的权重  
	for (int i = 0; i < rows; i++)
	{
		uchar* p = img1.ptr<uchar>(i);  //获取第i行的首地址
		uchar* t = trans.ptr<uchar>(i);
		uchar* d = dst.ptr<uchar>(i);
		for (int j = start; j < cols; j++)
		{
			//如果遇到图像trans中无像素的黑点，则完全拷贝img1中的数据
			if (t[j * 3] == 0 && t[j * 3 + 1] == 0 && t[j * 3 + 2] == 0)
			{
				alpha = 1;
			}
			else
			{
				//img1中像素的权重，与当前处理点距重叠区域左边界的距离成正比，实验证明，这种方法确实好  
				alpha = (processWidth - (j - start)) / processWidth;
			}

			d[j * 3] = p[j * 3] * alpha + t[j * 3] * (1 - alpha);
			d[j * 3 + 1] = p[j * 3 + 1] * alpha + t[j * 3 + 1] * (1 - alpha);
			d[j * 3 + 2] = p[j * 3 + 2] * alpha + t[j * 3 + 2] * (1 - alpha);

		}
	}

}







