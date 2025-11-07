If you want to do unit tests, put the following files in this directory.

$ tree .
.
└── nnstreamer
    ├── config
    │   ├── config_pipeline_imgclf.conf
    │   ├── config_pipeline_imgclf_key.conf
    │   ├── config_single_imgclf.conf
    │   ├── config_single_imgclf_key.conf
    │   └── config_single_llamacpp_async.conf
    ├── llamacpp_data
    │   └── tinyllama-1.1b-chat-v1.0.Q2_K.gguf
    ├── pytorch_data
    │   ├── mobilenetv2-quant_core-nnapi.pt
    │   └── orange_float.raw
    ├── snpe_data
    │   ├── inception_v3_quantized.dlc
    │   └── plastic_cup.raw
    └── test
        ├── add
        │   ├── metadata
        │   │   └── MANIFEST
        │   └── add.tflite
        ├── imgclf
        │   ├── metadata
        │   │   └── MANIFEST
        │   ├── labels.txt
        │   ├── mobilenet_v1_1.0_224_quant.tflite
        │   └── mobilenet_v1_1.0_224.tflite
        ├── orange.png
        ├── orange.raw
        └── test_video.mp4


Configuration example:
The configuration file is a json formatted string for ML-service feature, which describes the configuration to run an inference application with model-agnostic method.
If you implement new Android application with ML-service and need to get a model from application's internal storage, you can set the predefined entity string '@APP_DATA_PATH@'.
Then it will be replaced with the application's data path.

Below is an example of asynchronous inference using LLaMA C++.

config_single_llamacpp_async.conf
{
    "single" :
    {
        "framework" : "llamacpp",
        "model" : ["@APP_DATA_PATH@/nnstreamer/llamacpp_data/tinyllama-1.1b-chat-v1.0.Q2_K.gguf"],
        "custom" : "num_predict:32",
        "invoke_dynamic" : "true",
        "invoke_async" : "true"
    }
}
