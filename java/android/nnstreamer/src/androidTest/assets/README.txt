If you want to do unit tests, put the following files in this directory.

$ tree .
.
└── nnstreamer
    ├── pytorch_data
    │   ├── mobilenetv2-quant_core-nnapi.pt
    │   └── orange_float.raw
    ├── snpe_data
    │   ├── inception_v3_quantized.dlc
    │   ├── orange_299x299_uint8.raw
    │   └── plastic_cup.raw
    └── test
        ├── add
        │   ├── add.tflite
        │   └── metadata
        │       └── MANIFEST
        ├── imgclf
        │   ├── labels.txt
        │   ├── metadata
        │   │   └── MANIFEST
        │   ├── mobilenet_quant_v1_224.tflite
        │   ├── mobilenet_v1_1.0_224_quant.tflite
        │   ├── mobilenet_v1_1.0_224.tflite
        │   ├── orange.png
        │   └── orange.raw
        └── orange.raw
