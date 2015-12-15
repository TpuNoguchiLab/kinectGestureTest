#pragma once

#include "ofMain.h"
#include <kinect.h>
#include <Kinect.VisualGestureBuilder.h>

#define ERROR_CHECK( ret )											\
	if( FAILED( ret ) ){											\
	std::stringstream ss;										\
	ss << "failed " #ret " " << std::hex << ret << std::endl;	\
	throw std::runtime_error( ss.str().c_str() );				\
	}

template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL){
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}


typedef struct  JointState{
	int userNum;
	Joint joint[JointType::JointType_Count];

};

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y );
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);


	//kinect
	IKinectSensor *pSensor;
	bool initKinect();


	//color
	IColorFrameSource *pColorSource;
	IColorFrameSource *pColorFrameSource;
	IColorFrameReader *pColorReader;
	IFrameDescription* colorDescription;
	int colorWidth, colorHeight;
	unsigned int colorBytesPerPixels;

	//body
	IBodyFrameSource *pBodySource;
	IBodyFrameReader *pBodyReader;
	vector<JointState>jointList;


	//gesture
	IVisualGestureBuilderFrameReader *gestureFrameReader[BODY_COUNT];
	vector<IGesture *>gestures;

	//mapper
	ICoordinateMapper *pCoordinateMapper;

	//ofImage
	ofImage colorImage;


};
