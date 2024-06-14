package org.nnsuite.nnstreamer;

import android.Manifest;
import android.content.Context;
import android.content.res.AssetManager;
import android.os.Environment;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.GrantPermissionRule;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Files;
import java.net.ServerSocket;

import static org.junit.Assert.*;

/**
 * Common definition to test NNStreamer API.
 *
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class APITestCommon {
    private static boolean mInitialized = false;
    private static String mRootDirectory = null;

    private static String getRootDirectory() {
        return mInitialized ? mRootDirectory : null;
    }

    /**
     * Copy to the directory in assets to the path dstName
     */
    private static boolean copyAssetFolder(Context context, String srcName, String dstName) {
        try {
            boolean result = true;
            String fileList[] = context.getAssets().list(srcName);
            if (fileList == null) return false;

            if (fileList.length == 0) {
                result = copyAssetFile(context, srcName, dstName);
            } else {
                File file = new File(dstName);
                result = file.mkdirs();
                for (String filename : fileList) {
                    result &= copyAssetFolder(context, srcName + File.separator + filename, dstName + File.separator + filename);
                }
            }
            return result;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * Helper function of copyAssetFolder. Execute file copy process.
     */
    private static boolean copyAssetFile(Context context, String srcName, String dstName) {
        try {
            InputStream in = context.getAssets().open(srcName);
            File outFile = new File(dstName);
            if (outFile.exists()) {
                /* already exists */
                return true;
            }

            OutputStream out = new FileOutputStream(outFile);
            byte[] buffer = new byte[1024];
            int read;
            while ((read = in.read(buffer)) != -1) {
                out.write(buffer, 0, read);
            }
            in.close();
            out.close();
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }

    /**
     * Initializes NNStreamer API library.
     */
    public static void initNNStreamer() {
        if (!mInitialized) {
            try {
                Context context = InstrumentationRegistry.getTargetContext();
                mInitialized = NNStreamer.initialize(context);

                /* Copy nnstreamer folder in assets to the app's files directory */
                mRootDirectory = context.getFilesDir().getAbsolutePath();
                copyAssetFolder(context, "nnstreamer", mRootDirectory + "/nnstreamer/");
            } catch (Exception e) {
                fail();
            }
        }
    }

    /**
     * Gets the context for the test application.
     */
    public static Context getContext() {
        return InstrumentationRegistry.getTargetContext();
    }

    /**
     * Gets the File object of tensorflow-lite model.
     * Note that, to invoke model in the storage, the permission READ_EXTERNAL_STORAGE is required.
     */
    public static File getTFLiteImgModel() {
        String root = getRootDirectory();
        File model = new File(root + "/nnstreamer/test/imgclf/mobilenet_v1_1.0_224_quant.tflite");
        File meta = new File(root + "/nnstreamer/test/imgclf/metadata/MANIFEST");

        if (!model.exists() || !meta.exists()) {
            fail();
        }

        return model;
    }

    /**
     * Reads raw image file (orange) and returns TensorsData instance.
     */
    public static TensorsData readRawImageData() {
        String root = getRootDirectory();
        File raw = new File(root + "/nnstreamer/test/orange.raw");

        if (!raw.exists()) {
            fail();
        }

        TensorsInfo info = new TensorsInfo();
        info.addTensorInfo(NNStreamer.TensorType.UINT8, new int[]{3,224,224,1});

        int size = info.getTensorSize(0);
        TensorsData data = TensorsData.allocate(info);

        try {
            byte[] content = Files.readAllBytes(raw.toPath());
            if (content.length != size) {
                fail();
            }

            ByteBuffer buffer = TensorsData.allocateByteBuffer(size);
            buffer.put(content);

            data.setTensorData(0, buffer);
        } catch (Exception e) {
            fail();
        }

        return data;
    }

    /**
     * Gets the label index with max score, for tensorflow-lite image classification.
     */
    public static int getMaxScore(ByteBuffer buffer) {
        int index = -1;
        int maxScore = 0;

        if (isValidBuffer(buffer, 1001)) {
            for (int i = 0; i < 1001; i++) {
                /* convert unsigned byte */
                int score = (buffer.get(i) & 0xFF);

                if (score > maxScore) {
                    maxScore = score;
                    index = i;
                }
            }
        }

        return index;
    }

    /**
     * Reads raw float image file (orange) and returns TensorsData instance.
     */
    public static TensorsData readRawImageDataPytorch() {
        String root = getRootDirectory();
        File raw = new File(root + "/nnstreamer/pytorch_data/orange_float.raw");

        if (!raw.exists()) {
            fail();
        }

        TensorsInfo info = new TensorsInfo();
        info.addTensorInfo(NNStreamer.TensorType.FLOAT32, new int[]{224, 224, 3, 1});

        int size = info.getTensorSize(0);
        TensorsData data = TensorsData.allocate(info);

        try {
            byte[] content = Files.readAllBytes(raw.toPath());
            if (content.length != size) {
                fail();
            }

            ByteBuffer buffer = TensorsData.allocateByteBuffer(size);
            buffer.put(content);

            data.setTensorData(0, buffer);
        } catch (Exception e) {
            fail();
        }

        return data;
    }

    /**
     * Reads raw float image file (orange) and returns TensorsData instance.
     */
    public static TensorsData readRawImageDataSNAP() {
        String root = getRootDirectory();
        File raw = new File(root + "/nnstreamer/imgclf/orange_float_tflite.raw");

        if (!raw.exists()) {
            fail();
        }

        TensorsInfo info = new TensorsInfo();
        info.addTensorInfo(NNStreamer.TensorType.FLOAT32, new int[]{3, 224, 224, 1});

        int size = info.getTensorSize(0);
        TensorsData data = TensorsData.allocate(info);

        try {
            byte[] content = Files.readAllBytes(raw.toPath());
            if (content.length != size) {
                fail();
            }

            ByteBuffer buffer = TensorsData.allocateByteBuffer(size);
            buffer.put(content);

            data.setTensorData(0, buffer);
        } catch (Exception e) {
            fail();
        }

        return data;
    }

    /**
     * Gets the label index with max score for float buffer with given length.
     */
    public static int getMaxScoreFloatBuffer(ByteBuffer buffer, int length) {
        int index = -1;
        float maxScore = -Float.MAX_VALUE;

        if (isValidBuffer(buffer, 4 * length)) {
            for (int i = 0; i < length; i++) {
                /* convert to float */
                float score = buffer.getFloat(i * 4);

                if (score > maxScore) {
                    maxScore = score;
                    index = i;
                }
            }
        }

        return index;
    }

    /**
     * Reads raw float image file (plastic_cup) and returns TensorsData instance.
     */
    public static TensorsData readRawImageDataSNPE() {
        String root = getRootDirectory();
        File raw = new File(root + "/nnstreamer/snpe_data/plastic_cup.raw");

        if (!raw.exists()) {
            fail();
        }

        TensorsInfo info = new TensorsInfo();
        info.addTensorInfo(NNStreamer.TensorType.FLOAT32, new int[]{3, 299, 299, 1});

        int size = info.getTensorSize(0);
        TensorsData data = TensorsData.allocate(info);

        try {
            byte[] content = Files.readAllBytes(raw.toPath());
            if (content.length != size) {
                fail();
            }

            ByteBuffer buffer = TensorsData.allocateByteBuffer(size);
            buffer.put(content);

            data.setTensorData(0, buffer);
        } catch (Exception e) {
            fail();
        }

        return data;
    }

    /**
     * Gets the path string of tensorflow-lite add.tflite model.
     */
    public static String getTFLiteAddModelPath() {
        String root = getRootDirectory();
        return root + "/nnstreamer/test/add";
    }

    /**
     * Gets the File object of tensorflow-lite add.tflite model.
     * Note that, to invoke model in the storage, the permission READ_EXTERNAL_STORAGE is required.
     */
    public static File getTFLiteAddModel() {
        String path = getTFLiteAddModelPath();
        File model = new File(path + "/add.tflite");
        File meta = new File(path + "/metadata/MANIFEST");

        if (!model.exists() || !meta.exists()) {
            fail();
        }

        return model;
    }

    public enum SNAPComputingUnit {
        CPU("ComputingUnit:CPU"),
        GPU("ComputingUnit:GPU,GpuCacheSource:/sdcard/nnstreamer/"),
        DSP("ComputingUnit:DSP"),
        NPU("ComputingUnit:NPU");

        private String computingUnit;

        SNAPComputingUnit(String computingUnit) {
            this.computingUnit = computingUnit;
        }

        public String getOptionString() {
            return computingUnit;
        }
    }

    /**
     * Gets the File objects of Caffe model for SNAP.
     * Note that, to invoke model in the storage, the permission READ_EXTERNAL_STORAGE is required.
     */
    public static File[] getSNAPCaffeModel() {
        String root = getRootDirectory();

        File model = new File(root + "/nnstreamer/snap_data/prototxt/squeezenet.prototxt");
        File weight = new File(root + "/nnstreamer/snap_data/model/squeezenet.caffemodel");

        if (!model.exists() || !weight.exists()) {
            fail();
        }

        return new File[]{model, weight};
    }

    /**
     * Gets the option string to run Caffe model for SNAP.
     *
     * CPU: "custom=ModelFWType:CAFFE,ExecutionDataType:FLOAT32,ComputingUnit:CPU"
     * GPU: "custom=ModelFWType:CAFFE,ExecutionDataType:FLOAT32,ComputingUnit:GPU,GpuCacheSource:/sdcard/nnstreamer/"
     */
    public static String getSNAPCaffeOption(SNAPComputingUnit CUnit) {
        String option = "ModelFWType:CAFFE,ExecutionDataType:FLOAT32,InputFormat:NHWC,OutputFormat:NCHW,";
        option = option + CUnit.getOptionString();

        return option;
    }

    /**
     * Gets the option string to run Tensorflow model for SNAP.
     *
     * CPU: "custom=ModelFWType:TENSORFLOW,ExecutionDataType:FLOAT32,ComputingUnit:CPU"
     * GPU: Not supported for Tensorflow model
     * DSP: "custom=ModelFWType:TENSORFLOW,ExecutionDataType:FLOAT32,ComputingUnit:DSP"
     * NPU: "custom=ModelFWType:TENSORFLOW,ExecutionDataType:FLOAT32,ComputingUnit:NPU"
     */
    public static String getSNAPTensorflowOption(SNAPComputingUnit CUnit) {
        String option = "ModelFWType:TENSORFLOW,ExecutionDataType:FLOAT32,InputFormat:NHWC,OutputFormat:NHWC,";
        option = option + CUnit.getOptionString();
        return option;
    }

    /**
     * Gets the File objects of Tensorflow model for SNAP.
     * Note that, to invoke model in the storage, the permission READ_EXTERNAL_STORAGE is required.
     */
    public static File[] getSNAPTensorflowModel(SNAPComputingUnit CUnit) {
        String root = getRootDirectory();
        String model_path = "/nnstreamer/snap_data/model/";

        switch (CUnit) {
            case CPU:
                model_path = model_path + "yolo_new.pb";
                break;
            case DSP:
                model_path = model_path + "yolo_new_tf_quantized.dlc";
                break;
            case NPU:
                model_path = model_path + "yolo_new_tf_quantized_hta.dlc";
                break;
            case GPU:
            default:
                fail();
        }

        File model = new File(root + model_path);
        if (!model.exists()) {
            fail();
        }

        return new File[]{model};
    }

    /**
     * Gets the option string to run TensorFlow Lite model for SNAP.
     *
     * CPU: "custom=ModelFWType:TENSORFLOWLITE,ExecutionDataType:FLOAT32,ComputingUnit:CPU"
     * GPU: "custom=ModelFWType:TENSORFLOWLITE,ExecutionDataType:FLOAT32,ComputingUnit:GPU"
     * DSP: Not supported for TensorFlow Lite model
     * NPU: "custom=ModelFWType:TENSORFLOWLITE,ExecutionDataType:FLOAT32,ComputingUnit:NPU"
     */
    public static String getSNAPTensorflowLiteOption(SNAPComputingUnit CUnit) {
        String option = "ModelFWType:TENSORFLOWLITE,ExecutionDataType:FLOAT32,InputFormat:NHWC,OutputFormat:NHWC,";
        option = option + CUnit.getOptionString();
        return option;
    }

    /**
     * Gets the File objects of Tensorflow Lite model for SNAP.
     * Note that, to invoke model in the storage, the permission READ_EXTERNAL_STORAGE is required.
     */
    public static File[] getSNAPTensorflowLiteModel() {
        String root = getRootDirectory();
        String model_path = "/nnstreamer/test/imgclf/mobilenet_v1_1.0_224.tflite";

        File model = new File(root + model_path);
        if (!model.exists()) {
            fail();
        }

        return new File[]{model};
    }

    /**
     * Gets the File objects of SNPE model.
     */
    public static File getSNPEModel() {
        String root = getRootDirectory();

        File model = new File(root + "/nnstreamer/snpe_data/inception_v3_quantized.dlc");

        if (!model.exists()) {
            fail();
        }

        return model;
    }

    /**
     * Get the File object of SNPE model for testing multiple output.
     * The model is converted to dlc format with SNPE SDK and it's from
     * https://github.com/nnsuite/testcases/tree/master/DeepLearningModels/tensorflow/ssdlite_mobilenet_v2
     */
    public static File getMultiOutputSNPEModel() {
        String root = getRootDirectory();

        File model = new File(root + "/nnstreamer/snpe_data/ssdlite_mobilenet_v2.dlc");

        if (!model.exists()) {
            fail();
        }

        return model;
    }

    /**
     * Gets the File objects of Pytorch model.
     */
    public static File getPytorchModel() {
        String root = getRootDirectory();

        File model = new File(root + "/nnstreamer/pytorch_data/mobilenetv2-quant_core-nnapi.pt");

        if (!model.exists()) {
            fail();
        }

        return model;
    }

    /**
     * Verifies the byte buffer is direct buffer with native order.
     *
     * @param buffer   The byte buffer
     * @param expected The expected capacity
     *
     * @return True if the byte buffer is valid.
     */
    public static boolean isValidBuffer(ByteBuffer buffer, int expected) {
        if (buffer != null && buffer.isDirect() && buffer.order() == ByteOrder.nativeOrder()) {
            int capacity = buffer.capacity();

            return (capacity == expected);
        }

        return false;
    }

    /**
     * Returns available port number.
     */
    public static int getAvailablePort() {
        int port = 0;

        try (ServerSocket socket = new ServerSocket(0)) {
            socket.setReuseAddress(true);
            port = socket.getLocalPort();
        } catch (Exception ignored) {}

        if (port > 0) {
            return port;
        }

        throw new RuntimeException("Could not find any available port");
    }


    @Before
    public void setUp() {
        initNNStreamer();
    }

    @Test
    public void useAppContext() {
        Context context = InstrumentationRegistry.getTargetContext();

        assertEquals("org.nnsuite.nnstreamer.test", context.getPackageName());
    }

    @Test
    public void testInitWithInvalidCtx_n() {
        try {
            NNStreamer.initialize(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void enumTensorType() {
        assertEquals(NNStreamer.TensorType.INT32, NNStreamer.TensorType.valueOf("INT32"));
        assertEquals(NNStreamer.TensorType.UINT32, NNStreamer.TensorType.valueOf("UINT32"));
        assertEquals(NNStreamer.TensorType.INT16, NNStreamer.TensorType.valueOf("INT16"));
        assertEquals(NNStreamer.TensorType.UINT16, NNStreamer.TensorType.valueOf("UINT16"));
        assertEquals(NNStreamer.TensorType.INT8, NNStreamer.TensorType.valueOf("INT8"));
        assertEquals(NNStreamer.TensorType.UINT8, NNStreamer.TensorType.valueOf("UINT8"));
        assertEquals(NNStreamer.TensorType.FLOAT64, NNStreamer.TensorType.valueOf("FLOAT64"));
        assertEquals(NNStreamer.TensorType.FLOAT32, NNStreamer.TensorType.valueOf("FLOAT32"));
        assertEquals(NNStreamer.TensorType.INT64, NNStreamer.TensorType.valueOf("INT64"));
        assertEquals(NNStreamer.TensorType.UINT64, NNStreamer.TensorType.valueOf("UINT64"));
        assertEquals(NNStreamer.TensorType.UNKNOWN, NNStreamer.TensorType.valueOf("UNKNOWN"));
    }

    @Test
    public void enumNNFWType() {
        assertEquals(NNStreamer.NNFWType.TENSORFLOW_LITE, NNStreamer.NNFWType.valueOf("TENSORFLOW_LITE"));
        assertEquals(NNStreamer.NNFWType.SNAP, NNStreamer.NNFWType.valueOf("SNAP"));
        assertEquals(NNStreamer.NNFWType.NNFW, NNStreamer.NNFWType.valueOf("NNFW"));
        assertEquals(NNStreamer.NNFWType.SNPE, NNStreamer.NNFWType.valueOf("SNPE"));
        assertEquals(NNStreamer.NNFWType.PYTORCH, NNStreamer.NNFWType.valueOf("PYTORCH"));
        assertEquals(NNStreamer.NNFWType.MXNET, NNStreamer.NNFWType.valueOf("MXNET"));
        assertEquals(NNStreamer.NNFWType.UNKNOWN, NNStreamer.NNFWType.valueOf("UNKNOWN"));
    }
}
