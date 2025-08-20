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
 * - Provide interfaces to manage and control the machine-learning models and resources.<br>
 * - Provide interfaces to register a specific pipeline description.<br>
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

    private static native long nativeModelRegister(String name, String path, boolean activate, String description);
    private static native boolean nativeModelDelete(String name, long version);
    private static native boolean nativeModelActivate(String name, long version);
    private static native boolean nativeModelUpdateDescription(String name, long version, String description);
    private static native MLInformation[] nativeModelGet(String name, long version, boolean activated);

    private static native boolean nativeResourceAdd(String name, String path, String description);
    private static native boolean nativeResourceDelete(String name);
    private static native MLInformation[] nativeResourceGet(String name);

    private static native boolean nativePipelineSet(String name, String description);
    private static native String nativePipelineGet(String name);
    private static native boolean nativePipelineDelete(String name);

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

    /**
     * Provides interfaces to manage and control the machine-learning models.
     */
    public static class Model {
        /**
         * Registers new information of a neural network model.
         * Note that only one model can be activated with given name.
         * If same name is already registered in machine-learning service, this returns no error and old model will be deactivated when the flag activate is true.
         *
         * @param name The unique name to indicate the model.
         * @param path The path to neural network model.
         * @param activate The flag to set the model to be activated.
         * @param description Nullable, description for neural network model.
         *
         * @return The version of registered model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to register the neural network model.
         */
        public static long register(String name, String path, boolean activate, String description) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given model name is invalid.");
            }

            if (!isValidString(path)) {
                throw new IllegalArgumentException("Given model path is invalid.");
            }

            long version = nativeModelRegister(name, path, activate, description);

            if (version <= 0) {
                throw new IllegalStateException("Failed to register the model to machine-learning service.");
            }

            return version;
        }

        /**
         * Deletes a model information from machine-learning service.
         * Note that this does not remove the model file from file system.
         * If version is 0, machine-learning service will delete all information with given name.
         *
         * @param name The unique name for neural network model.
         * @param version The version of registered model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to delete the neural network model.
         */
        public static void delete(String name, long version) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given model name is invalid.");
            }

            if (version < 0) {
                throw new IllegalArgumentException("Given model version is invalid.");
            }

            if (!nativeModelDelete(name, version)) {
                throw new IllegalStateException("Failed to delete the model from machine-learning service.");
            }
        }

        /**
         * Activates a neural network model with specific version.
         *
         * @param name The unique name for neural network model.
         * @param version The version of registered model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to activate the neural network model.
         */
        public static void activate(String name, long version) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given model name is invalid.");
            }

            if (version <= 0) {
                throw new IllegalArgumentException("Given model version is invalid.");
            }

            if (!nativeModelActivate(name, version)) {
                throw new IllegalStateException("Failed to activate the neural network model.");
            }
        }

        /**
         * Updates the description of neural network model.
         *
         * @param name The unique name for neural network model.
         * @param version The version of registered model.
         * @param description The description for neural network model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to update the description of neural network model.
         */
        public static void updateDescription(String name, long version, String description) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given model name is invalid.");
            }

            if (!isValidString(description)) {
                throw new IllegalArgumentException("Given model description is invalid.");
            }

            if (version <= 0) {
                throw new IllegalArgumentException("Given model version is invalid.");
            }

            if (!nativeModelUpdateDescription(name, version, description)) {
                throw new IllegalStateException("Failed to update the model description.");
            }
        }

        /**
         * Gets the information of activated neural network model.
         *
         * @param name The unique name for neural network model.
         *
         * @return The information of activated neural network model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to get the information of neural network model.
         */
        public static MLInformation get(String name) {
            return retrieve(name, 0, true)[0];
        }

        /**
         * Gets the information of neural network model.
         * If version is 0, machine-learning service will retrieve all information with given name.
         *
         * @param name The unique name for neural network model.
         * @param version The version of registered model.
         *
         * @return The information of neural network model.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to get the information of neural network model.
         */
        public static MLInformation[] get(String name, long version) {
            return retrieve(name, version, false);
        }

        /**
         * Internal method to retrieve the information of neural network model.
         */
        private static MLInformation[] retrieve(String name, long version, boolean activated) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given model name is invalid.");
            }

            if (version < 0) {
                throw new IllegalArgumentException("Given model version is invalid.");
            }

            MLInformation[] info = nativeModelGet(name, version, activated);

            if (info == null) {
                throw new IllegalStateException("Failed to get the model information.");
            }

            return info;
        }

        /**
         * Private constructor to prevent the instantiation.
         */
        private Model() {}
    }

    /**
     * Provides interfaces to manage the machine-learning resources.
     */
    public static class Resource {
        /**
         * Adds new information of machine-learning resources those contain images, audio samples, binary files, and so on.
         * If same name is already registered in machine-learning service, this returns no error and the list of resource files will be updated.
         *
         * @param name The unique name for the machine-learning resources.
         * @param path The path to the machine-learning resources.
         * @param description Nullable, description for the machine-learning resources.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to add the machine-learning resources.
         */
        public static void add(String name, String path, String description) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given resource name is invalid.");
            }

            if (!isValidString(path)) {
                throw new IllegalArgumentException("Given resource path is invalid.");
            }

            if (!nativeResourceAdd(name, path, description)) {
                throw new IllegalStateException("Failed to add the resource to machine-learning service.");
            }
        }

        /**
         * Deletes the information of the machine-learning resources.
         * Note that this does not remove the resource files from file system.
         *
         * @param name The unique name for the machine-learning resources.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to delete the machine-learning resources.
         */
        public static void delete(String name) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given resource name is invalid.");
            }

            if (!nativeResourceDelete(name)) {
                throw new IllegalStateException("Failed to delete the resource from machine-learning service.");
            }
        }

        /**
         * Gets the information of the machine-learning resources.
         *
         * @param name The unique name for the machine-learning resources.
         *
         * @return The information of machine-learning resources.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to get the machine-learning resources.
         */
        public static MLInformation[] get(String name) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given resource name is invalid.");
            }

            MLInformation[] info = nativeResourceGet(name);

            if (info == null) {
                throw new IllegalStateException("Failed to get the resource information.");
            }

            return info;
        }

        /**
         * Private constructor to prevent the instantiation.
         */
        private Resource() {}
    }

    /**
     * Provides interfaces to register a specific pipeline description.
     *
     * @see org.nnsuite.nnstreamer.Pipeline#Pipeline(String)
     */
    public static class Pipeline {
        /**
         * Sets the pipeline description.
         * If the name already exists, the pipeline description is overwritten.
         *
         * @param name The unique name for the pipeline.
         * @param pipeline The pipeline description to be stored.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to set the pipeline description.
         */
        public static void set(String name, String pipeline) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given pipeline name is invalid.");
            }

            if (!isValidString(pipeline)) {
                throw new IllegalArgumentException("Given pipeline description is invalid.");
            }

            if (!nativePipelineSet(name, pipeline)) {
                throw new IllegalStateException("Failed to set the pipeline description to machine-learning service.");
            }
        }

        /**
         * Gets the pipeline description.
         * You can construct a {@link org.nnsuite.nnstreamer.Pipeline} instance using returned description.
         *
         * @param name The unique name for the pipeline.
         *
         * @return The pipeline description.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to get the pipeline description.
         */
        public static String get(String name) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given pipeline name is invalid.");
            }

            String pipeline = nativePipelineGet(name);

            if (pipeline == null) {
                throw new IllegalStateException("Failed to get the pipeline description from machine-learning service.");
            }

            return pipeline;
        }

        /**
         * Deletes the pipeline description.
         *
         * @param name The unique name for the pipeline.
         *
         * @throws IllegalArgumentException if given param is invalid.
         * @throws IllegalStateException if failed to delete the pipeline description.
         */
        public static void delete(String name) {
            if (!isValidString(name)) {
                throw new IllegalArgumentException("Given pipeline name is invalid.");
            }

            if (!nativePipelineDelete(name)) {
                throw new IllegalStateException("Failed to delete the pipeline description from machine-learning service.");
            }
        }

        /**
         * Private constructor to prevent the instantiation.
         */
        private Pipeline() {}
    }
}
