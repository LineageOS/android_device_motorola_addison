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

#ifndef ANDROID_AKMLOG_H
#define ANDROID_AKMLOG_H

#include <cutils/log.h>

/*** Constant definition ******************************************************/
#ifndef ALOGE
#ifdef LOGE
#define ALOGE	LOGE
#endif
#endif
#ifndef ALOGE_IF
#ifdef LOGE_IF
#define ALOGE_IF	LOGE_IF
#endif
#endif

#ifndef ALOGW
#ifdef LOGW
#define ALOGW	LOGW
#endif
#endif
#ifndef ALOGW_IF
#ifdef LOGW_IF
#define ALOGW_IF	LOGW_IF
#endif
#endif

#ifndef ALOGI
#ifdef LOGI
#define ALOGI	LOGI
#endif
#endif
#ifndef ALOGI_IF
#ifdef LOGI_IF
#define ALOGI_IF	LOGI_IF
#endif
#endif

#ifndef ALOGD
#ifdef LOGD
#define ALOGD	LOGD
#endif
#endif
#ifndef ALOGD_IF
#ifdef LOGD_IF
#define ALOGD_IF	LOGD_IF
#endif
#endif

#ifndef ALOGV
#ifdef LOGV
#define ALOGV	LOGV
#endif
#endif
#ifndef ALOGV_IF
#ifdef LOGV_IF
#define ALOGV_IF	LOGV_IF
#endif
#endif

#endif // ANDROID_AKMLOG_H

