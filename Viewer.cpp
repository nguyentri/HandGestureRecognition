/*******************************************************************************
*                                                                              *
*   Project name: Hand gesture recognition
*
*	Author: Nguyen Trong Tri
*                                                                              *
*******************************************************************************/

//#include <map>
//#include "HistoryBuffer.h"
//#include <NiteSampleUtilities.h>

#include "Viewer.h"
#include <opencv2/imgproc/imgproc.hpp> 
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/highgui/highgui_c.h>
#include <MWClosestPoint.h>
#include <OpenCVHandUtilities.h>

#define DEFAULT_DISPLAY_MODE	DISPLAY_MODE_DEPTH

#define MIN_NUM_CHUNKS(data_size, chunk_size)	((((data_size)-1) / (chunk_size) + 1))
#define MIN_CHUNKS_SIZE(data_size, chunk_size)	(MIN_NUM_CHUNKS(data_size, chunk_size) * (chunk_size))

Viewer* Viewer::ms_self = NULL;

void Viewer::CvIdle()
{
}
void Viewer::CvDisplay()
{
	Viewer::ms_self->display();
}
void Viewer::CvKeyboard(unsigned char key, int x, int y)
{
	Viewer::ms_self->onKey(key, x, y);
}


Viewer::Viewer(const char* strSampleName, openni::Device& device, openni::VideoStream& depth, openni::VideoStream& color) :
	m_device(device), m_depthStream(depth), m_colorStream(color), m_streams(NULL), m_eViewState(DEFAULT_DISPLAY_MODE), m_pTexMap(NULL)

{
	ms_self = this;
	strncpy(m_strSampleName, strSampleName, ONI_MAX_STR);
}
Viewer::~Viewer()
{
	delete[] m_pTexMap;

	ms_self = NULL;

	if (m_streams != NULL)
	{
		delete []m_streams;
	}
	
	free(IplThresHold);
	free(IplColor);	
}

