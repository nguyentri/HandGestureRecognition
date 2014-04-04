


#include <OpenCVHandUtilities.h>


#define SHOW_HAND_CONTOUR

void init_recording(struct ctx *ctx)
{
	int fps, width, height;

	fps = cvGetCaptureProperty(ctx->capture, CV_CAP_PROP_FPS);
	width = cvGetCaptureProperty(ctx->capture, CV_CAP_PROP_FRAME_WIDTH);
	height = cvGetCaptureProperty(ctx->capture, CV_CAP_PROP_FRAME_HEIGHT);

	if (fps < 0)
		fps = 10;

	ctx->writer = cvCreateVideoWriter(VIDEO_FILE, VIDEO_FORMAT, fps,
					  cvSize(width, height), 1);

	if (!ctx->writer) {
		fprintf(stderr, "Error initializing video writer\n");
		exit(1);
	}
}

class HandProcesing : public Viewer
{
	public:
		HandProcesing(const IplImage* IplOrg, const* Ipl IplThreshold);
		virtual ~HandProcesing();
	protected:
	
	private:
}


void init_ctx(struct ctx *ctx)
{
	ctx->thr_image = cvCreateImage(cvSize(640, 480), 8, 1);
	ctx->temp_image1 = cvCreateImage(cvSize(640, 480), 8, 1);
	ctx->temp_image3 = cvCreateImage(cvSize(640, 480), 8, 3);
	ctx->kernel = cvCreateStructuringElementEx(9, 9, 4, 4, CV_SHAPE_RECT,
						   NULL);
	ctx->contour_st = cvCreateMemStorage(0);
	ctx->hull_st = cvCreateMemStorage(0);
	ctx->temp_st = cvCreateMemStorage(0);
	ctx->fingers = (CvPoint *)calloc(NUM_FINGERS + 1, sizeof(CvPoint));
	ctx->defects = (CvPoint *)calloc(NUM_DEFECTS, sizeof(CvPoint));
}

void filter_and_threshold(struct ctx *ctx)
{

	///* Soften image */
	//cvSmooth(ctx->image, ctx->temp_image3, CV_GAUSSIAN, 11, 11, 0, 0);
	///* Remove some impulsive noise */
	//cvSmooth(ctx->temp_image3, ctx->temp_image3, CV_MEDIAN, 11, 11, 0, 0);

	//cvCvtColor(ctx->temp_image3, ctx->temp_image3, CV_BGR2HSV);

	//cvCvtColor(ctx->image, ctx->thr_image, CV_RGB2GRAY);

	///*
	// * Apply threshold on HSV values to detect skin color
	// */
	//cvInRangeS(ctx->temp_image3,
	//	   cvScalar(0, 0, 100,255),
	//	   cvScalar(0, 0, 100,255),
	//	   ctx->thr_image); 

	///* Apply morphological opening */
	//cvMorphologyEx(ctx->thr_image, ctx->thr_image, NULL, ctx->kernel,
	//	       CV_MOP_OPEN, 1);
 	cvSmooth(ctx->thr_image, ctx->thr_image, CV_MEDIAN, 13, 13, 0.5, 0.5);
}

void find_contour(struct ctx *ctx)
{
	double area, max_area = 0.0;
	CvSeq *contours, *tmp, *contour = NULL;

	/* cvFindContours modifies input image, so make a copy */
	cvCopy(ctx->thr_image, ctx->temp_image1, NULL);
	cvFindContours(ctx->temp_image1, ctx->temp_st, &contours,
		       sizeof(CvContour), CV_RETR_EXTERNAL,
		       CV_CHAIN_APPROX_SIMPLE, cvPoint(0, 0));

	/* Select contour having greatest area */
	for (tmp = contours; tmp; tmp = tmp->h_next) {
		area = fabs(cvContourArea(tmp, CV_WHOLE_SEQ, 0));
		if (area > max_area) {
			max_area = area;
			contour = tmp;
		}
	}

	/* Approximate contour with poly-line */
	if (contour) {
		contour = cvApproxPoly(contour, sizeof(CvContour),
				       ctx->contour_st, CV_POLY_APPROX_DP, 2,
				       1);
		ctx->contour = contour;
	}
	//ctx->contour = contours;

	////new source
	//IplImage *out_canny=cvCreateImage( cvSize(640, 480), IPL_DEPTH_8U, 1);
	//cvCanny(ctx->temp_image1, out_canny, 50.0 ,100.0, 3);
	////cvShowImage("Canny_Edge", out_canny);

	//int n=0;
	//int Nc = cvFindContours(
	//	out_canny,
	//	storage,
	//	&ctx->contour,
	//	sizeof(CvContour),
	//	CV_RETR_TREE,
	//	CV_CHAIN_APPROX_SIMPLE,
	//	cvPoint(0,0)// Try all four values and see what happens
	//	);
	// Draw_ConvexHull(ctx);
	//ctx->contour = contours;
}

