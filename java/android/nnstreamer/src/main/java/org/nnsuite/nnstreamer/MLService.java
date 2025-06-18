/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API
 * Copyright (C) 2025 Samsung Electronics Co., Ltd.
 */

package org.nnsuite.nnstreamer;

/**
 * Provides interfaces to construct and process a machine-learning service.<br>
 * <br>
 * {@link MLService} allows the following operations with NNStreamer:<br>
 * - Create a stream pipeline with NNStreamer and GStreamer plugins.<br>
 * - Open a machine-learning model and invoke an input data.<br>
 */
public final class MLService implements AutoCloseable {
    private long mHandle;
    private EventListener mEventListener;

    private native long nativeOpen(String config);
    private native void nativeClose(long handle);
    private native boolean nativeStart(long handle);
    private native boolean nativeStop(long handle);
    private native boolean nativeInputData(long handle, String name, TensorsData data);
    private native TensorsInfo nativeGetInputInfo(long handle, String name);
    private native TensorsInfo nativeGetOutputInfo(long handle, String name);
    private native boolean nativeSetInfo(long handle, String name, String value);
    private native String nativeGetInfo(long handle, String name);

    /**
     * Interface definition for the callbacks to be invoked when an event is received from machine-learning service.
     *
     * @see MLService#MLService(String, EventListener)
     */
    public interface EventListener {
        /**
         * Called when an output node receives new data.
         * If an application wants to accept data outputs, use this callback to get data from constructed machine-learning service.
         * Note that this is synchronously called and the buffer may be deallocated after the callback is finished.
         * Thus, if you need the data afterwards, copy the data to another buffer and return fast.
         * Do not spend too much time in the callback. It is recommended to use very small tensors.
         *
         * @param name The name of output node defined in the configuration.
         *             This may be null if the machine-learning service is constructed from model configuration.
         * @param data The output data (a single frame, tensor/tensors).
         */
        void onNewDataReceived(String name, TensorsData data);
    }

    /**
     * Constructs a new {@link MLService} instance with given configuration.
     *
     * @param config The absolute path of configuration file.
     * @param listener The function to be called when an event is received from machine-learning service.
     *
     * @throws IllegalArgumentException if given param is invalid.
     * @throws IllegalStateException if failed to construct the machine-learning service.
     */
    public MLService(String config, EventListener listener) {
        if (!isValidString(config)) {
            throw new IllegalArgumentException("Given configuration is invalid.");
        }

        if (listener == null) {
            throw new IllegalArgumentException("Given listener is invalid.");
        }

        mEventListener = listener;
        mHandle = nativeOpen(config);

        if (mHandle == 0) {
            throw new IllegalStateException("Failed to construct the machine-learning service.");
        }
    }

    /**
     * Starts the machine-learning service.
     *
     * @throws IllegalStateException if failed to start the machine-learning service.
     */
    public void start() {
        checkNativeHandle();

        if (!nativeStart(mHandle)) {
            throw new IllegalStateException("Failed to start the machine-learning service.");
        }
    }

    /**
     * Stops the machine-learning service.
     *
     * @throws IllegalStateException if failed to stop the machine-learning service.
     */
    public void stop() {
        checkNativeHandle();

        if (!nativeStop(mHandle)) {
            throw new IllegalStateException("Failed to stop the machine-learning service.");
        }
    }

    /**
     * Adds an input data to input node to be processed from the machine-learning service.
     *
     * @param name The name of input node defined in the configuration.
     *             Note that this is mandatory if the machine-learning service is constructed from pipeline configuration.
     * @param data The input data (a single frame, tensor/tensors).
     *
     * @throws IllegalArgumentException if given param is invalid.
     * @throws IllegalStateException if failed to push data to input node.
     */
    public void inputData(String name, TensorsData data) {
        checkNativeHandle();

        if (data == null) {
            throw new IllegalArgumentException("Given data is invalid.");
        }

        if (!nativeInputData(mHandle, name, data)) {
            throw new IllegalStateException("Failed to push data to input node.");
        }
    }

    /**
     * Gets the information (tensor dimension, type, name and so on) of input node.
     *
     * @param name The name of input node defined in the configuration.
     *             Note that this is mandatory if the machine-learning service is constructed from pipeline configuration.
     *
     * @return The tensors information.
     *
     * @throws IllegalStateException if failed to get the input information.
     */
    public TensorsInfo getInputInformation(String name) {
        checkNativeHandle();

        TensorsInfo info = nativeGetInputInfo(mHandle, name);

        if (info == null) {
            throw new IllegalStateException("Failed to get the input information.");
        }

        return info;
    }

    /**
     * Gets the information (tensor dimension, type, name and so on) of output node.
     *
     * @param name The name of output node defined in the configuration.
     *             Note that this is mandatory if the machine-learning service is constructed from pipeline configuration.
     *
     * @return The tensors information.
     *
     * @throws IllegalStateException if failed to get the output information.
     */
    public TensorsInfo getOutputInformation(String name) {
        checkNativeHandle();

        TensorsInfo info = nativeGetOutputInfo(mHandle, name);

        if (info == null) {
            throw new IllegalStateException("Failed to get the output information.");
        }

        return info;
    }

    /**
     * Sets the information for the machine-learning service.
     *
     * @param name The name of information.
     * @param value The value of information.
     *
     * @throws IllegalArgumentException if given param is invalid.
     * @throws IllegalStateException if failed to set the information.
     */
    public void setInformation(String name, String value) {
        checkNativeHandle();

        if (!isValidString(name)) {
            throw new IllegalArgumentException("Given information name is invalid.");
        }

        if (!isValidString(value)) {
            throw new IllegalArgumentException("Given information value is invalid.");
        }

        if (!nativeSetInfo(mHandle, name, value)) {
            throw new IllegalStateException("Failed to set the information.");
        }
    }

    /**
     * Gets the information from the machine-learning service.
     *
     * @param name The name of information.
     *
     * @return The value of information.
     *
     * @throws IllegalArgumentException if given param is invalid.
     * @throws IllegalStateException if failed to get the information.
     */
    public String getInformation(String name) {
        checkNativeHandle();

        if (!isValidString(name)) {
            throw new IllegalArgumentException("Given information name is invalid.");
        }

        String value = nativeGetInfo(mHandle, name);

        if (!isValidString(value)) {
            throw new IllegalStateException("Failed to get the information.");
        }

        return value;
    }

    /**
     * Internal method called from native when a new data is available.
     */
    private void newDataReceived(String name, TensorsData data) {
        synchronized(this) {
            if (mEventListener != null) {
                mEventListener.onNewDataReceived(name, data);
            }
        }
    }

    /**
     * Internal method to check native handle.
     *
     * @throws IllegalStateException if the machine-learning service is not constructed.
     */
    private void checkNativeHandle() {
        if (mHandle == 0) {
            throw new IllegalStateException("The machine-learning service is not constructed.");
        }
    }

    /**
     * Internal method to validate string.
     */
    private static boolean isValidString(String str) {
        return (str != null && !str.trim().isEmpty());
    }

    @Override
    protected void finalize() throws Throwable {
        try {
            close();
        } finally {
            super.finalize();
        }
    }

    @Override
    public void close() {
        synchronized(this) {
            mEventListener = null;
        }

        if (mHandle != 0) {
            nativeClose(mHandle);
            mHandle = 0;
        }
    }
}
