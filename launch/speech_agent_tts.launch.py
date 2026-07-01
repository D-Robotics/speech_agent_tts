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
from launch.actions import ExecuteProcess, DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from ament_index_python.packages import get_package_share_directory

import os


def generate_launch_description():
    pkg_share = get_package_share_directory('speech_agent_tts')
    setup_env = os.path.join(pkg_share, 'setup_env.sh')

    playback_device = LaunchConfiguration('playback_device')
    topic_sub = LaunchConfiguration('topic_sub')

    return LaunchDescription([
        DeclareLaunchArgument(
            'playback_device', default_value='plughw:0,0',
            description='audio playback device'),
        DeclareLaunchArgument(
            'topic_sub', default_value='/tts_text',
            description='tts text subscription topic'),

        ExecuteProcess(
            cmd=['bash', '-c',
                 'source ' + setup_env + ' && '
                 'exec ros2 run speech_agent_tts speech_agent_tts '
                 '--ros-args -p playback_device:="$1" -p topic_sub:="$2"',
                 'bash',
                 playback_device,
                 topic_sub],
            output='screen',
        ),
    ])