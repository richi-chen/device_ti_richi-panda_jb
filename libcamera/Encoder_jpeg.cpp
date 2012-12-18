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
 * @file Encoder_libjpeg.cpp
 *
 * This file encodes a YUV422I buffer to a jpeg
 * TODO(XXX): Need to support formats other than yuv422i
 *            Change interface to pre/post-proc algo framework
 *
 */

#undef  LOG_TAG
#define LOG_TAG "Encoder_libjpeg"

#include "Encoder_jpeg.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define LOGE	ALOGE
#define LOGW	ALOGW
#define LOGI	ALOGI

extern "C" {
#include "jpeglib.h"
#include "jerror.h"
}

namespace android {
#define ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

        struct string_pair {
                const char* string1;
                const char* string2;
        };
        static string_pair degress_to_exif_lut [] = {
                // degrees, exif_orientation
                {"0",	"1"},
                {"90",	"6"},
                {"180", "3"},
                {"270", "8"},
        };

	//Encoder struct
	struct libjpeg_destination_mgr : jpeg_destination_mgr
	{
		libjpeg_destination_mgr(uint8_t* input, int size);

		uint8_t* outbuf;
		int bufsize;
		size_t jpegsize;
	};

	static void libjpeg_init_destination (j_compress_ptr cinfo)
	{
		libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

		dest->next_output_byte = dest->outbuf;
		dest->free_in_buffer = dest->bufsize;
		dest->jpegsize = 0;
	}

	static boolean libjpeg_empty_output_buffer(j_compress_ptr cinfo)
	{
		libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;

		dest->next_output_byte = dest->outbuf;
		dest->free_in_buffer = dest->bufsize;
		return TRUE;
	}

	static void libjpeg_term_destination (j_compress_ptr cinfo)
	{
		libjpeg_destination_mgr* dest = (libjpeg_destination_mgr*)cinfo->dest;
		dest->jpegsize = dest->bufsize - dest->free_in_buffer;
	}

	libjpeg_destination_mgr::libjpeg_destination_mgr(uint8_t* dest, int destSize)
	{
		this->init_destination = libjpeg_init_destination;
		this->empty_output_buffer = libjpeg_empty_output_buffer;
		this->term_destination = libjpeg_term_destination;

		this->outbuf = dest;
		this->bufsize = destSize;

		jpegsize = 0;
	}

	//Convert YUV422 to RGB
	static int yuv422_to_rgb(void* pYUV, void* pRGB, int width, int height)
	{
		if (NULL == pYUV || NULL == pRGB)
		{
			return -1;
		}
		uint8_t* pYUVData = (uint8_t *)pYUV;
		uint8_t* pRGBData = (uint8_t *)pRGB;

		int Y1, U1, V1, Y2, R1, G1, B1, R2, G2, B2;
		int C1, D1, E1, C2;

		for (int i=0; i<height; ++i)
		{
			for (int j=0; j<width/2; ++j)
			{
				Y1 = *(pYUVData+i*width*2+j*4);
				V1 = *(pYUVData+i*width*2+j*4+1); //HACK, Swap U && V, have not find the result. Will fixed Later (Marvell-Ljfan)
				Y2 = *(pYUVData+i*width*2+j*4+2);
				U1 = *(pYUVData+i*width*2+j*4+3);
				C1 = Y1-16;
				C2 = Y2-16;
				D1 = U1-128;
				E1 = V1-128;
				R1 = ((298*C1 + 409*E1 + 128)>>8>255 ? 255 : (298*C1 + 409*E1 + 128)>>8);
				G1 = ((298*C1 - 100*D1 - 208*E1 + 128)>>8>255 ? 255 : (298*C1 - 100*D1 - 208*E1 + 128)>>8);
				B1 = ((298*C1+516*D1 +128)>>8>255 ? 255 : (298*C1+516*D1 +128)>>8);
				R2 = ((298*C2 + 409*E1 + 128)>>8>255 ? 255 : (298*C2 + 409*E1 + 128)>>8);
				G2 = ((298*C2 - 100*D1 - 208*E1 + 128)>>8>255 ? 255 : (298*C2 - 100*D1 - 208*E1 + 128)>>8);
				B2 = ((298*C2 + 516*D1 +128)>>8>255 ? 255 : (298*C2 + 516*D1 +128)>>8);
				*pRGBData++ = B1<0 ? 0 : B1;
				*pRGBData++ = G1<0 ? 0 : G1;
				*pRGBData++ = R1<0 ? 0 : R1;
				*pRGBData++ = B2<0 ? 0 : B2;
				*pRGBData++ = G2<0 ? 0 : G2;
				*pRGBData++ = R2<0 ? 0 : R2;
			}
		}
		return 0;
	}

