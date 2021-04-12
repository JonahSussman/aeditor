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

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

using namespace eff;

Worker::Worker() {
  printf("Constructor!\n");
  running = false;

  // av_register_all();

  // video_codec = nullptr;
  // audio_codec = nullptr;
  // video_codec_params = nullptr;
  // audio_codec_params = nullptr;

  video_stream_idx = -1;
  audio_stream_idx = -1;

  _packet = av_packet_alloc();
  // av_init_packet(packet);
}

Worker::~Worker() {

}

bool Worker::load(const char* filename) {
  av_register_all();

  format_ctx = avformat_alloc_context();
  if (format_ctx == nullptr) {
    fprintf(stderr, "[EFF] Error allocating format context\n");
  }

  if (int r = avformat_open_input(&format_ctx, filename, nullptr, nullptr); r != 0) {
    printf("[EFF] Could not open file. Code: %d\n", r);
    return false;
  }

  if (int r = avformat_find_stream_info(format_ctx, nullptr); r != 0) {
    printf("[EFF] Could not get stream info. Code: %d\n", r);
    return false;
  }

  av_dump_format(format_ctx, 0, filename, 0);

  /*for (int i = 0; i < format_ctx->nb_streams; ++i) {
    AVCodecContext* local_codec_ctx = format_ctx->streams[i]->codec;
    AVCodec* local_codec = avcodec_find_decoder(local_codec_ctx->codec_id);

    if (local_codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (video_stream_idx == -1) {
        video_stream_idx = i;
        video_codec_ctx = local_codec_ctx;
        video_codec = local_codec;

        if (video_codec == nullptr) {
          printf("[EFF] Unsupported video codec\n");
          return false;
        }
      }
      printf("Video Codec: resolution %d x %d\n", local_codec_ctx->width, local_codec_ctx->height);

    } else if (local_codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (audio_stream_idx == -1) {
        audio_stream_idx = i;
        audio_codec_ctx = local_codec_ctx;
        audio_codec = local_codec;

        if (audio_codec == nullptr) {
          printf("[EFF] Unsupported audio codec\n");
          return false;
        }
      }
      printf("Audio Codec: %d channels, sample rate %d\n", local_codec_ctx->channels, local_codec_ctx->sample_rate);
    }

    printf("\tCodec %s ID %d bit_rate %ld\n", local_codec->name, local_codec->id, local_codec_ctx->bit_rate);
  }*/

  for (int i = 0; i < format_ctx->nb_streams; ++i) {
    AVCodecParameters* local_codec_params = format_ctx->streams[i]->codecpar;
    AVCodec* local_codec = avcodec_find_decoder(local_codec_params->codec_id);

    if (local_codec == nullptr) {
      printf("[EFF] Unsupported codec!\n");
      return false;
    }
    if (local_codec_params == nullptr) {
      printf("[EFF] Could not get codec params\n");
      return false;
    }

    if (local_codec_params->codec_type == AVMEDIA_TYPE_VIDEO) {
      if (video_stream_idx == -1) {
        printf("> ");
        video_stream_idx = i;
        video_codec = local_codec;
        video_codec_params = local_codec_params;
      }

      printf("Video Codec: resolution %d x %d\n", local_codec_params->width, local_codec_params->height);
    } else if (local_codec_params->codec_type == AVMEDIA_TYPE_AUDIO) {
      if (audio_stream_idx == -1) {
        printf("> ");
        audio_stream_idx = i;
        audio_codec = local_codec;
        audio_codec_params = local_codec_params;
      }

      printf("Audio Codec: %d channels, sample rate %d\n", local_codec_params->channels, local_codec_params->sample_rate);
    } else {
      printf("Other Codec\n");
    }

    printf("\tCodec %s ID %d bit_rate %ld\n", local_codec->name, local_codec->id, local_codec_params->bit_rate);
  }

  printf("[EFF] Allocating contexts\n");

  video_codec_ctx = avcodec_alloc_context3(video_codec);
  audio_codec_ctx = avcodec_alloc_context3(audio_codec);

  if (video_codec_ctx == nullptr || audio_codec_ctx == nullptr) {
    printf("[EFF] Failed to allocate video and/or audio context\n");
    return false;
  }

  printf("[EFF] Parameters to context\n");
  if (int r = avcodec_parameters_to_context(video_codec_ctx, video_codec_params); r < 0) {
    printf("[EFF] Failed  to copy video params to context: %d\n", r);
    return false;
  }
  if (avcodec_parameters_to_context(audio_codec_ctx, audio_codec_params) < 0) {
    printf("[EFF] Failed to copy audio params to context\n");
    return false;
  }

  if (video_codec == nullptr || audio_codec == nullptr) {
    printf("[EFF] Unsupported codec\n");
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

bool Worker::decode() {
  bool waiting_msg = false;
  while (running) {
    if (
      video_frame_queue.size() >= MAX_VIDEO_QUEUE_SIZE ||
      audio_frame_queue.size() >= MAX_AUDIO_QUEUE_SIZE
    ) {
      if (!waiting_msg) fprintf(stderr, "[EFF] Queue filled, waiting...\n");
      waiting_msg = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    waiting_msg = false;

    // AVPacket* _packet = av_packet_alloc();

    if (int r = av_read_frame(format_ctx, _packet); r < 0) {
      printf("[EFF] Error reading frame. Code: %d\n", r);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }
    // printf("[EFF] packet->data %p\n", _packet->data);
    // printf("[EFF] packet->pts %ld\n", _packet->pts);

    if (_packet->stream_index == video_stream_idx) {
      int frame_finish;

      AVFrame* _frame = av_frame_alloc();

      printf("[EFF] Sending packet...\n");
      if ( int r = avcodec_send_packet(video_codec_ctx, _packet); r < 0 ) {
        printf("\033[1;31m[EFF] Failed: %d\033[0m\n", r);
        exit(1);
      }
      printf("[EFF] Receiving frame...\n");
      int r = avcodec_receive_frame(video_codec_ctx, _frame);
      if (r == AVERROR(EAGAIN) || r == AVERROR_EOF) {
        printf("[EFF] EAGIN or EOF. Skipping...\n");
        continue;
      } if (r < 0 ) {
        printf("\033[1;31m[EFF] Failed: %d\033[0m\n", r);
        exit(1);
      }

      video_mutex.lock();
      video_frame_queue.push(_frame);
      video_mutex.unlock();
      video_cv.notify_all();
    } else {
      printf("other frame.\n");
    }
  }
}

AVFrame* Worker::get_video_frame() {
  std::unique_lock<std::mutex> lock(video_mutex);
  video_cv.wait(lock, [&]{ return !video_frame_queue.empty(); });
  AVFrame* frame = video_frame_queue.front();
  video_frame_queue.pop();
  return frame;
}

bool Worker::start() {
  running = true;
  decode_thread = std::thread(&Worker::decode, this);

  return true;
}

bool Worker::join() {
  running = false;
  decode_thread.join();

  return true;
}