// CanonDSLRCameraDriver.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "CanonDSLRCameraDriver.h"

namespace pcl
{

	EdsError EDSCALLBACK handleBulbExposureTimeStateEvent (EdsStateEvent event,
		EdsUInt32 parameter,
		EdsVoid * context)
	{
		EdsError err = EDS_ERR_OK;
		EdsUInt32* pI = (EdsUInt32*) context;
		*pI=parameter;

		return err;
	}

	EdsError EDSCALLBACK handleShutdownStateEvent (EdsStateEvent event,
		EdsUInt32 parameter,
		EdsVoid * context)
	{
		EdsError err = EDS_ERR_OK;
		EdsBool* pI = (EdsBool*) context;
		*pI=false;

		return err;
	}


	EdsError EDSCALLBACK handleDirItemCreatedObjectEvent( EdsObjectEvent event,
		EdsBaseRef object,
		EdsVoid * context)
	{
		EdsError err = EDS_ERR_OK;
		
		EdsDirectoryItemRef* pdirItem=(EdsDirectoryItemRef*) context;
		*pdirItem=object;
		object=NULL;
		
		// Object must be released
		if(object)
		{
			EdsRelease(object);
		}
		return err;
	}


	const char* PixInsightCanonDSLRCameraDriver::getImageFileName()
	{
		return fileName;
	}

	EdsError PixInsightCanonDSLRCameraDriver::getFirstCamera(EdsCameraRef *camera)
	{
		EdsError err = EDS_ERR_OK;
		EdsCameraListRef cameraList = NULL;
		EdsUInt32 count = 0;
		// Get camera list
		err = EdsGetCameraList(&cameraList);
		// Get number of cameras
		if(err == EDS_ERR_OK)
		{
			err = EdsGetChildCount(cameraList, &count);
			if(count == 0)
			{
				err = EDS_ERR_DEVICE_NOT_FOUND;
			}
		}
		// Get first camera retrieved
		if(err == EDS_ERR_OK)
		{
			err = EdsGetChildAtIndex(cameraList , 0 , camera);
		}
		// Release camera list
		if(cameraList != NULL)
		{
			EdsRelease(cameraList);
			cameraList = NULL;
		}
		return err;
	}

	EdsError PixInsightCanonDSLRCameraDriver::downloadImage(EdsDirectoryItemRef directoryItem,const EdsChar* filePath)
	{
		EdsError err = EDS_ERR_OK;
		// Get directory item information
		EdsDirectoryItemInfo dirItemInfo;
		err = EdsGetDirectoryItemInfo(directoryItem, & dirItemInfo);

		//memcpy(fileName,dirItemInfo.szFileName,256);

		EdsStreamRef stream;
		err = EdsCreateFileStream(filePath,kEdsFileCreateDisposition_CreateAlways,kEdsAccess_ReadWrite,&stream);
		
		// Download image
		if(err == EDS_ERR_OK)
		{

			int blockSize = 1024 * 1024; // 1MB at a time
			int remainingBytes = dirItemInfo.size;
			do {
				if (remainingBytes < blockSize) {
					blockSize = (int)(remainingBytes / 512) * 512;
				}
				remainingBytes -= blockSize;
				err = EdsDownload( directoryItem, blockSize, stream);
			} while (remainingBytes > 512);
			// now we need to download the final block
			err = EdsDownload( directoryItem, remainingBytes, stream);
			
		}
		// Issue notification that download is complete
		if(err == EDS_ERR_OK)
		{
			err = EdsDownloadComplete(directoryItem);
		}
		
		// Release stream
		if( stream != NULL)
		{
			EdsRelease(stream);
			stream = NULL;
		}


		return err;
	}



	PixInsightCanonDSLRCameraDriver::PixInsightCanonDSLRCameraDriver():cameraType(TypeDSLR),camera(NULL),theImageRef(NULL),isSDKLoaded(false),isConnected(false),expTime(0),isImageReady(false)
	{
		EdsError err = EDS_ERR_OK;
		if (!isSDKLoaded)
			err = EdsInitializeSDK();

		//err handling

		if (err==EDS_ERR_OK)
			isSDKLoaded=true;
		else
			throw 1;

		//for threads
		CoInitializeEx( NULL, COINIT_APARTMENTTHREADED );

		//get first camera
		err = getFirstCamera(&camera);
		// err handling

		// Set event handler
		err = EdsSetCameraStateEventHandler(camera, kEdsStateEvent_BulbExposureTime,
		handleBulbExposureTimeStateEvent, (EdsVoid*) &expTime);
		// err handling

		err = EdsSetCameraStateEventHandler(camera, kEdsStateEvent_Shutdown,
		handleShutdownStateEvent, (EdsBool*) &isConnected);
		// err handling

		//Set event handler
		err = EdsSetObjectEventHandler(camera, kEdsObjectEvent_DirItemCreated,
		handleDirItemCreatedObjectEvent, (EdsVoid*) &theImageRef);
		// err handling
		
	}

