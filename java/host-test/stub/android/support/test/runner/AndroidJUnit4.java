/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API host test
 * Copyright (C) 2026 Samsung Electronics Co., Ltd.
 */

package android.support.test.runner;

import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.InitializationError;

/**
 * Host stub of android.support.test.runner.AndroidJUnit4, the plain JUnit 4 runner.
 */
public final class AndroidJUnit4 extends BlockJUnit4ClassRunner {
    public AndroidJUnit4(Class<?> klass) throws InitializationError {
        super(klass);
    }
}
