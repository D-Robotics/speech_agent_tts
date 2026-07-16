[English](./README.md) | [简体中文](./README_cn.md)

# speech_agent_tts

**speech_agent_tts** provides text-to-speech synthesis and audio playback functionality. It subscribes to text messages on the `/tts_text` topic, synthesizes the text into audio data, and plays it back through the ALSA interface.

## Environment Setup

Before running speech_agent_tts, verify that the audio device is properly configured. Use the following commands:

### 1. Check Audio Card Installation

```bash
cat /proc/asound/cards
# Expected output example
0 [duplexaudioi2s1]: simple-card - duplex-audio-i2s1
                      duplex-audio-i2s1
1 [duplexaudio    ]: simple-card - duplex-audio
                      duplex-audio
```

### 2. Check Capture and Playback Nodes

```bash
ls /dev/snd/
# Expected output example: by-path  controlC0  controlC1  pcmC0D0p  pcmC0D1c pcmC1D0c pcmC1D0p timer
```

As shown above, the audio card is correctly installed. Card 0 is the audio card, `pcmC0D1c` is the capture node, and `pcmC0D0p` is the playback node. Refer to the RDK user manual for detailed configuration instructions.

## How to Run

### 1. Build

The resource package is downloaded automatically during the build. To download it manually, run the following command to place the resource package in the project root directory before building:

```bash
wget https://archive.d-robotics.cc/TogetheROS/files/speech_solution/speech_agent_tts/speech_agent_tts_v1.0.0.tar.gz
```

- Build:

```bash
source /opt/tros/humble/setup.bash
colcon build --packages-select speech_agent_tts
```

### 2. Launch

```bash
ros2 launch speech_agent_tts speech_agent_tts.launch.py playback_device:=plughw:0,0 volume:=90 topic_sub:=/tts_text
```

After a successful launch, the node subscribes to the `/tts_text` topic (message type: `std_msgs/msg/String`) and synthesizes the text into speech for playback.

### 3. Playback Test

```bash
ros2 topic pub --once /tts_text std_msgs/msg/String "{data: 'Hello, world'}"
```

## Parameters

| Parameter | Description | Type | Default |
| --------- | ----------- | ---- | ------- |
| topic_sub | Subscribed text topic | std::string | "/tts_text" |
| playback_device | Audio playback device | std::string | "plughw:0,0" |
| voice_name | TTS voice name | std::string | "x_yezi_cpu5_10" |
| speed | TTS speech speed | int | 75 |
| pitch | TTS speech pitch | int | 80 |
| volume | TTS speech volume | int | 90 |

## Key Technical Details

### Audio Playback Parameters

| Parameter | Value |
| --------- | ----- |
| Sample Rate | 16000 Hz |
| Sample Format | S16_LE (16-bit signed little-endian) |
| Channels | 8 |
| Period Size | 1024 frames |
| Period Count | 8 |
| Buffer Strategy | Default buffer time |

### TTS Synthesis

- The synthesis engine outputs mono 16-bit PCM, which is fed into the playback queue via the `TtsDataCallback` callback function
- During playback, each sample is replicated across all 8 channels to form 8-channel interleaved PCM data

### Dependencies

```
speech_agent_tts
  ├── rclcpp, std_msgs
  ├── libtts_sdk.so (dynamically loaded at runtime)
  ├── libspeech_agent_tts_bootstrap.so
  ├── libasound  (ALSA)
  └── libdl
```

## FAQ

### 1. Unable to Open Audio Device

- Verify that the audio device is properly connected
- Verify that the audio device is correctly configured
- Verify that the playback node is correctly set

### 2. Abnormal Playback Sound

- Check that the cable connections are secure
- Verify that the speaker connections are secure