openni::Status Viewer::init(void)
{
	openni::VideoMode depthVideoMode;
	openni::VideoMode colorVideoMode;

	if (m_depthStream.isValid() && m_colorStream.isValid())
	{
		depthVideoMode = m_depthStream.getVideoMode();
		colorVideoMode = m_colorStream.getVideoMode();

		int depthWidth = depthVideoMode.getResolutionX();
		int depthHeight = depthVideoMode.getResolutionY();
		int colorWidth = colorVideoMode.getResolutionX();
		int colorHeight = colorVideoMode.getResolutionY();

		if (depthWidth == colorWidth &&
			depthHeight == colorHeight)
		{
			m_width = depthWidth;
			m_height = depthHeight;
		}
		else
		{
			printf("Error - expect color and depth to be in same resolution: D: %dx%d, C: %dx%d\n",
				depthWidth, depthHeight,
				colorWidth, colorHeight);
			return openni::STATUS_ERROR;
		}
	}
	else if (m_depthStream.isValid())
	{
		depthVideoMode = m_depthStream.getVideoMode();
		m_width = depthVideoMode.getResolutionX();
		m_height = depthVideoMode.getResolutionY();
	}
	else if (m_colorStream.isValid())
	{
		colorVideoMode = m_colorStream.getVideoMode();
		m_width = colorVideoMode.getResolutionX();
		m_height = colorVideoMode.getResolutionY();
	}
	else
	{
		printf("Error - expects at least one of the streams to be valid...\n");
		return openni::STATUS_ERROR;
	}

	m_streams = new openni::VideoStream*[2];
	m_streams[0] = &m_depthStream;
	m_streams[1] = &m_colorStream;

	// Texture map init
	m_nTexMapX = m_width;
	m_nTexMapY =m_height;
	m_pTexMap = new openni::RGB888Pixel[m_nTexMapX * m_nTexMapY];
	memset(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));

	return openni::STATUS_OK;

}
openni::Status Viewer::run()	//Does not return
{
	int key;
	
	Viewer::display();
	key = cvWaitKey(1);
	Viewer::onKey(key,NULL,NULL);
	
	return openni::STATUS_OK;
}
void Viewer::display(void)
{
	int changedIndex;
	openni::Status rc = openni::OpenNI::waitForAnyStream(m_streams, 2, &changedIndex);
	if (rc != openni::STATUS_OK)
	{
		printf("Wait failed\n");
		return;
	}

	switch (changedIndex)
	{
	case 0:
		m_depthStream.readFrame(&m_depthFrame); break;
	case 1:
		m_colorStream.readFrame(&m_colorFrame); break;
	default:
		printf("Error in wait\n");
	}

	if (m_depthFrame.isValid())
	{
		calculateHistogram(m_pDepthHist, MAX_DEPTH, m_depthFrame, this);
	}

	memset(m_pTexMap, 0, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));

	// check if we need to draw image frame to texture
	if ((m_eViewState == DISPLAY_MODE_OVERLAY ||
		m_eViewState == DISPLAY_MODE_IMAGE) && m_colorFrame.isValid())
	{
		const openni::RGB888Pixel* pImageRow = (const openni::RGB888Pixel*)m_colorFrame.getData();
		// 		openni::RGB888Pixel* pTexRow = m_pTexMap + m_colorFrame.getCropOriginY() * m_nTexMapX;
		// 		int rowSize = m_colorFrame.getStrideInBytes() / sizeof(openni::RGB888Pixel);
		// 
		// 		for (int y = 0; y < m_colorFrame.getHeight(); ++y)
		// 		{
		// 			const openni::RGB888Pixel* pImage = pImageRow;
		// 			openni::RGB888Pixel* pTex = pTexRow + m_colorFrame.getCropOriginX();
		// 
		// 			for (int x = 0; x < m_colorFrame.getWidth(); ++x, ++pImage, ++pTex)
		// 			{
		// 				*pTex = *pImage;
		// 			}
		// 
		// 			pImageRow += rowSize;
		// 			pTexRow += m_nTexMapX;
		// 		}
		memcpy(m_pTexMap, pImageRow, m_nTexMapX*m_nTexMapY*sizeof(openni::RGB888Pixel));
	}

	// check if we need to draw depth frame to texture
	if ((m_eViewState == DISPLAY_MODE_OVERLAY ||
		m_eViewState == DISPLAY_MODE_DEPTH) && m_depthFrame.isValid())
	{
		const openni::DepthPixel* pDepthRow = (const openni::DepthPixel*)m_depthFrame.getData();
		openni::RGB888Pixel* pTexRow = m_pTexMap + m_depthFrame.getCropOriginY() * m_nTexMapX;
		//int rowSize = m_depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel);
		int rowSize = m_width;
		/*Get closest point. */
		rc = closestPoint.getNextData(m_closest, m_depthFrame);// Get closest point.
		if (rc != openni::STATUS_OK)
		{
			printf("Error in get closest point");
		}

		/*ThresHold Image pixel position. */
		int idxpx = 0;
		for (int y = 0; y < m_height; ++y)
		{
			const openni::DepthPixel* pDepth = pDepthRow;
			//openni::RGB888Pixel* pTex = pTexRow + m_depthFrame.getCropOriginX();
			openni::RGB888Pixel* pTex = pTexRow;
			for (int x = 0; x < m_width; ++x, ++pDepth, ++pTex)
			{
				if (*pDepth !=0 && *pDepth<= MAX_DEPTH)
				{	
					int nHistValue = (int)m_pDepthHist[*pDepth];
					pTex->r = nHistValue;
					pTex->g = nHistValue;
					pTex->b = nHistValue;
					if (*pDepth < m_closest.Z + 100)// Compare closest point with depth pixel
					{
						IplThresHold->imageData[idxpx++] = 255;
					}
					else
					{
						IplThresHold->imageData[idxpx++] = 0;
					}
				}
				else
				{
					IplThresHold->imageData[idxpx++] = 0;
				}
			}

			pDepthRow += rowSize;
			pTexRow += m_nTexMapX;
		}
	}
	
	/*Set pTexMap to OPenCv data. */
 	cvSetData(IplColor, m_pTexMap, m_width*3);
 	cvCvtColor( IplColor, IplColor, CV_BGR2RGB);

	//Call hand processing
	cvctx.image = IplColor;
	cvctx.thr_image = IplThresHold;
	filter_and_threshold(&cvctx);
	find_contour(&cvctx);
	find_convex_hull(&cvctx);
	fingertip(&cvctx);
	//find_fingers(&cvctx);
	//Display the OpenCV texture map
	HandDisplay(&cvctx);
	//cvWriteFrame(cvctx.writer, cvctx.image);//Write frame
}

