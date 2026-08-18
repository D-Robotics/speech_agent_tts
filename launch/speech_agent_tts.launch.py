#!/usr/bin/env python3
# Copyright (c) 2024，D-Robotics.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from launch import LaunchDescription
from launch.actions import ExecuteProcess, DeclareLaunchArgument, LogInfo
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    pkg_share = get_package_share_directory('speech_agent_tts')
    setup_env = os.path.join(pkg_share, 'setup_env.sh')

    playback_device = LaunchConfiguration('playback_device')
    voice_name = LaunchConfiguration('voice_name')
    speed = LaunchConfiguration('speed')
    pitch = LaunchConfiguration('pitch')
    volume = LaunchConfiguration('volume')
    topic_sub = LaunchConfiguration('topic_sub')

    return LaunchDescription([
        DeclareLaunchArgument(
            'playback_device', default_value='plughw:0,0',
            description='audio playback device'),
        DeclareLaunchArgument(
            'voice_name', default_value='x_yezi_cpu5_10',
            description='TTS voice name'),
        DeclareLaunchArgument(
            'speed', default_value='75',
            description='TTS speech speed'),
        DeclareLaunchArgument(
            'pitch', default_value='80',
            description='TTS speech pitch'),
        DeclareLaunchArgument(
            'volume', default_value='90',
            description='TTS speech volume'),
        DeclareLaunchArgument(
            'topic_sub', default_value='/tts_text',
            description='tts text subscription topic'),

        LogInfo(
            msg=["\n",
                 "========== speech_agent_tts.launch.py usage ==========\n",
                 "usage:\n",
                 "  ros2 launch speech_agent_tts speech_agent_tts.launch.py ",
                 "[<argument_name>:=<value> ...]\n",
                 "\n",
                 "arguments (all optional, pass as <name>:=<value>):\n",
                 "  playback_device  audio playback device, default: 'plughw:0,0'\n",
                 "  voice_name       TTS voice name,        default: 'x_yezi_cpu5_10'\n",
                 "  speed            TTS speech speed,       default: '75'\n",
                 "  pitch            TTS speech pitch,       default: '80'\n",
                 "  volume           TTS speech volume,      default: '90'\n",
                 "  topic_sub        tts text subscription topic, "
                 "default: '/tts_text'\n",
                 "\n",
                 "examples:\n",
                 "  # launch with all default values\n",
                 "  ros2 launch speech_agent_tts speech_agent_tts.launch.py\n",
                 "  # launch with playback device and topic\n",
                 "  ros2 launch speech_agent_tts speech_agent_tts.launch.py ",
                 "playback_device:=plughw:0,0 topic_sub:=/tts_text\n",
                 "  # launch with custom speed and volume\n",
                 "  ros2 launch speech_agent_tts speech_agent_tts.launch.py ",
                 "speed:=80 volume:=70\n",
                 "\n",
                 "current values:\n",
                 "  playback_device=", playback_device, "\n",
                 "  voice_name=", voice_name, "\n",
                 "  speed=", speed, "\n",
                 "  pitch=", pitch, "\n",
                 "  volume=", volume, "\n",
                 "  topic_sub=", topic_sub, "\n",
                 "=======================================================\n"]),

        ExecuteProcess(
            cmd=['bash', '-c',
                 'source ' + setup_env + ' && '
                 'exec ros2 run speech_agent_tts speech_agent_tts '
                 '--ros-args -p playback_device:="$1" -p voice_name:="$2" '
                 '-p speed:="$3" -p pitch:="$4" -p volume:="$5" -p topic_sub:="$6"',
                 'bash',
                 playback_device,
                 voice_name,
                 speed,
                 pitch,
                 volume,
                 topic_sub],
            output='screen',
        ),
    ])