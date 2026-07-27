[English](./README.md) | 简体中文

# speech_agent_tts

**speech_agent_tts** 提供将文本转化为语音并播放的功能, 节点订阅/tts_text文本消息，并将文本合成为音频数据，然后通过ALSA接口进行播放。

## 环境搭建

运行 speech_agent_tts 前，需要确认音频设备设置正确。可使用如下命令检查：

### 1. 查看音频板是否安装成功

```bash
cat /proc/asound/cards
# 预期输出示例
0 [duplexaudioi2s1]: simple-card - duplex-audio-i2s1
                      duplex-audio-i2s1
1 [duplexaudio    ]: simple-card - duplex-audio
                      duplex-audio
```

### 2. 查看录音和播放节点

```bash
ls /dev/snd/
# 预期输出示例: by-path  controlC0  controlC1  pcmC0D0p  pcmC0D1c pcmC1D0c pcmC1D0p timer
```

如上所示，表示音频板安装正确，0表示音频板，pcmC0D1c是播放节点，pcmC0D0p是录音节点，具体设置方法参考 RDK 用户手册。

## 运行方式

### 1. 编译

`<TROS_DISTRO>` 为当前启用的 TROS 发行版 —— `humble` 或 `jazzy`，请替换为实际安装并 source 的发行版。

编译时会自动下载资源包，如果想手动下载资源包，可以使用下面命令将资源包下载到项目根目录，再进行编译

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_tts/speech_agent_tts_v1.0.0.tar.gz
```

- 编译

```bash
source /opt/tros/<TROS_DISTRO>/setup.bash
colcon build --packages-select speech_agent_tts
```

### 2. 启动

将资源包放到`/opt/tros/${TROS_DISTRO}/lib/`目录下
```bash
cp -r install/speech_agent_tts/lib/speech_agent_tts/ /opt/tros/${TROS_DISTRO}/lib/
```

- 启动

```bash
ros2 launch speech_agent_tts speech_agent_tts.launch.py playback_device:=plughw:0,0 volume:=90 topic_sub:=/tts_text
```

运行成功后，节点订阅 topic `/tts_text`（消息类型为 `std_msgs/msg/String`），将文本转化为语音信号播放。

### 3. 播放测试

```bash
ros2 topic pub --once /tts_text std_msgs/msg/String "{data: '你好,世界'}"
```

## 参数列表

| 参数名 | 解释 | 类型 | 默认值 |
| ------ | ---- | ---- | ------ |
| topic_sub | 订阅的文本 topic | std::string | "/tts_text" |
| playback_device | 语音播放音频设备 | std::string | "plughw:0,0" |
| voice_name | TTS 音色名称 | std::string | "x_yezi_cpu5_10" |
| speed | TTS 语音速度 | int | 75 |
| pitch | TTS 语音音调 | int | 80 |
| volume | TTS 语音音量 | int | 90 |

## 关键技术细节

### 音频播放参数

| 参数 | 值 |
| ---- | --- |
| 采样率 | 16000 Hz |
| 采样格式 | S16_LE（16-bit 有符号小端） |
| 通道数 | 8 |
| 周期大小 | 1024 frames |
| 周期数 | 8 |
| 缓冲区策略 | 使用默认 buffer time |

### TTS 合成

- 合成引擎输出单声道 16-bit PCM，通过回调函数 `TtsDataCallback` 传入播放队列
- 播放时将每个采样点复制到8个通道，形成8通道交错PCM数据

### 依赖关系

```
speech_agent_tts
  ├── rclcpp, std_msgs
  ├── libtts_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_tts_bootstrap.so
  ├── libasound  (ALSA)
  └── libdl
```

## 常见问题

### 1. 无法打开音频设备

- 确认音频设备连接是否正常
- 确认是否正确配置音频设备
- 确认是否正确设置播放节点

### 2. 播放声音异常

- 确认排线是否正常
- 确认喇叭连接是否正常
