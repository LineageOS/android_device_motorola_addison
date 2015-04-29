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

#define LOG_TAG "Sensors"

#include <hardware/sensors.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <linux/input.h>
#include <utils/Atomic.h>
#include <utils/Log.h>

#include "sensors.h"
#include "AccelSensor.h"
#include "ct406_hal.h"

/*****************************************************************************/

/* The SENSORS Module */
static const struct sensor_t sSensorList[] = {
	/* Accelerometer */
	{
		"accelerometer",
		"Kionix KXTJ2",
		1,	/* hw/sw version */
		SENSORS_ACCELERATION_HANDLE,
		SENSOR_TYPE_ACCELEROMETER,
		(2.0f * 9.81f),
		(9.81f / 1024),
		0.2f,		/* mA */
		5000,	/* microseconds */
		0,0,	/* fifo reserved events, fifo max events */
		"", "", 200000, SENSOR_FLAG_CONTINUOUS_MODE, { }
	},

	/* light sensor name */
	{
		"CT406 Light sensor",
		"TAOS",
		1,
		SENSORS_LIGHT_HANDLE,
		SENSOR_TYPE_LIGHT,
		27000.0f,
		1.0f,
		0.175f,
		20000,
		0, 0,	/* fifo reserved events, fifo max events */
		"", "", 0, SENSOR_FLAG_ON_CHANGE_MODE, { }
	},

	/* proximity sensor */
	{
		"CT406 Proximity sensor",
		"TAOS",
		1,
		SENSORS_PROXIMITY_HANDLE,
		SENSOR_TYPE_PROXIMITY,
		100.0f,
		5.0f,
		3.0f,
		0,
		0, 0,	/* fifo reserved events, fifo max events */
		"", "", 0, SENSOR_FLAG_ON_CHANGE_MODE | SENSOR_FLAG_WAKE_UP, { }
	},
};


static int open_sensors(const struct hw_module_t* module, const char* id,
						struct hw_device_t** device);


static int sensors__get_sensors_list(struct sensors_module_t* module,
									 struct sensor_t const** list) 
{
		*list = sSensorList;
		return ARRAY_SIZE(sSensorList);
}

static struct hw_module_methods_t sensors_module_methods = {
		open: open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
		common: {
				tag: HARDWARE_MODULE_TAG,
				version_major: 1,
				version_minor: 0,
				id: SENSORS_HARDWARE_MODULE_ID,
				name: "Quic Sensor module",
				author: "Quic",
				methods: &sensors_module_methods,
		},
		get_sensors_list: sensors__get_sensors_list,
};

struct sensors_poll_context_t {
	sensors_poll_device_1_t device; // must be first

		sensors_poll_context_t();
		~sensors_poll_context_t();
	int activate(int handle, int enabled);
	int setDelay(int handle, int64_t ns);
	int pollEvents(sensors_event_t* data, int count);
	int batch(int handle, int flags, int64_t ns, int64_t timeout);
	int flush(int handle);

private:
	enum {
		light_prox			= 0,
		accel			= 1,
		numSensorDrivers,
		numFds,
	};

	static const size_t wake = numFds - 1;
	static const char WAKE_MESSAGE = 'W';
	struct pollfd mPollFds[numFds];
	int mWritePipeFd;
	SensorBase* mSensors[numSensorDrivers];

	int handleToDriver(int handle) const {
		switch (handle) {
			case SENSORS_ACCELERATION_HANDLE:
				return accel;
			case SENSORS_PROXIMITY_HANDLE:
			case SENSORS_LIGHT_HANDLE:
				return light_prox;
		}
		return -EINVAL;
	}
};

/*****************************************************************************/

sensors_poll_context_t::sensors_poll_context_t()
{
	mSensors[light_prox] = new LightProxSensor();
	mPollFds[light_prox].fd = mSensors[light_prox]->getFd();
	mPollFds[light_prox].events = POLLIN;
	mPollFds[light_prox].revents = 0;

	mSensors[accel] = new AccelSensor();
	mPollFds[accel].fd = mSensors[accel]->getFd();
	mPollFds[accel].events = POLLIN;
	mPollFds[accel].revents = 0;

	int wakeFds[2];
	int result = pipe(wakeFds);
	ALOGE_IF(result<0, "error creating wake pipe (%s)", strerror(errno));
	fcntl(wakeFds[0], F_SETFL, O_NONBLOCK);
	fcntl(wakeFds[1], F_SETFL, O_NONBLOCK);
	mWritePipeFd = wakeFds[1];

	mPollFds[wake].fd = wakeFds[0];
	mPollFds[wake].events = POLLIN;
	mPollFds[wake].revents = 0;
}

sensors_poll_context_t::~sensors_poll_context_t() {
	for (int i=0 ; i<numSensorDrivers ; i++) {
		delete mSensors[i];
	}
	close(mPollFds[wake].fd);
	close(mWritePipeFd);
}

