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
    private MLService.NewEventCallback mEventCb = new MLService.NewEventCallback() {
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
            new MLService(null, mEventCb);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testEmptyConfig_n() {
        try {
            new MLService("", mEventCb);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInvalidConfig_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_invalid.conf";

            new MLService(config, mEventCb);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testNullCallback_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_single_imgclf.conf";

            new MLService(config, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInputNullData_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.start();

            service.inputData("input_img", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testInputInvalidNode_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.start();

            service.inputData("invalid_node", APITestCommon.readRawImageData());
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInputInfo() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

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
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.getInputInformation("invalid_node");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetOutputInfo() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

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
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.getOutputInformation("invalid_node");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoNullName_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.setInformation(null, "test_value");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoEmptyName_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.setInformation("", "test_value");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoNullValue_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.setInformation("test_info", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testSetInfoEmptyValue_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.setInformation("test_info", "");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoNullName_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.getInformation(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoEmptyName_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.getInformation("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfoInvalidName_n() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

            service.getInformation("invalid_name");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testGetInfo() {
        try {
            String config = APITestCommon.getConfigPath() + "/config_single_imgclf.conf";
            MLService service = new MLService(config, mEventCb);

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
            MLService service = new MLService(config, mEventCb);

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
