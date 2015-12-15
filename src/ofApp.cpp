#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowShape(1920, 1080);
	ofSetFrameRate(30);

	if (!initKinect()) exit();

	colorImage.allocate(colorWidth, colorHeight, OF_IMAGE_COLOR_ALPHA);

}

//--------------------------------------------------------------
void ofApp::update(){

	// color
	IColorFrame* pColorFrame = nullptr;
	HRESULT hResult = pColorReader->AcquireLatestFrame(&pColorFrame);

	if (SUCCEEDED(hResult)) {
		hResult = pColorFrame->CopyConvertedFrameDataToArray(colorHeight * colorWidth * colorBytesPerPixels, colorImage.getPixels(), ColorImageFormat_Rgba);
		colorImage.update();
	}

	//body
	IBodyFrame *pBodyFrame = nullptr;
	hResult = pBodyReader -> AcquireLatestFrame(&pBodyFrame);

	if (SUCCEEDED(hResult)) {
		IBody *pBody[BODY_COUNT] = {0};
		hResult = pBodyFrame -> GetAndRefreshBodyData(BODY_COUNT, pBody);

		if (SUCCEEDED(hResult)) {

			jointList.clear();


			for (int count = 0; count < BODY_COUNT; count++) {

				BOOLEAN tracked;
				ERROR_CHECK( pBody[count]->get_IsTracked( &tracked ) );

				UINT64 trackingId;
				ERROR_CHECK( pBody[count]->get_TrackingId( &trackingId ) );

				JointState state;
				state.userNum = count;
				jointList.push_back(state);

				if (SUCCEEDED(hResult) && tracked) {

					Joint joint[JointType::JointType_Count];
					hResult = pBody[count] -> GetJoints(JointType::JointType_Count, joint);

					for (int type = 0; type < JointType_Count; type++) {
						jointList.back().joint[type] = joint[type];
					}

				}


				// Tracking IDの登録
				IVisualGestureBuilderFrameSource *gestureFrameSource;
				ERROR_CHECK( gestureFrameReader[count]->get_VisualGestureBuilderFrameSource( &gestureFrameSource ) );
				gestureFrameSource->put_TrackingId( trackingId );

			}
		}

		for (int count = 0; count < BODY_COUNT; count++){
			SafeRelease(pBody[count]);
		}

	}

	//gesture
	for( int count = 0; count < BODY_COUNT; count++ ){
		// 最新のGesture Frameを取得
		IVisualGestureBuilderFrame *gestureFrame;
		HRESULT ret = gestureFrameReader[count]->CalculateAndAcquireLatestFrame( &gestureFrame );
		if( FAILED( ret ) ){
			continue;
		}

		// Tracking IDの登録確認
		BOOLEAN tracked;
		ERROR_CHECK( gestureFrame->get_IsTrackingIdValid( &tracked ) );
		if( !tracked ){
			continue;
		}

		// Gestureの認識結果を取得
		for (int i = 0; i < gestures.size(); i++) {

			// Gestureの種類(Discrete or Continuous)を取得
			GestureType gestureType;
			ERROR_CHECK( gestures[i]->get_GestureType( &gestureType ) );

			switch( gestureType ){
			case GestureType::GestureType_Discrete:
				{

					// Discrete Gestureの認識結果を取得
					IDiscreteGestureResult *gestureResult;
					ERROR_CHECK( gestureFrame->get_DiscreteGestureResult( gestures[i], &gestureResult ) );

					// 検出結果を取得
					BOOLEAN detected;
					ERROR_CHECK( gestureResult->get_Detected( &detected ) );
					if( !detected ){
						break;
					}

					// 検出結果の信頼値を取得
					float confidence;
					ERROR_CHECK( gestureResult->get_Confidence( &confidence ) );
					//cout << "信頼地：" << confidence << endl;


					// Gestureの名前を取得
					std::wstring buffer( BUFSIZ, L'\0' );
					ERROR_CHECK( gestures[i]->get_Name( BUFSIZ, &buffer[0] ) );

					const std::wstring& str = &buffer[0];
					const std::wstring::size_type last = str.find_last_not_of( L" " );
					if( last == std::wstring::npos ){
						throw std::runtime_error( "failed " __FUNCTION__ );
					}
					str.substr( 0, last + 1 );

					const std::wstring temp = &buffer[0];
					const std::string name( temp.begin(), temp.end() );
					//cout << "ジェスチャーの名：" << name << endl;

					break;
				}
			case GestureType::GestureType_Continuous:
				{

					// Continuous Gestureの認識結果を取得
					IContinuousGestureResult *gestureResult;
					ERROR_CHECK( gestureFrame->get_ContinuousGestureResult( gestures[i], &gestureResult ) );

					// 進度を取得
					float progress;
					ERROR_CHECK( gestureResult->get_Progress( &progress ) );
					cout << "進度：" << progress << endl;

					// Gestureの名前を取得
					std::wstring buffer( BUFSIZ, L'\0' );
					ERROR_CHECK( gestures[i]->get_Name( BUFSIZ, &buffer[0] ) );

					const std::wstring& str = &buffer[0];
					const std::wstring::size_type last = str.find_last_not_of( L" " );
					if( last == std::wstring::npos ){
						throw std::runtime_error( "failed " __FUNCTION__ );
					}
					str.substr( 0, last + 1 );

					const std::wstring temp = &buffer[0];
					const std::string name( temp.begin(), temp.end() );
					//cout << "ジェスチャーの名：" << name << endl;

					break;
				}
			default:
				break;
			}

		}
	}


	//release
	SafeRelease(pColorFrame);
	SafeRelease(pBodyFrame);

}

