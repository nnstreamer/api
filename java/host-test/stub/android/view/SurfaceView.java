/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package android.view;

import android.content.Context;

/**
 * Host stub of android.view.SurfaceView.
 */
public class SurfaceView {
    private final Surface mSurface = new Surface();

    public SurfaceView(Context context) {
    }

    public SurfaceHolder getHolder() {
        return new SurfaceHolder() {
            @Override
            public Surface getSurface() {
                return mSurface;
            }
        };
    }
}
