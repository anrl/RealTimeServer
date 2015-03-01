/*
 * imageHash.cpp
 *
 *  Created on: 2014-04-16
 *      Author: frye
 */

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <pthread.h>

#define cimg_use_jpeg
#include "CImg.h"

using namespace std;
using namespace cimg_library;

#if defined( _MSC_VER) || defined(_BORLANDC_)
typedef unsigned _uint64 ulong64;
typedef signed _int64 long64;
#else
typedef unsigned long long ulong64;
typedef signed long long long64;
#endif

CImg<float>* ph_dct_matrix(const int N){
    CImg<float> *ptr_matrix = new CImg<float>(N,N,1,1,1/sqrt((float)N));
    const float c1 = sqrt(2.0/N);
    for (int x=0;x<N;x++){
	for (int y=1;y<N;y++){
	    *ptr_matrix->data(x,y) = c1*cos((cimg::PI/2/N)*y*(2*x+1));
	}
    }
    return ptr_matrix;
}

/*int *imageHash(void* ptr){
	const char * buffer = (const char *) ptr;
	CImg<uint8_t> src(buffer, 160, 160, 1, 3);
    CImg<float> meanfilter(7,7,1,1,1);
    CImg<float> img;
    if (src.spectrum() == 3){
        img = src.RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else if (src.spectrum() == 4){
	int width = img.width();
        int height = img.height();
        int depth = img.depth();
	img = src.crop(0,0,0,0,width-1,height-1,depth-1,2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else {
	img = src.channel(0).get_convolve(meanfilter);
    }

    img.resize(32,32);
    CImg<float> *C  = ph_dct_matrix(32);
    CImg<float> Ctransp = C->get_transpose();
    CImg<float> dctImage = (*C)*img*Ctransp;
    CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;
    float median = subsec.median();
    ulong64 one = 0x0000000000000001;
    ulong64 hash = 0x0000000000000000;
    for (int i=0;i< 64;i++){
    	float current = subsec(i);
        if (current > median)
        	hash |= one;
        one = one << 1;
    }
//  	cout<<hash<<endl;
    delete C;

    return 0;
}*/

#include <cv.h>
#include <highgui.h>
using namespace cv;
void test(){
	unsigned char buff[30000], buff2[30000];
	Mat img;
	img = imread("split.jpg",CV_LOAD_IMAGE_COLOR);
	std::vector<uchar> array;
	array.assign(img.datastart, img.dataend);
	memcpy(buff, array.data(), array.size());

	for(int i=0;i<30000;i++){
		int no = i/3;
		switch (i%3){
		case 0:
			buff2[20000+no] = buff[i];
			break;
		case 1:
			buff2[10000+no] = buff[i];
			break;
		case 2:
			buff2[no] = buff[i];
			break;
		}
	}
	CImg<uint8_t> src(buff2, 100, 100, 1, 3);
	src.display();

}

void imageHash(Mat fr){
	Mat frame;
	ulong64 hash;
	double factor = 0.5;
	int width, height;
	if (frame.cols>320) {
		factor = 320.0/frame.cols;
		resize(frame, frame, Size(), factor, factor, INTER_AREA);
		width = 320;
		height = 240;
	}
	else{
		width = frame.cols;
		height = frame.rows;
	}
	int size = width * height * 3;
	unsigned char buff[size];
	int section = size / PIECE_NUM;
	int section_width = width / PIECE_NUM;
	int colorsection = section / 3;
	std::vector<uchar> array;
	array.assign(frame.datastart, frame.dataend);

	for(int i=0;i<size;i++){
		int no = i/3;
		int rowPos = no % width;
		int colPos = no / width;
		int sectionNo = rowPos / section_width;
		int sectionPos = no % section_width;
//		int colorNo = i/3;
		switch (i%3){
		case 0:
			buff[section*sectionNo + colorsection*2 + colPos*section_width + sectionPos] = array[i];
			break;
		case 1:
			buff[section*sectionNo + colorsection + colPos*section_width + sectionPos] = array[i];
			break;
		case 2:
			buff[section*sectionNo + colPos*section_width + sectionPos] = array[i];
			break;
		}
	}

	for(int i=0;i<PIECE_NUM;i++){
		CImg<uint8_t> src(buff+i*section, section_width, height, 1, 3);
		CImg<float> meanfilter(7,7,1,1,1);
		CImg<float> img;
		if (src.spectrum() == 3){
			img = src.RGBtoYCbCr().channel(0).get_convolve(meanfilter);
		} else if (src.spectrum() == 4){
		int width = img.width();
			int height = img.height();
			int depth = img.depth();
		img = src.crop(0,0,0,0,width-1,height-1,depth-1,2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
		} else {
		img = src.channel(0).get_convolve(meanfilter);
		}

		img.resize(32,32);

		CImg<float> *C  = ph_dct_matrix(32);
		CImg<float> Ctransp = C->get_transpose();
		CImg<float> dctImage = (*C)*img*Ctransp;
		CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;

		float median = subsec.median();
		ulong64 one = 0x0000000000000001;
		hash = 0x0000000000000000;
		for (int i=0;i< 64;i++){
		float current = subsec(i);
			if (current > median)
			hash |= one;
		one = one << 1;
		}
//		cout<<hash<<endl;
		delete C;
	}
    cout << "thread end"<<endl;
}

int image_Hash(unsigned char* buffer, ulong64 &hash, double factor){
	CImg<uint8_t> src(buffer, 80*factor, 480*factor, 1, 3);

    CImg<float> meanfilter(7,7,1,1,1);
    CImg<float> img;
    if (src.spectrum() == 3){
        img = src.RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else if (src.spectrum() == 4){
	int width = img.width();
        int height = img.height();
        int depth = img.depth();
	img = src.crop(0,0,0,0,width-1,height-1,depth-1,2).RGBtoYCbCr().channel(0).get_convolve(meanfilter);
    } else {
	img = src.channel(0).get_convolve(meanfilter);
    }

    img.resize(32,32);


    CImg<float> *C  = ph_dct_matrix(32);
    CImg<float> Ctransp = C->get_transpose();

    CImg<float> dctImage = (*C)*img*Ctransp;

    CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;

    float median = subsec.median();
    ulong64 one = 0x0000000000000001;
    hash = 0x0000000000000000;
    for (int i=0;i< 64;i++){
	float current = subsec(i);
        if (current > median)
	    hash |= one;
	one = one << 1;
    }

  	cout<<hash<<endl;
    delete C;

    return 0;
}