	void PixInsightCanonDSLRCameraDriver::Dispose()
	{
		// release camera ... reduce ref-count
		EdsRelease(camera);

		// release theImage  ... reduce ref-count
		EdsRelease(theImageRef);

		// for threads
		CoUninitialize();

		// unload library
		if(isSDKLoaded)
		{
			EdsTerminateSDK();
		}


	}

	void PixInsightCanonDSLRCameraDriver::SetLogger(void(*_theLogger)(String))
	{
		theLogger = _theLogger;
		theLogger(String("The Logger has been successfully set."));
	}

	short PixInsightCanonDSLRCameraDriver::BinX()
	{
		return -1;
	}

	short PixInsightCanonDSLRCameraDriver::BinY()
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetBinX(short binX)
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetBinY(short binY)
	{
		return -1;
	}

	IPixInsightCamera::CameraStateEnum PixInsightCanonDSLRCameraDriver::CameraState()
	{
		return CameraError;
	}

	long PixInsightCanonDSLRCameraDriver::CameraXSize()
	{
		return -1;

	}

	long PixInsightCanonDSLRCameraDriver::CameraYSize()
	{
		return -1;
	}

	bool PixInsightCanonDSLRCameraDriver::CanAbortExposure()
	{
		//ASCOM_WRAP_BOOL("CanAbortExposure");
		return false;
	}

	bool PixInsightCanonDSLRCameraDriver::CanAsymmetricBin()
	{
		//ASCOM_WRAP_BOOL("CanAsymmetricBin");
		return false;
	}

	bool PixInsightCanonDSLRCameraDriver::CanGetCoolerPower()
	{
		//ASCOM_WRAP_BOOL("CanGetCoolerPower");
		return false;
	}

	bool PixInsightCanonDSLRCameraDriver::CanPulseGuide()
	{
		//ASCOM_WRAP_BOOL("CanPulseGuide");
		return false;
	}

	bool PixInsightCanonDSLRCameraDriver::CanSetCCDTemperature()
	{
		return false;
	}

	bool PixInsightCanonDSLRCameraDriver::CanStopExposure()
	{
		//ASCOM_WRAP_BOOL("CanStopExposure");
		return false;
	}

	double PixInsightCanonDSLRCameraDriver::CCDTemperature()
	{
		return -1;
	}

	bool PixInsightCanonDSLRCameraDriver::Connected()
	{
		return isConnected;
	}

