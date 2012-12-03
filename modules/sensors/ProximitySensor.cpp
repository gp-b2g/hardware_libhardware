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
#include "ProximitySensor.h"
#define SENSOR_NAME     "proximity"
#define ps_enable_path              "/sys/devices/platform/stk-oss/ps_enable"
/*****************************************************************************/

ProximitySensor::ProximitySensor()
    : SensorBase(NULL, SENSOR_NAME),
      mPendingMask(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    mEnabled=0;
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_P;
    mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
    int fd = open(ps_enable_path,O_RDONLY);

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

ProximitySensor::~ProximitySensor()
{
    close_device();
}

int ProximitySensor::setInitialState() {   
    struct input_absinfo absinfo;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_PROXIMITY), &absinfo)) {
        mPendingEvent.distance = (float)(absinfo.value ? 10:0);
        mHasPendingEvent = true;
    }
	else
		LOGE("%s:ioctl failed!", __func__);			
    return 0;
}
int ProximitySensor::setEnable(int32_t handle, int enabled)
{
    int err = 0;
    int fd;
    char bEnable = (enabled?1:0);
    const char* strEnable[] = {"0","1"};
    if (bEnable != mEnabled) {
        fd = open(ps_enable_path,O_WRONLY);
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

bool ProximitySensor::hasPendingEvents() const
{
    return mHasPendingEvent;
}

int ProximitySensor::readEvents(sensors_event_t* data, int count)
{
    LOGD("LightSensor::readEvents count = %d", count);

     if (count < 1)
        return -EINVAL;

    if (mHasPendingEvent) {
        mHasPendingEvent = false;
        mPendingEvent.timestamp = getTimestamp();
        *data = mPendingEvent;
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
          //  processEvent(event->code, event->value);
          //  mInputReader.next();
           if (event->code == EVENT_TYPE_PROXIMITY) {          
		   	// In distance mode, SenseTek driver will report 0(near) or 1 (far)         
		   	// convert to float directly   
		   	mPendingEvent.distance = ((float)event->value)*10.0f;       
			LOGE("ProximitySensor readevents distance%f",mPendingEvent.distance);
		}
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);         
		if (mEnabled) {             
			*data++ = mPendingEvent;         
			count--;          
			numEventReceived++;        
		}
        } else {
            LOGE("STKProximitySensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
	   mInputReader.next();
    }
    return numEventReceived;
}

/*void ProximitySensor::processEvent(int code, int value)
{       
   switch (code) {		
   	case ABS_DISTANCE:               
		mPendingEvents[Proximity].version = sizeof(sensors_event_t);                
		mPendingEvents[Proximity].sensor = ID_P;                
		mPendingEvents[Proximity].type = SENSOR_TYPE_PROXIMITY;               
		memset(mPendingEvents[Proximity].data, 0, sizeof(mPendingEvents[Proximity].data));				             
		mPendingEvents[Proximity].distance = (value > 2) ? 0 : 5;				
		break;		
	case ABS_MISC:                
		mPendingEvents[Light].version = sizeof(sensors_event_t);                
		mPendingEvents[Light].sensor = ID_L;               
		mPendingEvents[Light].type = SENSOR_TYPE_LIGHT;                
		memset(mPendingEvents[Light].data, 0, sizeof(mPendingEvents[Light].data));                
		mPendingEvents[Light].light = value;			
		break;            
	default:	
		break;	  
	}
}
*/
int ProximitySensor::setDelay(int32_t handle, int64_t ns)
{
  	return 0;
}

int64_t ProximitySensor::getDelay(int32_t handle)
{
	return 0;
}

int ProximitySensor::getEnable(int32_t handle)
{
	return mEnabled;
}
/*int ProximitySensor::handle2id(int32_t handle)
{
    switch (handle) {
        case ID_L:
			return Light;
        case ID_P:
			return Proximity;
	default:
			LOGE("AkmSensor: unknown handle (%d)", handle);
			return -EINVAL;
    }
}
*/
