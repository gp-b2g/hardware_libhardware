/*
 * Copyright (C) 2008 The Android Open Source Project
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
//#define LOG_NDEBUG 0
//#define LOG_NDDEBUG 0
//#define LOG_NDIDEBUG 0

#include <linux/lightsensor.h>
#include <utils/Log.h>
#include <fcntl.h>
#include "LightSensor31XX.h"
#define SENSOR_NAME     "lightsensor-level"
#define als_enable_bin_path             "/sys/devices/platform/stk-oss/als_enable"
/*****************************************************************************/

LightSensor::LightSensor()
    : SensorBase(NULL, SENSOR_NAME),
      mPendingMask(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    memset(mPendingEvents.data, 0, sizeof(mPendingEvents.data));
    mEnabled=0;
    mPendingEvents.version = sizeof(sensors_event_t);
    mPendingEvents.sensor = ID_L;
    mPendingEvents.type = SENSOR_TYPE_LIGHT;
    int fd = open(als_enable_bin_path,O_RDONLY);

    if (fd>=0)
    {
        char flags = '0';
        if (read(fd, &flags, 1)==1)
        {
            if (flags=='1')
            {
                mEnabled = 1;
                setInitialState();
            }
        }
		else
			LOGE("%s:read fd failed!", __func__);				
        close(fd);
    }
	else
		LOGE("%s:open fd failed!", __func__);	

}

LightSensor::~LightSensor()
{
    close_device();
}

int LightSensor::setInitialState() {   
    struct input_absinfo absinfo;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_LIGHT), &absinfo)) {
        mPendingEvents.light = (float)absinfo.value;
        mHasPendingEvent = true;
    }
	else
		LOGE("%s:ioctl failed!", __func__);		
    return 0;
}
int LightSensor::setEnable(int32_t handle, int enabled)
{
    int err = 0;
    int fd;
    char bEnable = (enabled?1:0);
    const char* strEnable[] = {"0","1"};
    if (bEnable != mEnabled) {
        fd = open(als_enable_bin_path,O_WRONLY);
        if (fd>=0)
        {
            if(write(fd,enabled?strEnable[1]:strEnable[0],2) == -1)
			{
				LOGE("%s:write failed!", __func__);		
				return -3;
			}			
            mEnabled = bEnable;
            err = 0;
            if (mEnabled)
                setInitialState();
            close(fd);
        }
        else
		{
			LOGE("%s:open fd failed!", __func__);				
            err = -1;
		}

    }

    return err;
}

bool LightSensor::hasPendingEvents() const
{
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
     if (count < 1)
        return -EINVAL;
    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvents.timestamp = getTimestamp();
        *data = mPendingEvents;
        return mEnabled ? 1 : 0;
    }

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            if (event->code == EVENT_TYPE_LIGHT) {         
			mPendingEvents.light = (float)event->value;      
			//LOGE("LightSensor readEvents value=%f",mPendingEvents.light);
		}
        } else if (type == EV_SYN) {
            mPendingEvents.timestamp = timevalToNano(event->time);       
		if (mEnabled) {            
			*data++ = mPendingEvents;
			count--; 
			numEventReceived++; 
		}
        } else {
            LOGE("AkmSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
	 mInputReader.next();
    }
    return numEventReceived;
}

int LightSensor::setDelay(int32_t handle, int64_t ns)
{
  	return 0;
}

int64_t LightSensor::getDelay(int32_t handle)
{
	return 0;
}

int LightSensor::getEnable(int32_t handle)
{
	return mEnabled;

}

