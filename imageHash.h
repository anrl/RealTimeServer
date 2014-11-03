/*
 * imageHash.cpp
 *
 *  Created on: 2014-04-16
 *      Author: frye
 */

#include <stdio.h>
#include <iostream>
#include <stdint.h>
#include <time.h>
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

int64_t timespecDiff(struct timespec *timeA_p, struct timespec *timeB_p)
{
  return ((timeA_p->tv_sec * 1000000000) + timeA_p->tv_nsec) -
           ((timeB_p->tv_sec * 1000000000) + timeB_p->tv_nsec);
}

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

int *imageHash(void* ptr){
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
}

int image_Hash(const char* buffer, ulong64 &hash){
/*	FILE * pFile;
	char temp[2];
	int i=0;
	static unsigned char buffer[4096];
	pFile = fopen ("4.jpg" , "r");
	if (pFile == NULL) perror ("Error opening file");
	else
	{
		while ( ! feof (pFile) )
		{
	    	if ( fgets (temp, 2 , pFile) == NULL ) break;
	    	buffer[i] = temp[0];
			i++;
		}
		buffer[i] = '\0';
		fclose (pFile);
	}

	char file[] = "4.jpg";
	ulong64 hash;*/
//	int c=0;
//	while(buffer[c]!='\0') c++;
//	cout<<"length "<<c<<endl;
	CImg<uint8_t> src(buffer, 160, 160, 1, 3);
/*    CImg<uint8_t> src;
    try {
	src.load(file);
    } catch (CImgIOException ex){
	return -1;
    }*/
//	struct timespec start, end;
//	clock_gettime(CLOCK_MONOTONIC, &start);

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

//    clock_gettime(CLOCK_MONOTONIC, &end);
//    uint64_t timeElapsed = timespecDiff(&end, &start);
//    cout<<"first "<<timeElapsed/1000000<<endl;
//    clock_gettime(CLOCK_MONOTONIC, &start);

    img.resize(32,32);

//    clock_gettime(CLOCK_MONOTONIC, &end);
//    timeElapsed = timespecDiff(&end, &start);
//    cout<<"second "<<timeElapsed/1000000<<endl;
//    clock_gettime(CLOCK_MONOTONIC, &start);

    CImg<float> *C  = ph_dct_matrix(32);
    CImg<float> Ctransp = C->get_transpose();

    CImg<float> dctImage = (*C)*img*Ctransp;

    CImg<float> subsec = dctImage.crop(1,1,8,8).unroll('x');;

//    clock_gettime(CLOCK_MONOTONIC, &end);
//    timeElapsed = timespecDiff(&end, &start);
//    cout<<"third "<<timeElapsed/1000000<<endl;
//    clock_gettime(CLOCK_MONOTONIC, &start);


    float median = subsec.median();
    ulong64 one = 0x0000000000000001;
    hash = 0x0000000000000000;
    for (int i=0;i< 64;i++){
	float current = subsec(i);
        if (current > median)
	    hash |= one;
	one = one << 1;
    }

//    clock_gettime(CLOCK_MONOTONIC, &end);
//    timeElapsed = timespecDiff(&end, &start);
//    cout<<"last "<<timeElapsed/1000000<<endl;

  	cout<<hash<<endl;
    delete C;

    return 0;
}
