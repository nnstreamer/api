/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package android.content;

import android.content.res.AssetManager;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;

/**
 * Host stub of android.content.Context for running the instrumented tests on a desktop JVM.
 * The files directory is a temporary directory, and the assets are empty.
 */
public class Context {
    private final File mFilesDir;
    private final AssetManager mAssets = new AssetManager();

    public Context() {
        try {
            mFilesDir = Files.createTempDirectory("nns-host-test").toFile();
            mFilesDir.deleteOnExit();
        } catch (IOException e) {
            throw new IllegalStateException("Failed to create the files directory", e);
        }
    }

    public AssetManager getAssets() {
        return mAssets;
    }

    public File getFilesDir() {
        return mFilesDir;
    }

    public String getPackageName() {
        return "org.nnsuite.nnstreamer.test";
    }
}
