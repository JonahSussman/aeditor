/*
OVERARCHING IDEA:
  10 OPEN video_stream FROM video.mkv
  20 READ packet FROM video_stream INTO frame
  30 IF frame NOT COMPLETE GOTO 20
  40 DO _something_ WITH frame
  50 GOTO 20
*/

using namespace std;

#include <bits/stdc++.h>

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
}

int main(int argc, char *argv[]) {
  string filename = argv[1];

  av_register_all();

  // OPENING THE FILE
  // Holds a TON of useful information about the video file
  AVFormatContext *pFormatCtx = nullptr;

  // Open video file
  if (avformat_open_input(&pFormatCtx, filename.c_str(), nullptr, 0) != 0) {
    cerr << "ERROR: Could not open " << filename  << "!" << endl;
    return -1;
  }

  // Retrieve stream info
  if (avformat_find_stream_info(pFormatCtx, nullptr) < 0) {
    cerr << "ERROR: Could not obtain stream info for " << filename << "!" << endl;
    return -1;
  }

  // Useful debugging tool, dumps all info about file
  av_dump_format(pFormatCtx, 0, filename.c_str(), 0);

  // Finding the video stream
  AVCodecContext *pCodecCtxOrig = nullptr;
  AVCodecContext *pCodecCtx = nullptr;

  int videoStream = -1;

  for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
      videoStream = i;
      break;
    }
  }

  if (videoStream == -1) {
    cerr << "ERROR: Couldn't find video stream!" << endl;
    return -1;
  }
  pCodecCtx = pFormatCtx->streams[videoStream]->codec;

  // Codec context now found. We now have to find actual codec
  AVCodec* pCodec = nullptr;

  // Find decoder for video stream
  pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
  if (pCodec == nullptr) {
    cerr << "ERROR: Unsupported codec!" << endl;
    return -1;
  }

  // Copy context
  pCodecCtx = avcodec_alloc_context3(pCodec);
  if(avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0) {
    cerr << "ERROR: Couldn't copy codec context!" << endl;
    return -1;
  }

  // Open codec
  if (avcodec_open2(pCodecCtx, pCodec, nullptr) < 0) {
    cerr << "ERROR: Couldn't open codec!" << endl;
    return -1;
  }

  // STORING THE DATA
  AVFrame *pFrame = nullptr;
  pFrame = av_frame_alloc();

  AVFrame *pFrameRGB = nullptr;
  pFrameRGB = av_frame_alloc();
  if (pFrameRGB == nullptr || pFrame == nullptr) {
    cerr << "ERROR: Couldn't allocate frame memory" << endl;
    return -1;
  }

  int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
  uint8_t *buffer = (uint8_t*)av_malloc(numBytes * sizeof(uint8_t));

  // Assign appropriate parts of buffer to image planes in pFrameRGB
  // Note: pFrameRGB is an AVFrame, but AVFrame is superset of AVPicture
  avpicture_fill((AVPicture*)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

  // READING THE DATA
  struct SwsContext *sws_ctx = nullptr;

  return 0;
}