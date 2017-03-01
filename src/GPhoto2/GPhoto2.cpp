/*
Copyright (c) 2017 Keith Butters
All rights reserved.
*/

#include "GPhoto2.h"


std::unique_ptr<GPhoto2Camera> GPhoto2Camera::create()
{
	return std::unique_ptr<GPhoto2Camera>( new GPhoto2Camera() );
}

GPhoto2Camera::GPhoto2Camera()
{
	mCamContext = gp_context_new();
	gp_file_new( &mCameraFile );
}

GPhoto2Camera::~GPhoto2Camera()
{
	gp_file_free( mCameraFile );
	gp_camera_free( mCamera );
	gp_context_cancel( mCamContext );
}

void GPhoto2Camera::waitForCameraConnection( const uint retryTime, const char *model, const char *port )
{
	ci::app::console() << "Waiting for camera..." << endl;
	while ( !mConnected ) {
		init( model, port );
		this_thread::sleep_for( std::chrono::milliseconds( retryTime ) );
	}
	ci::app::console() << "Camera Initialized." << endl;
}

int GPhoto2Camera::init( const char *model, const char *port )
{
	killall_PTPCamera;

	CameraAbilitiesList *abilitiesList;
	CameraAbilities		abilities;
	GPPortInfoList		*portInfoList;
	GPPortInfo			portInfo;
	
	int result;

	if ( model && port ) {

		gp_abilities_list_new( &abilitiesList );
		result = gp_abilities_list_load( abilitiesList, mCamContext );
		if ( result < GP_OK ) ci::app::console() << "Loading abilities: " << gp_result_as_string( result ) << endl;

		int m = gp_abilities_list_lookup_model( abilitiesList, model );
		if ( m < GP_OK ) ci::app::console() << "Looking up Camera Model: " << gp_result_as_string( m ) << endl;
		
		result = gp_abilities_list_get_abilities( abilitiesList, m, &abilities );
		if ( result < GP_OK ) ci::app::console() << "Getting abilities: " << gp_result_as_string( result ) << endl;
		
		result = gp_camera_set_abilities( mCamera, abilities );
		if ( result < GP_OK ) ci::app::console() << "Setting abilities: " << gp_result_as_string( result ) << endl;
		
		result = gp_port_info_list_new( &portInfoList );
		if ( result < GP_OK ) ci::app::console() << "Creating new PortInfoList: " << gp_result_as_string( result ) << endl;
		
		result = gp_port_info_list_load ( portInfoList );
		if ( result < GP_OK ) ci::app::console() << "Loading PortInfoList: " << gp_result_as_string( result ) << endl;
		
		result = gp_port_info_list_count( portInfoList );
		if ( result < GP_OK ) ci::app::console() << "List count: " << gp_result_as_string( result ) << endl;
		
		int p = gp_port_info_list_lookup_path( portInfoList, port );
		if ( p < GP_OK ) ci::app::console() << "Looking up port: " << gp_result_as_string( p ) << endl;
		
		int r = gp_port_info_list_get_info( portInfoList, p, &portInfo );
		if ( result < GP_OK ) ci::app::console() << "Getting port info: " << gp_result_as_string( r ) << endl;
		
		result = gp_camera_set_port_info( mCamera, portInfo );
		
	} else {
		result = gp_camera_new( &mCamera );
		if ( result < GP_OK ) ci::app::console() << "Creating Camera: " << gp_result_as_string( result ) << endl;
	}
	
	result = gp_camera_init( mCamera, mCamContext );

	if ( result == GP_ERROR_MODEL_NOT_FOUND ) return 0;
	
	if ( result < GP_OK ) {
		ci::app::console() << "Error initializing camera: " << gp_result_as_string( result ) << endl;
		return 0;
	}

	mConnected = 1;
	return 1;
}

// List available cameras to the console
int GPhoto2Camera::listCameras()
{
	// Detect all connected cameras ans list them to the console
	gp_list_new( &mCamList );
	int count = gp_camera_autodetect ( mCamList, mCamContext );
	ci::app::console() << count << " Camera(s) detected." << endl;
	ci::app::console() << "-----------------------------" << endl;
	
	for ( int i = 0; i < gp_list_count( mCamList ); i++ ) {
		const char *name;
		const char *value;
		gp_list_get_name( mCamList, i, &name );
		gp_list_get_value( mCamList, i, &value );
		ci::app::console() << (i+1) << ". " << name << " - " << value << std::endl;
    }
	
	gp_list_free( mCamList );
	return 1;
}

/*	------------------------
	Camera Configuration
	------------------------ */
int GPhoto2Camera::listCameraConfigNames()
{
	return 1;
}

