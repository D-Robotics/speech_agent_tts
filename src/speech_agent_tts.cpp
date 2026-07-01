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

#include "speech_agent_tts/speech_agent_tts.h"

#include <alsa/asoundlib.h>

#include <string>

namespace speech_agent_tts {

SpeechAgentTTSNode::SpeechAgentTTSNode(rclcpp::Node::SharedPtr& nh) : nh_(nh) {
  nh_->declare_parameter<std::string>("playback_device", playback_device_name_);
  nh_->get_parameter<std::string>("playback_device", playback_device_name_);

  speaker_device_ = alsa_device_allocate();
  if (!speaker_device_) {
    RCLCPP_ERROR(nh_->get_logger(), "alloc speaker device error!");
    throw std::runtime_error("SpeechAgentTTSNode allocate alsa device failed");
  }
  speaker_device_->name = const_cast<char*>(playback_device_name_.c_str());
  speaker_device_->format = SND_PCM_FORMAT_S16;
  speaker_device_->direct = SND_PCM_STREAM_PLAYBACK;
  speaker_device_->rate = 16000;
  speaker_device_->channels = 8;
  speaker_device_->buffer_time = 0;  // use default buffer time
  speaker_device_->nperiods = 8;
  speaker_device_->period_size = 1024;

  auto ret = alsa_device_init(speaker_device_);
  if (ret < 0) {
    if (speaker_device_) {
      free(speaker_device_);
      speaker_device_ = nullptr;
    }
    RCLCPP_ERROR(nh_->get_logger(), "alsa_device_init speaker failed:%d", ret);
  }
  RCLCPP_INFO(nh_->get_logger(), "devices name:%s", speaker_device_->name);
  RCLCPP_INFO(nh_->get_logger(), "alsa_device_init success:%d", ret);

  TTSEngineInit();

  nh_->declare_parameter<std::string>("topic_sub", topic_subscription_name_);
  nh_->get_parameter<std::string>("topic_sub", topic_subscription_name_);

  text_subscription_ = nh_->create_subscription<std_msgs::msg::String>(
      topic_subscription_name_, 10,
      std::bind(&SpeechAgentTTSNode::MessageCallback, this, std::placeholders::_1));

  processing_thread_ = std::thread(&SpeechAgentTTSNode::ProcessMessages, this);
  playback_thread_ = std::thread(&SpeechAgentTTSNode::PlaybackMessages, this);
}

SpeechAgentTTSNode::~SpeechAgentTTSNode() {
  StopPlayback();
  // session 级销毁：必须在 speech_agent_tts_deinit 之前
  if (p_sa_tts_destroy_) {
    p_sa_tts_destroy_(tts_handle_);
  }

  // SDK 级销毁（替代原来的 sa_destroy + CLOSE_LIBRARY）
  speech_agent_tts_deinit(lib_handle_);
  lib_handle_ = nullptr;

  if (speaker_device_) {
    alsa_device_deinit(speaker_device_);
    alsa_device_free(speaker_device_);
    speaker_device_ = nullptr;
  }
}

void SpeechAgentTTSNode::TTSEngineInit() {
  std::string tros_distro
      = std::string(std::getenv("TROS_DISTRO")? std::getenv("TROS_DISTRO") : "");
  std::string speech_agent_resource = "/opt/tros/" + tros_distro + "/lib/speech_agent_tts";

  // 使用 speech_agent_bootstrap 替代原来的动态库加载和 sa_init
  lib_handle_ = speech_agent_tts_init(speech_agent_resource.c_str());
  if (!lib_handle_) {
    RCLCPP_ERROR(nh_->get_logger(), "speech_agent_tts_init failed");
    return;
  }
  RCLCPP_INFO(nh_->get_logger(), "tts init success");

  // 缓存所有函数指针，避免重复 LOAD_SYMBOL
  sa_tts_create_t p_sa_tts_create = (sa_tts_create_t)LOAD_SYMBOL(lib_handle_, "sa_tts_create");
  p_sa_tts_synthesize_ = (sa_tts_synthesize_t)LOAD_SYMBOL(lib_handle_, "sa_tts_synthesize");
  p_sa_tts_destroy_ = (sa_tts_destroy_t)LOAD_SYMBOL(lib_handle_, "sa_tts_destroy");
  if (!p_sa_tts_create || !p_sa_tts_synthesize_ || !p_sa_tts_destroy_) {
    RCLCPP_ERROR(nh_->get_logger(), "load tts symbol failed");
    return;
  }

  // sa_tts_create 是 session 级操作
  int ret = p_sa_tts_create(&tts_handle_, TtsDataCallback, this);
  if (ret) {
    if (speaker_device_) {
      free(speaker_device_);
      speaker_device_ = nullptr;
    }
    RCLCPP_ERROR(nh_->get_logger(), "sa_tts_create failed:%d", ret);
  }
  RCLCPP_INFO(nh_->get_logger(), "sa_tts_create success");

}

void SpeechAgentTTSNode::MessageCallback(const std_msgs::msg::String::SharedPtr msg) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (message_queue_.size() >= kMaxMessageQueueSize) {
    // Discard the oldest message if the queue size exceeds the limit
    RCLCPP_WARN(nh_->get_logger(), "drop msg:[%s]", message_queue_.front()->data.c_str());
    message_queue_.pop();
  }
  message_queue_.push(msg);
  cv_.notify_one();
}

