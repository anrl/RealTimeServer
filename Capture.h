#include <opencv2/opencv.hpp>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>

#include <cv.h>
#include <highgui.h>

using namespace cv;

string getFrame(){
	int cameraNumber = 1;
//	VideoWriter writer("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(640, 480));

	VideoCapture camera;
	camera.open(cameraNumber);
    if(!camera.isOpened()) {
        std::cerr<<"Error opening camera"<<std::endl;
        exit(1);
    }

//    int frameCount=0;
//    vector<int> compression_params;
//    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
//    compression_params.push_back(100);
//	while(true) {
		Mat cameraFrame;
		camera>>cameraFrame;
		Mat output = cameraFrame(Rect(0,0,100,100));
		std::string img(output.begin<unsigned char>(), output.end<unsigned char>());
		std::cout<<img.size()<<std::endl;
		return img;
//	}

}
void MyCapture() {

	int cameraNumber = 0;
	VideoWriter writer("VideoTest.avi", CV_FOURCC('M', 'J', 'P', 'G'), 25.0, Size(640, 480));

	VideoCapture camera;
	camera.open(cameraNumber);
    if(!camera.isOpened()) {
        std::cerr<<"Error opening camera"<<std::endl;
        exit(1);
    }
//	camera.set(CV_CAP_PROP_FRAME_WIDTH,1080);
//	camera.set(CV_CAP_PROP_FRAME_HEIGHT,720);

    int frameCount=0;
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
    compression_params.push_back(100);
	while(true) {
		Mat cameraFrame;

		std::stringstream temp;
		temp << frameCount;
		string str = temp.str();

		string frameName = "Frame"+str+".jpg";
		std::cout<<frameName<<std::endl;
		camera>>cameraFrame;
//		Mat cameraFrame = cameraFrame(Rect(0,0,100,100));
//		imwrite(frameName, cameraFrame, compression_params);
//		imwrite(frameName, cameraFrame, compression_params);

		writer<<cameraFrame;
		if (cameraFrame.empty()){
			std::cerr<<"No frame read from camera"<<std::endl;
			exit(1);
		}
		imshow("Camera",cameraFrame);

		char keypress = cv::waitKey(20);
		if (keypress==27) {
			break;
		}
		frameCount++;
	}
}


void MyCapture2() {
	CvCapture* capture = NULL;
	capture = cvCreateCameraCapture( 0 );

	IplImage *frames = cvQueryFrame(capture);

	// get a frame size to be used by writer structure
	CvSize size = cvSize (
		(int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH),

		(int)cvGetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT)
	);

	// declare writer structure
	// use FOURCC ( Four Character Code ) MJPG, the motion jpeg codec
	// output file is specified by first argument
	CvVideoWriter *writer = cvCreateVideoWriter(
		"myVideo.avi",
		CV_FOURCC('M','J','P','G'),
		30, // set fps
		size
	);
	//Create a new window
	cvNamedWindow( "Recording ...press ESC to stop !", CV_WINDOW_AUTOSIZE );
	// show capture in the window and record to a file
	// record until user hits ESC key
	while(1) {

	 frames = cvQueryFrame( capture );
	if( !frames ) break;
		cvShowImage( "Recording ...press ESC to stop !", frames );
		cvWriteFrame( writer, frames );

		char c = cvWaitKey(33);
		if( c == 27 ) break;
	}
	cvReleaseVideoWriter( &writer );
	cvReleaseCapture ( &capture );
	cvDestroyWindow ( "Recording ...press ESC to stop !");

}
