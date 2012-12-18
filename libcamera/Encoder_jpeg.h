/*
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
* @file Encoder_libjpeg.h
*
* This defines API for camerahal to encode YUV using libjpeg
*
*/

#ifndef ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H
#define ANDROID_CAMERA_HARDWARE_ENCODER_LIBJPEG_H

#include <utils/threads.h>
#include <utils/RefBase.h>
#include <utils/Log.h>
#include <camera/Camera.h>
#include <camera/CameraParameters.h>

extern "C" {
#include "jhead.h"
}
namespace android {
/**
 * libjpeg encoder class - uses libjpeg to encode yuv
 */

#define MAX_EXIF_TAGS_SUPPORTED 30
typedef void (*encoder_libjpeg_callback_t) (void* main_jpeg, void* thumb_jpeg, void* back_buffer, void* exif);

static const char TAG_MODEL[] = "Model";
static const char TAG_MAKE[] = "Make";
static const char TAG_FOCALLENGTH[] = "FocalLength";
static const char TAG_DATETIME[] = "DateTime";
static const char TAG_IMAGE_WIDTH[] = "ImageWidth";
static const char TAG_IMAGE_LENGTH[] = "ImageLength";
static const char TAG_GPS_LAT[] = "GPSLatitude";
static const char TAG_GPS_LAT_REF[] = "GPSLatitudeRef";
static const char TAG_GPS_LONG[] = "GPSLongitude";
static const char TAG_GPS_LONG_REF[] = "GPSLongitudeRef";
static const char TAG_GPS_ALT[] = "GPSAltitude";
static const char TAG_GPS_ALT_REF[] = "GPSAltitudeRef";
static const char TAG_GPS_MAP_DATUM[] = "GPSMapDatum";
static const char TAG_GPS_PROCESSING_METHOD[] = "GPSProcessingMethod";
static const char TAG_GPS_VERSION_ID[] = "GPSVersionID";
static const char TAG_GPS_TIMESTAMP[] = "GPSTimeStamp";
static const char TAG_GPS_DATESTAMP[] = "GPSDateStamp";
static const char TAG_ORIENTATION[] = "Orientation";

//Paremeter use for encoder jpeg
typedef struct
{
	uint8_t* src;
	int src_size;
	uint8_t* dst;
	int dst_size;
	int quality;
	int in_width;
	int in_height;
	int out_width;
	int out_height;
	const char* format;
	size_t jpeg_size;
}jpeg_encoder_params;

#define GPS_MIN_DIV                 60
#define GPS_SEC_DIV                 60
#define GPS_SEC_ACCURACY            1000
#define GPS_TIMESTAMP_SIZE          6
#define GPS_DATESTAMP_SIZE          11
#define GPS_REF_SIZE                2
#define GPS_MAPDATUM_SIZE           100
#define GPS_PROCESSING_SIZE         100
#define GPS_VERSION_SIZE            4
#define GPS_NORTH_REF               "N"
#define GPS_SOUTH_REF               "S"
#define GPS_EAST_REF                "E"
#define GPS_WEST_REF                "W"

//Function use for to encoder a jpeg.
size_t encode_jpeg(jpeg_encoder_params* input);

typedef  struct _GPSData
{
	int mLongDeg, mLongMin, mLongSec, mLongSecDiv;
	char mLongRef[GPS_REF_SIZE];
	bool mLongValid;
	int mLatDeg, mLatMin, mLatSec, mLatSecDiv;
	char mLatRef[GPS_REF_SIZE];
	bool mLatValid;
	int mAltitude;
	unsigned char mAltitudeRef;
	bool mAltitudeValid;
	char mMapDatum[GPS_MAPDATUM_SIZE];
	bool mMapDatumValid;
	char mVersionId[GPS_VERSION_SIZE];
	bool mVersionIdValid;
	char mProcMethod[GPS_PROCESSING_SIZE];
	bool mProcMethodValid;
	char mDatestamp[GPS_DATESTAMP_SIZE];
	bool mDatestampValid;
	uint32_t mTimeStampHour;
	uint32_t mTimeStampMin;
	uint32_t mTimeStampSec;
	bool mTimeStampValid;
}GPSData;

typedef  struct _EXIFData
{
	GPSData mGPSData;
	bool mMakeValid;
	bool mModelValid;
}EXIFData;

class ExifElementsTable {
    public:
        ExifElementsTable() :
        gps_tag_count(0), exif_tag_count(0), position(0),jpeg_opened(false) {
            mEXIFData.mGPSData.mAltitudeValid = false;
            mEXIFData.mGPSData.mDatestampValid = false;
            mEXIFData.mGPSData.mLatValid = false;
            mEXIFData.mGPSData.mLongValid = false;
            mEXIFData.mGPSData.mMapDatumValid = false;
            mEXIFData.mGPSData.mProcMethodValid = false;
            mEXIFData.mGPSData.mVersionIdValid = false;
            mEXIFData.mGPSData.mTimeStampValid = false;
#ifdef ANDROID_API_JB_OR_LATER
            has_datetime_tag = false;
#endif
        }
        ~ExifElementsTable();

        status_t insertElement(const char* tag, const char* value);
        void insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size);
        status_t insertExifThumbnailImage(const char*, int);
        void saveJpeg(unsigned char* picture, size_t jpeg_size);
        static const char* degreesToExifOrientation(const char*);
        static void stringToRational(const char*, unsigned int*, unsigned int*);
        static bool isAsciiTag(const char* tag);
        int setParametersEXIF(const CameraParameters &params);
        int convertGPSCoord(double coord,int &deg,int &min,int &sec,int &secDivisor);
        int setupEXIF();
    private:
        ExifElement_t table[MAX_EXIF_TAGS_SUPPORTED];
        unsigned int gps_tag_count;
        unsigned int exif_tag_count;
        unsigned int position;
        bool jpeg_opened;
        EXIFData mEXIFData;
#ifdef ANDROID_API_JB_OR_LATER
        bool has_datetime_tag;
#endif
};

}//For namespace
#endif
