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

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "BmaSensor.h"

#define BMA_DATA_NAME				"BMA250 3-axis accelerometer"
#define BMA_MAX_SAMPLE_RATE_VAL	11 /* 200 Hz */
#define RANGE                        8
#define CONVERT                     (GRAVITY_EARTH / (1024.0f))   //4096
#define CONVERT_X                   (CONVERT)
#define CONVERT_Y                   (CONVERT)
#define CONVERT_Z                   (CONVERT)
#ifdef SENSORHAL_ACC_BAMLIS3DH
#define SENSOR_NAME		"lis3dh_acc"
#define ACC_ENABLE                "enable_device"
#define ACC_DEALY                  "pollrate_ms"
#else
#define SENSOR_NAME		"bma2x2,mc32x0"
#define ACC_ENABLE                "enable"
#define ACC_DEALY                  "delay"
#endif
#define CONVERTARG              GRAVITY_EARTH/(1000.0f)

//#define BMA_UNIT_CONVERSION(value) ((value) * GRAVITY_EARTH / (720.0f))

/*****************************************************************************/

BmaSensor::BmaSensor()
    : SensorBase(NULL, SENSOR_NAME),
      mEnabled(0),
      mDelay(-1),
      mInputReader(4),
      mHasPendingEvent(false)
{
    mPendingEvent.version = sizeof(sensors_event_t);
    mPendingEvent.sensor = ID_A;
    mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
    memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
    if (data_fd >= 0) {
	#ifdef SENSORHAL_ACC_BAMLIS3DH
        strcpy(input_sysfs_path, "/sys/bus/i2c/devices/1-0019/");
	#else
	strcpy(input_sysfs_path, "/sys/class/input/");
       strcat(input_sysfs_path, input_name);
       strcat(input_sysfs_path, "/device/");
      #endif
        input_sysfs_path_len = strlen(input_sysfs_path);
		LOGD("BmaSensor: sysfs_path=%s", input_sysfs_path);
    } else {
		input_sysfs_path[0] = '\0';
		input_sysfs_path_len = 0;
	}
		
}

BmaSensor::~BmaSensor() {
    if (mEnabled) {
        setEnable(0, 0);
    }
}

int BmaSensor::setInitialState() {
    struct input_absinfo absinfo;

	if (mEnabled) {
	#ifdef SENSORHAL_ACC_BAMLIS3DH
    	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
			mPendingEvent.acceleration.y = -(absinfo.value)*CONVERTARG;
		}
    	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
			mPendingEvent.acceleration.x =-(absinfo.value)*CONVERTARG;
		}
		if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
			mPendingEvent.acceleration.z = (absinfo.value)*CONVERTARG;
		}
	#else
	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_X), &absinfo)) {
			mPendingEvent.acceleration.x = (absinfo.value)*CONVERT_Y;
		}
    	if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Y), &absinfo)) {
			mPendingEvent.acceleration.x = (absinfo.value)*CONVERT_X;
		}
		if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_ACCEL_Z), &absinfo)) {
			mPendingEvent.acceleration.z = -(absinfo.value)*CONVERT_Z;
		}
	#endif
	}
    return 0;
}

bool BmaSensor::hasPendingEvents() const {
    return mHasPendingEvent;
}

