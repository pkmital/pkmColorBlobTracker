#include "testApp.h"
#include <stdlib.h>

testApp::~testApp()
{
	fclose(fp);
}

//--------------------------------------------------------------
void testApp::setup() {
	bTrain = false;
	bCalibrated = false;
	
	
	MODE = MODE_CALIBRATION;
	currentView = currentPoint = 0;
	initializeCameraMatrices();
	initializeObjectPoints();
	initOutputFile();
	
#ifdef _USE_LIVE_VIDEO
	vidGrabber[0].setVerbose(true);
	vidGrabber[0].initGrabber(W,H);
	//vidGrabber[1].setVerbose(true);
	//vidGrabber[1].initGrabber(W,H);
#else
	vidPlayer.loadMovie(ofToDataPath("test2.mov"));
	vidPlayer.setLoopState(OF_LOOP_NORMAL);
	vidPlayer.play();
#endif
	
	/*
	gui.setup("warp", 0, 0, 1024, 768);
	gui.addPanel("warper", 3);
	gui.setWhichPanel(0);
	
	hue_min_threshold = 170;
	hue_max_threshold = 255;
	
	gui.addSlider("Hue Min Threshold", "H_MIN", hue_min_threshold, 0, 179, true);
	gui.addSlider("Hue Max Threshold", "H_MAX", hue_max_threshold, 0, 179, true);
	*/
	
	
	//reserve memory for cv images
	rgb.allocate(W, H);
	hsb.allocate(W, H);
	hue.allocate(W, H);
	sat.allocate(W, H);
	bri.allocate(W, H);
	filtered.allocate(W, H);
	
	blobTracker.setListener(this);
	
	ofSetWindowShape(W*3, H*2);
	ofSetVerticalSync(true);
	ofSetFrameRate(30);
	ofSetBackgroundAuto(true);
	ofBackground(0,0,0);
}
void testApp::initOutputFile()
{
	char buf[80];
	sprintf(buf, "%02d%02d%02d-%02d.%02d.%02d.txt\n",
					ofGetDay(), ofGetMonth(), ofGetYear(),
			ofGetHours(), ofGetMinutes(), ofGetSeconds());
	fp = fopen(ofToDataPath(buf).c_str(), "w+");
}

void testApp::initializeCameraMatrices()
{
	double focalX = CAPTURE_WIDTH_OVER_2;
	double focalY = CAPTURE_HEIGHT_OVER_2;
	double centerX = CAPTURE_WIDTH_OVER_2;
	double centerY = CAPTURE_HEIGHT_OVER_2;
	double tangentDistX = 0.0;
	double tangentDistY = 0.0;
	double radialDistX = 0;//-0.150;		
	double radialDistY = 0;//0.024;
	double camIntrinsics[] = { focalX, 0, centerX, 0, focalY, centerY, 0, 0, 1 };
	double distortionCoeffs[] = { radialDistX, radialDistY, tangentDistX, tangentDistY, 0 };
	cameraMatrix = cv::Mat(3, 3, CV_64F, camIntrinsics);
	distCoeffs = cv::Mat(1, 4, CV_64F, distortionCoeffs); 
	vector<double> rv = vector<double>(3); 
	rvec = cv::Mat(rv); 
	vector<double> tv = vector<double>(3); 
	tv[0]=0;tv[1]=0;tv[2]=0; 
	tvec = cv::Mat(tv); 
	rotationMatrix = cv::Mat(3,3,CV_64F);
	homographyMatrix = cv::Mat(3,3,CV_64F);
	
}

