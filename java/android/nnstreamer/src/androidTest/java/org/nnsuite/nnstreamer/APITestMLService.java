package org.nnsuite.nnstreamer;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.junit.Assert.*;

import java.nio.ByteBuffer;

/**
 * Testcases for MLService.
 */
@RunWith(AndroidJUnit4.class)
public class APITestMLService {
    private int mReceived = 0;
    private boolean mInvalidState = false;
    private boolean mIsPipeline = false;

    /**
     * The event callback for image classification model.
     */
    private MLService.EventListener mEventListener = new MLService.EventListener() {
        @Override
        public void onNewDataReceived(String name, TensorsData data) {
            if (mIsPipeline) {
                if (name == null || !name.equals("result_clf")) {
                    mInvalidState = true;
                    return;
                }
            }

            if (data == null || data.getTensorsCount() != 1) {
                mInvalidState = true;
                return;
            }

            ByteBuffer buffer = data.getTensorData(0);
            int labelIndex = APITestCommon.getMaxScore(buffer);

            /* check label index (orange) */
            if (labelIndex != 951) {
                mInvalidState = true;
            }

            mReceived++;
        }
    };

    @Before
    public void setUp() {
        APITestCommon.initNNStreamer();
    }

    @Test
    public void testNullConfig_n() {
        try {
            new MLService(null, mEventListener);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testEmptyConfig_n() {
        try {
            new MLService("", mEventListener);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInvalidConfig_n() {
        String config = APITestCommon.getConfigPath() + "/config_invalid.conf";

        try {
            new MLService(config, mEventListener);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testNullCallback_n() {
        String config = APITestCommon.getConfigPath() + "/config_single_imgclf.conf";

        try {
            new MLService(config, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInputNullData_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.start();

            service.inputData("input_img", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInputInvalidNode_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.start();

            service.inputData("invalid_node", APITestCommon.readRawImageData());
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInputInfo() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            TensorsInfo info = service.getInputInformation("input_img");

            assertEquals(1, info.getTensorsCount());
            assertEquals(NNStreamer.TensorType.UINT8, info.getTensorType(0));
            assertArrayEquals(new int[]{3,224,224,1}, info.getTensorDimension(0));

            service.close();
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testGetInputInfoInvalidNode_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.getInputInformation("invalid_node");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetOutputInfo() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            TensorsInfo info = service.getOutputInformation("result_clf");

            assertEquals(1, info.getTensorsCount());
            assertEquals(NNStreamer.TensorType.UINT8, info.getTensorType(0));
            assertArrayEquals(new int[]{1001,1}, info.getTensorDimension(0));

            service.close();
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testGetOutputInfoInvalidNode_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.getOutputInformation("invalid_node");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoNullName_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.setInformation(null, "test_value");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoEmptyName_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.setInformation("", "test_value");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoNullValue_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.setInformation("test_info", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoEmptyValue_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.setInformation("test_info", "");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoNullName_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.getInformation(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoEmptyName_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.getInformation("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoInvalidName_n() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.getInformation("invalid_name");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfo() {
        String config = APITestCommon.getConfigPath() + "/config_single_imgclf.conf";

        try {
            MLService service = new MLService(config, mEventListener);

            service.setInformation("test_info", "test_value");

            assertEquals("0.5", service.getInformation("threshold"));
            assertEquals("test_value", service.getInformation("test_info"));

            service.close();
        } catch (Exception e) {
            fail();
        }
    }

    /**
     * Runs image classification with configuration.
     */
    private void runImageClassification(String config, boolean isPipeline) {
        mIsPipeline = isPipeline;

        try {
            MLService service = new MLService(config, mEventListener);

            service.start();

            /* push input buffer */
            TensorsData input = APITestCommon.readRawImageData();

            for (int i = 0; i < 5; i++) {
                service.inputData("input_img", input);
                Thread.sleep(100);
            }

            /* sleep 200 to invoke */
            Thread.sleep(200);

            /* check received data from output node */
            assertFalse(mInvalidState);
            assertTrue(mReceived > 0);

            service.close();
        } catch (Exception e) {
            fail();
        }
    }

    @Test
    public void testRunPipeline() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";

        runImageClassification(config, true);
    }

    @Test
    public void testRunSingleShot() {
        String config = APITestCommon.getConfigPath() + "/config_single_imgclf.conf";

        runImageClassification(config, false);
    }
}
