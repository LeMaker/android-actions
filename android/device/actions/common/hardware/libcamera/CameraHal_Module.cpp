/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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
* @file CameraHal.cpp
*
* This file maps the Camera Hardware Interface to V4L2.
*
*/


#include "CameraHalDebug.h"

#include <utils/threads.h>

#include "CameraHal.h"
#include "CameraProperties.h"
#include "ActCameraParameters.h"


static android::CameraProperties gCameraProperties;
static android::CameraHal* gCameraHals[MAX_CAMERAS_SUPPORTED];
static unsigned int gCamerasOpen = 0;
static android::Mutex gCameraHalDeviceLock;

static int camera_device_open(const hw_module_t* module, const char* name,
                              hw_device_t** device);
static int camera_device_close(hw_device_t* device);
static int camera_get_number_of_cameras(void);
static int camera_get_camera_info(int camera_id, struct camera_info *info);

static struct hw_module_methods_t camera_module_methods =
{
open:
    camera_device_open
};

camera_module_t HAL_MODULE_INFO_SYM =
{
common:
    {
tag:
        HARDWARE_MODULE_TAG,
        version_major: 1,
        version_minor: 0,
id:
        CAMERA_HARDWARE_MODULE_ID,
name: "Actions CameraHal Module"
        ,
author: "TI"
        ,
methods:
        &camera_module_methods,
dso:
        NULL, /* remove compilation warnings */
reserved:
        {0}, /* remove compilation warnings */
    },
get_number_of_cameras:
    camera_get_number_of_cameras,
get_camera_info:
    camera_get_camera_info,
};

typedef struct ti_camera_device
{
    camera_device_t base;
    /* TI specific "private" data can go here (base.priv) */
    int cameraid;
} ti_camera_device_t;


/*******************************************************************
 * implementation of camera_device_ops functions
 *******************************************************************/

int camera_set_preview_window(struct camera_device * device,
                              struct preview_stream_ops *window)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->setPreviewWindow(window);
    LOG_IF_FUNCTION_NAME_EXIT(rv);

    return rv;
}

void camera_set_callbacks(struct camera_device * device,
                          camera_notify_callback notify_cb,
                          camera_data_callback data_cb,
                          camera_data_timestamp_callback data_cb_timestamp,
                          camera_request_memory get_memory,
                          void *user)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->setCallbacks(notify_cb, data_cb, data_cb_timestamp, get_memory, user);//
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

void camera_enable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->enableMsgType(msg_type);
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

