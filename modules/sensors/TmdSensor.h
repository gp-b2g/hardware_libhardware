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

#ifndef ANDROID_LIGHT_SENSOR_H
#define ANDROID_LIGHT_SENSOR_H

#include <stdint.h>
#include <errno.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/
#define PROXIMITY_THRESHOLD_GP2A  5.0f
struct input_event;

class TmdSensor : public SensorBase {
public:
			TmdSensor();
	virtual  ~TmdSensor();

	virtual int setEnable(int32_t handle, int enabled);
	virtual int readEvents(sensors_event_t* data, int count);
	virtual bool hasPendingEvents(void) const;
	void processEvent(int code, int value);
	int setInitialState(void);
	int getFd() const;
	virtual int setDelay(int32_t handle, int64_t ns);
	 virtual int64_t getDelay(int32_t handle);
	virtual int  getEnable(int32_t handle);		//rockie
	float indexToValue(size_t index) const;

	int mAlsEnabled;
	int mProxEnabled;
	bool mHasPendingEvent;
private:
	 enum {
	        Light       = 0,
	        Proximity   = 1,
	        numSensors
	    };
	 int mEnabled[numSensors];
	InputEventCircularReader mInputReader;
	sensors_event_t mPendingEvent;
};


/*****************************************************************************/

#endif  // ANDROID_LIGHT_SENSOR_H
