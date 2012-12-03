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
#include "TmdSensor.h"
#include "Tmd27713.h"
#include <cutils/log.h>

/*****************************************************************************/

TmdSensor::TmdSensor(): SensorBase("/dev/tmd27713", "tmd27713"),
	mAlsEnabled(0),
	mProxEnabled(0),
	mHasPendingEvent(false),
	mInputReader(32)

{
	struct taos_cfg *taos_cfgp;
	open_device();
	if (data_fd >= 0) {
		ioctl(dev_fd, TAOS_IOCTL_SENSOR_ON, 0);
		ioctl(dev_fd, TAOS_IOCTL_PROX_CALIBRATE, 0);
		ioctl(dev_fd, TAOS_IOCTL_ALS_CALIBRATE, 0);
	}
}

TmdSensor::~TmdSensor(){
	ioctl(dev_fd, TAOS_IOCTL_SENSOR_OFF, 0);
}

int TmdSensor::setInitialState() {
	struct input_absinfo absinfo;
    if (!ioctl(data_fd, EVIOCGABS(EVENT_TYPE_PROXIMITY), &absinfo)) {
        // make sure to report an event immediately
        mHasPendingEvent = true;
       mPendingEvent.distance = indexToValue(absinfo.value);
   }
	return 0;
}

int TmdSensor::setEnable(int32_t handle, int en) {
	int flags = en ? 1 : 0;
	int err = 0;

	if ((handle == ID_L) && (flags != mAlsEnabled)) {
		mAlsEnabled = flags;
		LOGE("\nmAlsEnabled flags = %d\n", flags);
		if (flags == 1) {
			ioctl(dev_fd,TAOS_IOCTL_ALS_ON,0);
		} else {
			ioctl(dev_fd,TAOS_IOCTL_ALS_OFF,0);
		}
	}
	if ((handle == ID_P) && (flags != mProxEnabled)) {
		mProxEnabled = flags;
		LOGE("\nmPsEnabled flags = %d\n", flags);
		if (flags == 1) {
			ioctl(dev_fd,TAOS_IOCTL_PROX_ON,0);
		} else {
			ioctl(dev_fd,TAOS_IOCTL_PROX_OFF,0);
		}
	}
	return err;
}

bool TmdSensor::hasPendingEvents() const {
	return mHasPendingEvent;
}

int TmdSensor::readEvents(sensors_event_t* data, int count)
{
	if (count < 1)
		return -EINVAL;

	ssize_t n = mInputReader.fill(data_fd);
	if (n < 0)
		return n;

	int numEventReceived = 0;
	input_event const* event;

	while (count && mInputReader.readEvent(&event))
		{
		int type = event->type;
		if (type == EV_ABS) {
			processEvent(event->code, event->value);
		} else if (type == EV_SYN) {
			int64_t time = timevalToNano(event->time);
			mPendingEvent.timestamp = time;
			*data++ = mPendingEvent;
			count--;
			numEventReceived++;
		} else {
			LOGE("Tsl27713 Sensor: unknown event (type=%d, code=%d)",
				type, event->code);
			}
			mInputReader.next();
		}
	return numEventReceived;
}

void TmdSensor::processEvent(int code, int value)
{
	switch (code) {
		case ABS_DISTANCE:
			mPendingEvent.version = sizeof(sensors_event_t);
			mPendingEvent.sensor = ID_P;
			mPendingEvent.type = SENSOR_TYPE_PROXIMITY;
			memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
			mPendingEvent.distance = (value > 0)? 5:0;
			break;
		case ABS_MISC:
			mPendingEvent.version = sizeof(sensors_event_t);
			mPendingEvent.sensor = ID_L;
			mPendingEvent.type = SENSOR_TYPE_LIGHT;
			memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
			mPendingEvent.light = value;
			break;
		default:
			break;
		}

}
int TmdSensor::getFd() const
{
	return data_fd;
}

int TmdSensor::getEnable(int32_t handle) {
	bool en = false;
	if(handle == ID_L)
		en = mAlsEnabled ? true : false;
	if(handle == ID_P)
		en = mProxEnabled ? true : false;
	return en;
}
int TmdSensor::setDelay(int32_t handle, int64_t ns) {
	return 0;
};

float TmdSensor::indexToValue(size_t index) const
{
    return index * PROXIMITY_THRESHOLD_GP2A;
}

int64_t TmdSensor::getDelay(int32_t handle)
{
	return 0;
}