	//Encoder rgb24 to jpeg
	static int rgb24_to_jpeg(uint8_t *rgb24data, libjpeg_destination_mgr* dest_mgr, jpeg_encoder_params* input)
	{
		struct jpeg_compress_struct cinfo;
		struct jpeg_error_mgr jerr;
		cinfo.err = jpeg_std_error(&jerr);

		jpeg_create_compress(&cinfo);

		cinfo.dest = dest_mgr;
		cinfo.image_width = input->out_width;
		cinfo.image_height = input->out_height;
		cinfo.input_components = 3;
		cinfo.in_color_space = JCS_RGB;
		cinfo.input_gamma = 1;

		jpeg_set_defaults(&cinfo);
		jpeg_set_quality(&cinfo, input->quality, TRUE);
		cinfo.dct_method = JDCT_IFAST;


		jpeg_start_compress(&cinfo, TRUE);

		JSAMPROW row_pointer[1];
		int row_stride;

		row_stride = cinfo.image_width * 3;

		while (cinfo.next_scanline < input->in_height) {
			row_pointer[0] = &rgb24data[cinfo.next_scanline * row_stride];
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
		}

		jpeg_finish_compress(&cinfo);
		jpeg_destroy_compress(&cinfo);
		return 0;
	}

	/* private member functions */
	size_t encode_jpeg(jpeg_encoder_params* input) {
		jpeg_compress_struct    cinfo;
		jpeg_error_mgr jerr;
		jpeg_destination_mgr jdest;
		uint8_t* src = NULL, *resize_src = NULL;
		uint8_t* row_tmp = NULL;
		uint8_t* row_src = NULL;
		uint8_t* row_uv = NULL; // used only for NV12

		uint8_t* pRGB = NULL; // used only for yuv2

		int out_width = 0, in_width = 0;
		int out_height = 0, in_height = 0;
		int bpp = 2; // for uyvy

		if (!input) {
			return 0;
		}
		LOGW("encoding...      \n\t"
				"in_width:        %d\n\t"
				"in_height        %d\n\t"
				"out_width:       %d\n\t"
				"out_height:      %d\n\t"
				"input->src:      %p\n\t"
				"input->dst:      %p\n\t"
				"input->quality:  %d\n\t"
				"input->src_size: %d\n\t"
				"input->dst_size: %d\n\t"
				"input->format:   %s\n",
				input->in_width,
				input->in_height,
				input->out_width,
				input->out_height,
				input->src,
				input->dst,
				input->quality,
				input->src_size,
				input->dst_size,
				input->format);

		out_width = input->out_width;
		out_height = input->out_height;
		in_width = input->in_width;
		in_height = input->in_height;
		src = input->src;
		input->jpeg_size = 0;

		libjpeg_destination_mgr dest_mgr(input->dst, input->dst_size);
		//Use for convert color
		pRGB = (uint8_t *)calloc(1, in_width * in_height * 3);


		// param check...
		if ((in_width < 2)          || (out_width < 2)      || (in_height < 2)       || (out_height < 2)
			||(input->src == NULL)  || (input->dst == NULL) || (input->quality < 1)  || (input->src_size < 1)
			||(input->dst_size < 1) || (input->format == NULL)
			)
		{
			goto exit;
		}

		if(strcmp(input->format, "yuv422i-yuyv") == 0)
		{
			LOGW("Encoder: format PIXEL_FORMAT_YUV422I");
			yuv422_to_rgb(src, pRGB, in_width, in_height);
			rgb24_to_jpeg(pRGB, &dest_mgr, input);
		}
		else
		{
			LOGW("Nothing to do!!!\n");
			goto exit;
		}

exit:
		//release buffer memory
		free(pRGB);
		pRGB = NULL;
		input->jpeg_size = dest_mgr.jpegsize;

		LOGW("dest_mgr.jpegsize %d\n", dest_mgr.jpegsize);

		return dest_mgr.jpegsize;
	}

        /* public static functions */
        const char* ExifElementsTable::degreesToExifOrientation(const char* degrees) {
	        for (unsigned int i = 0; i < ARRAY_SIZE(degress_to_exif_lut); i++) {
		        if (!strcmp(degrees, degress_to_exif_lut[i].string1)) {
			        return degress_to_exif_lut[i].string2;
		        }
	        }
	        return NULL;
        }

