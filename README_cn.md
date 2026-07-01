[English](./README.md) | 简体中文

# speech_agent_tts

**speech_agent_tts** 提供将文本转化为语音播放的功能。节点订阅文本消息，调用 TTS SDK（`libtts_sdk.so`）将文本合成为 PCM 音频数据，然后通过 ALSA 接口进行8通道播放。

## 架构设计

节点采用双线程模型：

- **ProcessMessages 线程**：从 ROS2 订阅 `/tts_text` 获取文本，按标点符号和空格分词后，逐段调用 `sa_tts_synthesize` 进行语音合成。
- **PlaybackMessages 线程**：接收合成后的 PCM 数据，将单声道数据复制为8通道交错格式，通过 ALSA 写入声卡播放。

TTS SDK 的初始化（`sa_init`）和动态库加载（`libtts_sdk.so`）由 `speech_agent_bootstrap` 包中的 `speech_agent_tts_init` / `speech_agent_tts_deinit` 封装，内置鉴权凭证，调用方无需关心。

## 环境搭建

运行 speech_agent_tts 前，需要确认音频设备设置正确。可使用如下命令检查：

```bash
ls /dev/snd/
# 预期输出示例: by-path  controlC0  pcmC0D0c  pcmC0D0p  timer
```

如果出现 `pcmC0D0p`（播放设备）则表示设置正确。具体设置方法参考 RDK 用户手册。

## 运行方式

### 1. 下载资源包

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_tts/speech_agent_tts_v1.0.0.tar.gz
tar -zxvf speech_agent_tts_v1.0.0.tar.gz -C /opt/tros/humble/lib/
```

### 2. 编译

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_tts
```

### 3. 启动

```bash
ros2 launch speech_agent_tts speech_agent_tts.launch.py playback_device:=plughw:0,0 topic_sub:=/tts_text
```

运行成功后，节点订阅 topic `/tts_text`（消息类型为 `std_msgs/msg/String`），将文本转化为语音信号播放。

### 4. 测试播放

```bash
ros2 topic pub --once /tts_text std_msgs/msg/String "{data: '你好,世界'}"
```

## 参数列表

| 参数名 | 解释 | 类型 | 默认值 |
| ------ | ---- | ---- | ------ |
| topic_sub | 订阅的文本 topic | std::string | "/tts_text" |
| playback_device | 语音播放音频设备 | std::string | "plughw:0,0" |

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