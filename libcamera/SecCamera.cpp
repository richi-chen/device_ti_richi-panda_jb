/*
 * Copyright 2008, The Android Open Source Project
 * Copyright 2010, Samsung Electronics Co. LTD
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

/*
************************************
* Filename: SecCamera.cpp
* Author:   Sachin P. Kamat
* Purpose:  This file interacts with the Camera and JPEG drivers.
*************************************
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "SecCamera"

#include <utils/Log.h>

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/poll.h>
#include "SecCamera.h"
#include <cutils/properties.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#define CAMERA_DUMP_TIME

#define LOGE	ALOGE
#define LOGW	ALOGW
#define LOGI	ALOGI

using namespace android;

#define CHECK(return_value)                                          \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail. errno: %s, m_camera_id = %d\n",           \
             __func__, __LINE__, strerror(errno), m_camera_id);      \
        return -1;                                                   \
    }


#define CHECK_PTR(return_value)                                      \
    if (return_value < 0) {                                          \
        LOGE("%s::%d fail, errno: %s, m_camera_id = %d\n",           \
             __func__,__LINE__, strerror(errno), m_camera_id);       \
        return NULL;                                                 \
    }

#define ALIGN_TO_32B(x)   ((((x) + (1 <<  5) - 1) >>  5) <<  5)
#define ALIGN_TO_128B(x)  ((((x) + (1 <<  7) - 1) >>  7) <<  7)
#define ALIGN_TO_8KB(x)   ((((x) + (1 << 13) - 1) >> 13) << 13)

namespace android {

// ======================================================================
// Camera controls

static struct timeval time_start;
static struct timeval time_stop;
static int fimc_v4l2_qbuf(int fp, int index);

unsigned long measure_time(struct timeval *start, struct timeval *stop)
{
    unsigned long sec, usec, time;

    sec = stop->tv_sec - start->tv_sec;

    if (stop->tv_usec >= start->tv_usec) {
        usec = stop->tv_usec - start->tv_usec;
    } else {
        usec = stop->tv_usec + 1000000 - start->tv_usec;
        sec--;
    }

    time = (sec * 1000000) + usec;

    return time;
}

static int get_pixel_depth(unsigned int fmt)
{
    int depth = 0;

    switch (fmt) {
    case V4L2_PIX_FMT_NV12:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV12T:
        depth = 12;
        break;
    case V4L2_PIX_FMT_NV21:
        depth = 12;
        break;
    case V4L2_PIX_FMT_YVU420:
        depth = 12;
        break;
    case V4L2_PIX_FMT_YUV420:
        depth = 12;
        break;

    case V4L2_PIX_FMT_RGB565:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_YVYU:
    case V4L2_PIX_FMT_UYVY:
    case V4L2_PIX_FMT_VYUY:
    case V4L2_PIX_FMT_NV16:
    case V4L2_PIX_FMT_NV61:
    case V4L2_PIX_FMT_YUV422P:
        depth = 16;
        break;

    case V4L2_PIX_FMT_RGB32:
        depth = 32;
        break;
    }

    return depth;
}

#define ALIGN_W(x)      (((x) + 0x7F) & (~0x7F))    // Set as multiple of 128
#define ALIGN_H(x)      (((x) + 0x1F) & (~0x1F))    // Set as multiple of 32
#define ALIGN_BUF(x)    (((x) + 0x1FFF)& (~0x1FFF)) // Set as multiple of 8K
char CAMERA_DEV_NAME[20];

static int fimc_poll(struct pollfd *events)
{
    int ret =0;

    /* 10 second delay is because sensor can take a long time
     * to do auto focus and capture in dark settings
     */
    /*ret = poll(events, 1, 10000);
    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in 10 secs..\n", __func__);
        return ret;
    }*/

    return ret;
}

int SecCamera::previewPoll(bool preview)
{
    int ret = 0;

    /*if (preview) {
        ret = poll(&m_events_c, 1, 1000);
    } else {
        ret = poll(&m_events_c2, 1, 1000);
    }

    if (ret < 0) {
        LOGE("ERR(%s):poll error\n", __func__);
        return ret;
    }

    if (ret == 0) {
        LOGE("ERR(%s):No data in 1 secs.. Camera Device Reset \n", __func__);
        return ret;
    }*/

    return ret;
}

static int xioctl(int fd, int request, void * arg)
{
    int r;

    do r = ioctl (fd, request, arg);
    while (-1 == r && EINTR == errno);

    return r;
}

int find_camera()
{
    DIR* dp;
    struct dirent* entry;
    struct stat statbuf;
    int size[2];
    bool find_one = false;
    bool find_keyword = false;
    char keyword[PROPERTY_VALUE_MAX];

    //default device name is /dev/video0
    strncpy(CAMERA_DEV_NAME, "/dev/video0", 20);

    if (property_get("camera.keyword", keyword, NULL) > 0) {
       find_keyword = true;
    }

    if ((dp = opendir("/dev/")) == 0)
    {
        LOGW("Open /dev/ folder failed\n");
        return 0;
    }

    while ((entry = readdir(dp)) != 0)
    {
        struct stat st;
        int    fd = -1;
        struct v4l2_capability cap;
        char dev_name[20];
        char cmd_line[30];

        if (!strstr(entry->d_name, "video"))
        {
            continue;
        }
        sprintf(dev_name, "/dev/%s", entry->d_name);
        LOGW("Will check %s", dev_name);

        if (-1 == stat (dev_name, &st))
        {
            LOGW ("Cannot identify '%s': %d, %s\n", dev_name, errno, strerror (errno));
            continue;
        }

        if (!S_ISCHR (st.st_mode)) {
            LOGW ("%s is no device\n", dev_name);
            continue;
        }

        fd = open (dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
            LOGW ("Cannot open '%s': %d, %s\n", dev_name, errno, strerror (errno));
            continue;
        }

        if (-1 == xioctl (fd, VIDIOC_QUERYCAP, &cap))
        {
            close(fd);
            continue;
        }

        LOGW("Driver info: %s", cap.card);
        if(!find_keyword || strstr((const char*)cap.card, keyword))
        {
            close(fd);
            LOGW("Find camera with keyword");
            strncpy(CAMERA_DEV_NAME, dev_name, 19);
            find_one = true;
            break;
        }
        LOGW("Find camera %s", dev_name);
        strncpy(CAMERA_DEV_NAME, dev_name, 19);
        close(fd);
        find_one = true;
    }

    closedir(dp);
    CAMERA_DEV_NAME[19] = '\0';

    return find_one;
}

static int fimc_v4l2_querycap(int fp)
{
    struct v4l2_capability cap;
    int ret = 0;

    ret = xioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYCAP failed\n", __func__);
        return -1;
    }

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        LOGE("ERR(%s):no capture devices\n", __func__);
        return -1;
    }

    return ret;
}

static const __u8* fimc_v4l2_enuminput(int fp, int index)
{
    static struct v4l2_input input;

    input.index = index;
    if (ioctl(fp, VIDIOC_ENUMINPUT, &input) != 0) {
        LOGE("ERR(%s):No matching index found\n", __func__);
        return NULL;
    }
    LOGI("Name of input channel[%d] is %s\n", input.index, input.name);

    return input.name;
}


static int fimc_v4l2_s_input(int fp, int index)
{
    struct v4l2_input input;
    int ret;

    input.index = index;

    ret = ioctl(fp, VIDIOC_S_INPUT, &input);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_INPUT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_s_fmt(int fp, int width, int height, unsigned int fmt, int flag_capture)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    LOGW(">>>>>>%s %d: set new size and format. W=%d, H=%d, format =%x", __FUNCTION__, __LINE__, width, height, fmt);
    memset(&pixfmt, 0, sizeof(pixfmt));

    pixfmt.width = width;
    pixfmt.height = height;
    pixfmt.pixelformat = fmt;

    pixfmt.sizeimage = (width * height * get_pixel_depth(fmt)) / 8;

    pixfmt.field = V4L2_FIELD_NONE;

    v4l2_fmt.fmt.pix = pixfmt;

    /* Set up for capture */
    ret = xioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return -1;
    }

    return 0;
}

