/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package android.support.test;

import android.content.Context;

/**
 * Host stub of android.support.test.InstrumentationRegistry.
 */
public final class InstrumentationRegistry {
    private static Context sContext;

    public static synchronized Context getTargetContext() {
        if (sContext == null) {
            sContext = new Context();
        }

        return sContext;
    }
}