const char * GPhoto2Camera::getCameraConfigValue( const char *name )
{

	CameraWidget		*widget, *child;
	CameraWidgetType	type;
	int	result;
	const char *value;
	
	result = gp_camera_get_config ( mCamera, &widget, mCamContext );
	
	if ( result < GP_OK ) {
		ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
		return 0;
	}
	
	result = gp_camera_get_single_config( mCamera, name, &child, mCamContext);
	
	if ( result == GP_OK ) {
		widget = child;
	} else {
		
		result = gp_camera_get_config( mCamera, &widget, mCamContext );
		if ( result < GP_OK ) {
			ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
			return 0;
		}
		
		result = gp_widget_get_child_by_name( widget, name, &child );
		if ( result < GP_OK ){
			result = gp_widget_get_child_by_label( widget, name, &child );
			if ( result < GP_OK ){
				ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
			}
		}
	}
	
	result = gp_widget_get_type( widget, &type );
	if ( result < GP_OK ) {
		ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
		return 0;
	}

	switch ( type ) {
	// char *
	case GP_WIDGET_TEXT:
	case GP_WIDGET_MENU:
	case GP_WIDGET_RADIO: {
		result = gp_widget_get_value ( child, &value );
		if ( result < GP_OK ) {
			ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
		}
		break;
	}
	// int
	case GP_WIDGET_TOGGLE:
	case GP_WIDGET_DATE: {
		int t;
		result = gp_widget_get_value ( child, &t );
		if ( result < GP_OK ) {
			ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
		}
		value = to_string( t ).c_str();
		break;
	}
	// float
	case GP_WIDGET_RANGE: {
		float f;
		result = gp_widget_get_value ( child, &f );
		if ( result < GP_OK ) {
			ci::app::console() << "Error retrieving camera configuration: " << gp_result_as_string( result ) << endl;
		}
		value = to_string( f ).c_str();
		break;
	}
	case GP_WIDGET_WINDOW:
	case GP_WIDGET_SECTION:
	case GP_WIDGET_BUTTON:{
		value = "";
		break;
	}
	}
	
	return value;
}

/*	------------------------
	Capturing Still Images
	------------------------ */

// Capture a photo to a cinder surface
int GPhoto2Camera::capture( ci::Surface *surface )
{
	auto myImage = captureToImage();
	if ( myImage != NULL ) {
		*surface = ci::Surface( myImage );
	}
	
	return 1;
}

// Capture a photo to a cinder gl Texture
int GPhoto2Camera::capture( ci::gl::Texture2dRef *texture )
{
	auto myImage = captureToImage();
	if ( myImage != NULL ) {
		*texture = ci::gl::Texture2d::create( myImage );
	}
	
	return 1;
}

// Capture a Preview to a cinder Surface
int GPhoto2Camera::capturePreview( ci::Surface *surface )
{
	auto myImage = capturePreviewToImage();
	if ( myImage != NULL ) {
		*surface = ci::Surface( myImage );
	}

	return 1;
}

// Capture a Preview to a cinder gl Texture
int GPhoto2Camera::capturePreview( ci::gl::Texture2dRef *texture)
{
	auto myImage = capturePreviewToImage();
	if ( myImage != NULL ) {
		*texture = ci::gl::Texture2d::create( myImage );
	}
	
	return 1;
}

// Internals
cinder::ImageSourceRef GPhoto2Camera::captureToImage()
{
	CameraFilePath filePath;
	
	// Needed for Canon. Gets overwritten on capture.
	strcpy( filePath.folder, "/" );
	strcpy( filePath.name, "tmp.jpg" );

	// Capture
	int result = gp_camera_capture( mCamera, GP_CAPTURE_IMAGE, &filePath, mCamContext);
	
	if ( result == GP_ERROR_IO ) {
		mConnected = false;
		return NULL;
	}
	
	if ( result < GP_OK ) {
		ci::app::console() << "Error capturing image (" << result << ") " << gp_result_as_string( result ) << endl;
		return NULL;
	}
	
	string folder( filePath.folder );
	string name( filePath.name );
	string path = folder + name;

	result = gp_camera_file_get( mCamera, filePath.folder, filePath.name, GP_FILE_TYPE_NORMAL, mCameraFile, mCamContext );
	
	if ( result < GP_OK ) {
		ci::app::console() << "Error retrieving camera file (" << result << ") " << gp_result_as_string( result ) << endl;
		return NULL;
	}
	
	unsigned long size;
	char *mimeType;
	char *data;
	
	gp_file_get_data_and_size( mCameraFile, (const char**)&data, &size );
	gp_file_get_mime_type( mCameraFile, (const char**)&mimeType );
	
	ci::ImageSourceRef myImage;
	
	try {
		auto myBuffer = ci::Buffer::create( data, size );
		myImage = ci::loadImage( ci::DataSourceBuffer::create( myBuffer ), ci::ImageSource::Options(), mimeType );
	} catch( cinder::ImageIoExceptionFailedLoad e ) {
		ci::app::console() << "Error: " << e.what() << endl;
	}
	
	return myImage;
}

cinder::ImageSourceRef GPhoto2Camera::capturePreviewToImage()
{
	ci::ImageSourceRef		myImage;

	// Capture the preview image
	int result = gp_camera_capture_preview( mCamera, mCameraFile, mCamContext);
	
	if ( result == GP_ERROR_IO ) {
		mConnected = false;
		return NULL;
	}
	
//	// Do something if capuring a preview is not supported by the camera
//	if ( result == GP_ERROR_NOT_SUPPORTED ) {
//		return NULL;
//	}
	
	if ( result < GP_OK ) {
		ci::app::console() << "Error capturing preview (" << result << ") " << gp_result_as_string( result ) << endl;
		return NULL;
	}
	
	unsigned long size;
	char *mimeType;
	char *data;
	
	gp_file_get_data_and_size( mCameraFile, (const char**)&data, &size );
	gp_file_get_mime_type( mCameraFile, (const char**)&mimeType );
	
	try {
		auto myBuffer = ci::Buffer::create( data, size );
		myImage = ci::loadImage( ci::DataSourceBuffer::create( myBuffer ), ci::ImageSource::Options(), mimeType );
	} catch( cinder::ImageIoExceptionFailedLoad e ) {
		ci::app::console() << "Error: " << e.what() << endl;
	}
	
	return myImage;
}