static int fimc_v4l2_s_fmt_cap(int fp, int width, int height, unsigned int fmt)
{
    struct v4l2_format v4l2_fmt;
    struct v4l2_pix_format pixfmt;
    int ret;

    memset(&pixfmt, 0, sizeof(pixfmt));

    v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    pixfmt.width = width;
    pixfmt.height = height;
    pixfmt.pixelformat = fmt;
    if (fmt == V4L2_PIX_FMT_JPEG) {
        pixfmt.colorspace = V4L2_COLORSPACE_JPEG;
    }

    pixfmt.sizeimage = (width * height * get_pixel_depth(fmt)) / 8;

    v4l2_fmt.fmt.pix = pixfmt;

    //LOGE("ori_w %d, ori_h %d, w %d, h %d\n", width, height, v4l2_fmt.fmt.pix.width, v4l2_fmt.fmt.pix.height);

    /* Set up for capture */
    ret = ioctl(fp, VIDIOC_S_FMT, &v4l2_fmt);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_FMT failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_enum_fmt(int fp, unsigned int fmt)
{
    struct v4l2_fmtdesc fmtdesc;
    int found = 0;

    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmtdesc.index = 0;

    while (xioctl(fp, VIDIOC_ENUM_FMT, &fmtdesc) == 0) {
        if (fmtdesc.pixelformat == fmt) {
            LOGW("passed fmt = %#x found pixel format[%d]: %s\n", fmt, fmtdesc.index, fmtdesc.description);
            found = 1;
            break;
        }

        fmtdesc.index++;
    }

    if (!found) {
        LOGE("unsupported pixel format\n");
        return -1;
    }

    return 0;
}

static int fimc_v4l2_reqbufs(int fp, enum v4l2_buf_type type, int nr_bufs)
{
    struct v4l2_requestbuffers req;
    int ret;

    req.count = nr_bufs;
    req.type = type;
    req.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(fp, VIDIOC_REQBUFS, &req);
    if(nr_bufs)
    {
        LOGI("\n\n\n %s %d: Important! <<<<<Request buffer=%d, Actual get=%d>>>>\n\n", __FUNCTION__, __LINE__, nr_bufs, req.count);
    }

    //nr_bufs == 0 is use to free previous requested buffer
    if(nr_bufs != 0 && req.count < 2)
    {
        LOGE("ERR(%s): Actual get less then 2 buffer. Can not do preview\n", __func__);
        return -1;
    }
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_REQBUFS failed\n", __func__);
        return -1;
    }

    return req.count;
}

static int fimc_v4l2_mmap_for_Preview(int fp, int index, struct fimc_buffer *buffer, enum v4l2_buf_type type)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    LOGI("%s :", __func__);

    v4l2_buf.type = type;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = xioctl(fp , VIDIOC_QUERYBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QUERYBUF failed\n", __func__);
        return -1;
    }

    buffer->length = v4l2_buf.length;
    if ((buffer->start = (char *)mmap(0, v4l2_buf.length,
                                         PROT_READ | PROT_WRITE, MAP_SHARED,
                                         fp, v4l2_buf.m.offset)) < 0) {
         LOGE("%s %d] mmap() failed\n",__func__, __LINE__);
         return -1;
    }

    LOGI("%s: buffer->start = %p v4l2_buf.length = %d",
         __func__, buffer->start, v4l2_buf.length);

    return 0;
}

static int fimc_v4l2_munmap_for_Preview(int fp, int index, struct fimc_buffer *buffer, enum v4l2_buf_type type)
{
    struct v4l2_buffer v4l2_buf;
    int ret;
    LOGW("!!!!!!!!!%s %d. For %d st buffer", __FUNCTION__, __LINE__, index);
    if (-1 == munmap(buffer->start, buffer->length)) {
         LOGE("%s %d] munmap() failed\n",__func__, __LINE__);
         return -1;
    }

    return 0;
}

static int fimc_v4l2_streamon(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    ret = xioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMON failed\n", __func__);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_streamoff(int fp)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret;

    LOGW("%s :", __func__);
    ret = xioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_STREAMOFF failed. errno: %s, fp = %d\n", __func__, strerror(errno), fp);
        return ret;
    }

    return ret;
}

static int fimc_v4l2_qbuf(int fp, int index)
{
    struct v4l2_buffer v4l2_buf;
    int ret;

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;
    v4l2_buf.index = index;

    ret = xioctl(fp, VIDIOC_QBUF, &v4l2_buf);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_QBUF failed. errno: %s, fp = %d\n", __func__, strerror(errno), fp);
        return ret;
    }

    return 0;
}

static int fimc_v4l2_dqbuf(int fp)
{
    struct v4l2_buffer v4l2_buf;
    int ret;
	fd_set fds;
	struct timeval tv;
	int r;
	
	FD_ZERO (&fds);
	FD_SET (fp, &fds);
	
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	
	r = select (fp + 1, &fds, NULL, NULL, &tv);

    v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_buf.memory = V4L2_MEMORY_MMAP;

    ret = xioctl(fp, VIDIOC_DQBUF, &v4l2_buf);
    if (ret < 0) {
        //LOGE("ERR(%s):VIDIOC_DQBUF failed, dropped frame. Error = %s fp = %d\n", __func__, strerror(errno), fp);
        return ret;
    }

    return v4l2_buf.index;
}

static int fimc_v4l2_g_ctrl(int fp, unsigned int id)
{
    struct v4l2_control ctrl;
    /*int ret;

    ctrl.id = id;

    ret = xioctl(fp, VIDIOC_G_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s): VIDIOC_G_CTRL(id = 0x%x (%d)) failed, ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, ret);
        return ret;
    }*/

    return ctrl.value;
}

static int fimc_v4l2_s_ctrl(int fp, unsigned int id, unsigned int value)
{
    struct v4l2_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.value = value;

    ret = xioctl(fp, VIDIOC_S_CTRL, &ctrl);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_CTRL(id = %#x (%d), value = %d) failed ret = %d\n",
             __func__, id, id-V4L2_CID_PRIVATE_BASE, value, ret);

        return ret;
    }

    return ctrl.value;
}

static int fimc_v4l2_s_ext_ctrl(int fp, unsigned int id, void *value)
{
    /*struct v4l2_ext_controls ctrls;
    struct v4l2_ext_control ctrl;
    int ret;

    ctrl.id = id;
    ctrl.reserved = value;

    ctrls.ctrl_class = V4L2_CTRL_CLASS_CAMERA;
    ctrls.count = 1;
    ctrls.controls = &ctrl;

    ret = xioctl(fp, VIDIOC_S_EXT_CTRLS, &ctrls);
    if (ret < 0)
        LOGE("ERR(%s):VIDIOC_S_EXT_CTRLS failed\n", __func__);*/

    return 0;//ret;
}

static int fimc_v4l2_g_parm(int fp, struct v4l2_streamparm *streamparm)
{
    int ret;

   /* streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = xioctl(fp, VIDIOC_G_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_G_PARM failed\n", __func__);
        return -1;
    }

    LOGW("%s : timeperframe: numerator %d, denominator %d\n", __func__,
            streamparm->parm.capture.timeperframe.numerator,
            streamparm->parm.capture.timeperframe.denominator);*/

    return 0;
}

static int fimc_v4l2_s_parm(int fp, struct v4l2_streamparm *streamparm)
{
    int ret;

    /*streamparm->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = xioctl(fp, VIDIOC_S_PARM, streamparm);
    if (ret < 0) {
        LOGE("ERR(%s):VIDIOC_S_PARM failed\n", __func__);
        return ret;
    }*/

    return 0;
}

