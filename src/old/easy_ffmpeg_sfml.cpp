#include "easy_ffmpeg_sfml.hpp"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libswresample/swresample.h>
  #include <libavutil/opt.h>
}

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

using namespace eff;

Video::Video() {
  state = STOPPED;
}

Video::~Video() {
  printf("[EFF] Video deconstructor needs work\n");
  state = STOPPED;
  if (worker.running) worker.join();

  video_update_thread.join();
}

bool Video::load(const char* filename) {
  worker.load("data/episode_720.mkv");

  video_frame     = av_frame_alloc();
  video_frame_RGB = av_frame_alloc();
  width  = worker.video.codec_ctx->width;
  height = worker.video.codec_ctx->height;
  bytes = avpicture_get_size(AV_PIX_FMT_RGB24, width, height);
  data = new uint8_t[width * height * 4];
  buffer = (uint8_t*) av_malloc(bytes * sizeof(uint8_t));

  avpicture_fill(
    (AVPicture*)video_frame_RGB,
    buffer,
    AV_PIX_FMT_RGB24,
    width,
    height
  );

  sws_ctx = sws_getContext(
    width, height,
    worker.video.codec_ctx->pix_fmt,
    width, height,
    AV_PIX_FMT_RGB24, SWS_BILINEAR,
    nullptr, nullptr, nullptr
  );

  texture.create(width, height);
  sprite.setTexture(texture);

  worker.start();

  audio_stream.init(this);
}

bool Video::update_video_data() {
  double last_pts = 0.0;
  while(state == PLAYING) {
    AVFrame* frame = worker.get_video_frame();
    // printf("[EFF] UPDATE_VIDEO_DATA sws_scale\n");
    sws_scale(
      sws_ctx,
      (uint8_t const* const*) frame->data, frame->linesize, 0, height,
      video_frame_RGB->data, video_frame_RGB->linesize
    );
  
    // printf("[EFF] UPDATE_VIDEO_DATA copy data\n");
    texture_mutex.lock();
    for (int i = 0, j = 0; i < width * height * 3; i += 3, j += 4) {
      data[j]   = video_frame_RGB->data[0][i];
      data[j+1] = video_frame_RGB->data[0][i+1];
      data[j+2] = video_frame_RGB->data[0][i+2];
      data[j+3] = 255;
    }
    texture_mutex.unlock();

    double this_pts = ((double)frame->pts * worker.video.time_base.num) / (worker.video.time_base.den);
    double time_to_wait = this_pts - last_pts;
    last_pts = this_pts;
    // I'm going to have to do some math for syncing ....

    av_frame_free(&frame);

    // printf("[EFF] Video waiting %f\n", time_to_wait);
    
    std::this_thread::sleep_for(std::chrono::duration<double>(time_to_wait));
  }
}

void Video::SFMLAudioStream::init(Video* video) {
  this->video = video;
  worker      = &(video->worker);
  audio       = &(worker->audio);
  channels    = audio->codec_ctx->channels;
  sample_rate = audio->codec_ctx->sample_rate;

  initialize(channels, sample_rate);
  printf("[EFF] Audio stream initialized. channels: %d, sample_rate: %d\n", channels, sample_rate);
}

bool Video::SFMLAudioStream::onGetData(Chunk& data) {
  // printf("[EFF] onGetData\n");
  const int bytes = av_get_bytes_per_sample(audio->codec_ctx->sample_fmt);
  // if (bytes != 4) {
  //   printf("[EFF] Bytes per sample: %d. Aborting\n", bytes);
  //   exit(1);
  // }
  // if (audio->codec_ctx->sample_fmt != AV_SAMPLE_FMT_FLTP) {
  //   printf("[EFF] Data format not float32 planar, it is code: %d. Aborting\n");
  //   exit(1);
  // }

  // if (!av_sample_fmt_is_planar(audio->codec_ctx->sample_fmt)) {
  //   printf("[EFF] Data is non planar. Aborting\n");
  //   exit(1);
  // }

  std::vector<int16_t> samples;

  while (samples.size() < 4196) {
    AVFrame* frame = worker->get_audio_frame();
    // printf("[EFF] Audio samples in frame: %d\n", frame->nb_samples);

    // printf("[EFF] Data is planar.\n");
    for (int s = 0; s < frame->nb_samples; ++s) {
      for (int c = 0; c < audio->codec_ctx->channels; ++c) {
        // uint8_t* buffer = frame->extended_data[c];
        int32_t val = reinterpret_cast<int32_t*>(frame->extended_data[c])[s];
        // if (s % 4800 == 0 ) printf("sample value: %d\n", (int16_t)(32767 * *reinterpret_cast<float*>(&val)));
        samples.push_back((int16_t)(32767 * *reinterpret_cast<float*>(&val)));
      }
    }

    av_frame_free(&frame);
  }

  // for (int i = 0; i < samples.size() / 10; i++) {
  //   samples[i] = 5000*((i/256) % 2);
  // } 
  // samples.shrink_to_fit();
  data.samples = samples.data();
  data.sampleCount = samples.size();

  return true;
}

void Video::SFMLAudioStream::onSeek(sf::Time timeOffset) {
  printf("onSeek\n");
}

// re-implement drawable
void Video::show(sf::RenderWindow& window) {
  // printf("[EFF] SHOW lock mutex\n");
  texture_mutex.lock();
  // printf("[EFF] SHOW draw sprite\n");
  texture.update(data);
  window.draw(sprite);
  // printf("[EFF] SHOW unlock mutex\n");
  texture_mutex.unlock();
}

bool Video::play() {
  state = PLAYING;
  video_update_thread = std::thread(&Video::update_video_data, this);
  audio_stream.play();

  int num = worker.video.time_base.num;
  int den = worker.video.time_base.den;

  printf("num/den = %d/%d\n", num, den);

  return true;
}

