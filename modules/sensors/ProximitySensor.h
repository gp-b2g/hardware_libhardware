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

#ifndef ANDROID_PROXIMITY_SENSOR_H
#define ANDROID_PROXIMITY_SENSOR_H

#include "sensors.h"
#include "SensorBase.h"
#include "InputEventReader.h"

/*****************************************************************************/

struct input_event;

class ProximitySensor : public SensorBase {
public:
            ProximitySensor();
    virtual ~ProximitySensor();
    virtual int readEvents(sensors_event_t* data, int count);
  //  virtual bool hasPendingEvents() const;
    //virtual int enable(int32_t handle, int enabled);
    virtual bool hasPendingEvents() const;
    virtual int setDelay(int32_t handle, int64_t ns);
    virtual int setEnable(int32_t handle, int enabled);
    virtual int64_t getDelay(int32_t handle);
    virtual int getEnable(int32_t handle);
     int setInitialState();
private:

    int mEnabled;
    uint32_t mPendingMask;
    InputEventCircularReader mInputReader;
    sensors_event_t mPendingEvent;
    bool mHasPendingEvent;
    //char input_sysfs_path[256];
    //int input_sysfs_path_len;
   // int alsEnabled;
   // int psEnabled;

   // void processEvent(int code, int value);
 //  int handle2id(int32_t handle);
   

};

/*****************************************************************************/

#endif  // ANDROID_LIGHT_SENSOR_H