// ======================================================================
// Constructor & Destructor

SecCamera::SecCamera() :
            m_flag_init(0),
            m_camera_id(CAMERA_ID_BACK),
            m_cam_fd(-1),
            m_preview_v4lformat(V4L2_PIX_FMT_NV21), //m_preview_v4lformat is only identify which format clent request. Not V4L2 used.
            m_preview_width      (0),
            m_preview_height     (0),
            m_preview_max_width  (DEFAULT_PREVIEW_WIDTH),
            m_preview_max_height (DEFAULT_PREVIEW_HEIGHT),
            m_snapshot_v4lformat(-1),
            m_snapshot_width      (0),
            m_snapshot_height     (0),
            m_snapshot_max_width  (DEFAULT_SNAPSHOT_WIDTH),
            m_snapshot_max_height (DEFAULT_SNAPSHOT_HEIGHT),
            m_angle(-1),
            m_anti_banding(-1),
            m_wdr(-1),
            m_anti_shake(-1),
            m_zoom_level(-1),
            m_object_tracking(-1),
            m_smart_auto(-1),
            m_beauty_shot(-1),
            m_vintage_mode(-1),
            m_face_detect(-1),
            m_gps_enabled(false),
            m_gps_latitude(-1),
            m_gps_longitude(-1),
            m_gps_altitude(-1),
            m_gps_timestamp(-1),
            m_vtmode(0),
            m_sensor_mode(-1),
            m_shot_mode(-1),
            m_exif_orientation(-1),
            m_blur_level(-1),
            m_chk_dataline(-1),
            m_video_gamma(-1),
            m_slow_ae(-1),
            m_camera_af_flag(-1),
            m_flag_camera_start(0),
            m_flag_camera_picture_start(0),
            m_flag_request_buffer(0),
            m_jpeg_thumbnail_width (0),
            m_jpeg_thumbnail_height(0),
            m_jpeg_quality(100),
            m_preview_buf_num(4),
            bNeedDumpTime(false)
{
    m_params = (struct sec_cam_parm*)&m_streamparm.parm.raw_data;
    struct v4l2_captureparm capture;
    m_params->capture.timeperframe.numerator = 1;
    m_params->capture.timeperframe.denominator = 0;
    m_params->contrast = -1;
    m_params->effects = -1;
    m_params->brightness = -1;
    m_params->flash_mode = -1;
    m_params->focus_mode = -1;
    m_params->iso = -1;
    m_params->metering = -1;
    m_params->saturation = -1;
    m_params->scene_mode = -1;
    m_params->sharpness = -1;
    m_params->white_balance = -1;

    memset(&m_capture_buf, 0, sizeof(m_capture_buf));

    LOGW("%s :", __func__);
}

SecCamera::~SecCamera()
{
    LOGW("%s :", __func__);
}

int SecCamera::initCamera(int index)
{
    LOGW("%s :", __func__);
    int ret = 0;

    if (!m_flag_init) {
        /* Arun C
         * Reset the lense position only during camera starts; don't do
         * reset between shot to shot
         */
        m_camera_af_flag = -1;

        m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR);
        if (m_cam_fd < 0) {
            LOGE("ERR(%s):Cannot open %s (error : %s)\n", __func__, CAMERA_DEV_NAME, strerror(errno));
            LOGI("(%s): will research camera agian!", __func__);
            find_camera();
            m_cam_fd = open(CAMERA_DEV_NAME, O_RDWR);
            if(m_cam_fd < 0) {
                LOGE("ERR(%s):Cannot open %s (error : %s) after research\n", __func__, CAMERA_DEV_NAME, strerror(errno));
                return -1;
            }
        }
        LOGI("%s: open(%s) --> m_cam_fd %d", __FUNCTION__, CAMERA_DEV_NAME, m_cam_fd);

        ret = fimc_v4l2_querycap(m_cam_fd);
        CHECK(ret);

        m_camera_id = index;

        sprintf(support_preview_size, "");
        sprintf(default_picture_size, "");
        {
            int largestSize[2];
            struct v4l2_fmtdesc fmtdesc;
            fmtdesc.index=0;
            fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
            largestSize[0] = 0;
            largestSize[1] = 0;

            while(xioctl(m_cam_fd, VIDIOC_ENUM_FMT, &fmtdesc)!=-1)
            {
                fmtdesc.index++;

                v4l2_frmsizeenum frame;
                frame.index = 0;
                frame.pixel_format = fmtdesc.pixelformat;
                frame.type = V4L2_FRMSIZE_TYPE_DISCRETE;

                //Only check YUYV format now, for other format, we will support later
                if(fmtdesc.pixelformat != V4L2_PIX_FMT_YUYV)
                {
                    continue;
                }

                while(xioctl(m_cam_fd, VIDIOC_ENUM_FRAMESIZES, &frame)!=-1)
                {
                    if(frame.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                    {
                        frame.index++;
                        char temp_size[20];

                        if(frame.discrete.width > largestSize[0])
                        {
                            largestSize[0] = frame.discrete.width;
                            largestSize[1] = frame.discrete.height;
                        }	
                        sprintf(temp_size, "%dx%d", frame.discrete.width, frame.discrete.height);
                        if(!strstr(support_preview_size, temp_size))
                        {
                             if(!strstr(support_preview_size, "x"))
                             {
                                  sprintf(support_preview_size, "%s%dx%d", support_preview_size, frame.discrete.width, frame.discrete.height);
                             }
                             else
                             {
                                 sprintf(support_preview_size, "%s,%dx%d", support_preview_size, frame.discrete.width, frame.discrete.height);
                             }
                       }
                    }
                    else if (frame.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
                    {
                        LOGW("Do not support continuous mode now");
                        break;
                    }
                }
            }
            sprintf(default_picture_size, "%dx%d", largestSize[0], largestSize[1]);
            //Add 640x480 and 320x240 to enable the picture size setting UI in Camera.apk
            if(strstr(support_preview_size, "640x480") && !strstr(default_picture_size, "640x480"))
            {
                sprintf(default_picture_size, "%s,640x480", default_picture_size);
            }
            if(strstr(support_preview_size, "320x240") && !strstr(default_picture_size, "320x240"))
            {
                sprintf(default_picture_size, "%s,320x240", default_picture_size);
            }
            m_preview_max_width  = largestSize[0];
            m_preview_max_height  = largestSize[1];
            m_snapshot_max_width  = largestSize[0];
            m_snapshot_max_height = largestSize[1];
        }

        LOGW("Support preview size is %s, default picture size is %s ", support_preview_size, default_picture_size);
        //Get state for Exposure
        {
            struct v4l2_control control;
            memset (&ExposureCtrl, 0, sizeof (ExposureCtrl));
            ExposureCtrl.id = V4L2_CID_EXPOSURE;
            if (-1 == xioctl (m_cam_fd, VIDIOC_QUERYCTRL, &ExposureCtrl)) {
                LOGI("Query Exposure faile: Not supported");
            } else if (ExposureCtrl.flags & V4L2_CTRL_FLAG_DISABLED) {
                LOGI ("Exposure: Not supported");
            } else {
                LOGI("Exposure info:%d, min=%d, max=%d, step=%d", ExposureCtrl.default_value, ExposureCtrl.minimum, ExposureCtrl.maximum, ExposureCtrl.step);
            }
        }
        m_flag_init = 1;
        LOGI("%s : initialized", __FUNCTION__);
    }
    return 0;
}

void SecCamera::resetCamera()
{
    LOGW("%s :", __func__);
    DeinitCamera();
    initCamera(m_camera_id);
}

int SecCamera::isCameraInit()
{
   return m_flag_init;
}

int SecCamera::setFormatAndAllocBuffer(int width, int height, unsigned int fmt)
{
    int ret;
    //Step 1
    if(m_flag_request_buffer)
    {
        LOGW("!!!!!!!!!%s %d.  Free previous buffers", __FUNCTION__, __LINE__);
        fimc_v4l2_streamoff(m_cam_fd);	
			
        for (int i = 0; i < m_preview_buf_num; i++) {
            ret = fimc_v4l2_qbuf(m_cam_fd, i);
            //CHECK(ret);
        }
		
        for (int i = 0; i < m_preview_buf_num; i++) {
            ret = fimc_v4l2_munmap_for_Preview(m_cam_fd, i, &m_preview_buf[i], V4L2_BUF_TYPE_VIDEO_CAPTURE);
            //CHECK(ret);
        }

        fimc_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, 0);		
    }

    fimc_v4l2_s_fmt(m_cam_fd, width, height, fmt, 0);

    //Step 4. To request the buffer.
    m_preview_buf_num = fimc_v4l2_reqbufs(m_cam_fd, V4L2_BUF_TYPE_VIDEO_CAPTURE, MAX_BUFFERS);
    CHECK(m_preview_buf_num);
	
    for (int i = 0; i < m_preview_buf_num; i++) {
        fimc_v4l2_mmap_for_Preview(m_cam_fd, i, &m_preview_buf[i], V4L2_BUF_TYPE_VIDEO_CAPTURE);
        //CHECK(ret);
    }

    for (int i = 0; i < m_preview_buf_num; i++) {
        ret = fimc_v4l2_qbuf(m_cam_fd, i);
	    CHECK(ret);
    }

    m_flag_request_buffer = 1;
    return 0;
}