	int PixInsightCanonDSLRCameraDriver::ConnectCamera(  )
	{
		if (!isSDKLoaded)
			return -1;

		// Open session with camera
		EdsError err = EDS_ERR_OK;
		err = EdsOpenSession(camera);
		if(err == EDS_ERR_OK)
		{
			isConnected=true;
			return 1;
		}else if (err == EDS_ERR_INVALID_HANDLE)
		{
			err = getFirstCamera(&camera);
			return err;
		}
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::DisconnectCamera()
	{
		if (!isSDKLoaded)
			return -1;

		// Open session with camera
		EdsError err = EDS_ERR_OK;
		if (isConnected)
			err = EdsCloseSession(camera);

		if(err == EDS_ERR_OK)
		{
			isConnected=false;
			return 1;
		}
		return -1;
		//return theCameraPtr2.SetProperty( "Connected", false );
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetConnected(bool connectCamera)
	{
		if( connectCamera == true )
		{
			if( this->Connected() )
				return 1;
			else
				return this->ConnectCamera();
		}
		else
		{
			if( this->Connected() )
				return this->DisconnectCamera( );
			else
				return 1;
		}
	}

	bool PixInsightCanonDSLRCameraDriver::CoolerOn()
	{
		//ASCOM_WRAP_BOOL("CoolerOn");		
		return false;
	}

	int PixInsightCanonDSLRCameraDriver::SetCoolerOn(bool coolerOn)
	{
		return 0;
	}
	double PixInsightCanonDSLRCameraDriver::CoolerPower()
	{
		return 0;
	}

	String PixInsightCanonDSLRCameraDriver::Description()
	{
		return String("Canon DSLR Driver");
	}

	double PixInsightCanonDSLRCameraDriver::ElectronsPerADU()
	{
		return 1;
	}

	double PixInsightCanonDSLRCameraDriver::FullWellCapacity()
	{
		return 1;
	}

	bool PixInsightCanonDSLRCameraDriver::HasShutter()
	{
		//ASCOM_WRAP_BOOL("HasShutter");
		return false;
	}

	double PixInsightCanonDSLRCameraDriver::HeatSinkTemperature()
	{
		return 1;
	}


	//Application needs to own this memory...
	//This may need to be a pointer instead...

	void PixInsightCanonDSLRCameraDriver::downloadImageFromCamera(const EdsChar*  filePath)
	{
		
		EdsError err = EDS_ERR_OK;

		

		err = downloadImage(theImageRef,filePath);

		/*EdsImageRef imageRef;
		err = EdsCreateImageRef( stream,&imageRef );

		EdsImageInfo imageInfo;
		err = EdsGetImageInfo(imageRef, kEdsImageSrc_FullView,&imageInfo );*/

		

		
	}


	void PixInsightCanonDSLRCameraDriver::ImageArray(UInt16Image *theImage)
	{

		
		EdsStreamRef stream;
		EdsError err = EDS_ERR_OK;
		EdsCreateMemoryStreamFromPointer(**theImage,theImage->ImageSize(),&stream);

		
		//err = downloadImage(theImageRef,stream);

		EdsImageRef imageRef;
		err = EdsCreateImageRef( stream,&imageRef );

		EdsImageInfo imageInfo;
		err = EdsGetImageInfo(imageRef, kEdsImageSrc_FullView,&imageInfo );

		// Release stream
		if( stream != NULL)
		{
			EdsRelease(stream);
			stream = NULL;
		}

	}

	bool PixInsightCanonDSLRCameraDriver::ImageReady()
	{
		return isImageReady;
	}

	bool PixInsightCanonDSLRCameraDriver::IsPulseGuiding()
	{
		//ASCOM_WRAP_BOOL("IsPulseGuiding");
		return false;
	}

	String PixInsightCanonDSLRCameraDriver::LastError()
	{
		return String("");
	}
	double PixInsightCanonDSLRCameraDriver::LastExposureDuration()
	{
		return 1;
	}
	// Reports the actual exposure start in the FITS-standard CCYY-MM-DDThh:mm:ss[.sss...] format.
	String PixInsightCanonDSLRCameraDriver::LastExposureStartTime()
	{	
		return String("");
	}

	long PixInsightCanonDSLRCameraDriver::MaxADU()
	{
		return 1;
	}

	short PixInsightCanonDSLRCameraDriver::MaxBinX()
	{
		return -1;
	}

	short PixInsightCanonDSLRCameraDriver::MaxBinY()
	{
		return -1;
	}

	long PixInsightCanonDSLRCameraDriver::NumX()
	{
		return 4770;
	}

	long PixInsightCanonDSLRCameraDriver::NumY()
	{
		return 3178;
	}

	int PixInsightCanonDSLRCameraDriver::SetNumX(long numX)
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetNumY(long numY)
	{
		return -1;
	}

	double PixInsightCanonDSLRCameraDriver::PixelSizeX()
	{
		return -1;
	}

	double PixInsightCanonDSLRCameraDriver::PixelSizeY()
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetCCDTemperature(double ccdTemp)
	{
		return -1;	
	}

	double PixInsightCanonDSLRCameraDriver::GetSetCCDTemperature()
	{
		return -1;
	}

	double PixInsightCanonDSLRCameraDriver::StartX()
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetStartX(double setX)
	{
		return -1;	
	}

	double PixInsightCanonDSLRCameraDriver::StartY()
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::SetStartY(double setY)
	{
		return -1;
	}

	int PixInsightCanonDSLRCameraDriver::AbortExposure()
	{
		return 0;
	}

	int PixInsightCanonDSLRCameraDriver::PulseGuide(IPixInsightCamera::GuideDirection direction, long duration)
	{
		return -1;	
	}

	void PixInsightCanonDSLRCameraDriver::SetupDialog()
	{
	}

	void PixInsightCanonDSLRCameraDriver::StartExposure(double duration)
	{
		EdsError err;
		bool locked = false;
		
		err = EdsSendStatusCommand( camera, kEdsCameraStatusCommand_UILock, 0);
		if(err == EDS_ERR_OK)
		{
			locked = true;
		}
		if(err == EDS_ERR_OK)
		{
			err = EdsSendCommand( camera, kEdsCameraCommand_BulbStart, 0);
		}
		if(err != EDS_ERR_OK && locked)
		{
			err = EdsSendStatusCommand (camera, kEdsCameraStatusCommand_UIUnLock, 0);
		}
		expTime=0;
		while (expTime<=duration){
		}
		isImageReady=true;
		StopExposure(); 
	}

	void PixInsightCanonDSLRCameraDriver::StopExposure()
	{
		EdsError err;
		err = EdsSendCommand( camera ,kEdsCameraCommand_BulbEnd, 0);
		EdsSendStatusCommand(camera, kEdsCameraStatusCommand_UIUnLock, 0);
		

	}

}
