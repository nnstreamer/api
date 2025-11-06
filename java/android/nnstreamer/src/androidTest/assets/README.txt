If you want to do unit tests, put the following files in this directory.

$ tree .
.
└── nnstreamer
    ├── config
    │   ├── config_pipeline_imgclf.conf
    │   ├── config_pipeline_imgclf_key.conf
    │   ├── config_single_imgclf.conf
    │   └── config_single_imgclf_key.conf
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
