


#include <iostream>
#include <vector>
#include <conio.h>
#include <stdio.h>
#include <OpenNI.h>
#include <opencv2/core/core.hpp>
#include <opencv/highgui.h>
#include <opencv2/imgproc/imgproc.hpp> 
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>

#define C_DEPTH_STREAM 0
#define C_COLOR_STREAM 1

#define C_NUM_STREAMS 2

#define C_MODE_COLOR 0x01
#define C_MODE_DEPTH 0x02
#define C_MODE_ALIGNED 0x04

#define C_STREAM_TIMEOUT 2000

#define MAX_DEPTH	4000

extern float m_pDepthHist[MAX_DEPTH];

#ifndef _OPENCV_HAND_UTILITIES_
#define _OPENCV_HAND_UTILITIES_

#define VIDEO_FILE	"video.avi"
#define VIDEO_FORMAT	CV_FOURCC('M', 'J', 'P', 'G')
#define NUM_FINGERS	5
#define NUM_DEFECTS	8

#define RED     CV_RGB(255, 0, 0)
#define GREEN   CV_RGB(0, 255, 0)
#define BLUE    CV_RGB(0, 0, 255)
#define YELLOW  CV_RGB(255, 255, 0)
#define PURPLE  CV_RGB(255, 0, 255)
#define GREY    CV_RGB(200, 200, 200)

typedef struct ctx {
	CvCapture	*capture;	/* Capture handle */
	CvVideoWriter	*writer;	/* File recording handle */

	IplImage	*frame;
	IplImage	*image;		/* Input image */
	IplImage	*thr_image;	/* After filtering and thresholding */
	IplImage	*temp_image1;	/* Temporary image (1 channel) */
	IplImage	*temp_image3;	/* Temporary image (3 channels) */

	CvSeq		*contour;	/* Hand contour */
	CvSeq		*hull;		/* Hand convex hull */

	CvPoint		hand_center;
	CvPoint		*fingers;	/* Detected fingers positions */
	CvPoint		*defects;	/* Convexity defects depth points */

	CvMemStorage	*hull_st;
	CvMemStorage	*contour_st;
	CvMemStorage	*temp_st;
	CvMemStorage	*defects_st;
	CvMemStorage	*handbox_str;
				
	IplConvKernel	*kernel;	/* Kernel for morph operations */

    CvSeq* palm; //palm seq
	CvMemStorage* palmstorage;

	CvSeq* fingerseq;
	CvMemStorage* fingerstorage;

	int		num_fingers;
	int		hand_radius;
	int		num_defects;
} ctxxx;

extern ctxxx cvctx;

/*Function to initialize recording video. */
extern void init_recording(struct ctx *ctx);

/*Function to initialize image windows. */
extern void init_windows(void);

/*Function to initialize struct ctx. */
extern void init_ctx(struct ctx *ctx);

extern void filter_and_threshold(struct ctx *ctx);

extern void find_contour(struct ctx *ctx);

extern void find_convex_hull(struct ctx *ctx);

extern void find_fingers(struct ctx *ctx);

extern void HandDisplay(struct ctx *ctx);

//other unilities
void fingertip(struct ctx* ctx);

#endif