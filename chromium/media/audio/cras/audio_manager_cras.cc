// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/audio/cras/audio_manager_cras.h"

#include <stddef.h>

#include <algorithm>
#include <map>
#include <utility>

#include "base/bind.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/metrics/field_trial_params.h"
#include "base/nix/xdg_util.h"
#include "base/stl_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/system/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"
#include "media/audio/audio_device_description.h"
#include "media/audio/audio_features.h"
#include "media/audio/cras/cras_input.h"
#include "media/audio/cras/cras_unified.h"
#include "media/base/channel_layout.h"
#include "media/base/limits.h"
#include "media/base/localized_strings.h"

namespace media {
namespace {

// Default sample rate for input and output streams.
const int kDefaultSampleRate = 48000;

// Default input buffer size.
const int kDefaultInputBufferSize = 1024;

// Default output buffer size.
const int kDefaultOutputBufferSize = 512;

}  // namespace

bool AudioManagerCras::HasAudioOutputDevices() {
  return true;
}

bool AudioManagerCras::HasAudioInputDevices() {
  return true;
}

AudioManagerCras::AudioManagerCras(
    std::unique_ptr<AudioThread> audio_thread,
    AudioLogFactory* audio_log_factory)
    : AudioManagerCrasBase(std::move(audio_thread), audio_log_factory),
      main_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_ptr_factory_(this) {
  weak_this_ = weak_ptr_factory_.GetWeakPtr();
}

AudioManagerCras::~AudioManagerCras() = default;

void AudioManagerCras::GetAudioInputDeviceNames(
    AudioDeviceNames* device_names) {
  device_names->push_back(AudioDeviceName::CreateDefault());
}

void AudioManagerCras::GetAudioOutputDeviceNames(
    AudioDeviceNames* device_names) {
  device_names->push_back(AudioDeviceName::CreateDefault());
}

AudioParameters AudioManagerCras::GetInputStreamParameters(
    const std::string& device_id) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());

  int user_buffer_size = GetUserBufferSize();
  int buffer_size =
      user_buffer_size ? user_buffer_size : kDefaultInputBufferSize;

  AudioParameters params(
      AudioParameters::AUDIO_PCM_LOW_LATENCY, CHANNEL_LAYOUT_STEREO,
      kDefaultSampleRate, buffer_size,
      AudioParameters::HardwareCapabilities(limits::kMinAudioBufferSize,
                                            limits::kMaxAudioBufferSize));
  return params;
}

std::string AudioManagerCras::GetDefaultInputDeviceID() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return base::NumberToString(GetPrimaryActiveInputNode());
}

std::string AudioManagerCras::GetDefaultOutputDeviceID() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return base::NumberToString(GetPrimaryActiveOutputNode());
}

AudioParameters AudioManagerCras::GetPreferredOutputStreamParameters(
    const std::string& output_device_id,
    const AudioParameters& input_params) {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  ChannelLayout channel_layout = CHANNEL_LAYOUT_STEREO;
  int sample_rate = kDefaultSampleRate;
  int buffer_size = GetUserBufferSize();
  if (input_params.IsValid()) {
    channel_layout = input_params.channel_layout();
    sample_rate = input_params.sample_rate();
    if (!buffer_size)  // Not user-provided.
      buffer_size =
          std::min(static_cast<int>(limits::kMaxAudioBufferSize),
                   std::max(static_cast<int>(limits::kMinAudioBufferSize),
                            input_params.frames_per_buffer()));
  }

  if (!buffer_size)  // Not user-provided.
    buffer_size = kDefaultOutputBufferSize;

  return AudioParameters(
      AudioParameters::AUDIO_PCM_LOW_LATENCY, channel_layout, sample_rate,
      buffer_size,
      AudioParameters::HardwareCapabilities(limits::kMinAudioBufferSize,
                                            limits::kMaxAudioBufferSize));
}

uint64_t AudioManagerCras::GetPrimaryActiveInputNode() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return 0;
}

uint64_t AudioManagerCras::GetPrimaryActiveOutputNode() {
  DCHECK(GetTaskRunner()->BelongsToCurrentThread());
  return 0;
}

bool AudioManagerCras::IsDefault(const std::string& device_id, bool is_input) {
  return true;
}

}  // namespace media
