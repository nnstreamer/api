/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file tizen_error.h
 * @date 24 October 2019
 * @brief C-API internal header emulating tizen_error.h for Non-Tizen platforms
 * @see	https://github.com/nnstreamer/nnstreamer
 * @author Wook Song <wook16.song@samsung.com>
 * @bug No known bugs except for NYI items
 */

#ifndef __NTZN_TIZEN_ERROR_H__
#define __NTZN_TIZEN_ERROR_H__

#include <errno.h>

/**
 @ref: https://gitlab.freedesktop.org/dude/gst-plugins-base/commit/89095e7f91cfbfe625ec2522da49053f1f98baf8
 */
#if !defined(ESTRPIPE)
#define ESTRPIPE EPIPE
#endif /* !defined(ESTRPIPE) */

#define TIZEN_ERROR_NONE (0)
#define TIZEN_ERROR_INVALID_PARAMETER (-EINVAL)
#define TIZEN_ERROR_STREAMS_PIPE (-ESTRPIPE)
#define TIZEN_ERROR_TRY_AGAIN (-EAGAIN)
#define TIZEN_ERROR_UNKNOWN (-1073741824LL)
#define TIZEN_ERROR_TIMED_OUT (TIZEN_ERROR_UNKNOWN + 1)
#define TIZEN_ERROR_NOT_SUPPORTED (TIZEN_ERROR_UNKNOWN + 2)
#define TIZEN_ERROR_PERMISSION_DENIED (-EACCES)
#define TIZEN_ERROR_OUT_OF_MEMORY (-ENOMEM)
#define TIZEN_ERROR_IO_ERROR (-EIO)

#endif /* __NTZN_TIZEN_ERROR_H__ */