void Viewer::onKey(unsigned char key, int /*x*/, int /*y*/)
{
	switch (key)
	{
	case 27:
		m_depthStream.stop();
		m_colorStream.stop();
		m_depthStream.destroy();
		m_colorStream.destroy();
		m_device.close();
		openni::OpenNI::shutdown();

		exit (1);
	case '1':
		m_eViewState = DISPLAY_MODE_OVERLAY;
		m_device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_DEPTH_TO_COLOR);
		break;
	case '2':
		m_eViewState = DISPLAY_MODE_DEPTH;
		m_device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_OFF);
		break;
	case '3':
		m_eViewState = DISPLAY_MODE_IMAGE;
		m_device.setImageRegistrationMode(openni::IMAGE_REGISTRATION_OFF);
		break;
	case 'm':
		m_depthStream.setMirroringEnabled(!m_depthStream.getMirroringEnabled());
		m_colorStream.setMirroringEnabled(!m_colorStream.getMirroringEnabled());
		break;
	}
	
}

openni::Status Viewer::initOpenCv(void)
{
	/*OPENCV initialize window. */
	cvNamedWindow("OrignalImg", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("ThresHoldImg", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("OrignalImg", 50, 50);
	cvMoveWindow("ThresHoldImg", 700, 50);
	
	/* Threshold Image*/
	IplThresHold = cvCreateImageHeader(cvSize(m_width, m_height), IPL_DEPTH_8U, 1);
	IplThresHold->imageData = (char*)malloc(m_width*m_height*3);
	/*Original Image. */
	IplColor = cvCreateImageHeader(cvSize(m_width, m_height), IPL_DEPTH_8U, 3);

	//HandPoints = (Point*)calloc();

	return openni::STATUS_OK;
}

void calculateHistogram(int* pHistogram, int histogramSize, const openni::VideoFrameRef& depthFrame, Viewer* pViewerObj)
{
	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)depthFrame.getData();
	int* pHistogram_temp = new int[histogramSize];
	//int width = depthFrame.getWidth();
	//int height = depthFrame.getHeight();
	// Calculate the accumulative histogram (the yellow display...)
	memset(pHistogram, 0, histogramSize*sizeof(int));
	memset(pHistogram_temp, 0, histogramSize*sizeof(int));
	int restOfRow = depthFrame.getStrideInBytes() / sizeof(openni::DepthPixel) - pViewerObj->m_width;

	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < pViewerObj->m_height; ++y)
	{
		for (int x = 0; x < pViewerObj->m_width; ++x, ++pDepth)
		{
			if (*pDepth != 0 && *pDepth <= MAX_DEPTH)
			{
				pHistogram_temp[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}
	if (nNumberOfPoints)
	{
		for (int nIndex=1; nIndex<histogramSize; nIndex++)
		{
			pHistogram_temp[nIndex] += pHistogram_temp[nIndex-1];
			pHistogram[nIndex] = (int)(256 * (1.0f - ((float)pHistogram_temp[nIndex] / nNumberOfPoints)));
		}
	}

}