void find_convex_hull(struct ctx *ctx)
{
	CvSeq *defects;
	CvConvexityDefect *defect_array;
	int i;
	int x = 0, y = 0;
	int dist = 0;

	ctx->hull = NULL;

	if (!ctx->contour)
		return;

	ctx->hull = cvConvexHull2(ctx->contour, ctx->hull_st, CV_CLOCKWISE, 0);


	if (ctx->hull) {

		pt0 = **CV_GET_SEQ_ELEM( CvPoint*, ctx->hull, ctx->hull->total - 1 );


		for(int i = 0; i < ctx->hull->total; i++ )
		{
			/*Draw convect hull */
			pt = **CV_GET_SEQ_ELEM( CvPoint*, ctx->hull, i );
			//printf("%d,%d\n",pt.x,pt.y);
			cvLine( ctx->image, pt0, pt, YELLOW,2,8,0);
			pt0 = pt;
		}


		/* Get convexity defects of contour w.r.t. the convex hull */
		defects = cvConvexityDefects(ctx->contour, ctx->hull,
					     ctx->defects_st);

		if (defects && defects->total) {
			defect_array = (CvConvexityDefect *)calloc(defects->total,
					      sizeof(CvConvexityDefect));
			cvCvtSeqToArray(defects, defect_array, CV_WHOLE_SEQ);


			//printf("%d,%d\n",pt.x,pt.y);
			cvLine( frame, pt0, pt, YELLOW,2,8,0);
			pt0 = pt;

			/* Average depth points to get hand center */
			for (i = 0; i < defects->total && i < NUM_DEFECTS; i++) {
				x += defect_array[i].depth_point->x;
				y += defect_array[i].depth_point->y;

				ctx->defects[i] = cvPoint(defect_array[i].depth_point->x,
							  defect_array[i].depth_point->y);
			}

			x /= defects->total;
			y /= defects->total;

			ctx->num_defects = defects->total;
			ctx->hand_center = cvPoint(x, y);

			/* Compute hand radius as mean of distances of
			   defects' depth point to hand center */
			for (i = 0; i < defects->total; i++) {
				int d = (x - defect_array[i].depth_point->x) *
					(x - defect_array[i].depth_point->x) +
					(y - defect_array[i].depth_point->y) *
					(y - defect_array[i].depth_point->y);

				dist += sqrt(d);
			}

			ctx->hand_radius = dist / defects->total;
			free(defect_array);
		}
	}
}

void find_fingers(struct ctx *ctx)
{
	int n;
	int i;
	CvPoint *points;
	CvPoint max_point;
	int dist1 = 0, dist2 = 0;

	ctx->num_fingers = 0;

	if (!ctx->contour || !ctx->hull)
		return;

	n = ctx->contour->total;
	points = (CvPoint *)calloc(n, sizeof(CvPoint));

	cvCvtSeqToArray(ctx->contour, points, CV_WHOLE_SEQ);

	/*
	 * Fingers are detected as points where the distance to the center
	 * is a local maximum
	 */
	for (i = 0; i < n; i++) {
		int dist;
		int cx = ctx->hand_center.x;
		int cy = ctx->hand_center.y;

		dist = (cx - points[i].x) * (cx - points[i].x) +
		    (cy - points[i].y) * (cy - points[i].y);

		if (dist < dist1 && dist1 > dist2 && max_point.x != 0
		    && max_point.y < cvGetSize(ctx->image).height - 10) {

			ctx->fingers[ctx->num_fingers++] = max_point;
			if (ctx->num_fingers >= NUM_FINGERS + 1)
				break;
		}

		dist2 = dist1;
		dist1 = dist;
		max_point = points[i];
	}

	free(points);
}

void display(struct ctx *ctx)
{
	int i;

//	if (ctx->num_fingers == NUM_FINGERS) {

#if defined(SHOW_HAND_CONTOUR)
		cvDrawContours(ctx->image, ctx->contour, BLUE, GREEN, 0, 1,
			       CV_AA, cvPoint(0, 0));
#endif
		cvCircle(ctx->image, ctx->hand_center, 5, PURPLE, 1, CV_AA, 0);
		cvCircle(ctx->image, ctx->hand_center, ctx->hand_radius,
			 RED, 1, CV_AA, 0);

		for (i = 0; i < ctx->num_fingers; i++) {

			cvCircle(ctx->image, ctx->fingers[i], 10,
				 GREEN, 3, CV_AA, 0);

			cvLine(ctx->image, ctx->hand_center, ctx->fingers[i],
			       YELLOW, 1, CV_AA, 0);
		}

		for (i = 0; i < ctx->num_defects; i++) {
			cvCircle(ctx->image, ctx->defects[i], 2,
				 GREEN, 2, CV_AA, 0);
		}
//	}

	cvShowImage("output", ctx->image);
	cvShowImage("thresholded", ctx->thr_image);
}

void calculateHistogram(float* pHistogram, int histogramSize, const openni::VideoFrameRef& frame)
{
	const openni::DepthPixel* pDepth = (const openni::DepthPixel*)frame.getData();
	// Calculate the accumulative histogram (the yellow display...)
	memset(pHistogram, 0, histogramSize*sizeof(float));
	int restOfRow = frame.getStrideInBytes() / sizeof(openni::DepthPixel) - frame.getWidth();
	int height = frame.getHeight();
	int width = frame.getWidth();

	unsigned int nNumberOfPoints = 0;
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x, ++pDepth)
		{
			if (*pDepth != 0)
			{
				pHistogram[*pDepth]++;
				nNumberOfPoints++;
			}
		}
		pDepth += restOfRow;
	}
	for (int nIndex=1; nIndex<histogramSize; nIndex++)
	{
		pHistogram[nIndex] += pHistogram[nIndex-1];
	}
	if (nNumberOfPoints)
	{
		for (int nIndex=1; nIndex<histogramSize; nIndex++)
		{
			pHistogram[nIndex] = (256 * (1.0f - (pHistogram[nIndex] / nNumberOfPoints)));
		}
	}
}

int wasKeyboardHit()
{
	return (int)_kbhit();
}

