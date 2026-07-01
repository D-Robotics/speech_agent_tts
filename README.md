English | [简体中文](./README_cn.md)

# speech_agent_tts

**speech_agent_tts** provides text-to-speech playback functionality. The node subscribes to text messages, calls the TTS SDK (`libtts_sdk.so`) to synthesize text into PCM audio data, then plays it back through an 8-channel ALSA device.

## Architecture

The node uses a dual-thread model:

- **ProcessMessages thread**: Subscribes to `/tts_text` via ROS2, splits the text by punctuation and whitespace, then calls `sa_tts_synthesize` segment by segment for speech synthesis.
- **PlaybackMessages thread**: Receives synthesized PCM data, duplicates each mono sample across all 8 channels into interleaved format, and writes to the sound card via ALSA.

TTS SDK initialization (`sa_init`) and dynamic library loading (`libtts_sdk.so`) are encapsulated by `speech_agent_tts_init` / `speech_agent_tts_deinit` from the `speech_agent_bootstrap` package, with embedded credentials — callers don't need to manage them.

## Environment Setup

Before running speech_agent_tts, verify the audio device is correctly configured:

```bash
ls /dev/snd/
# Expected output example: by-path  controlC0  pcmC0D0c  pcmC0D0p  timer
```

If a playback device like `pcmC0D0p` appears, the setup is correct. Refer to the RDK User Manual for detailed setup instructions.

## How to Run

### 1. DownLoad the resource pack

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_tts/speech_agent_tts_v1.0.0.tar.gz
tar -zxvf speech_agent_tts_v1.0.0.tar.gz -C /opt/tros/humble/lib/
```

### 2. Build

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_tts
```

### 3. Launch

```bash
ros2 launch speech_agent_tts speech_agent_tts.launch.py playback_device:=plughw:0,0 topic_sub:=/tts_text
```

Once running, the node subscribes to the `/tts_text` topic (message type: `std_msgs/msg/String`) and converts text to speech.

### 4. Test playback

```bash
ros2 topic pub --once /tts_text std_msgs/msg/String "{data: 'hello world'}"
```

## Parameters

| Parameter | Description | Type | Default |
| --------- | ----------- | ---- | ------- |
| topic_sub | Subscribed text topic | std::string | "/tts_text" |
| playback_device | Audio playback device | std::string | "plughw:0,0" |

## Key Technical Details

### Audio Playback Configuration

| Parameter | Value |
| --------- | ----- |
| Sample rate | 16000 Hz |
| Sample format | S16_LE (16-bit signed little-endian) |
| Channels | 8 |
| Period size | 1024 frames |
| Number of periods | 8 |
| Buffer strategy | Default buffer time |

### TTS Synthesis

- The synthesis engine outputs mono 16-bit PCM, delivered to the playback queue via the `TtsDataCallback` callback
- During playback, each mono sample is duplicated to all 8 channels to form 8-channel interleaved PCM data

### Dependencies

```
speech_agent_tts
  ├── rclcpp, std_msgs
  ├── libtts_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_tts_bootstrap.so
  ├── libasound  (ALSA)
  └── libdl
```