        void ExifElementsTable::stringToRational(const char* str, unsigned int* num, unsigned int* den) {
	        int len;
	        char * tempVal = NULL;

	        if (str != NULL) {
		        len = strlen(str);
		        tempVal = (char*) malloc( sizeof(char) * (len + 1));
	        }

	        if (tempVal != NULL) {
		        // convert the decimal string into a rational
		        size_t den_len;
		        char *ctx;
		        unsigned int numerator = 0;
		        unsigned int denominator = 0;
		        char* temp = NULL;

		        memset(tempVal, '\0', len + 1);
		        strncpy(tempVal, str, len);
		        temp = strtok_r(tempVal, ".", &ctx);

		        if (temp != NULL)
			        numerator = atoi(temp);

		        if (!numerator)
			        numerator = 1;

		        temp = strtok_r(NULL, ".", &ctx);
		        if (temp != NULL) {
			        den_len = strlen(temp);
			        if(HUGE_VAL == den_len ) {
				        den_len = 0;
			        }

			        denominator = static_cast<unsigned int>(pow(10, den_len));
			        numerator = numerator * denominator + atoi(temp);
		        } else {
			        denominator = 1;
		        }

		        free(tempVal);

		        *num = numerator;
		        *den = denominator;
	        }
        }

        bool ExifElementsTable::isAsciiTag(const char* tag) {
	        // TODO(XXX): Add tags as necessary
	        return (strcmp(tag, TAG_GPS_PROCESSING_METHOD) == 0);
        }

        void ExifElementsTable::insertExifToJpeg(unsigned char* jpeg, size_t jpeg_size) {
	        ReadMode_t read_mode = (ReadMode_t)(READ_METADATA | READ_IMAGE);

	        ResetJpgfile();
	        if (ReadJpegSectionsFromBuffer(jpeg, jpeg_size, read_mode)) {
		        jpeg_opened = true;
#ifdef ANDROID_API_JB_OR_LATER
		        create_EXIF(table, exif_tag_count, gps_tag_count, has_datetime_tag);
#else
		        create_EXIF(table, exif_tag_count, gps_tag_count);
#endif
	        }
        }

        int ExifElementsTable::insertExifThumbnailImage(const char* thumb, int len) {
	        int ret = 0;

	        if ((len > 0) && jpeg_opened) {
		        printf("ImageInfo.ThumbnailOffset = %d, ImageInfo.ThumbnailAtEnd = %d ", ImageInfo.ThumbnailOffset, ImageInfo.ThumbnailAtEnd);
		        ret = ReplaceThumbnailFromBuffer(thumb, len);
		        printf("insertExifThumbnailImage. ReplaceThumbnail(). ret=%d", ret);
	        }

	        return ret;
        }

        void ExifElementsTable::saveJpeg(unsigned char* jpeg, size_t jpeg_size) {
	        if (jpeg_opened) {
		        WriteJpegToBuffer(jpeg, jpeg_size);
		        DiscardData();
		        jpeg_opened = false;
	        }
        }

        /* public functions */
        ExifElementsTable::~ExifElementsTable() {
	        int num_elements = gps_tag_count + exif_tag_count;

	        for (int i = 0; i < num_elements; i++) {
	                	if (table[i].Value) {
		                	free(table[i].Value);
		        }
	        }

	        if (jpeg_opened) {
		        DiscardData();
	        }
        }

        int ExifElementsTable::insertElement(const char* tag, const char* value) {
	        int value_length = 0;
	        int ret = 0;

	        if (!value || !tag) {
		        return -1;
	        }

	        if (position >= MAX_EXIF_TAGS_SUPPORTED) {
		        printf("Max number of EXIF elements already inserted");
		        return 0;
	        }

	        if (isAsciiTag(tag)) {
		        value_length = sizeof(ExifAsciiPrefix) + strlen(value + sizeof(ExifAsciiPrefix));
	        } else {
		        value_length = strlen(value);
	        }

	        if (IsGpsTag(tag)) {
		        table[position].GpsTag = TRUE;
		        table[position].Tag = GpsTagNameToValue(tag);
		        gps_tag_count++;
	        } else {
		        table[position].GpsTag = FALSE;
		        table[position].Tag = TagNameToValue(tag);
		        exif_tag_count++;

		        if (strcmp(tag, TAG_DATETIME) == 0) {
		#ifdef ANDROID_API_JB_OR_LATER
		            has_datetime_tag = true;
		#else
		            // jhead isn't taking datetime tag...this is a WA
		            ImageInfo.numDateTimeTags = 1;
		            memcpy(ImageInfo.DateTime, value,
		                   MIN(ARRAY_SIZE(ImageInfo.DateTime), value_length + 1));
		#endif
		        }
	        }

	        table[position].DataLength = 0;
	        table[position].Value = (char*) malloc(sizeof(char) * (value_length + 1));

	        if (table[position].Value) {
		        memcpy(table[position].Value, value, value_length + 1);
		        table[position].DataLength = value_length + 1;
	        }

	        position++;
	        return ret;
        }
	
