//#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/stitching/stitcher.hpp"


using namespace std;
using namespace cv;

Mat Resize(Mat src){
	Mat dst;
//	resize(src, dst, Size(), 0.5, 0.5, INTER_CUBIC); // upscale 2x
	// or
	resize(src, dst, Size(720, 480), 0, 0, INTER_CUBIC);
	return dst;
}


void stichpic(){
	bool try_use_gpu = false;
	vector<Mat> imgs;
	string result_name = "result.jpg";
	Mat img=imread("6.jpg");
//	img = Resize(img);

	imgs.push_back(img);
	img=imread("7.jpg");
//	img = Resize(img);

	imgs.push_back(img);
//	img=imread("8.jpg");
//	imgs.push_back(img);

	Mat pano;
	Stitcher stitcher = Stitcher::createDefault(try_use_gpu);
	Stitcher::Status status = stitcher.stitch(imgs, pano);

	if (status != Stitcher::OK)
		cout << "Can't stitch images, error code = " << int(status) << endl;

//	imwrite(result_name, pano);
}


#include <stdio.h>
#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"


/** @function main */
void stichpic2()
{

// Load the images
 Mat image1= imread("5.jpg");
 Mat image2= imread("4.jpg");

 //image1 = Resize(image1);
 //image2 = Resize(image2);

 Mat gray_image1;
 Mat gray_image2;
 // Convert to Grayscale
 cvtColor( image1, gray_image1, CV_RGB2GRAY );
 cvtColor( image2, gray_image2, CV_RGB2GRAY );

 //imshow("first image",image2);
 //imshow("second image",image1);

if( !gray_image1.data || !gray_image2.data )
 { std::cout<< " --(!) Error reading images " << std::endl;}

//-- Step 1: Detect the keypoints using SURF Detector
 int minHessian = 400;

SurfFeatureDetector detector( minHessian );

std::vector< KeyPoint > keypoints_object, keypoints_scene;

detector.detect( gray_image1, keypoints_object );
 detector.detect( gray_image2, keypoints_scene );

//-- Step 2: Calculate descriptors (feature vectors)
 SurfDescriptorExtractor extractor;

Mat descriptors_object, descriptors_scene;

extractor.compute( gray_image1, keypoints_object, descriptors_object );
 extractor.compute( gray_image2, keypoints_scene, descriptors_scene );

//-- Step 3: Matching descriptor vectors using FLANN matcher
 FlannBasedMatcher matcher;
 std::vector< DMatch > matches;
 matcher.match( descriptors_object, descriptors_scene, matches );

double max_dist = 0; double min_dist = 100;

//-- Quick calculation of max and min distances between keypoints
 for( int i = 0; i < descriptors_object.rows; i++ )
 { double dist = matches[i].distance;
 if( dist < min_dist ) min_dist = dist;
 if( dist > max_dist ) max_dist = dist;
 }

printf("-- Max dist : %f \n", max_dist );
 printf("-- Min dist : %f \n", min_dist );

//-- Use only "good" matches (i.e. whose distance is less than 3*min_dist )
 std::vector< DMatch > good_matches;

for( int i = 0; i < descriptors_object.rows; i++ )
 { if( matches[i].distance < 3*min_dist )
 { good_matches.push_back( matches[i]); }
 }
 std::vector< Point2f > obj;
 std::vector< Point2f > scene;

for( int i = 0; i < good_matches.size(); i++ )
 {
 //-- Get the keypoints from the good matches
 obj.push_back( keypoints_object[ good_matches[i].queryIdx ].pt );
 scene.push_back( keypoints_scene[ good_matches[i].trainIdx ].pt );
 }

// Find the Homography Matrix
 Mat H = findHomography( obj, scene, CV_RANSAC );
 // Use the Homography Matrix to warp the images
 cv::Mat result;
 warpPerspective(image1,result,H,cv::Size(image1.cols+image2.cols,image1.rows));
 cv::Mat half(result,cv::Rect(0,0,image2.cols,image2.rows));
 image2.copyTo(half);
 //imshow( "Result", result );

 //waitKey(0);
 }