void camera_disable_msg_type(struct camera_device * device, int32_t msg_type)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->disableMsgType(msg_type);
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_msg_type_enabled(struct camera_device * device, int32_t msg_type)
{
    ti_camera_device_t* ti_dev = NULL;
    int rv = -EINVAL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return 0;

    ti_dev = (ti_camera_device_t*) device;
    LOG_IF_FUNCTION_NAME_EXIT(rv);

    rv = gCameraHals[ti_dev->cameraid]->msgTypeEnabled(msg_type);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_start_preview(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->startPreview();
    LOG_IF_FUNCTION_NAME_EXIT(rv);

    return rv;
}

void camera_stop_preview(struct camera_device * device)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->stopPreview();
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_preview_enabled(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->previewEnabled();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_store_meta_data_in_buffers(struct camera_device * device, int enable)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    //  TODO: meta data buffer not current supported
    rv = gCameraHals[ti_dev->cameraid]->storeMetaDataInBuffers(enable);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
    //return enable ? android::INVALID_OPERATION: android::OK;
}

int camera_start_recording(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->startRecording();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

void camera_stop_recording(struct camera_device * device)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->stopRecording();
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_recording_enabled(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->recordingEnabled();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

void camera_release_recording_frame(struct camera_device * device,
                                    const void *opaque)
{
    ti_camera_device_t* ti_dev = NULL;

    //LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->releaseRecordingFrame(opaque);
    //LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_auto_focus(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->autoFocus();
    CAMHAL_LOGDB("camera_auto_focus ret=%d", rv);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_cancel_auto_focus(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->cancelAutoFocus();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_take_picture(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->takePicture();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_cancel_picture(struct camera_device * device)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->cancelPicture();
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_set_parameters(struct camera_device * device, const char *params)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->setParameters(params);
    CAMHAL_LOGDB("camera_set_parameters ret=%d", rv);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

char* camera_get_parameters(struct camera_device * device)
{
    char* param = NULL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return NULL;

    ti_dev = (ti_camera_device_t*) device;

    param = gCameraHals[ti_dev->cameraid]->getParameters();
    LOG_IF_FUNCTION_NAME_EXIT(0);

    return param;
}

static void camera_put_parameters(struct camera_device *device, char *parms)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->putParameters(parms);
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_send_command(struct camera_device * device,
                        int32_t cmd, int32_t arg1, int32_t arg2)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->sendCommand(cmd, arg1, arg2);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

void camera_release(struct camera_device * device)
{
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    if(!device)
        return;

    ti_dev = (ti_camera_device_t*) device;

    gCameraHals[ti_dev->cameraid]->release();
    LOG_IF_FUNCTION_NAME_EXIT(0);
}

int camera_dump(struct camera_device * device, int fd)
{
    int rv = -EINVAL;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME
    if(!device)
        return rv;

    ti_dev = (ti_camera_device_t*) device;

    rv = gCameraHals[ti_dev->cameraid]->dump(fd);
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

extern "C" void heaptracker_show_leaked_memory(void);

int camera_device_close(hw_device_t* device)
{
    int ret = 0;
    ti_camera_device_t* ti_dev = NULL;

    LOG_IF_FUNCTION_NAME

    android::Mutex::Autolock lock(gCameraHalDeviceLock);

    if (!device)
    {
        ret = -EINVAL;
        goto done;
    }

    ti_dev = (ti_camera_device_t*) device;

    if (ti_dev)
    {
        if (gCameraHals[ti_dev->cameraid])
        {
            delete gCameraHals[ti_dev->cameraid];
            gCameraHals[ti_dev->cameraid] = NULL;
            gCamerasOpen--;
        }

        if (ti_dev->base.ops)
        {
            free(ti_dev->base.ops);
        }
        free(ti_dev);
    }
done:
#ifdef CAMERA_HEAPTRACKER
    heaptracker_show_leaked_memory();
#endif

    LOG_IF_FUNCTION_NAME_EXIT(ret);
    return ret;
}



/*******************************************************************
 * implementation of camera_module functions
 *******************************************************************/

/* open device handle to one of the cameras
 *
 * assume camera service will keep singleton of each camera
 * so this function will always only be called once per camera instance
 */

int camera_device_open(const hw_module_t* module, const char* name,
                       hw_device_t** device)
{
    int rv = 0;
    int num_cameras = 0;
    int cameraid;
    ti_camera_device_t* camera_device = NULL;
    camera_device_ops_t* camera_ops = NULL;
    android::CameraHal* camera = NULL;
    android::CameraProperties::Properties* properties = NULL;

    android::Mutex::Autolock lock(gCameraHalDeviceLock);

    LOG_IF_FUNCTION_NAME

    if (name != NULL)
    {
        cameraid = atoi(name);
        num_cameras = gCameraProperties.camerasSupported();

        if(cameraid > num_cameras)
        {
            CAMHAL_LOGEB("camera service provided cameraid out of bounds, "
                  "cameraid = %d, num supported = %d",
                  cameraid, num_cameras);
            rv = -EINVAL;
            goto fail;
        }

        if(gCamerasOpen >= MAX_SIMUL_CAMERAS_SUPPORTED)
        {
            CAMHAL_LOGEA("maximum number of cameras already open");
			/**
            *BUGFIX: cts double modules opening and previewing problem.
            *ActionsCode(author:liyuan, change_code)
            */
            rv = -EUSERS;
            goto fail;
        }

        camera_device = (ti_camera_device_t*)malloc(sizeof(*camera_device));
        if(!camera_device)
        {
            CAMHAL_LOGEA("camera_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        camera_ops = (camera_device_ops_t*)malloc(sizeof(*camera_ops));
        if(!camera_ops)
        {
            CAMHAL_LOGEA("camera_ops allocation fail");
            rv = -ENOMEM;
            goto fail;
        }

        memset(camera_device, 0, sizeof(*camera_device));
        memset(camera_ops, 0, sizeof(*camera_ops));

        camera_device->base.common.tag = HARDWARE_DEVICE_TAG;
        camera_device->base.common.version = 0;
        camera_device->base.common.module = (hw_module_t *)(module);
        camera_device->base.common.close = camera_device_close;
        camera_device->base.ops = camera_ops;

        camera_ops->set_preview_window = camera_set_preview_window;
        camera_ops->set_callbacks = camera_set_callbacks;//
        camera_ops->enable_msg_type = camera_enable_msg_type;
        camera_ops->disable_msg_type = camera_disable_msg_type;
        camera_ops->msg_type_enabled = camera_msg_type_enabled;
        camera_ops->start_preview = camera_start_preview;
        camera_ops->stop_preview = camera_stop_preview;
        camera_ops->preview_enabled = camera_preview_enabled;
        camera_ops->store_meta_data_in_buffers = camera_store_meta_data_in_buffers;
        camera_ops->start_recording = camera_start_recording;
        camera_ops->stop_recording = camera_stop_recording;
        camera_ops->recording_enabled = camera_recording_enabled;
        camera_ops->release_recording_frame = camera_release_recording_frame;
        camera_ops->auto_focus = camera_auto_focus;
        camera_ops->cancel_auto_focus = camera_cancel_auto_focus;
        camera_ops->take_picture = camera_take_picture;
        camera_ops->cancel_picture = camera_cancel_picture;
        camera_ops->set_parameters = camera_set_parameters;
        camera_ops->get_parameters = camera_get_parameters;
        camera_ops->put_parameters = camera_put_parameters;
        camera_ops->send_command = camera_send_command;
        camera_ops->release = camera_release;
        camera_ops->dump = camera_dump;

        *device = &camera_device->base.common;

        // -------- TI specific stuff --------

        camera_device->cameraid = cameraid;

        if(gCameraProperties.getProperties(cameraid, &properties) < 0)
        {
            CAMHAL_LOGEA("Couldn't get camera properties");
            rv = -ENOMEM;
            goto fail;
        }

        camera = new android::CameraHal(cameraid);

        if(!camera)
        {
            CAMHAL_LOGEA("Couldn't create instance of CameraHal class");
            rv = -ENOMEM;
            goto fail;
        }

        if(properties && (camera->initialize(properties) != android::NO_ERROR))
        {
            CAMHAL_LOGEA("Couldn't initialize camera instance");
            rv = -ENODEV;
            goto fail;
        }

        gCameraHals[cameraid] = camera;
        gCamerasOpen++;

    }
    LOG_IF_FUNCTION_NAME_EXIT(rv);

    return rv;

fail:
    if(camera_device)
    {
        free(camera_device);
        camera_device = NULL;
    }
    if(camera_ops)
    {
        free(camera_ops);
        camera_ops = NULL;
    }
    if(camera)
    {
        delete camera;
        camera = NULL;
    }
    *device = NULL;
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}

int camera_get_number_of_cameras(void)
{
    int num_cameras = MAX_CAMERAS_SUPPORTED;

    int ret = android::NO_ERROR;

    bool has_camera_opened = false;


#if 1
    android::Mutex::Autolock lock(gCameraHalDeviceLock);
    for(int i =0; i < MAX_CAMERAS_SUPPORTED; i++)
    {
        if(gCameraHals[i] != NULL)
        {
            has_camera_opened = true;
            break;
        }
    }

    if(has_camera_opened)
    {
        ret = gCameraProperties.initialize();
    }
    else
    {
        ret = gCameraProperties.initializeForced();
    }
    if(ret != android::NO_ERROR)
    {
        CAMHAL_LOGEA("Unable to create or initializeForced CameraProperties");
        return -EINVAL;
    }

    num_cameras = gCameraProperties.camerasSupported();
    CAMHAL_LOGVB("camera_get_number_of_cameras, num_cameras = %d", num_cameras);
#endif

    ALOGV("CameraHal Version:%s\n",CAMERAHAL_VERSION);

    return num_cameras;
}

int camera_get_camera_info(int camera_id, struct camera_info *info)
{
    int rv = 0;
    int face_value = CAMERA_FACING_BACK;
    int orientation = 0;
    const char *valstr = NULL;
    android::CameraProperties::Properties* properties = NULL;

    LOG_IF_FUNCTION_NAME
    // this going to be the first call from camera service
    // initialize camera properties here...
    if(gCameraProperties.initialize() != android::NO_ERROR)
    {
        CAMHAL_LOGEA("Unable to create or initialize CameraProperties");
        return -EINVAL;
    }

    //Get camera properties for camera index
    if(gCameraProperties.getProperties(camera_id, &properties) < 0)
    {
        CAMHAL_LOGEA("Couldn't get camera properties");
        rv = -EINVAL;
        goto end;
    }

    if(properties)
    {
        valstr = properties->get(android::CameraProperties::FACING_INDEX);
        if(valstr != NULL)
        {
            if (strcmp(valstr, (const char *) android::ActCameraParameters::FACING_FRONT) == 0)
            {
                face_value = CAMERA_FACING_FRONT;
            }
            else if (strcmp(valstr, (const char *) android::ActCameraParameters::FACING_BACK) == 0)
            {
                face_value = CAMERA_FACING_BACK;
            }
        }

        valstr = properties->get(android::CameraProperties::ORIENTATION_INDEX);
        if(valstr != NULL)
        {
            orientation = atoi(valstr);
        }
    }
    else
    {
        CAMHAL_LOGEB("getProperties() returned a NULL property set for Camera id %d", camera_id);
    }

    info->facing = face_value;
    info->orientation = orientation;

end:
    LOG_IF_FUNCTION_NAME_EXIT(rv);
    return rv;
}





