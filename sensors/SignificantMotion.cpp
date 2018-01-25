/*--------------------------------------------------------------------------
Copyright (c) 2015, The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#include <fcntl.h>
#include <errno.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/select.h>
#include <cutils/log.h>

#include "SignificantMotion.h"
#include "sensors.h"

/*****************************************************************************/
        SmdSensor::SmdSensor(struct SensorContext *context)
: SensorBase(NULL, NULL, context),
        mInputReader(4)
{
        int handle = -1;

        memset(mPendingEvent.data, 0, sizeof(mPendingEvent.data));
        mPendingEvent.version = sizeof(sensors_event_t);
        mPendingEvent.sensor = context->sensor->handle;
        mPendingEvent.type = SENSOR_TYPE_SIGNIFICANT_MOTION;
        mUseAbsTimeStamp = false;

        /* SMD sensor has only one valid event */
        mPendingEvent.data[0] = 1.0f;

        data_fd = context->data_fd;
        strlcpy(input_sysfs_path, context->enable_path, sizeof(input_sysfs_path));
        input_sysfs_path_len = strlen(input_sysfs_path);

        mEnabled = 0;
}

SmdSensor::~SmdSensor() {
        if (mEnabled) {
                enable(0, 0);
        }
}

int SmdSensor::enable(int32_t, int en) {
        int flags = en ? 1 : 0;
        int fd;
        char buf[2];
        int err;

        if (flags == mEnabled) {
                ALOGW("significant motion sensor already %s\n", flags ? "enabled" : "disabled");
                return 0;
        }

        strlcpy(&input_sysfs_path[input_sysfs_path_len],
                        SYSFS_ENABLE, SYSFS_MAXLEN);
        fd = open(input_sysfs_path, O_RDWR);
        if (fd < 0) {
                ALOGE("open %s failed.(%s)\n", input_sysfs_path, strerror(errno));
                return -1;
        }

        buf[1] = 0;
        buf[0] = flags ? '1':'0';

        if (write(fd, buf, sizeof(buf)) < (ssize_t)sizeof(buf)) {
                ALOGE("write %s to %s failed.(%s)\n", buf, input_sysfs_path, strerror(errno));
                close(fd);
                return -1;
        }
        close(fd);

        mEnabled = flags;

        return 0;
}

int SmdSensor::readEvents(sensors_event_t* data, int count)
{
        int hasEvent = 0;

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
                        float value = event->value;
                        if (event->code == ABS_MISC) {
                                hasEvent = 1;
                        }
                } else if (type == EV_SYN) {
                        switch (event->code) {
                                case SYN_TIME_SEC:
                                        mUseAbsTimeStamp = true;
                                        report_time = event->value*1000000000LL;
                                        break;
                                case SYN_TIME_NSEC:
                                        mUseAbsTimeStamp = true;
                                        mPendingEvent.timestamp = report_time+event->value;
                                        break;
                                case SYN_REPORT:
                                        if (!mUseAbsTimeStamp)
                                                mPendingEvent.timestamp = timevalToNano(event->time);

                                        if (mEnabled && hasEvent) {
                                                *data++ = mPendingEvent;
                                                numEventReceived++;
                                                count--;

                                                /* one-shot sensor disabled automatically */
                                                mEnabled = 0;
                                                hasEvent = 0;
                                        } else {
                                                ALOGE("invalid significant motion sensor event while disabled\n");
                                        }
                                        break;
                        }
                } else {
                        ALOGE("SmdSensor: unknown event (type=%d, code=%d)",
                                        type, event->code);
                }
                mInputReader.next();
        }

        ALOGD("%d SMD event received. timestamp:%lld\n", numEventReceived, mPendingEvent.timestamp);

        return numEventReceived;
}

