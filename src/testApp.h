#ifndef _TEST_APP
#define _TEST_APP

#include "ofMain.h"
#include <vector>
#include <opencv2/opencv.hpp>
#include "ofxOsc.h"

#include "ofxOpenCv.h"

//#define _USE_LIVE_VIDEO		

const int MAX_BLOBS = 1;
const int NUM_CAMERAS = 1;
const int W = 320;
const int H = 240;
const int CAPTURE_WIDTH_OVER_2 = W/2;
const int CAPTURE_HEIGHT_OVER_2 = H/2;
const int NUM_VIEWS = 5;
const int NUM_CALIBRATION_PTS = 10;

#include "ofxControlPanel.h"
#include "ofVideoGrabber.h"

//#include "CvPixelBackgroundGMM.h"
#include "ofCvBlobTracker.h"
#include "pkmBlobTracker.h"

class testApp : public ofBaseApp, public ofCvBlobListener{

	enum APP_MODE {
		MODE_TRACKING,
		MODE_CALIBRATION
	};
	
public:

	// app callbacks
	~testApp();
	void 	setup();
	void	update();
	void	draw();

	// ui callbacks
	void	keyPressed		(int key);
	void	mouseMoved		(int x, int y );
	void	mouseDragged	(int x, int y, int button);
	void	mousePressed	(int x, int y, int button);
	void	mouseReleased	(int x, int y, int button);
	void	windowResized	(int w, int h);
	
	// blob callbacks
	void	blobOn			( int x, int y, int id, int order );
	void	blobMoved		( int x, int y, int id, int order );    
	void	blobOff			( int x, int y, int id, int order );   
	
	// calibration init
	void	initializeCameraMatrices	();
	void	initializeObjectPoints		();
	void	initOutputFile				();
	void	calibrateUserPosition		();

	// camera/video
#ifdef _USE_LIVE_VIDEO
	ofVideoGrabber			vidGrabber[NUM_CAMERAS];
#else
	ofVideoPlayer			vidPlayer;
#endif	
	
	// raw pixel input
	unsigned char			*inImage;
	
	// opencv container for input
	ofxCvColorImage			colorImg[NUM_CAMERAS];
	ofxCvColorImage			colorImgUnDistorted[NUM_CAMERAS];
    //ofxCvGrayscaleImage 	grayImage[NUM_CAMERAS];

	bool bBlob;
	

	// gui
	ofxControlPanel			gui;
	

	// variables for hsv color tracking
	ofxCvColorImage			rgb,hsb;
    ofxCvGrayscaleImage		hue,sat,bri,filtered; 
    ofCvContourFinder		contours;
    ofCvBlobTracker		    blobTracker;	
	
	// hue to find from ui input
    int						findHue;
	
	// calibration variables
	cv::Mat					objectPoints,
							cameraMatrix,
							homographyMatrix,
							distCoeffs,
							rvec,
							tvec,
							rotationMatrix,
							imagePoints;
	vector<cv::Point2f>		iP;
	int						currentPoint, currentView;
	
	FILE					*fp;
	
	bool					bTrain, bCalibrated;
	APP_MODE				MODE;

};

#endif
