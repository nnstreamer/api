/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package android.content.res;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;

/**
 * Host stub of android.content.res.AssetManager. The host test has no assets.
 */
public final class AssetManager {
    public String[] list(String path) throws IOException {
        return null;
    }

    public InputStream open(String fileName) throws IOException {
        throw new FileNotFoundException(fileName);
    }
}