int SecCamera::getPreviewBufferNumber()
{
    return m_preview_buf_num;
}

void SecCamera::DeinitCamera()
{
    LOGW("%s :", __func__);

    if (m_flag_init) {

        stopPreview();
        stopRecord();

        /* close m_cam_fd after stopRecord() because stopRecord()
         * uses m_cam_fd to change frame rate
         */
        LOGI("DeinitCamera: m_cam_fd(%d)", m_cam_fd);
        if (m_cam_fd > -1) {
            close(m_cam_fd);
            m_cam_fd = -1;
        }

        m_flag_init = 0;
    }
    else LOGI("%s : already deinitialized", __FUNCTION__);
}


int SecCamera::getCameraFd(void)
{
    return m_cam_fd;
}

// ======================================================================
// Preview

int SecCamera::startPreview(void)
{
    v4l2_streamparm streamparm;
    struct sec_cam_parm *parms;
    parms = (struct sec_cam_parm*)&streamparm.parm.raw_data;
    LOGW("%s :", __func__);

    // aleady started
    if (m_flag_camera_start > 0) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    /* enum_fmt, s_fmt sample */
    int ret = fimc_v4l2_enum_fmt(m_cam_fd, V4L2_PIX_FMT_YUYV);
    CHECK(ret);
    //Fixed format as YUYV422
    ret = setFormatAndAllocBuffer(m_preview_width, m_preview_height, V4L2_PIX_FMT_YUYV);
    CHECK(ret);

    LOGW("%s : m_preview_width: %d m_preview_height: %d m_angle: %d\n",
            __func__, m_preview_width, m_preview_height, m_angle);

    ret = fimc_v4l2_streamon(m_cam_fd);
    CHECK(ret);

    m_flag_camera_start = 1;

    LOGW("%s: got the first frame of the preview\n", __func__);

    return 0;
}

int SecCamera::stopPreview(void)
{
    int ret;
    struct stat st;

    LOGW("%s :", __func__);

    if (m_flag_camera_start == 0) {
        LOGW("%s: doing nothing because m_flag_camera_start is zero", __func__);
        return 0;
    }

    if (m_params->flash_mode == FLASH_MODE_TORCH)
        setFlashMode(FLASH_MODE_OFF);

    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    ret = fimc_v4l2_streamoff(m_cam_fd);
    //If camera have lost, also set m_flag_camera_start to 0
    if (-1 == stat (CAMERA_DEV_NAME, &st)) {
        m_flag_camera_start = 0;
    }
    CHECK(ret);

    m_flag_camera_start = 0;

    return ret;
}

//Recording
int SecCamera::startRecord(void)
{
    int ret, i;

    LOGW("%s :", __func__);

    // aleady started
    if (m_flag_record_start > 0) {
        LOGE("ERR(%s):Preview was already started\n", __func__);
        return 0;
    }

// chenhg
#if 0
    /* enum_fmt, s_fmt sample */
    ret = fimc_v4l2_enum_fmt(m_cam_fd, V4L2_PIX_FMT_YUYV);
    CHECK(ret);

    LOGI("%s: m_recording_width = %d, m_recording_height = %d\n",
         __func__, m_recording_width, m_recording_height);

    //ret = fimc_v4l2_s_fmt(m_cam_fd, m_recording_width,m_recording_height, V4L2_PIX_FMT_YUYV, 0);
    ret = setFormatAndAllocBuffer(m_recording_width,m_recording_height, V4L2_PIX_FMT_YUYV);
    CHECK(ret);

    ret = fimc_v4l2_streamon(m_cam_fd);
    CHECK(ret);
#endif
    m_flag_record_start = 1;
    Mutex::Autolock lock(mRecordLock);
    for (int index = 0; index < MAX_VIDEO_BUFFERS; index++) {
        record_frame_status[index] = 0;
    }

    return 0;
}

int SecCamera::stopRecord(void)
{
    int ret;

    LOGW("%s :", __func__);

    if (m_flag_record_start == 0) {
        LOGW("%s: doing nothing because m_flag_record_start is zero", __func__);
        return 0;
    }

    m_flag_record_start = 0;
#if 0 //Need Double check
    ret = fimc_v4l2_streamoff(m_cam_fd);
    CHECK(ret);
#endif
    //ret = fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FRAME_RATE,
    //                        FRAME_RATE_AUTO);
    //CHECK(ret);

    return 0;
}

int SecCamera::getPreview()
{
    int index;
    int ret;

    if (m_flag_camera_start == 0) {
        LOGE("ERR(%s):Start Camera Device Reset m_flag_camera_start=%d\n", __func__, m_flag_camera_start);
        /*
         * When there is no data for more than 1 second from the camera we inform
         * the FIMC driver by calling fimc_v4l2_s_input() with a special value = 1000
         * FIMC driver identify that there is something wrong with the camera
         * and it restarts the sensor.
         */
        /*stopPreview();
        // Reset Only Camera Device 
        ret = fimc_v4l2_querycap(m_cam_fd);
        CHECK(ret);
        if (fimc_v4l2_enuminput(m_cam_fd, m_camera_id))
            return -1;
        ret = fimc_v4l2_s_input(m_cam_fd, 1000);
        CHECK(ret);
        ret = startPreview();
        if (ret < 0) {
            LOGE("ERR(%s): startPreview() return %d\n", __func__, ret);
            return 0;
        }*/
        return 0;
    }

#ifdef CAMERA_DUMP_TIME
    char property[100];

    if (property_get("camera.dumptime", property, NULL) > 0) {
        bNeedDumpTime = atoi(property);
    } else {
        bNeedDumpTime = false;
    }
#endif

    if(bNeedDumpTime) {
        LOGI(">>>>>>>>>>>Start dequeue buffer");
    }

    index = fimc_v4l2_dqbuf(m_cam_fd);
    if (!(0 <= index && index < m_preview_buf_num)) {
        //LOGE("ERR(%s):wrong index = %d\n", __func__, index);
        return -1;
    }

    if(bNeedDumpTime) {
        LOGI(">>>>>>>>>>>Start queue buffer");
    }

    ret = fimc_v4l2_qbuf(m_cam_fd, index);
    CHECK(ret);

    if(bNeedDumpTime) {
        LOGI(">>>>>>>>>>>End queue buffer");
    }

    return index;
}

