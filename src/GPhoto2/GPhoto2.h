/*
Copyright (c) 2017 Keith Butters
All rights reserved.
*/

#pragma once

#include <stdlib.h>

#include "cinder/Cinder.h"
#include "cinder/Surface.h"
#include <gphoto2/gphoto2-camera.h>
#include <gphoto2/gphoto2-abilities-list.h>
#include <gphoto2/gphoto2-port.h>

#define killall_PTPCamera system( "killall -KILL PTPCamera &> /dev/null" );

using namespace std;

typedef std::shared_ptr<class GPhoto2Camera> GPhoto2CameraRef;

class GPhoto2Camera {
public:

	static std::unique_ptr<GPhoto2Camera> create();
	~GPhoto2Camera();
	
	// Wait for the camera to be connected, and initialize it
	void waitForCameraConnection( const uint retryTime = 5000, const char *model = NULL, const char *port = NULL );
	// Initialize a camera directly
	int init( const char *model = NULL, const char *port = NULL );
	// Check for a connected camera
	bool isConnected() { return mConnected; };
	// Printing important things to the console
	int listCameras();
	
	
	// Capture stills from the camera
	int capture();
	int capture( ci::fs::path *path );
	int capture( ci::Surface *surface );
	int capture( ci::gl::Texture2dRef *texture );
	
	// Capture the camera's preview image
	int capturePreview();
	int capturePreview( ci::fs::path *path );
	int capturePreview( ci::Surface *surface );
	int capturePreview( ci::gl::Texture2dRef *texture );
	
	// Camera Configuration Settings
	int listCameraConfigNames();
	const char * getCameraConfigValue( const char * name );
	vector<char *> getConfigChoices( const char * name);
	int setCameraConfigValue( const char *name, const char * value );
	
	// Convenience methods for common settings
	const char * getAperture();
	const char * getISO();
	const char * getShutterspeed();
	const char * getFocalLength();
	const char * getImageQuality();
	const char * getBatteryLevel();
	
	int setAperture( float f );
	int setISO( uint iso );
	int setShutterspeed( uint speed );
	int setFocalLength();
	int setAutoFocus( bool autoFocus = true );

	
//	// Capturing Video from the camera
//	int captureVideo( uint time );
//	int captureVideo( uint time, ci::fs::path *path );

	
private:
	GPhoto2Camera();
	
	bool mConnected = false;
	
	GPContext				*mCamContext;
	CameraList				*mCamList;
	Camera					*mCamera;
	CameraList				*mCameraConfigList;
	CameraAbilitiesList		*mCamAbilitiesList;
	CameraFile				*mCameraFile;

	cinder::ImageSourceRef	captureToImage();
	cinder::ImageSourceRef	capturePreviewToImage();
};
