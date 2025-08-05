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

    @Test
    public void testModelRegisterNullName_n() {
        String model = APITestCommon.getTFLiteImgModel().getAbsolutePath();

        try {
            MLService.Model.register(null, model, false, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelRegisterEmptyName_n() {
        String model = APITestCommon.getTFLiteImgModel().getAbsolutePath();

        try {
            MLService.Model.register("", model, false, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelRegisterNullPath_n() {
        try {
            MLService.Model.register("test-name", null, false, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelRegisterEmptyPath_n() {
        try {
            MLService.Model.register("test-name", "", false, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelRegisterInvalidPath_n() {
        String path = APITestCommon.getRootDirectory() + "/invalid_path/invalid_model.tflite";

        try {
            MLService.Model.register("test-name", path, false, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelDeleteNullName_n() {
        try {
            MLService.Model.delete(null, 0);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelDeleteEmptyName_n() {
        try {
            MLService.Model.delete("", 0);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelDeleteUnregisteredName_n() {
        try {
            MLService.Model.delete("test-unregistered", 0);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelDeleteInvalidVersion_n() {
        try {
            MLService.Model.delete("test-name", -1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelDeleteActivated_n() {
        String name = "test-model";
        String model = APITestCommon.getTFLiteAddModel().getAbsolutePath();

        try {
            long version = MLService.Model.register(name, model, true, null);

            MLService.Model.delete(name, version);
            fail();
        } catch (Exception e) {
            /* expected */
        } finally {
            MLService.Model.delete(name, 0);
        }
    }

    @Test
    public void testModelActivateNullName_n() {
        try {
            MLService.Model.activate(null, 1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelActivateEmptyName_n() {
        try {
            MLService.Model.activate("", 1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelActivateUnregisteredName_n() {
        try {
            MLService.Model.activate("test-unregistered", 1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelActivateInvalidVersion_n() {
        try {
            MLService.Model.activate("test-name", -1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionNullName_n() {
        try {
            MLService.Model.updateDescription(null, 1, "test-description");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionEmptyName_n() {
        try {
            MLService.Model.updateDescription("", 1, "test-description");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionUnregisteredName_n() {
        try {
            MLService.Model.updateDescription("test-unregistered", 1, "test-description");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionNullDescription_n() {
        try {
            MLService.Model.updateDescription("test-unregistered", 1, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionEmptyDescription_n() {
        try {
            MLService.Model.updateDescription("test-unregistered", 1, "");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelUpdateDescriptionInvalidVersion_n() {
        try {
            MLService.Model.updateDescription("test-unregistered", -1, "test-description");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelGetNullName_n() {
        try {
            MLService.Model.get(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelGetEmptyName_n() {
        try {
            MLService.Model.get("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelGetUnregisteredName_n() {
        try {
            MLService.Model.get("test-unregistered", 1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModelGetInvalidVersion_n() {
        try {
            MLService.Model.get("test-name", -1);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testModel() {
        String name = "test-model";
        String model1 = APITestCommon.getTFLiteAddModel().getAbsolutePath();
        String model2 = APITestCommon.getTFLiteImgModel().getAbsolutePath();

        try {
            MLInformation[] models;
            MLInformation activated;
            long version1, version2;

            version1 = MLService.Model.register(name, model1, true, "test-model1");
            version2 = MLService.Model.register(name, model2, false, "test-model2");

            assertTrue(version1 > 0);
            assertTrue(version2 > 0);
            assertTrue(version1 != version2);

            /* check registered models */
            models = MLService.Model.get(name, 0);
            assertEquals(2, models.length);

            assertEquals(Long.toString(version1), models[0].get("version"));
            assertEquals(model1, models[0].get("path"));
            assertEquals("test-model1", models[0].get("description"));
            assertEquals("T", models[0].get("active"));

            assertEquals(Long.toString(version2), models[1].get("version"));
            assertEquals(model2, models[1].get("path"));
            assertEquals("test-model2", models[1].get("description"));
            assertEquals("F", models[1].get("active"));

            /* check active model */
            activated = MLService.Model.get(name);

            assertEquals(Long.toString(version1), activated.get("version"));
            assertEquals(model1, activated.get("path"));
            assertEquals("test-model1", activated.get("description"));
            assertEquals("T", activated.get("active"));

            MLService.Model.activate(name, version2);
            MLService.Model.updateDescription(name, version2, "activated");

            activated = MLService.Model.get(name);

            assertEquals(Long.toString(version2), activated.get("version"));
            assertEquals(model2, activated.get("path"));
            assertEquals("activated", activated.get("description"));
            assertEquals("T", activated.get("active"));

            models = MLService.Model.get(name, version1);
            assertEquals(1, models.length);

            assertEquals(Long.toString(version1), models[0].get("version"));
            assertEquals(model1, models[0].get("path"));
            assertEquals("test-model1", models[0].get("description"));
            assertEquals("F", models[0].get("active"));

            /* delete model */
            MLService.Model.delete(name, version1);

            models = MLService.Model.get(name, 0);
            assertEquals(1, models.length);

            assertEquals(Long.toString(version2), models[0].get("version"));
            assertEquals(model2, models[0].get("path"));
            assertEquals("activated", models[0].get("description"));
            assertEquals("T", models[0].get("active"));
        } catch (Exception e) {
            fail();
        } finally {
            MLService.Model.delete(name, 0);
        }

        try {
            /* no registered model in machine-learning service */
            MLService.Model.delete(name, 0);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceAddNullName_n() {
        String res = APITestCommon.getTestVideoPath();

        try {
            MLService.Resource.add(null, res, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceAddEmptyName_n() {
        String res = APITestCommon.getTestVideoPath();

        try {
            MLService.Resource.add("", res, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceAddNullPath_n() {
        try {
            MLService.Resource.add("test-name", null, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceAddEmptyPath_n() {
        try {
            MLService.Resource.add("test-name", "", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceAddInvalidPath_n() {
        String path = APITestCommon.getRootDirectory() + "/invalid_path/invalid_res.dat";

        try {
            MLService.Resource.add("test-name", path, null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceDeleteNullName_n() {
        try {
            MLService.Resource.delete(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceDeleteEmptyName_n() {
        try {
            MLService.Resource.delete("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceDeleteUnregisteredName_n() {
        try {
            MLService.Resource.delete("test-unregistered");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceGetNullName_n() {
        try {
            MLService.Resource.get(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceGetEmptyName_n() {
        try {
            MLService.Resource.get("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResourceGetUnregisteredName_n() {
        try {
            MLService.Resource.get("test-unregistered");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testResource() {
        String name = "test-resource";
        String res1 = APITestCommon.getTestVideoPath();
        String res2= APITestCommon.getTFLiteAddModel().getAbsolutePath();
        String res3= APITestCommon.getTFLiteImgModel().getAbsolutePath();

        try {
            MLInformation[] resources;

            MLService.Resource.add(name, res1, "test-resource1");
            MLService.Resource.add(name, res2, "test-resource2");

            /* check registered resources */
            resources = MLService.Resource.get(name);
            assertEquals(2, resources.length);

            assertEquals(res1, resources[0].get("path"));
            assertEquals("test-resource1", resources[0].get("description"));

            assertEquals(res2, resources[1].get("path"));
            assertEquals("test-resource2", resources[1].get("description"));

            MLService.Resource.add(name, res3, "test-resource3");

            resources = MLService.Resource.get(name);
            assertEquals(3, resources.length);

            assertEquals(res3, resources[2].get("path"));
            assertEquals("test-resource3", resources[2].get("description"));
        } catch (Exception e) {
            fail();
        } finally {
            MLService.Resource.delete(name);
        }

        try {
            /* no registered resource in machine-learning service */
            MLService.Resource.delete(name);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineSetNullName_n() {
        try {
            MLService.Pipeline.set(null, "fakesrc ! fakesink");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineSetEmptyName_n() {
        try {
            MLService.Pipeline.set("", "fakesrc ! fakesink");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineSetNullDescription_n() {
        try {
            MLService.Pipeline.set("test-name", null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineSetEmptyDescription_n() {
        try {
            MLService.Pipeline.set("test-name", "");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineGetNullName_n() {
        try {
            MLService.Pipeline.get(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineGetEmptyName_n() {
        try {
            MLService.Pipeline.get("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineGetInvalidName_n() {
        try {
            MLService.Pipeline.get("test-invalid-name");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineDeleteNullName_n() {
        try {
            MLService.Pipeline.delete(null);
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineDeleteEmptyName_n() {
        try {
            MLService.Pipeline.delete("");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipelineDeleteInvalidName_n() {
        try {
            MLService.Pipeline.delete("test-invalid-name");
            fail();
        } catch (Exception e) {
            /* expected */
        }
    }

    @Test
    public void testPipeline() {
        String name = "test-pipeline";
        String pipeline = "fakesrc ! fakesink";

        try {
            MLService.Pipeline.set(name, pipeline);
            assertEquals(pipeline, MLService.Pipeline.get(name));

            /* update pipeline description */
            String updated = "fakesrc name=testsrc ! fakesink name=testsink";

            MLService.Pipeline.set(name, updated);
            assertEquals(updated, MLService.Pipeline.get(name));
        } catch (Exception e) {
            fail();
        } finally {
            MLService.Pipeline.delete(name);
        }

        try {
            /* no registered pipeline in machine-learning service */
            MLService.Pipeline.get(name);
            fail();
        } catch (Exception e) {
            /* expected */
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

    @Test
    public void testRunPipelineRegistered() {
        String config = APITestCommon.getConfigPath() + "/config_pipeline_imgclf_key.conf";
        String model = APITestCommon.getTFLiteImgModel().getAbsolutePath();
        String name = "test-pipeline-imgclf";
        String pipeline = "appsrc name=input_img " +
                "caps=other/tensors,format=static,num_tensors=1,types=uint8,dimensions=3:224:224:1,framerate=0/1 ! " +
                "tensor_filter framework=tensorflow-lite model=" + model + " ! " +
                "tensor_sink name=result_clf";

        try {
            MLService.Pipeline.set(name, pipeline);

            runImageClassification(config, true);
        } catch (Exception e) {
            fail();
        } finally {
            MLService.Pipeline.delete(name);
        }
    }

    @Test
    public void testRunSingleShotRegistered() {
        String config = APITestCommon.getConfigPath() + "/config_single_imgclf_key.conf";
        String model = APITestCommon.getTFLiteImgModel().getAbsolutePath();
        String name = "test-single-imgclf";

        try {
            MLService.Model.register(name, model, true, null);

            runImageClassification(config, false);
        } catch (Exception e) {
            fail();
        } finally {
            MLService.Model.delete(name, 0);
        }
    }
}