int SecCamera::getRecordFrame()
{
    if (m_flag_record_start == 0) {
        LOGE("%s: m_flag_record_start is 0", __func__);
        return -1;
    }
#if 0
    previewPoll(false);
    return fimc_v4l2_dqbuf(m_cam_fd);
#else
    Mutex::Autolock lock(mRecordLock);
    for (int index = 0; index < MAX_VIDEO_BUFFERS; index++) {
        if (record_frame_status[index] == 0) {
            record_frame_status[index] = 1;
            return index;
        }
    }
    return -1;
#endif
}

int SecCamera::releaseRecordFrame(int index)
{
    if (!m_flag_record_start) {
        /* this can happen when recording frames are returned after
         * the recording is stopped at the driver level.  we don't
         * need to return the buffers in this case and we've seen
         * cases where fimc could crash if we called qbuf and it
         * wasn't expecting it.
         */
        LOGI("%s: recording not in progress, ignoring", __func__);
        return 0;
    }

#if 0
    return fimc_v4l2_qbuf(m_cam_fd, index);
#else
    Mutex::Autolock lock(mRecordLock);
    record_frame_status[index] = 0;
    return 0;
#endif
}

int SecCamera::setPreviewSize(int width, int height, int pixel_format)
{
    LOGW("%s(width(%d), height(%d), format(%d))", __func__, width, height, pixel_format);

    int v4lpixelformat = pixel_format;

    if (v4lpixelformat == V4L2_PIX_FMT_YUV420)
        LOGW("PreviewFormat:V4L2_PIX_FMT_YUV420");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV12)
        LOGW("PreviewFormat:V4L2_PIX_FMT_NV12");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV12T)
        LOGW("PreviewFormat:V4L2_PIX_FMT_NV12T");
    else if (v4lpixelformat == V4L2_PIX_FMT_NV21)
        LOGW("PreviewFormat:V4L2_PIX_FMT_NV21");
    else if(v4lpixelformat == V4L2_PIX_FMT_YVU420)
        LOGW("PreviewFormat:V4L2_PIX_FMT_YVU420");
    else if (v4lpixelformat == V4L2_PIX_FMT_YUV422P)
        LOGW("PreviewFormat:V4L2_PIX_FMT_YUV422P");
    else if (v4lpixelformat == V4L2_PIX_FMT_YUYV)
        LOGW("PreviewFormat:V4L2_PIX_FMT_YUYV");
    else if (v4lpixelformat == V4L2_PIX_FMT_RGB565)
        LOGW("PreviewFormat:V4L2_PIX_FMT_RGB565");
    else
        LOGW("PreviewFormat:UnknownFormat");

    m_preview_width  = width;
    m_preview_height = height;
    m_preview_v4lformat = v4lpixelformat;

    return 0;
}

int SecCamera::getPreviewSize(int *width, int *height, int *frame_size)
{
    *width  = m_preview_width;
    *height = m_preview_height;
    *frame_size = m_frameSize(m_preview_v4lformat, m_preview_width, m_preview_height);

    return 0;
}

int SecCamera::getPreviewMaxSize(int *width, int *height)
{
    *width  = m_preview_max_width;
    *height = m_preview_max_height;

    return 0;
}

int SecCamera::getPreviewPixelFormat(void)
{
    return m_preview_v4lformat;
}


// ======================================================================
// Snapshot
/*
 * Devide getJpeg() as two funcs, setSnapshotCmd() & getJpeg() because of the shutter sound timing.
 * Here, just send the capture cmd to camera ISP to start JPEG capture.
 */
int SecCamera::setSnapshotCmd(void)
{
    LOGW("%s :", __func__);

    int ret = 0;

    LOG_TIME_DEFINE(0)
    LOG_TIME_DEFINE(1)
    if (m_flag_camera_start > 0  || m_flag_record_start > 0) {
        LOG_TIME_START(0)
        LOGW("WARN(%s):Camera was in preview or record, should have been stopped\n", __func__);
    }
	   
    if(m_flag_camera_picture_start)
    {
	LOGW("Alread in pictur mode");
    }

   m_flag_camera_picture_start = 1;

    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return 0;
    }

    LOG_TIME_START(1) // prepare
    int nframe = 1;

    ret = fimc_v4l2_enum_fmt(m_cam_fd, m_snapshot_v4lformat);
    CHECK(ret);

    //ret = fimc_v4l2_s_fmt(m_cam_fd, m_snapshot_width, m_snapshot_height, 0);
    LOGW("Set take picture size as [%d, %d]", m_snapshot_width, m_snapshot_height);
    ret = setFormatAndAllocBuffer(m_snapshot_width, m_snapshot_height, V4L2_PIX_FMT_YUYV);	
    CHECK(ret);

    ret = fimc_v4l2_streamon(m_cam_fd);
    CHECK(ret);
    LOG_TIME_END(1)

    return 0;
}

int SecCamera::endSnapshot(void)
{
    int ret;

    LOGI("%s :", __func__);
    /*if (m_capture_buf.start) {
        munmap(m_capture_buf.start, m_capture_buf.length);
        LOGI("munmap():virt. addr %p size = %d\n",
             m_capture_buf.start, m_capture_buf.length);
        m_capture_buf.start = NULL;
        m_capture_buf.length = 0;
    }*/
    return 0;
}

/*
 * Set Jpeg quality & exif info and get JPEG data from camera ISP
 */
unsigned char* SecCamera::getJpeg(int *jpeg_size, unsigned int *phyaddr)
{
    LOGW("%s :", __func__);

    int index, ret = 0;
    unsigned char *addr;

    LOG_TIME_DEFINE(2)
    ret = fimc_v4l2_streamon(m_cam_fd);
   // CHECK_PTR(ret);

    if(bNeedDumpTime) {
        LOGI(">>>>>>>>>>>Start dequeue buffer for take picture");
    }

    // capture, use the 2th buffer for encode, we find after change size, some camera's 1th buffer have garbage
    index = fimc_v4l2_dqbuf(m_cam_fd);
    fimc_v4l2_qbuf(m_cam_fd, index);
    index = fimc_v4l2_dqbuf(m_cam_fd);
    /*if (index != 0) {
        LOGE("ERR(%s):wrong index = %d\n", __func__, index);
        return NULL;
    }*/

    if(bNeedDumpTime) {
        LOGI(">>>>>>>>>>>End dequeue buffer for take pictur");
    }

    *phyaddr = (unsigned int)(m_preview_buf[index].start) ;
    *jpeg_size = m_preview_buf[index].length;

    LOG_TIME_START(2) // post
    //ret = fimc_v4l2_streamoff(m_cam_fd);
    CHECK_PTR(ret);
    LOG_TIME_END(2)

    return (unsigned char *)phyaddr;
}

int SecCamera::getPostViewOffset(void)
{
    return m_postview_offset;
}

int SecCamera::setSnapshotSize(int width, int height)
{
    LOGW("%s(width(%d), height(%d))", __func__, width, height);

    m_snapshot_width  = width;
    m_snapshot_height = height;

    return 0;
}

int SecCamera::getSnapshotSize(int *width, int *height, int *frame_size)
{
    *width  = m_snapshot_width;
    *height = m_snapshot_height;

    int frame = 0;

    frame = m_frameSize(m_snapshot_v4lformat, m_snapshot_width, m_snapshot_height);

    // set it big.
    if (frame == 0)
        frame = m_snapshot_width * m_snapshot_height * BPP;

    *frame_size = frame;

    return 0;
}

int SecCamera::getSnapshotMaxSize(int *width, int *height)
{
    *width  = m_snapshot_max_width;
    *height = m_snapshot_max_height;

    return 0;
}

