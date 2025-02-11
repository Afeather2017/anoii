# Anoii

高效网络库

## 编译

### linux x86 x64

```shell
mkdir x86_64
cd x86_64
cmake ..
```

### 安卓

1. 安装ndk

2. 执行命令

里面的android30是一个示例，根据你的手机版本选择合适的版本，比如android21等等。android30的动态连接在Redmi Note 7上可以正常运行，而android21不行。

```shell
mkdir android
cd android
cmake -DCMAKE_C_COMPILER=/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android30-clang  -DCMAKE_CXX_COMPILER=/opt/android-ndk/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android30-clang++ -DCMAKE_VERBOSE_MAKEFILE=ON ../
```

### arm linux

```shell
mkdir arm
cd arm
cmake -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++ ..
```

### 其他cmake参数

1. 静态连接 `-DCMAKE_C_FLAGS=-static -DCMAKE_CXX_FLAGS=-static`

有时候静态连接还是有点用处的，尤其是arm编译的时候。

但是我在安卓上试过了静态连接，虽然能编译，但执行的时候出现了内存对齐的报错：

```
executable's TLS segment is underaligned: alignment is 8 (skew 0), needs to be at least 64 for ARM64 Bionic
```

2. debug模式编译 `-DCMAKE_BUILD_TYPE=Debug`

3. 导出编译命令 `CMAKE_EXPORT_COMPILE_COMMANDS=ON`