	int ExifElementsTable::setParametersEXIF(const CameraParameters &params)
	{
		int ret = NO_ERROR;
		const char *valstr = NULL;
		double gpsPos;

		if( ( valstr = params.get(CameraParameters::KEY_GPS_LATITUDE) ) != NULL )
		{
			gpsPos = strtod(valstr, NULL);
	
			if ( convertGPSCoord(gpsPos,
								 mEXIFData.mGPSData.mLatDeg,
								 mEXIFData.mGPSData.mLatMin,
								 mEXIFData.mGPSData.mLatSec,
								 mEXIFData.mGPSData.mLatSecDiv ) == NO_ERROR )
			{
	
				if ( 0 < gpsPos )
				{
					strncpy(mEXIFData.mGPSData.mLatRef, GPS_NORTH_REF, GPS_REF_SIZE);
				}
				else
				{
					strncpy(mEXIFData.mGPSData.mLatRef, GPS_SOUTH_REF, GPS_REF_SIZE);
				}
	
				mEXIFData.mGPSData.mLatValid = true;
			}
			else
			{
				mEXIFData.mGPSData.mLatValid = false;
			}
		}
		else
		{
			mEXIFData.mGPSData.mLatValid = false;
		}
	
		if( ( valstr = params.get(CameraParameters::KEY_GPS_LONGITUDE) ) != NULL )
		{
			gpsPos = strtod(valstr, NULL);
	
			if ( convertGPSCoord(gpsPos,
								 mEXIFData.mGPSData.mLongDeg,
								 mEXIFData.mGPSData.mLongMin,
								 mEXIFData.mGPSData.mLongSec,
								 mEXIFData.mGPSData.mLongSecDiv) == NO_ERROR )
			{
	
				if ( 0 < gpsPos )
				{
					strncpy(mEXIFData.mGPSData.mLongRef, GPS_EAST_REF, GPS_REF_SIZE);
				}
				else
				{
					strncpy(mEXIFData.mGPSData.mLongRef, GPS_WEST_REF, GPS_REF_SIZE);
				}
	
				mEXIFData.mGPSData.mLongValid= true;
			}
			else
			{
				mEXIFData.mGPSData.mLongValid = false;
			}
		}
		else
		{
			mEXIFData.mGPSData.mLongValid = false;
		}
	
		if( ( valstr = params.get(CameraParameters::KEY_GPS_ALTITUDE) ) != NULL )
		{
			gpsPos = strtod(valstr, NULL);
			mEXIFData.mGPSData.mAltitude = floor(fabs(gpsPos));
			if (gpsPos < 0) {
				mEXIFData.mGPSData.mAltitudeRef = 1;
			} else {
				mEXIFData.mGPSData.mAltitudeRef = 0;
			}
			mEXIFData.mGPSData.mAltitudeValid = true;
			}
		else
		{
			mEXIFData.mGPSData.mAltitudeValid= false;
		}
	
		if( (valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP)) != NULL )
		{
			long gpsTimestamp = strtol(valstr, NULL, 10);
			struct tm *timeinfo = gmtime( ( time_t * ) & (gpsTimestamp) );
			if ( NULL != timeinfo )
			{
				mEXIFData.mGPSData.mTimeStampHour = timeinfo->tm_hour;
				mEXIFData.mGPSData.mTimeStampMin = timeinfo->tm_min;
				mEXIFData.mGPSData.mTimeStampSec = timeinfo->tm_sec;
				mEXIFData.mGPSData.mTimeStampValid = true;
			}
			else
			{
				mEXIFData.mGPSData.mTimeStampValid = false;
			}
		}
		else
		{
			mEXIFData.mGPSData.mTimeStampValid = false;
		}
	
		if( ( valstr = params.get(CameraParameters::KEY_GPS_TIMESTAMP) ) != NULL )
		{
			long gpsDatestamp = strtol(valstr, NULL, 10);
			struct tm *timeinfo = gmtime( ( time_t * ) & (gpsDatestamp) );
			if ( NULL != timeinfo )
			{
				strftime(mEXIFData.mGPSData.mDatestamp, GPS_DATESTAMP_SIZE, "%Y:%m:%d", timeinfo);
				mEXIFData.mGPSData.mDatestampValid = true;
			}
			else
			{
				mEXIFData.mGPSData.mDatestampValid = false;
			}
		}
		else
		{
			mEXIFData.mGPSData.mDatestampValid = false;
		}
	
