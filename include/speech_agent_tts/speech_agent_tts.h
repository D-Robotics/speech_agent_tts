// Copyright (c) 2024，D-Robotics.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SPEECH_AGENT_TTS_INCLUDE_H_
#define SPEECH_AGENT_TTS_INCLUDE_H_

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "tts_sdk.h"
#include "utils/alsa_device.h"
#include "dynamic_lib_loader.h"
#include "speech_agent_bootstrap/speech_agent_bootstrap.h"

namespace speech_agent_tts {

class SpeechAgentTTSNode {
 public:
  SpeechAgentTTSNode(rclcpp::Node::SharedPtr& nh);
  ~SpeechAgentTTSNode();

  void OnGetText(const std_msgs::msg::String::SharedPtr msg);

 private:
  rclcpp::Node::SharedPtr nh_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr text_subscription_;
  std::string topic_subscription_name_ = "/tts_text";
  std::string playback_device_name_ = "plughw:0,0";

  // 定义与头文件中签名完全一致的函数指针类型
  typedef int (*sa_tts_create_t)(sa_tts_handle*, sa_tts_callback_t, void*);
  typedef int (*sa_tts_synthesize_t)(sa_tts_handle, const char*, const sa_tts_params*);
  typedef void (*sa_tts_destroy_t)(sa_tts_handle);

  void MessageCallback(const std_msgs::msg::String::SharedPtr msg);

  static void TtsDataCallback(const void* pcm_data, size_t pcm_len, int is_end, void* user_data);

  void TTSEngineInit();

  void ProcessMessages();

  void PlaybackMessages();

  void StopPlayback();

  std::queue<std_msgs::msg::String::SharedPtr> message_queue_;
  std::mutex mutex_;
  std::condition_variable cv_;

  std::queue<std::pair<std::unique_ptr<int16_t[]>, int>> playback_queue_;
  std::mutex playback_mutex_;
  std::condition_variable cv_playback_;

  std::atomic<bool> stop_playback_{false};
  std::thread processing_thread_;
  std::thread playback_thread_;

  static constexpr size_t kMaxMessageQueueSize = 10;
  static constexpr size_t kMaxPlaybackQueueSize = 1000;

  sa_tts_handle tts_handle_;
  alsa_device_t* speaker_device_ = nullptr;

  sa_tts_synthesize_t p_sa_tts_synthesize_ = nullptr;
  sa_tts_destroy_t p_sa_tts_destroy_ = nullptr;
  LIB_HANDLE lib_handle_ = nullptr;
};

}  // namespace speech_agent_tts

#endif
