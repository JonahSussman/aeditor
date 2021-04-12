/*
  I was completely baffled that there was no simple and easy to implement
  C++ library that harnesses the awesome power of ffmpeg. That's why I created
  this library. Easy FFMPEG or EFF for short. It's designed in such a way that 
  it's platform agnostic. The fundamental concept is you tell it where to
  find a video file and it gets to work spawning decoder threads, which then
  output to a queue.
  
  EFF requires C++17 and the full libav* suite.
*/

#include "easy_ffmpeg.hpp"

#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

using namespace eff;

Worker::Worker() {
  running = false;

  format_ctx = avformat_alloc_context();
  frame  = av_frame_alloc();
  packet = av_packet_alloc();

  video_stream_idx = -1;
  audio_stream_idx = -1;
}

Worker::~Worker() {
  close();
}

std::string Worker::shell(const char* cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}

bool Worker::load(const char* filename) {
  av_register_all();
  avcodec_register_all();

  // fprintf(stderr, "ls result %s", shell("ls data").c_str());

  if (avformat_open_input(&format_ctx, filename, nullptr, nullptr) < 0) {
    // but... like why is it not opening
    fprintf(stderr, "[EFF] Could not open file %s\n", filename);
    return false;
  }

  av_dump_format(format_ctx, 0, filename, 0);

  printf("Format %s, duration %ld us\n", format_ctx->iformat->long_name, format_ctx->duration);

  int res = avformat_find_stream_info(format_ctx, nullptr);
  if (res < 0) {
    char str[256];
    av_strerror(res, str, 256);
    fprintf(stderr, "[EFF] Could not open stream info, code: %s\n", str);
    return false;
  }

  for (int i = 0; i < format_ctx->nb_streams; ++i) {
    AVCodecParameters* local_params = format_ctx->streams[i]->codecpar;
    AVCodec* local_codec = avcodec_find_decoder(local_params->codec_id);

    if (local_codec == nullptr) {
      printf("[EFF] Unsupported codec!");
      return false;
    }

    if (local_params->codec_type == AVMEDIA_TYPE_VIDEO) {
      printf("Video codec: resolution %dx%d\n", local_params->width, local_params->height);

      if (video_stream_idx == -1) {
        video_stream_idx = i;
        video_codec = local_codec;
        video_codec_params = local_params;
      }
    } else if (local_params->codec_type == AVMEDIA_TYPE_AUDIO) {
      printf("Audio codec: %d channels, sample rate %d\n", local_params->channels, local_params->sample_rate);

      if (audio_stream_idx == -1) {
        audio_stream_idx = i;
        audio_codec = local_codec;
        audio_codec_params = local_params;
      }
    }

    printf("\tCodec %s ID %d bitrate %ld\n", local_codec->long_name, local_codec->id, format_ctx->bit_rate);
  }

  video_codec_ctx = avcodec_alloc_context3(video_codec);
  audio_codec_ctx = avcodec_alloc_context3(audio_codec);

  if (video_codec_ctx == nullptr || audio_codec_ctx == nullptr) {
    printf("[EFF] Failed to allocate video and/or audio context\n");
    return false;
  }

  if (avcodec_parameters_to_context(video_codec_ctx, video_codec_params) < 0) {
    printf("[EFF] Failed  to copy video params to context\n");
    return false;
  }
  if (avcodec_parameters_to_context(audio_codec_ctx, audio_codec_params) < 0) {
    printf("[EFF] Failed to copy audio params to context\n");
    return false;
  }

  if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0) {
    printf("[EFF] Failed to open video codec\n");
    return false;
  }    
  if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0) {
    printf("[EFF] Failed to open audio codec\n");
    return false;
  }

  this->filename = filename;

  return true;
}

bool Worker::start() {
  running = true;
  decode_thread = std::thread(&Worker::decode, this);

  return true;
}

bool Worker::join() {
  running = false;
  decode_thread.join();
}

bool Worker::close() {
  printf("Releasing all resources\n");

  avformat_close_input(&format_ctx);
  av_packet_free(&packet);
  av_frame_free(&frame);
  avcodec_free_context(&video_codec_ctx);
  avcodec_free_context(&audio_codec_ctx);

  return true;
}

// Private
bool Worker::decode() {
  bool lastfill = false;
  fprintf(stderr, "DECODE Packet address: %p\n", packet);
  while (running) {
    if (
      video_frame_queue.size() >= MAX_VIDEO_QUEUE_SIZE ||
      audio_frame_queue.size() >= MAX_AUDIO_QUEUE_SIZE
    ) {
      if (!lastfill) fprintf(stderr, "[EFF] Queue filled, waiting...\n");
      lastfill = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    lastfill = false;

    if (av_read_frame(format_ctx, packet) < 0) {
      fprintf(stderr, "[EFF] Error reading frame...\n");
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    fprintf(stderr, "DECODE packet: %p\n", packet);
    if (packet->data == nullptr) fprintf(stderr, "PACKET IS NULL\n");

    if (packet->stream_index == video_stream_idx) {
      // Decode video packet
      avcodec_send_packet(video_codec_ctx, packet);
      avcodec_receive_frame(video_codec_ctx, frame);

      fprintf(stderr, "DECODE video frame: %p\n", frame);

      video_mutex.lock();
      AVFrame* new_frame = new AVFrame;
      *new_frame = *frame;
      video_frame_queue.push(new_frame);
      fprintf(stderr, "DECODE video push(): %p\n", frame);
      video_mutex.unlock();

      video_cv.notify_all();

      // int response = avcodec_send_packet(video_codec_ctx, packet);
      
      // if (response < 0) {
      //   fprintf(stderr, "[EFF] Error sending packet to decoder\n");
      // }

      // while (response >= 0) {
      //   response = avcodec_receive_frame(video_codec_ctx, frame);

      //   if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
      //     break;
      //   } else if (response < 0) {
      //     fprintf(stderr, "[EFF] Error receiving frame from decoder\n");
      //   }

      //   video_mutex.lock();
      //   video_frame_queue.push(frame);
      //   fprintf(stderr, "push(): %p\n", frame);
      //   video_mutex.unlock();

      //   video_cv.notify_all();
      // }

    } 
    else if (packet->stream_index = audio_stream_idx) {
      avcodec_send_packet(video_codec_ctx, packet);
      avcodec_receive_frame(video_codec_ctx, frame);

      fprintf(stderr, "audio frame: %p\n", frame);

      audio_mutex.lock();
      AVFrame* new_frame = new AVFrame;
      *new_frame = *frame;
      audio_frame_queue.push(frame);
      fprintf(stderr, "audio push(): %p\n", new_frame);
      audio_mutex.unlock();

      audio_cv.notify_all();

      // Decode audio packet

    } 
    else {
      fprintf(stderr, "DECODE invalid stream_index %d\n", packet->stream_index);
      return false;
    }

    av_packet_unref(packet);
  }
  return true;
}

// Probably better way of doing this
AVFrame* Worker::get_video_frame() {
  fprintf(stderr, "POP QUEUE\n");
  std::unique_lock<std::mutex> lock(video_mutex);
  // fprintf(stderr, " lock made\n");
  video_cv.wait(lock, [&]{ return !video_frame_queue.empty(); });
  // fprintf(stderr, " mutex locked\n");
  AVFrame* frame = video_frame_queue.front();
  fprintf(stderr, "POP QUEUE size() %lu\n", video_frame_queue.size());
  // fprintf(stderr, " pointer aquired\n");
  video_frame_queue.pop();
  // fprintf(stderr, " queue popped\n");
  return frame;
}

AVFrame* Worker::get_audio_frame() {
  
}