		if( ( valstr = params.get(CameraParameters::KEY_GPS_PROCESSING_METHOD) ) != NULL )
			{
			strncpy(mEXIFData.mGPSData.mProcMethod, valstr, GPS_PROCESSING_SIZE-1);
			mEXIFData.mGPSData.mProcMethodValid = true;
			}
		else
		{
			mEXIFData.mGPSData.mProcMethodValid = false;
		}

		return ret;
	}

	int ExifElementsTable::convertGPSCoord(double coord,int &deg,int &min,int &sec,int &secDivisor)
	{
		double tmp;
	
		if ( coord == 0 ) {
	
			LOGE("Invalid GPS coordinate");
	
			return -EINVAL;
		}
	
		deg = (int) floor(fabs(coord));
		tmp = ( fabs(coord) - floor(fabs(coord)) ) * GPS_MIN_DIV;
		min = (int) floor(tmp);
		tmp = ( tmp - floor(tmp) ) * ( GPS_SEC_DIV * GPS_SEC_ACCURACY );
		sec = (int) floor(tmp);
		secDivisor = GPS_SEC_ACCURACY;
	
		if( sec >= ( GPS_SEC_DIV * GPS_SEC_ACCURACY ) ) {
			sec = 0;
			min += 1;
		}
	
		if( min >= 60 ) {
			min = 0;
			deg += 1;
		}

		return NO_ERROR;
	}

	int ExifElementsTable::setupEXIF()
	{
		status_t ret = NO_ERROR;
		
		 if ( mEXIFData.mGPSData.mLatValid  )
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
			         sizeof(temp_value)/sizeof(char) - 1,
			         "%d/%d,%d/%d,%d/%d",
			         abs(mEXIFData.mGPSData.mLatDeg), 1,
			         abs(mEXIFData.mGPSData.mLatMin), 1,
			         abs(mEXIFData.mGPSData.mLatSec), abs(mEXIFData.mGPSData.mLatSecDiv));
			insertElement(TAG_GPS_LAT, temp_value);
		}
	
		if ( mEXIFData.mGPSData.mLatValid )
		{
			insertElement(TAG_GPS_LAT_REF, mEXIFData.mGPSData.mLatRef);
		}
	
		 if (  mEXIFData.mGPSData.mLongValid )
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
					 sizeof(temp_value)/sizeof(char) - 1,
					 "%d/%d,%d/%d,%d/%d",
					 abs(mEXIFData.mGPSData.mLongDeg), 1,
					 abs(mEXIFData.mGPSData.mLongMin), 1,
					 abs(mEXIFData.mGPSData.mLongSec), abs(mEXIFData.mGPSData.mLongSecDiv));
			insertElement(TAG_GPS_LONG, temp_value);
		}
	
		if (  mEXIFData.mGPSData.mLongValid  )
		{
			insertElement(TAG_GPS_LONG_REF, mEXIFData.mGPSData.mLongRef);
		}
	
		if ( mEXIFData.mGPSData.mAltitudeValid )
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
					 sizeof(temp_value)/sizeof(char) - 1,
					 "%d/%d",
					 abs( mEXIFData.mGPSData.mAltitude), 1);
			insertElement(TAG_GPS_ALT, temp_value);
		}
	
		if (  mEXIFData.mGPSData.mAltitudeValid) 
		{
			char temp_value[5];
			snprintf(temp_value,
					 sizeof(temp_value)/sizeof(char) - 1,
					 "%d", mEXIFData.mGPSData.mAltitudeRef);
			insertElement(TAG_GPS_ALT_REF, temp_value);
		}
		
		if (  mEXIFData.mGPSData.mTimeStampValid ) 
		{
			char temp_value[256]; // arbitrarily long string
			snprintf(temp_value,
					 sizeof(temp_value)/sizeof(char) - 1,
					 "%d/%d,%d/%d,%d/%d",
					 mEXIFData.mGPSData.mTimeStampHour, 1,
					 mEXIFData.mGPSData.mTimeStampMin, 1,
					 mEXIFData.mGPSData.mTimeStampSec, 1);
			insertElement(TAG_GPS_TIMESTAMP, temp_value);
		}

		if ( mEXIFData.mGPSData.mDatestampValid ) 
		{
			insertElement(TAG_GPS_DATESTAMP, mEXIFData.mGPSData.mDatestamp);
		}
	
		return ret;
	}
}
