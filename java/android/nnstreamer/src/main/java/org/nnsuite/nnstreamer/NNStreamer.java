/* SPDX-License-Identifier: Apache-2.0 */
/*
 * NNStreamer Android API
 * Copyright (C) 2019 Samsung Electronics Co., Ltd.
 */

package org.nnsuite.nnstreamer;

import java.util.Locale;

/**
 * Defines the types and limits in NNStreamer.<br>
 * To use NNStreamer, an application should call {@code NNStreamer.initialize(this)} with its context.<br>
 * <br>
 * NNStreamer is a set of GStreamer plugins that allow GStreamer developers to adopt neural network models easily and efficiently
 * and neural network developers to manage stream pipelines and their filters easily and efficiently.<br>
 * <br>
 * Note that, to open a machine learning model in the storage,
 * the permission {@code Manifest.permission.READ_EXTERNAL_STORAGE} is required before constructing the pipeline.
 * <br>
 * See <a href="https://github.com/nnstreamer/nnstreamer">NNStreamer repository</a> for the details.
 */
public final class NNStreamer {
    /**
     * The maximum rank that NNStreamer supports.
     */
    public static final int TENSOR_RANK_LIMIT = 4;

    /**
     * The maximum number of tensor that {@link TensorsData} instance may have.
     */
    public static final int TENSOR_SIZE_LIMIT = 16;

    /**
     * The enumeration for supported frameworks in NNStreamer.
     *
     * @see #isAvailable(NNFWType)
     */
    public enum NNFWType {
        /**
         * <a href="https://www.tensorflow.org/lite">TensorFlow Lite</a> is an open source
         * deep learning framework for on-device inference.<br>
         * <br>
         * Custom options (available from TensorFlow Lite 2.3.0 and Android 10)<br>
         * - Delegate: the hardware acceleration of TensorFlow Lite model (GPU/NNAPI/XNNPACK delegates are available)<br>
         * - NumThreads: the number of threads available to the interpreter
         */
        TENSORFLOW_LITE,
        /**
         * SNAP (Samsung Neural Acceleration Platform)
         * supports <a href="https://developer.samsung.com/neural">Samsung Neural SDK</a>
         * (Version 2.0, run only on Samsung devices).<br>
         * To construct a pipeline with SNAP, developer should set the custom option string
         * to specify the neural network and data format.<br>
         * <br>
         * Custom options<br>
         * - ModelFWType: the type of model (TensorFlow Lite/TensorFlow/Caffe)<br>
         * - ExecutionDataType: the execution data type for SNAP (default float32)<br>
         * - ComputingUnit: the computing unit to execute the model (default CPU)<br>
         * - CpuThreadCount: the number of CPU threads to be executed (optional, default 4 if ComputingUnit is CPU)<br>
         * - GpuCacheSource: the absolute path to GPU Kernel caching (mandatory if ComputingUnit is GPU)
         */
        SNAP,
        /**
         * NNFW is on-device neural network inference framework, which is developed by SR (Samsung Research).<br>
         * See <a href="https://github.com/Samsung/ONE">ONE (On-device Neural Engine) repository</a> for the details.
         */
        NNFW,
        /**
         * <a href="https://developer.qualcomm.com/docs/snpe/index.html">SNPE</a> (Snapdragon Neural Processing Engine)
         * is a Qualcomm Snapdragon software accelerated runtime for the execution of deep neural networks.<br>
         * <br>
         * Custom options<br>
         * - Runtime: the computing unit to execute the model (default CPU)<br>
         * - CPUFallback: CPU fallback mode (default false)
         */
        SNPE,
        /**
         * <a href="https://pytorch.org/mobile/home/">PyTorch Mobile</a>
         * is an on-device solution for the open source machine learning framework, PyTorch.
         */
        PYTORCH,
        /**
         * <a href="https://github.com/apache/incubator-mxnet">Apache MXNet</a>
         * is a deep learning framework designed for both efficiency and flexibility.
         */
        MXNET,
        /**
         * Unknown framework (usually error)
         */
        UNKNOWN
    }

    /**
     * The enumeration for possible data type of tensor in NNStreamer.
     */
    public enum TensorType {
        /** Integer 32bit */ INT32,
        /** Unsigned integer 32bit */ UINT32,
        /** Integer 16bit */ INT16,
        /** Unsigned integer 16bit */ UINT16,
        /** Integer 8bit */ INT8,
        /** Unsigned integer 8bit */ UINT8,
        /** Float 64bit */ FLOAT64,
        /** Float 32bit */ FLOAT32,
        /** Integer 64bit */ INT64,
        /** Unsigned integer 64bit */ UINT64,
        /** Unknown data type (usually error) */ UNKNOWN
    }

    private static native boolean nativeInitialize(Object app);
    private static native boolean nativeCheckNNFWAvailability(int fw);
    private static native String nativeGetVersion();

    /**
     * Initializes GStreamer and NNStreamer, registering the plugins and loading necessary libraries.
     *
     * @param app The application context
     *
     * @return true if successfully initialized
     *
     * @throws IllegalArgumentException if given param is invalid
     */
    public static boolean initialize(android.content.Context app) {
        if (app == null) {
            throw new IllegalArgumentException("Given context is invalid");
        }

        try {
@BUILD_ANDROID@            System.loadLibrary("gstreamer_android");
            System.loadLibrary("nnstreamer-native");

@BUILD_ANDROID@            org.freedesktop.gstreamer.GStreamer.init(app);
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }

        return nativeInitialize(app);
    }

    /**
     * Checks the neural network framework is available.
     *
     * @param fw The neural network framework
     *
     * @return true if the neural network framework is available
     */
    public static boolean isAvailable(NNFWType fw) {
        boolean available = nativeCheckNNFWAvailability(fw.ordinal());

        /* sub-plugin for given framework is available */
@BUILD_ANDROID@        if (available) {
@BUILD_ANDROID@            Locale locale = Locale.getDefault();
@BUILD_ANDROID@            String manufacturer = android.os.Build.MANUFACTURER.toLowerCase(locale);
@BUILD_ANDROID@            String hardware = android.os.Build.HARDWARE.toLowerCase(locale);
@BUILD_ANDROID@
@BUILD_ANDROID@            switch (fw) {
@BUILD_ANDROID@                case SNPE:
@BUILD_ANDROID@                    if (!hardware.startsWith("qcom")) {
@BUILD_ANDROID@                        available = false;
@BUILD_ANDROID@                    }
@BUILD_ANDROID@                    break;
@BUILD_ANDROID@                case SNAP:
@BUILD_ANDROID@                    if (!manufacturer.equals("samsung") ||
@BUILD_ANDROID@                        !(hardware.startsWith("qcom") || hardware.startsWith("exynos"))) {
@BUILD_ANDROID@                        available = false;
@BUILD_ANDROID@                    }
@BUILD_ANDROID@                    break;
@BUILD_ANDROID@                default:
@BUILD_ANDROID@                    break;
@BUILD_ANDROID@            }
@BUILD_ANDROID@        }

        return available;
    }

    /**
     * Gets the version string of NNStreamer.
     *
     * @return The version string
     */
    public static String getVersion() {
        return nativeGetVersion();
    }

    /**
     * Private constructor to prevent the instantiation.
     */
    private NNStreamer() {}
}