int BmaSensor::setEnable(int32_t handle, int enabled) {
  
	int err = 0;
	char buffer[2];
	/* handle check */
	if (handle != ID_A) {
		LOGE("BmaSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}
	buffer[0] = '\0';
	buffer[1] = '\0';
	if (mEnabled <= 0) {
		if(enabled) buffer[0] = '1';
	} else if (mEnabled == 1) {
		if(!enabled) buffer[0] = '0';
	}
    if (buffer[0] != '\0') {
        strcpy(&input_sysfs_path[input_sysfs_path_len], ACC_ENABLE);
		err = write_sys_attribute(input_sysfs_path, buffer, 1);
		if (err != 0) {
			return err;
		}
		LOGD("BmaSensor: Control set %s", buffer);
   		setInitialState();
    }

	if (enabled) {
		mEnabled++;
		if (mEnabled > 32767) mEnabled = 32767;
	} else {
		mEnabled--;
		if (mEnabled < 0) mEnabled = 0;
	}
	LOGD("BmaSensor: mEnabled = %d", mEnabled);

    return err;


}

int BmaSensor::setDelay(int32_t handle, int64_t delay_ns)
{
	int err = 0;
	int rate_val;
	int32_t us; 
	char buffer[16];
	int bytes;
	/* handle check */
	if (handle != ID_A) {
		LOGE("BmaSensor: Invalid handle (%d)", handle);
		return -EINVAL;
	}

	if (mDelay != delay_ns) {
		
	 int ms=delay_ns/1000000;
    	strcpy(&input_sysfs_path[input_sysfs_path_len], ACC_DEALY);
   		bytes = sprintf(buffer, "%d", ms);
		err = write_sys_attribute(input_sysfs_path, buffer, bytes);
		if (err == 0) {
			mDelay = delay_ns;
		}
	}

	return err;
}

int64_t BmaSensor::getDelay(int32_t handle)
{
	return (handle == ID_A) ? mDelay : 0;
}

int BmaSensor::getEnable(int32_t handle)
{
	return (handle == ID_A) ? mEnabled : 0;
}

int BmaSensor::readEvents(sensors_event_t* data, int count)
{
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
            float value = event->value;
	    #ifdef SENSORHAL_ACC_BAMLIS3DH
            if (event->code == EVENT_TYPE_ACCEL_X) {
                mPendingEvent.acceleration.y= -(value)*CONVERTARG;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                mPendingEvent.acceleration.x=-(value)*CONVERTARG;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                mPendingEvent.acceleration.z = (value)*CONVERTARG;
            }
	    #else
	    if (event->code == EVENT_TYPE_ACCEL_X) {
                 mPendingEvent.acceleration.y = (value)*CONVERT_Y;
            } else if (event->code == EVENT_TYPE_ACCEL_Y) {
                 mPendingEvent.acceleration.x =(value)*CONVERT_X;
            } else if (event->code == EVENT_TYPE_ACCEL_Z) {
                  mPendingEvent.acceleration.z = -(value)*CONVERT_Z;
            }
	   #endif
	//LOGE("bmasensor readEvents x: %d y: %d z:%d\n ",mPendingEvent.acceleration.x,mPendingEvent.acceleration.y,
		//	mPendingEvent.acceleration.z);
        } else if (type == EV_SYN) {
            mPendingEvent.timestamp = timevalToNano(event->time);
	     mPendingEvent.sensor = ID_A;
	     mPendingEvent.type = SENSOR_TYPE_ACCELEROMETER;
	     mPendingEvent.acceleration.status = SENSOR_STATUS_ACCURACY_HIGH;

            if (mEnabled) {
                *data++ = mPendingEvent;
                count--;
                numEventReceived++;
            }
        } else {
            LOGE("BmaSensor: unknown event (type=%d, code=%d)",
                    type, event->code);
        }
        mInputReader.next();
    }

    return numEventReceived;
}
int BmaSensor::setAccel()
{
	int err;
	int16_t acc[3];
	acc[0] = (int16_t)(mPendingEvent.acceleration.x / GRAVITY_EARTH * AKSC_LSG);
	acc[1] = (int16_t)(mPendingEvent.acceleration.y / GRAVITY_EARTH * AKSC_LSG);
	acc[2] = (int16_t)(mPendingEvent.acceleration.z / GRAVITY_EARTH * AKSC_LSG);
	//strcpy(&input_sysfs_path[input_sysfs_path_len], "accel");
	err = write_sys_attribute("/sys/class/compass/akm8963/accel", (char*)acc, 6);
	/*if (err < 0) {
		LOGD("AkmSensor: %s write failed.",
			&input_sysfs_path[input_sysfs_path_len]);
	}*/
	return err;
}