int sensors_poll_context_t::activate(int handle, int enabled) {
	int index = handleToDriver(handle);
	if (index < 0) return index;
	int err =  mSensors[index]->enable(handle, enabled);
	if (enabled && !err) {
		const char wakeMessage(WAKE_MESSAGE);
		int result = write(mWritePipeFd, &wakeMessage, 1);
		ALOGE_IF(result<0, "error sending wake message (%s)", strerror(errno));
	}
	return err;
}

int sensors_poll_context_t::setDelay(int handle, int64_t ns) {

	int index = handleToDriver(handle);
	if (index < 0) return index;
	return mSensors[index]->setDelay(handle, ns);
}

int sensors_poll_context_t::pollEvents(sensors_event_t* data, int count)
{
	int nbEvents = 0;
	int n = 0;

	do {
		// see if we have some leftover from the last poll()
		for (int i=0 ; count && i<numSensorDrivers ; i++) {
			SensorBase* const sensor(mSensors[i]);
			if ((mPollFds[i].revents & POLLIN) || (sensor->hasPendingEvents())) {
				int nb = sensor->readEvents(data, count);
				if (nb < count) {
					// no more data for this sensor
					mPollFds[i].revents = 0;
				}
				count -= nb;
				nbEvents += nb;
				data += nb;
			}
		}

		if (count) {
			// we still have some room, so try to see if we can get
			// some events immediately or just wait if we don't have
			// anything to return
			do {
				n = poll(mPollFds, numFds, nbEvents ? 0 : -1);
			} while (n < 0 && errno == EINTR);
			if (n<0) {
				ALOGE("poll() failed (%s)", strerror(errno));
				return -errno;
			}
			if (mPollFds[wake].revents & POLLIN) {
				char msg;
				int result = read(mPollFds[wake].fd, &msg, 1);
				ALOGE_IF(result<0, "error reading from wake pipe (%s)", strerror(errno));
				ALOGE_IF(msg != WAKE_MESSAGE, "unknown message on wake queue (0x%02x)", int(msg));
				mPollFds[wake].revents = 0;
			}
		}
		// if we have events and space, go read them
	} while (n && count);

	return nbEvents;
}

int sensors_poll_context_t::batch(int handle, int flags, int64_t ns, int64_t timeout)
{
	return setDelay(handle, ns);
}
int sensors_poll_context_t::flush(int handle)
{
	int drv = handleToDriver(handle);
	return mSensors[drv]->flush(handle);
}

/*****************************************************************************/

static int poll__close(struct hw_device_t *dev)
{
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	if (ctx) {
		delete ctx;
	}
	return 0;
}

static int poll__activate(struct sensors_poll_device_t *dev,
		int handle, int enabled) {
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	return ctx->activate(handle, enabled);
}

static int poll__setDelay(struct sensors_poll_device_t *dev,
		int handle, int64_t ns) {
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	return ctx->setDelay(handle, ns);
}

/*!
 * \brief Implement Android HAL poll()
 *
 * From [source.android.com](https://source.android.com/devices/sensors/hal-interface.html)
 *
 * Returns an array of sensor data by filling the data argument. This function
 * must block until events are available. It will return the number of events
 * read on success, or a negative error number in case of an error.
 *
 * The number of events returned in data must be less or equal to the count
 * argument. This function shall never return 0 (no event).
 *
 * \param[in]  dev   the device to poll
 * \param[out] data  the returned data items
 * \param[in]  count the maximum number of returned data items
 *
 * \returns negative on failure, the number of returned data items on success,
 *          and never 0.
 */
static int poll__poll(struct sensors_poll_device_t *dev,
		sensors_event_t* data, int count) {
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	int ret = 0;
	do {
	    ret = ctx->pollEvents(data, count);
	} while( ret == 0 );
	return ret;
}

static int poll__batch(sensors_poll_device_1_t *dev,
		int handle, int flags, int64_t ns, int64_t timeout) {
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	return ctx->batch(handle, flags, ns, timeout);
}

static int poll__flush(sensors_poll_device_1_t *dev,
		int handle) {
	sensors_poll_context_t *ctx = (sensors_poll_context_t *)dev;
	return ctx->flush(handle);
}

/*****************************************************************************/

/** Open a new instance of a sensor device using name */
static int open_sensors(const struct hw_module_t* module, const char* id,
						struct hw_device_t** device)
{
		int status = -EINVAL;
		sensors_poll_context_t *dev = new sensors_poll_context_t();

		memset(&dev->device, 0, sizeof(sensors_poll_device_1_t));

		dev->device.common.tag = HARDWARE_DEVICE_TAG;
		dev->device.common.version  = SENSORS_DEVICE_API_VERSION_1_0;
		dev->device.common.module   = const_cast<hw_module_t*>(module);
		dev->device.common.close	= poll__close;
		dev->device.activate		= poll__activate;
		dev->device.setDelay		= poll__setDelay;
		dev->device.poll		= poll__poll;
		dev->device.batch		= poll__batch;
		dev->device.flush		= poll__flush;

		*device = &dev->device.common;
		status = 0;

		return status;
}