void testApp::initializeObjectPoints()
{
	vector<cv::Point2f> oP;
	// 
	//  1    2    3
	//    10   11
	//  4    5    6
	//    12   13
	//  7    8    9
	//
	
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(-1, 1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(0, 1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(1, 1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(-1, 0));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(0, 0));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(1, 0));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{	
		oP.push_back(cv::Point2f(-1, -1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(0, -1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(1, -1));
	}
	for(int i = 0; i < NUM_VIEWS; i++)
	{
		oP.push_back(cv::Point2f(0.5, 0.5));
	}
	
	objectPoints = cv::Mat(oP);
	
}

//--------------------------------------------------------------
void testApp::update()
{
	ofBackground(0,0,0);
	gui.update();
#ifdef _USE_LIVE_VIDEO
	vidGrabber[0].grabFrame();
	if (vidGrabber[0].isFrameNew())
	{
		rgb.setFromPixels(vidGrabber[0].getPixels(),W,H);
#else
	vidPlayer.update();
	if (vidPlayer.isFrameNew()) 
	{
		rgb.setFromPixels(vidPlayer.getPixels(),W,H);
#endif
		//mirror horizontal
		//rgb.mirror(false, true);
		
		
		// camera calibration
		rgb.undistort(-0.150,					// radial dist x
					  0.024,					// radial dist y
					  0.0,						// tangent dist x
					  0.0,						// tangent dist y
					  CAPTURE_WIDTH_OVER_2,		// focal x
					  CAPTURE_HEIGHT_OVER_2,	// focal y
					  CAPTURE_WIDTH_OVER_2,		// center x
					  CAPTURE_HEIGHT_OVER_2);	// center y
		
		
		// duplicate rgb
		hsb = rgb;
		
		// convert to hsb
		hsb.convertRgbToHsv();
		
		// store the three channels as grayscale images
		hsb.convertToGrayscalePlanarImages(hue, sat, bri);
		
		// blur
		hue.blurGaussian(5);
		
		// filter image based on the hue value 
		for (int i=0; i<W*H; i++) {
			filtered.getPixels()[i] = ofInRange(hue.getPixels()[i],findHue-5,findHue+5) ? 255 : 0;
		}
		
		// run the contour finder on the filtered image to find blobs with a certain hue
		contours.findContours(filtered, 50, W*H/2, MAX_BLOBS, false);
		blobTracker.trackBlobs( contours.blobs );
	
/*		// train the current point
		if (bTrain && blobTracker.blobs.size() > 0) {
			// if we have to view the current point more times
			if (currentView < NUM_VIEWS) {
				// keep the point
				iP.push_back(cv::Point2f(blobTracker.blobs[0].center.x, 
										 blobTracker.blobs[0].center.y));
				currentView++;
			}
			
			// we have enough views for this point, move on
			if (currentView == NUM_VIEWS) {
				bTrain = false;
				currentPoint++;
				currentView = 0;
			}
			
			// we're done collecting all the points
			if (currentPoint == NUM_CALIBRATION_PTS) {
				calibrateUserPosition();
			}
		}
*/
		
	}
}

void testApp::calibrateUserPosition()
{
	imagePoints = cv::Mat(iP);
	
#if 0

	// do the fitting
	solvePnP(objectPoints, imagePoints, cameraMatrix, distCoeffs, rvec, tvec, false);
	
	// output translation vector
	cout << "translation vector:" << endl;
	cv::MatConstIterator_<double> it = tvec.begin<double>(); 
	cv::MatConstIterator_<double> it_end = tvec.end<double>(); 
	for(;it != it_end; it++) 
		cout << *it << endl; 
	
//	for(;it != it_end; it++) 
//		cout << *it << endl; 
	
	
	// output rotation vector
	cout << "rotation vector:" << endl;
	it = rvec.begin<double>(); 
	it_end = rvec.end<double>(); 
	for(;it != it_end; it++) 
		cout << *it << endl; 
	
	
	cv::Rodrigues(cv::Mat(rvec), rotationMatrix);
	cout << "Rotation Matrix:" << endl;
	
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			cout << rotationMatrix.at<double>(i,j) << " ";
			
		}
		cout << endl;
	}
#else
	homographyMatrix = findHomography(objectPoints, imagePoints, CV_RANSAC);//CV_LMEDS);
	cout << "homographyMatrix:" << endl;
	
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			cout << homographyMatrix.at<double>(i,j) << " ";
			
		}
		cout << endl;
	}
	
#endif
	MODE = MODE_TRACKING;
}
	
//--------------------------------------------------------------
void testApp::draw(){
	ofSetColor(255,255,255);
	
	// draw all cv images
	rgb.draw(0,0);
	hsb.draw(W*2,0);
	hue.draw(0,H);
	sat.draw(W,H);
	bri.draw(W*2,H);
	filtered.draw(W,0);
	//contours.draw(0,0);
	
	// blob drawing
	blobTracker.draw(0,0);
	
	char buf[80];
	sprintf(buf, "current point: %d", currentPoint+1);
	ofDrawBitmapString(buf, 20, 20);
	if(MODE == MODE_CALIBRATION)
		sprintf(buf, "MODE: Calibration (Click the %d points)", NUM_CALIBRATION_PTS);
	else {
		sprintf(buf, "MODE: Tracking (Click to track a color)");
	}
	ofDrawBitmapString(buf, 20, 50);

}

void testApp::keyPressed  (int key){
	switch (key){
#ifdef _USE_LIVE_VIDEO
		case 's':
			vidGrabber[0].videoSettings();
			break;
#endif
		case 'f':
			ofToggleFullscreen();
			break;
		
		case ' ':
			iP.clear();
			currentPoint = 0;
			break;
		case '1':
			MODE = MODE_CALIBRATION;
			break;
		case '2':
			MODE = MODE_TRACKING;
			break;
		default:
			break;
	}
}
	
//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button) {
	
	if (MODE == MODE_CALIBRATION) {
		// if we have to view the current point more times
		while (currentView < NUM_VIEWS) {
			// keep the point
			float noise = (((random() % 100) / 100.0) - 0.5) * 2.0;
			float noise2 = (((random() % 100) / 100.0) - 0.5) * 2.0;
			float x_pt = (x+noise)/((float)W);
			float y_pt = (y+noise2)/((float)H);
			iP.push_back(cv::Point2f(x_pt,y_pt));
			printf("x: %f, y: %f\n", x_pt, y_pt);
			currentView++;
		}
		
		// we have enough views for this point, move on
		bTrain = false;
		currentPoint++;
		currentView = 0;
		
		
		// we're done collecting all the points
		if (currentPoint == NUM_CALIBRATION_PTS) {
			calibrateUserPosition();
		}		
	}
	else if(MODE == MODE_TRACKING) {
		// calculate local mouse x,y in image
		int mx = x % W;
		int my = y % H;
		
		// get hue value on mouse position
		findHue = hue.getPixels()[my*W+mx];
	}

	
	gui.mousePressed(x, y, button);
}
	
//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
	gui.mouseDragged(x, y, button);
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
	gui.mouseReleased();
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
	
}
	
void testApp::blobOn( int x, int y, int id, int order )
{
	
}
void testApp::blobMoved( int x, int y, int id, int order )
{
	if (MODE==MODE_TRACKING) {
		fprintf(fp, "%02d%02d%02d-%02d:%02d.%02d:%f,%f\n",
				ofGetDay(), ofGetMonth(), ofGetYear(),
				ofGetHours(), ofGetMinutes(), ofGetSeconds(),
				(x-W/2.0f)/(W/2.0f),
				(y-H/2.0f)/(H/2.0f));
	}
}
void testApp::blobOff( int x, int y, int id, int order )
{
	
}
	