//--------------------------------------------------------------
void ofApp::draw(){

	ofSetColor(255);
	colorImage.draw(0, 0);

	for (int i = 0; i < jointList.size(); i++) {
		for (int type = 0; type < JointType_Count; type++) {
			ColorSpacePoint colorSpacePoint = { 0 };
			pCoordinateMapper->MapCameraPointToColorSpace( jointList[i].joint[type].Position, &colorSpacePoint );
			int x = static_cast<int>( colorSpacePoint.X );
			int y = static_cast<int>( colorSpacePoint.Y );
			if( ( x >= 0 ) && ( x < colorWidth ) && ( y >= 0 ) && ( y < colorHeight ) ){

				ofSetColor(255,0,0);
				ofCircle(x,y,10);

			}
		}
	}

}


bool ofApp::initKinect() {
	//senspr
	HRESULT hResult = S_OK;
	hResult = GetDefaultKinectSensor(&pSensor);
	if (FAILED(hResult)) {
		std::cerr << "Error : GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::Open()" << std::endl;
		return -1;
	}

	//color
	hResult = pSensor->get_ColorFrameSource(&pColorSource);
	if (FAILED(hResult)){
		std::cerr << "Error : IKinectSensor::get_ColorFrameSource()" << std::endl;
		return -1;
	}

	hResult = pColorSource->OpenReader(&pColorReader);
	if (FAILED(hResult)){
		std::cerr << "Error : IColorFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	hResult = pColorSource->CreateFrameDescription(ColorImageFormat::ColorImageFormat_Rgba, &colorDescription);
	if (FAILED(hResult)){
		std::cerr << "Error : IColorFrameSource::get_FrameDescription()" << std::endl;
		return -1;
	}
	colorDescription->get_Width(&colorWidth);
	colorDescription->get_Height(&colorHeight);
	colorDescription->get_BytesPerPixel(&colorBytesPerPixels);

	//body
	hResult = pSensor -> get_BodyFrameSource(&pBodySource);
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_BodyFrameSource()" << std::endl;
		return -1;
	}
	hResult = pBodySource->OpenReader( &pBodyReader );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IBodyFrameSource::OpenReader()" << std::endl;
		return -1;
	}

	//mapper
	hResult = pSensor->get_CoordinateMapper( &pCoordinateMapper );
	if( FAILED( hResult ) ){
		std::cerr << "Error : IKinectSensor::get_CoordinateMapper()" << std::endl;
		return -1;
	}


	//gesture
	for( int count = 0; count < BODY_COUNT; count++ ){
		// Gesture Frame Sourceを取得
		IVisualGestureBuilderFrameSource *gestureFrameSource;
		ERROR_CHECK( CreateVisualGestureBuilderFrameSource( pSensor, 0, &gestureFrameSource ) );

		// Gesture Frame Readerを開く
		ERROR_CHECK( gestureFrameSource->OpenReader( &gestureFrameReader[count] ) );
		SafeRelease(gestureFrameSource);
	}

	// ファイル(*.gbd)からGesture Databaseを読み込む
	IVisualGestureBuilderDatabase *gestureDatabase;
	ERROR_CHECK( CreateVisualGestureBuilderDatabaseInstanceFromFile( L"SampleDatabase.gbd", &gestureDatabase ) );

	// Gesture Databaseに含まれるGestureの数を取得
	UINT gestureCount;
	ERROR_CHECK( gestureDatabase->get_AvailableGesturesCount( &gestureCount ) );
	cout << gestureCount << endl;

	// Gestureを取得
	gestures.resize( gestureCount );
	ERROR_CHECK( gestureDatabase->get_AvailableGestures( gestureCount, &gestures[0] ) );

	for( int count = 0; count < BODY_COUNT; count++ ){
		// Gestureを登録
		IVisualGestureBuilderFrameSource *gestureFrameSource;
		ERROR_CHECK( gestureFrameReader[count]->get_VisualGestureBuilderFrameSource( &gestureFrameSource ) );
		ERROR_CHECK( gestureFrameSource->AddGestures( gestureCount, &gestures[0] ) );

		for (int i = 0; i < gestures.size(); i++) {
			ERROR_CHECK( gestureFrameSource->SetIsEnabled( gestures[i], TRUE ) );
		}

	}
	SafeRelease(gestureDatabase);


	return 1;
}




//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
