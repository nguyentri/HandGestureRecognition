/*******************************************************************************
*                                                                              *
*   Project name: Hand gesture recognition
*
*	Author: Nguyen Trong Tri
*                                                                              *
*******************************************************************************/

#ifndef _NITE_HAND_VIEWER_H_
#define _NITE_HAND_VIEWER_H_

//#include "NiTE.h"
#include "OpenNI.h"
#include <opencv2/core/core.hpp>
#include <MWClosestPoint.h>

#define MAX_DEPTH 4000

enum DisplayModes
{
	DISPLAY_MODE_OVERLAY,
	DISPLAY_MODE_DEPTH,
	DISPLAY_MODE_IMAGE
};

class Viewer
{

public:
	Viewer(const char* strSampleName, openni::Device& device, openni::VideoStream& depth, openni::VideoStream& color);
	virtual ~Viewer();

	virtual openni::Status init();
	virtual openni::Status run();	//Does not return

	virtual openni::Status initOpenCv();

	friend void calculateHistogram(int* pHistogram, int histogramSize, const openni::VideoFrameRef& depthFrame, Viewer* pViewerObj);

protected:
	virtual void display();
	virtual void displayPostDraw(){};	// Overload to draw over the screen image

	virtual void onKey(unsigned char key, int x, int y);

	/*OpenNI video frame reference. */
	openni::VideoFrameRef		m_depthFrame;
	openni::VideoFrameRef		m_colorFrame;

	/*OPenNI device. */
	openni::Device&			m_device;
	openni::VideoStream&			m_depthStream;
	openni::VideoStream&			m_colorStream;
	openni::VideoStream**		m_streams;

	/*OPENCV image structure.  */ 
	IplImage* IplThresHold;
	//IplImage* IplDepth;
 	IplImage* IplColor;

	//CvPoint* HandPoints;

private:
	Viewer(const Viewer&);
	Viewer& operator=(Viewer&);

	static Viewer* ms_self;
	static void CvIdle();
	static void CvDisplay();
	static void CvKeyboard(unsigned char key, int x, int y);

	int			m_pDepthHist[MAX_DEPTH];
	char			m_strSampleName[ONI_MAX_STR];
	unsigned int		m_nTexMapX;
	unsigned int		m_nTexMapY;
	DisplayModes		m_eViewState;
	openni::RGB888Pixel*	m_pTexMap;
	int			m_width;
	int			m_height;

	closest_point::ClosestPoint closestPoint;
	/*Get closest point. */
	closest_point::IntPoint3D m_closest;	
};

#endif // _NITE_HAND_VIEWER_H_
