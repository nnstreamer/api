# NNStreamer API for Java

The Java API and its JNI wrapper for the Android API of NNStreamer.

- `android/`: the Android library project. `android/nnstreamer/src/main/jni` is the
  JNI wrapper, which is also built on Ubuntu by meson when `java-home` is given.
- `build-nnstreamer-android.sh`: build the Android library (AAR). Use
  `--build_test=yes` to build the instrumented tests as well, and `--run_test=yes`
  to run them on a connected device.
- `build-nnstreamer-ubuntu.sh`: build the Java API as a jar for Ubuntu.
- `test-nnstreamer-ubuntu.sh`: run the instrumented tests that do not require an
  Android device on a desktop JVM, against the JNI wrapper built by meson. The
  tests that cannot run on the host are listed in `host-test/exclude.txt`, and the
  Android classes they use are stubbed in `host-test/stub`.

```
meson setup build -Djava-home=$JAVA_HOME -Denable-test=false -Denable-ml-service=false
ninja -C build
bash java/test-nnstreamer-ubuntu.sh --ml_api_dir=. --build_dir=build
```