int SecCamera::setSnapshotPixelFormat(int pixel_format)
{
    int v4lpixelformat= pixel_format;

    if (m_snapshot_v4lformat != v4lpixelformat) {
        m_snapshot_v4lformat = v4lpixelformat;
    }

#if defined(LOG_NDEBUG) && LOG_NDEBUG == 0
    if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUV420)
        LOGE("%s : SnapshotFormat:V4L2_PIX_FMT_YUV420", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV12)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV12", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV12T)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV12T", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_NV21)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_NV21", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUV422P)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_YUV422P", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_YUYV)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_YUYV", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_UYVY)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_UYVY", __func__);
    else if (m_snapshot_v4lformat == V4L2_PIX_FMT_RGB565)
        LOGD("%s : SnapshotFormat:V4L2_PIX_FMT_RGB565", __func__);
    else
        LOGD("SnapshotFormat:UnknownFormat");
#endif
    return 0;
}

int SecCamera::getSnapshotPixelFormat(void)
{
    return m_snapshot_v4lformat;
}

// ======================================================================
// Settings

int SecCamera::getCameraId(void)
{
    return m_camera_id;
}

// -----------------------------------

int SecCamera::setAutofocus(void)
{
    LOGW("%s :", __func__);

    /*if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SET_AUTO_FOCUS, AUTO_FOCUS_ON) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SET_AUTO_FOCUS", __func__);
        return -1;
    }*/

    return 0;
}

int SecCamera::getAutoFocusResult(void)
{
    int af_result, count, ret;

    af_result = 1;
    LOGW("%s : AF was successful, returning %d", __func__, af_result);
    return af_result;
}

int SecCamera::cancelAutofocus(void)
{
    LOGW("%s :", __func__);

    return 0;
}

// -----------------------------------

int SecCamera::zoomIn(void)
{
    LOGW("%s :", __func__);
    return 0;
}

int SecCamera::zoomOut(void)
{
    LOGW("%s :", __func__);
    return 0;
}

// -----------------------------------

int SecCamera::SetRotate(int angle)
{
    LOGE("%s(angle(%d))", __func__, angle);

    /*if (m_angle != angle) {
        switch (angle) {
        case -360:
        case    0:
        case  360:
            m_angle = 0;
            break;

        case -270:
        case   90:
            m_angle = 90;
            break;

        case -180:
        case  180:
            m_angle = 180;
            break;

        case  -90:
        case  270:
            m_angle = 270;
            break;

        default:
            LOGE("ERR(%s):Invalid angle(%d)", __func__, angle);
            return -1;
        }

        if (m_flag_camera_start) {
            if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_ROTATION, angle) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_ROTATION", __func__);
                return -1;
            }
        }
    }*/

    return 0;
}

int SecCamera::getRotate(void)
{
    LOGW("%s : angle(%d)", __func__, m_angle);
    return m_angle;
}

int SecCamera::setFrameRate(int frame_rate)
{
    LOGW("%s(FrameRate(%d))", __func__, frame_rate);

    /*if (frame_rate < FRAME_RATE_AUTO || FRAME_RATE_MAX < frame_rate )
        LOGE("ERR(%s):Invalid frame_rate(%d)", __func__, frame_rate);

    if (m_params->capture.timeperframe.denominator != (unsigned)frame_rate) {
        m_params->capture.timeperframe.denominator = frame_rate;
        if (m_flag_camera_start) {
            if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FRAME_RATE, frame_rate) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FRAME_RATE", __func__);
                return -1;
            }
        }
    }*/

    return 0;
}

// -----------------------------------

int SecCamera::setVerticalMirror(void)
{
    LOGW("%s :", __func__);

    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_VFLIP, 0) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_VFLIP", __func__);
        return -1;
    }*/

    return 0;
}

int SecCamera::setHorizontalMirror(void)
{
    LOGW("%s :", __func__);

    if (m_cam_fd <= 0) {
        LOGE("ERR(%s):Camera was closed\n", __func__);
        return -1;
    }

    /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_HFLIP, 0) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_HFLIP", __func__);
        return -1;
    }*/

    return 0;
}

// -----------------------------------
//Now only support auto-white-balane
int SecCamera::setWhiteBalance()
{
    LOGW("%s(set white_balance)", __func__);
    if (m_params->white_balance != 1) {
        m_params->white_balance = 1;
        if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_AUTO_WHITE_BALANCE, 1) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_WHITE_BALANCE", __func__);
            return -1;
        }
        LOGI("Set to auto-white-balance");
    }

    return 0;
}

int SecCamera::getWhiteBalance(void)
{
    LOGW("%s : white_balance(%d)", __func__, m_params->white_balance);
    return m_params->white_balance;
}

// -----------------------------------

int SecCamera::setBrightness(int brightness)
{
    LOGW("%s(brightness(%d))", __func__, brightness);

    m_params->brightness = brightness;
    if (m_flag_camera_start) {
        if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_EXPOSURE, brightness) < 0) {
            LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_BRIGHTNESS", __func__);
            return -1;
        }
    }

    return 0;
}

int SecCamera::getBrightness(void)
{
    LOGW("%s : brightness(%d)", __func__, m_params->brightness);
    return m_params->brightness;
}

// -----------------------------------