void SpeechAgentTTSNode::TtsDataCallback(const void* pcm_data, size_t pcm_len, int is_end, void* user_data) {
  SpeechAgentTTSNode* self = static_cast<SpeechAgentTTSNode*>(user_data);
  std::lock_guard<std::mutex> playback_lock(self->playback_mutex_);
  std::unique_ptr<int16_t[]> pcm_data_int16 = std::make_unique<int16_t[]>(pcm_len / sizeof(int16_t));
  std::memcpy(pcm_data_int16.get(), pcm_data, pcm_len);
  if (self->playback_queue_.size() >= kMaxPlaybackQueueSize) {
    // Discard the oldest PCM data if the queue size exceeds the limit
    self->playback_queue_.pop();
  }
  // RCLCPP_INFO(self->nh_->get_logger(), ">>>> pcm_len size: %zu", pcm_len);
  self->playback_queue_.push(std::make_pair(std::move(pcm_data_int16), pcm_len/sizeof(int16_t)));
  self->cv_playback_.notify_one();
  if (is_end) {
    RCLCPP_INFO(self->nh_->get_logger(), "[TTS] synthesis complete, total");
  }
}

void SpeechAgentTTSNode::ProcessMessages() {
  if (!p_sa_tts_synthesize_) {
    RCLCPP_ERROR(nh_->get_logger(), "sa_tts_synthesize not loaded");
    return;
  }

  while (rclcpp::ok()) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock,
             [this] { return !message_queue_.empty() || stop_playback_; });

    if (stop_playback_) {
      break;
    }

    while (!message_queue_.empty()) {
      auto message = message_queue_.front();
      message_queue_.pop();
      int ret = p_sa_tts_synthesize_(tts_handle_, message->data.c_str(), nullptr);
      if (ret != SA_OK) {
        RCLCPP_ERROR(nh_->get_logger(), "Synthesis failed with error code:%d", ret);
      }
    }
  }
}

void SpeechAgentTTSNode::PlaybackMessages() {
  snd_pcm_prepare(speaker_device_->handle);

  // 固定单周期8通道缓冲，全程只分配一次，无动态扩容
  const size_t period_frames = speaker_device_->period_size;
  const size_t buf_8ch_samples = period_frames * 8;
  std::vector<int16_t> buf_8ch(buf_8ch_samples, 0);

  while (rclcpp::ok()) {
    std::unique_lock<std::mutex> lock(playback_mutex_);
    cv_playback_.wait(
        lock, [this] { return !playback_queue_.empty() || stop_playback_; });

    if (stop_playback_ && playback_queue_.empty()) {
      break;
    }

    // 批量取出当前队列所有数据，取出后立即解锁，耗时运算不再持有锁
    std::vector<std::pair<std::unique_ptr<int16_t[]>, size_t>> pending_list;
    while (!playback_queue_.empty()) {
      pending_list.emplace_back(std::move(playback_queue_.front()));
      playback_queue_.pop();
    }
    lock.unlock();

    // 遍历每一段单通道TTS PCM
    for (auto& item : pending_list) {
      std::unique_ptr<int16_t[]> mono_data = std::move(item.first);
      const size_t mono_sample_cnt = item.second;
      int16_t* mono_ptr = mono_data.get();
      size_t read_pos = 0;

      // 分段流式转8通道交错并写入声卡
      while (read_pos < mono_sample_cnt) {
        // 当前周期最多处理多少单声道帧
        size_t curr_frames = std::min(period_frames, mono_sample_cnt - read_pos);
        size_t buf_idx = 0;

        // 填充8通道交错缓冲
        for (size_t f = 0; f < curr_frames; f++) {
          int16_t val = mono_ptr[read_pos + f];
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
          buf_8ch[buf_idx++] = val;
        }

        // 调用原有alsa写入接口
        snd_pcm_sframes_t ret_write = alsa_device_write(speaker_device_, buf_8ch.data(), curr_frames);
        if (ret_write < 0) {
          if (ret_write == -EPIPE) {
            RCLCPP_WARN(nh_->get_logger(), "play underrun recover");
            snd_pcm_prepare(speaker_device_->handle);
          } else {
            RCLCPP_WARN(nh_->get_logger(), "alsa write err:%s", snd_strerror(ret_write));
          }
        }

        read_pos += curr_frames;
      }
    }
    // 重置PCM状态，避免下次写入时因buffer已空而underrun             
    snd_pcm_drain(speaker_device_->handle);                                                    
    snd_pcm_prepare(speaker_device_->handle);
  }
  snd_pcm_drop(speaker_device_->handle);
}

void SpeechAgentTTSNode::StopPlayback() {
  if (!stop_playback_) {
    stop_playback_ = true;
    cv_.notify_one();
    cv_playback_.notify_one();
    if (processing_thread_.joinable()) {
      processing_thread_.join();
    }
    if (playback_thread_.joinable()) {
      playback_thread_.join();
    }
  }
}

}  // namespace speech_agent_tts