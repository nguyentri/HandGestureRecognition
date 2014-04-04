
#include <conio.h>
#include <stdio.h>
#include <OpenNI.h>
#include <OpenCVHandUtilities.h>

#define SHOW_HAND_CONTOUR

CvPoint pt0,pt,p,armcenter;//center:掌心點  armcenter:輪廓中心點

ctxxx cvctx = { };

void init_capture(struct ctx *ctx)
{
}

void init_recording(struct ctx *ctx)
{
	int fps = 30, width = 640, height = 480;

	ctx->writer = cvCreateVideoWriter(VIDEO_FILE, VIDEO_FORMAT, fps,
					  cvSize(width, height), 1);

	if (!ctx->writer) {
		fprintf(stderr, "Error initializing video writer\n");
		exit(1);
	}
}

void init_windows(void)
{
	cvNamedWindow("output", CV_WINDOW_AUTOSIZE);
	cvNamedWindow("thresholded", CV_WINDOW_AUTOSIZE);
	cvMoveWindow("output", 50, 50);
	cvMoveWindow("thresholded", 700, 50);
}

void init_ctx(struct ctx *ctx)
{
	ctx->thr_image = cvCreateImage(cvSize(640, 480), 8, 1);
	ctx->temp_image1 = cvCreateImage(cvSize(640, 480), 8, 1);
//	ctx->temp_image3 = cvCreateImage(cvSize(640, 480), 8, 3);
	ctx->kernel = cvCreateStructuringElementEx(9, 9, 4, 4, CV_SHAPE_RECT,
						   NULL);
	ctx->contour_st = cvCreateMemStorage(0);
	ctx->hull_st = cvCreateMemStorage(0);
	ctx->temp_st = cvCreateMemStorage(0);
	ctx->handbox_str = cvCreateMemStorage(0);
	ctx->fingers = (CvPoint *)calloc(NUM_FINGERS + 1, sizeof(CvPoint));
	ctx->defects = (CvPoint *)calloc(NUM_DEFECTS, sizeof(CvPoint));

	ctx->palmstorage = cvCreateMemStorage(0);
    ctx->palm = cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),ctx->palmstorage); //儲存黃點的序列
	ctx->fingerstorage = cvCreateMemStorage(0);
	ctx->fingerseq = cvCreateSeq(CV_SEQ_ELTYPE_POINT,sizeof(CvSeq),sizeof(CvPoint),ctx->fingerstorage);
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

	//=* Apply morphological opening */
	//cvMorphologyEx(ctx->thr_image, ctx->thr_image, NULL, ctx->kernel,
	//	       CV_MOP_OPEN, 1);
	
	//ctx->temp_image3 = cvCreateImage(cvSize(handbox.size.width, handbox.size.height), 8, 1);

	//cvErode(ctx->thr_image,ctx->thr_image,0,1);
	cvDilate(ctx->thr_image,ctx->thr_image,0,1);  //膨脹 

 	cvSmooth(ctx->thr_image, ctx->thr_image, CV_MEDIAN, 11, 11, 0.5, 0.5);
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

	CvBox2D handbox = cvMinAreaRect2(ctx->contour);

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


		for(i = 0; i < ctx->hull->total; i++ )
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
// 
// 		for(int i=0;i<defects->total;i++)
// 		{
// 			CvConvexityDefect* d=(CvConvexityDefect*)cvGetSeqElem(defects	,i);
// 
// 			if(d->depth > 10) //處理大於10的 
// 			{
// 				p.x = d->depth_point->x;
// 				p.y = d->depth_point->y;
// 				cvCircle(ctx->image,p,3,GREEN,-1,CV_AA,0);
// 				cvSeqPush(ctx->palm,&p);
// 			}
// 		}


		if (defects && defects->total) {
			defect_array = (CvConvexityDefect *)calloc(defects->total,
					      sizeof(CvConvexityDefect));
			cvCvtSeqToArray(defects, defect_array, CV_WHOLE_SEQ);


			//printf("%d,%d\n",pt.x,pt.y);
		//	cvLine( ctx->image, pt0, pt, YELLOW,2,8,0);
		//	pt0 = pt;

			/* Average depth points to get hand center */
			ctx->num_defects  = 0;
			for (i = 0; i < defects->total; i++) {
				x += defect_array[i].depth_point->x;
				y += defect_array[i].depth_point->y;

				if (defect_array[i].depth > 10) // remove defects <= 10mm
				{
					ctx->defects[i] = cvPoint(defect_array[i].depth_point->x,
						defect_array[i].depth_point->y);
					ctx->num_defects++;
				}
			}

			x /= defects->total;
			y /= defects->total;

//			ctx->num_defects = defects->total;
			ctx->hand_center = cvPoint(x, y);

			/* Compute hand radius as mean of distances of
			   defects' depth point to hand center */
			for (i = 0; i < defects->total; i++) {
				int d = (x - defect_array[i].depth_point->x) *
					(x - defect_array[i].depth_point->x) +
					(y - defect_array[i].depth_point->y) *
					(y - defect_array[i].depth_point->y);

				dist += (int)sqrt(d);
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

void HandDisplay(struct ctx *ctx)
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
// 
// 		for (i = 0; i < ctx->num_fingers; i++) {
// 
// 			cvCircle(ctx->image, ctx->fingers[i], 10,
// 				 GREEN, 3, CV_AA, 0);
// 
// 		cvLine(ctx->image, ctx->hand_center, ctx->fingers[i],
// 			       YELLOW, 1, CV_AA, 0);
// 		}

		for (i = 0; i < ctx->num_defects; i++) {
			cvCircle(ctx->image, ctx->defects[i], 2,
				 GREEN, 2, CV_AA, 0);
		}
//	}
	cvShowImage("OrignalImg", ctx->image);
	cvShowImage("ThresHoldImg", ctx->thr_image);
}



void fingertip(struct ctx* ctx)
{    
	int dotproduct,i;
	float length1,length2,angle,minangle,length;
	CvPoint vector1,vector2, vector3,min,minp1,minp2;
	CvPoint fingertip[20];
	CvPoint *p1,*p2,*p;
	int tiplocation[20];
	int count = 0;
	bool signal = false;

	for(i=0;i<ctx->contour->total;i++)
	{
		p1 = (CvPoint*)cvGetSeqElem(ctx->contour,i);
		p = (CvPoint*)cvGetSeqElem(ctx->contour,(i+20)%ctx->contour->total);
		p2 = (CvPoint*)cvGetSeqElem(ctx->contour,(i+40)%ctx->contour->total);
		vector1.x = p->x - p1->x;
		vector1.y = p->y - p1->y;
		vector2.x = p->x - p2->x;
		vector2.y = p->y - p2->y;
		dotproduct = (vector1.x*vector2.x) + (vector1.y*vector2.y); 
		length1 = sqrtf((vector1.x*vector1.x)+(vector1.y*vector1.y));
		length2 = sqrtf((vector2.x*vector2.x)+(vector2.y*vector2.y));
		angle = fabs(dotproduct/(length1*length2));    

		///////////////////////////////////if start
		if(angle < 0.2)
		{
			if(!signal)//如果是第一點 
			{
				signal = true;
				min.x = p->x;
				min.y = p->y;
				minp1.x = p1->x;
				minp1.y = p1->y;
				minp2.x = p2->x;
				minp2.y = p2->y;
				minangle = angle;
			}      
			else//如果不是第一點 
			{
				if(angle <= minangle)//如果angle值更小 
				{

					min.x = p->x;
					min.y = p->y;
					minp1.x = p1->x;
					minp1.y = p1->y;
					minp2.x = p2->x;
					minp2.y = p2->y;
					minangle = angle;
				}
			}

		}//////////////////////////////////////////////////////////if end   

		else//else start
		{
			if(signal)
			{
				signal = false;
				CvPoint l1,l2,l3;//temp1為中心到指尖, vector1為指尖到p1, vector2為指尖到p2
				l1.x = min.x - armcenter.x;
				l1.y = min.y - armcenter.y;

				l2.x = minp1.x - armcenter.x;
				l2.y = minp1.y - armcenter.y;

				l3.x = minp2.x - armcenter.x;
				l3.y = minp2.y - armcenter.y;

				length = sqrtf((l1.x*l1.x)+(l1.y*l1.y));
				length1 = sqrtf((l2.x*l2.x)+(l2.y*l2.y));
				length2 = sqrtf((l3.x*l3.x)+(l3.y*l3.y));    

				if(length > length1 && length > length2)
				{
					//cvCircle(frame,min,6,CV_RGB(0,255,0),-1,8,0);
					fingertip[count] = min;
					tiplocation[count] = i+20;
					count = count + 1;
				}

				else if(length < length1 && length < length2)
				{
					//cvCircle(frame,min,8,CV_RGB(0,0,255),-1,8,0);
					//cvCircle(virtualhand,min,8,CV_RGB(255,255,255),-1,8,0);
					cvSeqPush(ctx->palm,&min);
					//fingertip[count] = min;
					//tiplocation[count] = i+20;
					//count = count + 1;
				}    
			}
		}//else end

	}//for end	

	for(i=0;i<count;i++)
	{
		if( (tiplocation[i] - tiplocation[i-1]) > 40)
		{
			if( fingertip[i].x >= 630  || fingertip[i].y >= 470 )
			{
				cvCircle(ctx->image,fingertip[i],6,CV_RGB(50,200,250),-1,8,0);                           
			}

			else
			{
				//cvCircle(frame,fingertip[i],6,CV_RGB(0,255,0),-1,8,0);
				//cvCircle(virtualhand,fingertip[i],6,CV_RGB(0,255,0),-1,8,0);
				//cvLine(virtualhand,fingertip[i],armcenter,CV_RGB(255,0,0),3,CV_AA,0);
				cvSeqPush(ctx->fingerseq,&fingertip[i]);
			}
		}
	}	    //cvClearSeq(ctx->fingerseq);    
}