int SecCamera::setImageEffect(int image_effect)
{
    LOGW("%s(image_effect(%d))", __func__, image_effect);

    if (image_effect <= IMAGE_EFFECT_BASE || IMAGE_EFFECT_MAX <= image_effect) {
        LOGE("ERR(%s):Invalid image_effect(%d)", __func__, image_effect);
        return -1;
    }

    if (m_params->effects != image_effect) {
        m_params->effects = image_effect;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_EFFECT, image_effect) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_EFFECT", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getImageEffect(void)
{
    LOGW("%s : image_effect(%d)", __func__, m_params->effects);
    return m_params->effects;
}

//======================================================================
int SecCamera::setSceneMode(int scene_mode)
{
    LOGW("%s(scene_mode(%d))", __func__, scene_mode);

    if (scene_mode <= SCENE_MODE_BASE || SCENE_MODE_MAX <= scene_mode) {
        LOGE("ERR(%s):Invalid scene_mode (%d)", __func__, scene_mode);
        return -1;
    }

    if (m_params->scene_mode != scene_mode) {
        m_params->scene_mode = scene_mode;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SCENE_MODE, scene_mode) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SCENE_MODE", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getSceneMode(void)
{
    return m_params->scene_mode;
}

//======================================================================

int SecCamera::setFlashMode(int flash_mode)
{
    LOGW("%s(flash_mode(%d))", __func__, flash_mode);

    /*if (flash_mode <= FLASH_MODE_BASE || FLASH_MODE_MAX <= flash_mode) {
        LOGE("ERR(%s):Invalid flash_mode (%d)", __func__, flash_mode);
        return -1;
    }

    if (m_params->flash_mode != flash_mode) {
        m_params->flash_mode = flash_mode;
        if (m_flag_camera_start) {
            if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FLASH_MODE, flash_mode) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FLASH_MODE", __func__);
                return -1;
            }
        }
    }*/

    return 0;
}

int SecCamera::getFlashMode(void)
{
    return m_params->flash_mode;
}

//======================================================================

int SecCamera::setISO(int iso_value)
{
    LOGW("%s(iso_value(%d))", __func__, iso_value);
    if (iso_value < ISO_AUTO || ISO_MAX <= iso_value) {
        LOGE("ERR(%s):Invalid iso_value (%d)", __func__, iso_value);
        return -1;
    }

    if (m_params->iso != iso_value) {
        m_params->iso = iso_value;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ISO, iso_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ISO", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getISO(void)
{
    return m_params->iso;
}

//======================================================================

int SecCamera::setContrast(int contrast_value)
{
    LOGW("%s(contrast_value(%d))", __func__, contrast_value);

    if (contrast_value < CONTRAST_MINUS_2 || CONTRAST_MAX <= contrast_value) {
        LOGE("ERR(%s):Invalid contrast_value (%d)", __func__, contrast_value);
        return -1;
    }

    if (m_params->contrast != contrast_value) {
        m_params->contrast = contrast_value;
        if (m_flag_camera_start) {
            if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CONTRAST, contrast_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_CONTRAST", __func__);
                return -1;
            }
        }
    }

    return 0;
}

int SecCamera::getContrast(void)
{
    return m_params->contrast;
}

//======================================================================

int SecCamera::setSaturation(int saturation_value)
{
    LOGW("%s(saturation_value(%d))", __func__, saturation_value);

    if (saturation_value <SATURATION_MINUS_2 || SATURATION_MAX<= saturation_value) {
        LOGE("ERR(%s):Invalid saturation_value (%d)", __func__, saturation_value);
        return -1;
    }

    if (m_params->saturation != saturation_value) {
        m_params->saturation = saturation_value;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SATURATION, saturation_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SATURATION", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getSaturation(void)
{
    return m_params->saturation;
}

//======================================================================

int SecCamera::setExposure(int exposure_value)
{
    LOGW("%s(exposure_value))", __func__);

    m_params->exposure = exposure_value;
    //only support to auto now.
    if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_EXPOSURE_AUTO, exposure_value) < 0) {
        LOGE("ERR(%s):Fail on V4L2_CID_EXPOSURE_AUTO", __func__);
        return -1;
    } else {
        LOGW("%s %d. Set exposure to %d", __FUNCTION__, __LINE__, exposure_value);
    }
  
    return 0;
}

int SecCamera::getExposure(void)
{
    return m_params->exposure;
}

//======================================================================

int SecCamera::setSharpness(int sharpness_value)
{
    LOGW("%s(sharpness_value(%d))", __func__, sharpness_value);

    if (sharpness_value < SHARPNESS_MINUS_2 || SHARPNESS_MAX <= sharpness_value) {
        LOGE("ERR(%s):Invalid sharpness_value (%d)", __func__, sharpness_value);
        return -1;
    }

    if (m_params->sharpness != sharpness_value) {
        m_params->sharpness = sharpness_value;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SHARPNESS, sharpness_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SHARPNESS", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getSharpness(void)
{
    return m_params->sharpness;
}

//======================================================================

int SecCamera::setWDR(int wdr_value)
{
    LOGW("%s(wdr_value(%d))", __func__, wdr_value);

    if (wdr_value < WDR_OFF || WDR_MAX <= wdr_value) {
        LOGE("ERR(%s):Invalid wdr_value (%d)", __func__, wdr_value);
        return -1;
    }

    if (m_wdr != wdr_value) {
        m_wdr = wdr_value;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_WDR, wdr_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_WDR", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getWDR(void)
{
    return m_wdr;
}

//======================================================================

int SecCamera::setAntiShake(int anti_shake)
{
    LOGW("%s(anti_shake(%d))", __func__, anti_shake);

    if (anti_shake < ANTI_SHAKE_OFF || ANTI_SHAKE_MAX <= anti_shake) {
        LOGE("ERR(%s):Invalid anti_shake (%d)", __func__, anti_shake);
        return -1;
    }

    if (m_anti_shake != anti_shake) {
        m_anti_shake = anti_shake;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ANTI_SHAKE, anti_shake) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ANTI_SHAKE", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getAntiShake(void)
{
    return m_anti_shake;
}

//======================================================================


int SecCamera::setMetering(int metering_value)
{
    LOGW("%s(metering (%d))", __func__, metering_value);

    if (metering_value <= METERING_BASE || METERING_MAX <= metering_value) {
        LOGE("ERR(%s):Invalid metering_value (%d)", __func__, metering_value);
        return -1;
    }

    if (m_params->metering != metering_value) {
        m_params->metering = metering_value;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_METERING, metering_value) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_METERING", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getMetering(void)
{
    return m_params->metering;
}

//======================================================================

int SecCamera::setJpegQuality(int jpeg_quality)
{
    LOGW("%s(jpeg_quality (%d))", __func__, jpeg_quality);

    if (jpeg_quality < JPEG_QUALITY_ECONOMY || JPEG_QUALITY_MAX <= jpeg_quality) {
        LOGE("ERR(%s):Invalid jpeg_quality (%d)", __func__, jpeg_quality);
        return -1;
    }

    if (m_jpeg_quality != jpeg_quality) {
        m_jpeg_quality = jpeg_quality;
    }

    return 0;
}

int SecCamera::getJpegQuality(void)
{
    return m_jpeg_quality;
}

//======================================================================

int SecCamera::setZoom(int zoom_level)
{
    LOGW("%s(zoom_level (%d))", __func__, zoom_level);

    if (zoom_level < ZOOM_LEVEL_0 || ZOOM_LEVEL_MAX <= zoom_level) {
        LOGE("ERR(%s):Invalid zoom_level (%d)", __func__, zoom_level);
        return -1;
    }

    if (m_zoom_level != zoom_level) {
        m_zoom_level = zoom_level;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_ZOOM, zoom_level) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_ZOOM", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getZoom(void)
{
    return m_zoom_level;
}

//======================================================================

int SecCamera::setFocusMode(int focus_mode)
{
    LOGW("%s(focus_mode(%d))", __func__, focus_mode);

    if (FOCUS_MODE_MAX <= focus_mode) {
        LOGE("ERR(%s):Invalid focus_mode (%d)", __func__, focus_mode);
        return -1;
    }

    if (m_params->focus_mode != focus_mode) {
        m_params->focus_mode = focus_mode;

        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_FOCUS_MODE, focus_mode) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_FOCUS_MODE", __func__);
                return -1;
            }*/
        }
    }

    return 0;
}

int SecCamera::getFocusMode(void)
{
    return m_params->focus_mode;
}

//======================================================================

int SecCamera::setGPSLatitude(const char *gps_latitude)
{
    LOGW("%s(gps_latitude(%s))", __func__, gps_latitude);
    if (gps_latitude == NULL)
        m_gps_enabled = false;
    else {
        m_gps_enabled = true;
        m_gps_latitude = lround(strtod(gps_latitude, NULL) * 10000000);
    }

    LOGW("%s(m_gps_latitude(%ld))", __func__, m_gps_latitude);
    return 0;
}

int SecCamera::setGPSLongitude(const char *gps_longitude)
{
    LOGW("%s(gps_longitude(%s))", __func__, gps_longitude);
    if (gps_longitude == NULL)
        m_gps_enabled = false;
    else {
        m_gps_enabled = true;
        m_gps_longitude = lround(strtod(gps_longitude, NULL) * 10000000);
    }

    LOGW("%s(m_gps_longitude(%ld))", __func__, m_gps_longitude);
    return 0;
}

int SecCamera::setGPSAltitude(const char *gps_altitude)
{
    LOGW("%s(gps_altitude(%s))", __func__, gps_altitude);
    if (gps_altitude == NULL)
        m_gps_altitude = 0;
    else {
        m_gps_altitude = lround(strtod(gps_altitude, NULL) * 100);
    }

    LOGW("%s(m_gps_altitude(%ld))", __func__, m_gps_altitude);
    return 0;
}

int SecCamera::setGPSTimeStamp(const char *gps_timestamp)
{
    LOGW("%s(gps_timestamp(%s))", __func__, gps_timestamp);
    if (gps_timestamp == NULL)
        m_gps_timestamp = 0;
    else
        m_gps_timestamp = atol(gps_timestamp);

    LOGW("%s(m_gps_timestamp(%ld))", __func__, m_gps_timestamp);
    return 0;
}

int SecCamera::setGPSProcessingMethod(const char *gps_processing_method)
{
    LOGW("%s(gps_processing_method(%s))", __func__, gps_processing_method);
    /*memset(mExifInfo.gps_processing_method, 0, sizeof(mExifInfo.gps_processing_method));
    if (gps_processing_method != NULL) {
        size_t len = strlen(gps_processing_method);
        if (len > sizeof(mExifInfo.gps_processing_method)) {
            len = sizeof(mExifInfo.gps_processing_method);
        }
        memcpy(mExifInfo.gps_processing_method, gps_processing_method, len);
    }*/
    return 0;
}

//======================================================================

int SecCamera::setGamma(int gamma)
{
     LOGW("%s(gamma(%d))", __func__, gamma);

     if (gamma < GAMMA_OFF || GAMMA_MAX <= gamma) {
         LOGE("ERR(%s):Invalid gamma (%d)", __func__, gamma);
         return -1;
     }

     if (m_video_gamma != gamma) {
         m_video_gamma = gamma;
         if (m_flag_camera_start) {
             /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SET_GAMMA, gamma) < 0) {
                 LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SET_GAMMA", __func__);
                 return -1;
             }*/
         }
     }

     return 0;
}

//======================================================================

int SecCamera::setSlowAE(int slow_ae)
{
     LOGW("%s(slow_ae(%d))", __func__, slow_ae);

     if (slow_ae < GAMMA_OFF || GAMMA_MAX <= slow_ae) {
         LOGE("ERR(%s):Invalid slow_ae (%d)", __func__, slow_ae);
         return -1;
     }

     if (m_slow_ae!= slow_ae) {
         m_slow_ae = slow_ae;
         if (m_flag_camera_start) {
             /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_SET_SLOW_AE, slow_ae) < 0) {
                 LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_SET_SLOW_AE", __func__);
                 return -1;
             }*/
         }
     }

     return 0;
}

//======================================================================
int SecCamera::setRecordingSize(int width, int height)
{
     LOGW("%s(width(%d), height(%d))", __func__, width, height);

     m_recording_width  = width;
     m_recording_height = height;

     return 0;
}

int SecCamera::getRecordingSize(int* width, int* height)
{
     *width = m_recording_width;
     *height = m_recording_height;

     return 0;
}

//======================================================================

int SecCamera::setExifOrientationInfo(int orientationInfo)
{
     LOGW("%s(orientationInfo(%d))", __func__, orientationInfo);

     if (orientationInfo < 0) {
         LOGE("ERR(%s):Invalid orientationInfo (%d)", __func__, orientationInfo);
         return -1;
     }
     m_exif_orientation = orientationInfo;

     return 0;
}

/*Video call*/
int SecCamera::setVTmode(int vtmode)
{
    LOGW("%s(vtmode (%d))", __func__, vtmode);

    if (vtmode < VT_MODE_OFF || VT_MODE_MAX <= vtmode) {
        LOGE("ERR(%s):Invalid vtmode (%d)", __func__, vtmode);
        return -1;
    }

    if (m_vtmode != vtmode) {
        m_vtmode = vtmode;
    }

    return 0;
}

/* Camcorder fix fps */
int SecCamera::setSensorMode(int sensor_mode)
{
    LOGW("%s(sensor_mode (%d))", __func__, sensor_mode);

    if (sensor_mode < SENSOR_MODE_CAMERA || SENSOR_MODE_MOVIE < sensor_mode) {
        LOGE("ERR(%s):Invalid sensor mode (%d)", __func__, sensor_mode);
        return -1;
    }

    if (m_sensor_mode != sensor_mode) {
        m_sensor_mode = sensor_mode;
    }

    return 0;
}

/*  Shot mode   */
/*  SINGLE = 0
*   CONTINUOUS = 1
*   PANORAMA = 2
*   SMILE = 3
*   SELF = 6
*/
int SecCamera::setShotMode(int shot_mode)
{
    LOGW("%s(shot_mode (%d))", __func__, shot_mode);
    if (shot_mode < SHOT_MODE_SINGLE || SHOT_MODE_SELF < shot_mode) {
        LOGE("ERR(%s):Invalid shot_mode (%d)", __func__, shot_mode);
        return -1;
    }
    m_shot_mode = shot_mode;

    return 0;
}

int SecCamera::getVTmode(void)
{
    return m_vtmode;
}

int SecCamera::setBlur(int blur_level)
{
    LOGW("%s(level (%d))", __func__, blur_level);

    if (blur_level < BLUR_LEVEL_0 || BLUR_LEVEL_MAX <= blur_level) {
        LOGE("ERR(%s):Invalid level (%d)", __func__, blur_level);
        return -1;
    }

    if (m_blur_level != blur_level) {
        m_blur_level = blur_level;
        if (m_flag_camera_start) {
            /*if (fimc_v4l2_s_ctrl(m_cam_fd, V4L2_CID_CAMERA_VGA_BLUR, blur_level) < 0) {
                LOGE("ERR(%s):Fail on V4L2_CID_CAMERA_VGA_BLUR", __func__);
                return -1;
            }*/
        }
    }
    return 0;
}

int SecCamera::getBlur(void)
{
    return m_blur_level;
}

int SecCamera::setDataLineCheck(int chk_dataline)
{
    LOGW("%s(chk_dataline (%d))", __func__, chk_dataline);

    if (chk_dataline < CHK_DATALINE_OFF || CHK_DATALINE_MAX <= chk_dataline) {
        LOGE("ERR(%s):Invalid chk_dataline (%d)", __func__, chk_dataline);
        return -1;
    }

    m_chk_dataline = chk_dataline;

    return 0;
}

int SecCamera::getDataLineCheck(void)
{
    return m_chk_dataline;
}

char* SecCamera::getSupportPreviewSize()
{
    return support_preview_size;
}

char* SecCamera::getSupportPictureSize()
{
    return default_picture_size;
}

// ======================================================================
// Jpeg

int SecCamera::setJpegThumbnailSize(int width, int height)
{
    LOGW("%s(width(%d), height(%d))", __func__, width, height);

    m_jpeg_thumbnail_width  = width;
    m_jpeg_thumbnail_height = height;

    return 0;
}

int SecCamera::getJpegThumbnailSize(int *width, int  *height)
{
    if (width)
        *width   = m_jpeg_thumbnail_width;
    if (height)
        *height  = m_jpeg_thumbnail_height;

    return 0;
}

// ======================================================================
// Conversions

inline int SecCamera::m_frameSize(int format, int width, int height)
{
    int size = 0;

    switch (format) {
    case V4L2_PIX_FMT_YUV420:
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_NV21:
    case V4L2_PIX_FMT_YVU420:
        size = (width * height * 3 / 2);
        break;

    case V4L2_PIX_FMT_NV12T:
        size = ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height)) +
                            ALIGN_TO_8KB(ALIGN_TO_128B(width) * ALIGN_TO_32B(height / 2));
        break;

    case V4L2_PIX_FMT_YUV422P:
    case V4L2_PIX_FMT_YUYV:
    case V4L2_PIX_FMT_UYVY:
        size = (width * height * 2);
        break;

    default :
        LOGE("ERR(%s):Invalid V4L2 pixel format(%d)\n", __func__, format);
    case V4L2_PIX_FMT_RGB565:
        size = (width * height * BPP);
        break;
    }

    return size;
}
status_t SecCamera::getExposureState(int * enable, int *default_value , int *min_value , int *max_value , int *step_value )
{
    *enable = ExposureCtrl.flags & V4L2_CTRL_FLAG_DISABLED;
    *default_value = ExposureCtrl.default_value;
    *min_value     = ExposureCtrl.minimum;
    *max_value     = ExposureCtrl.maximum;
    *step_value    = ExposureCtrl.step;

    return NO_ERROR;
}

status_t SecCamera::dump(int fd)
{
    const size_t SIZE = 256;
    char buffer[SIZE];
    String8 result;
    snprintf(buffer, 255, "dump(%d)\n", fd);
    result.append(buffer);
    ::write(fd, result.string(), result.size());
    return NO_ERROR;
}

double SecCamera::jpeg_ratio = 0.7;
int SecCamera::interleaveDataSize = 5242880;
int SecCamera::jpegLineLength = 636;

}; // namespace android
