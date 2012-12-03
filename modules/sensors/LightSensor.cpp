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

#include "LightSensor.h"
#define SENSOR_NAME     "ltr558"
/*****************************************************************************/

LightSensor::LightSensor()
    : SensorBase(NULL, SENSOR_NAME),
      mPendingMask(0),
      mInputReader(4),
      mHasPendingEvent(false)
{
    memset(mPendingEvents, 0, sizeof(mPendingEvents));
	for (int i=0; i<numSensors; i++) {
		mEnabled[i] = 0;
	}
    mPendingEvents[Light].version = sizeof(sensors_event_t);
    mPendingEvents[Light].sensor = ID_L;
    mPendingEvents[Light].type = SENSOR_TYPE_LIGHT;

    mPendingEvents[Proximity].version = sizeof(sensors_event_t);
    mPendingEvents[Proximity].sensor = ID_P;
    mPendingEvents[Proximity].type = SENSOR_TYPE_PROXIMITY;
    if (data_fd >= 0) {
        strcpy(input_sysfs_path, "/sys/class/input/");
        strcat(input_sysfs_path, input_name);
        strcat(input_sysfs_path, "/device/");
        input_sysfs_path_len = strlen(input_sysfs_path);
		LOGE("LightSensor: sysfs_path=%s", input_sysfs_path);
    } else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}

}

LightSensor::~LightSensor()
{
    close_device();
}

int LightSensor::setInitialState() {   
	return 0;
}
int LightSensor::setEnable(int32_t handle, int enabled)
{
	int id = handle2id(handle);
	int err = 0;
	char buffer[2];

	switch (id) {
	case Light:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_als");
		break;
	case Proximity:
		strcpy(&input_sysfs_path[input_sysfs_path_len], "enable_ps");
		break;
	default:
		LOGE("llightSensor: unknown handle (%d)", handle);
		return -EINVAL;
	}

	buffer[0] = '\0';
	buffer[1] = '\0';

	if (mEnabled[id] <= 0) {
		if(enabled) buffer[0] = '1';
	} else if (mEnabled[id] == 1) {
		if(!enabled) buffer[0] = '0';
	}

    if (buffer[0] != '\0') {
		err = write_sys_attribute(input_sysfs_path, buffer, 1);
		if (err != 0) {
			return err;
		}
		LOGD("lightSensor: set %s to %s",
			&input_sysfs_path[input_sysfs_path_len], buffer);
    }

	if (enabled) {
		(mEnabled[id])++;
		if (mEnabled[id] > 32767) mEnabled[id] = 32767;
	} else {
		(mEnabled[id])--;
		if (mEnabled[id] < 0) mEnabled[id] = 0;
	}
	LOGD("LightSensor: mEnabled[%d] = %d", id, mEnabled[id]);

    return err;
}

bool LightSensor::hasPendingEvents() const
{
    return mHasPendingEvent;
}

int LightSensor::readEvents(sensors_event_t* data, int count)
{
    LOGD("LightSensor::readEvents count = %d", count);

     if (count < 1)
        return -EINVAL;

    ssize_t n = mInputReader.fill(data_fd);
    if (n < 0)
        return n;

    int numEventReceived = 0;
    input_event const* event;

    while (count && mInputReader.readEvent(&event)) {
        int type = event->type;
        if (type == EV_ABS) {
            processEvent(event->code, event->value);
            mInputReader.next();
        } else if (type == EV_SYN) {
            int64_t time = timevalToNano(event->time);
            for (int j=0 ; count && mPendingMask && j<numSensors ; j++) {
                if (mPendingMask & (1<<j)) {
                    mPendingMask &= ~(1<<j);
                    mPendingEvents[j].timestamp = time;
					//LOGD("data=%8.5f,%8.5f,%8.5f",
						//mPendingEvents[j].data[0],
						//mPendingEvents[j].data[1],
						//mPendingEvents[j].data[2]);
                    if (mEnabled[j]) {
                        *data++ = mPendingEvents[j];
                        count--;
                        numEventReceived++;
                    }
                }
            }
            if (!mPendingMask) {
                mInputReader.next();
            }
        } else {
            LOGE("AkmSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
            mInputReader.next();
        }
    }
    return numEventReceived;
}

void LightSensor::processEvent(int code, int value)
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
	int id = handle2id(handle);
	if (id >= 0) {
		return mEnabled[id];
	} else {
		return 0;
	}
}
int LightSensor::handle2id(int32_t handle)
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

