#include <bits/stdc++.h>
#include <mutex>
#include <condition_variable>

#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
}

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000

namespace ig  = ImGui;
namespace igs = ImGui::SFML;

std::string shell(const char* cmd) {
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

// Static declarations
static int QUIT = 0;
static sf::RenderWindow window(
    sf::VideoMode(1600, 900), 
    "Video Editor", 
    sf::Style::Titlebar | sf::Style::Close
);


struct Interval {
    std::string caption;
    double begin;
};

struct PacketQueue {
    AVPacketList* first;
    AVPacketList* last;
    int nb_packets;
    int size;
    std::mutex mutex;
    std::condition_variable cond;

    PacketQueue() {
        nb_packets = 0;
        size = 0;
    }

    int push(AVPacket* packet) {
        // fprintf(stderr, "PacketQueue push() begin\n");
        AVPacketList *new_packet;
        if (av_dup_packet(packet) < 0) {
            return -1;
        }
        // fprintf(stderr, "PacketQueue push() av_dup_packet >= 0\n");

        new_packet = (AVPacketList*)av_malloc(sizeof(AVPacketList));
        if (new_packet == nullptr) {
            return -1;
        }
        // fprintf(stderr, "PacketQueue push() av_malloc successful\n");

        new_packet->pkt  = *packet;
        new_packet->next = nullptr;

        // Data here is important, so we use a mutex to block everyone, and a
        // lock to tell everyone when it's okay to use again
        {
            std::unique_lock<std::mutex> lock(mutex);
            // fprintf(stderr, "PacketQueue push() unique_lock created with mutex %p\n", (void*)&mutex);

            // mutex.lock();
            // fprintf(stderr, "PacketQueue push() mutex %p locked\n", (void*)&mutex);

            if (last == nullptr) {
                first = new_packet;
            } else {
                last->next = new_packet;
            }
            last = new_packet;
            ++nb_packets;
            size += new_packet->pkt.size;

            // mutex.unlock();
            // fprintf(stderr, "PacketQueue push() mutex %p unlocked\n", (void*)&mutex);
            // fprintf(stderr, "PacketQueue push() unlocking other threads\n");
            cond.notify_all();
        }

        // if (mutex.try_lock()) {
        //     fprintf(stderr, "PacketQueue push() mutex %p not in use wayy after\n", (void*)&mutex);
        //     mutex.unlock();
        // } else {
        //     fprintf(stderr, "PacketQueue push() mutex %p already in use wayy after!\n", (void*)&mutex);
        // }
        
        return 0;
    }

    int pop(AVPacket* packet, int block) {
        fprintf(stderr, "PacketQueue pop() begin, packets: %d\n", nb_packets);

        AVPacketList* return_packet;
        int return_code = -1;

            if (mutex.try_lock()) {
            fprintf(stderr, "PacketQueue push() mutex %p not in use wayy after\n", (void*)&mutex);
            mutex.unlock();
        } else {
            fprintf(stderr, "PacketQueue push() mutex %p already in use wayy after!\n", (void*)&mutex);
        }
        

        mutex.lock();

        // fprintf(stderr, "PacketQueue pop() mutex %p locked\n", (void*)&mutex);

        while (!QUIT) {
            return_packet = first;

            if (return_packet) {
                // fprintf(stderr, "PacketQueue pop() first=non-null\n");

                first = return_packet->next;
                if (first = nullptr) last = nullptr;
                nb_packets--;
                size -= return_packet->pkt.size;
                *packet = return_packet->pkt;
                av_free(return_packet);
                return_code = 1;
                break;
            } else if (!block) {
                // fprintf(stderr, "PacketQueue pop() block = 0\n");

                return_code = 0;
                break;
            } else {
                // fprintf(stderr, "PacketQueue pop() locking thread\n");
                mutex.unlock();
                // fprintf(stderr, "PacketQueue pop() mutex %p unlocked\n", (void*)&mutex);
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    // fprintf(stderr, "PacketQueue pop() thread locked, waiting...\n");
                    cond.wait(lock);

                    // fprintf(stderr, "PacketQueue pop() thread unlocked\n");
                }
                mutex.lock();
                // fprintf(stderr, "PacketQueue pop() mutex %p locked\n", (void*)&mutex);
            }
        }

        mutex.unlock();
        // fprintf(stderr, "PacketQueue pop() mutex %p unlocked\n", (void*)&mutex);
        // fprintf(stderr, "PacketQueue pop() return code %d\n", return_code);
        return return_code;
    }
};
static PacketQueue audio_packet_queue;

static class FFAudioStream : public sf::SoundStream {
public:
    AVCodecContext* audio_codec_ctx;

    void init(
        unsigned int channel_count, 
        unsigned int sample_rate,
        AVCodecContext* audio_codec_ctx
    ) {

        std::cerr << "audio_codec_ctx->request_sample_fmt: " << av_get_sample_fmt_name(audio_codec_ctx->request_sample_fmt) << std::endl;
        initialize(channel_count, sample_rate);
        this->audio_codec_ctx = audio_codec_ctx;
    }
private:
    // struct Chunk {
    //   const sf::Int16* samples;
    //   std::size_t      sampleCount;
    // };
    bool onGetData(Chunk& data) override {
        fprintf(stderr, "onGetData() begin\n");

        AVPacket* packet;
        AVFrame*  audio_frame;
        std::vector<uint8_t> audio_buffer;
        std::vector<uint16_t> packet_data;
        int data_size = 0;

        audio_frame = av_frame_alloc();
        packet = av_packet_alloc();

        while (true) {
            while (packet_data.size() > 0) {
                fprintf(stderr, "packet data: \n");
                for (int i = 0; i < packet_data.size(); ++i) {
                    fprintf(stderr, "  %d", packet_data[i]);
                    if ((i%8) == 7) fprintf(stderr, "\n");
                }
                
                fprintf(stderr, "onGetData() packet_data.size() > 0\n");
                int got_frame;
                fprintf(stderr, "onGetData() before avcodec_decode_audio4\n");
                if (avcodec_decode_audio4(audio_codec_ctx, audio_frame, &got_frame, packet) < 0) {
                    fprintf(stderr, "onGetData() error, clearing\n");
                    packet_data.clear();
                    break;
                } else {
                    fprintf(stderr, "onGetData() success, continuing\n");
                }

                if (got_frame) {
                    fprintf(stderr, "onGetData() got_frame = 1\n");
                    data_size = av_samples_get_buffer_size(
                        nullptr,
                        audio_codec_ctx->channels,
                        audio_frame->nb_samples,
                        audio_codec_ctx->sample_fmt,
                        1
                    );

                    fprintf(stderr, "onGetData() data_size: %d", data_size);

                    for (int i = 0; i < data_size; ++i) {
                        audio_buffer.push_back(audio_frame->data[0][i]);
                    }
                }

                if (data_size <= 0) {
                    fprintf(stderr, "onGetData() data_size <= 0\n");
                    continue;
                }

                data.sampleCount = audio_buffer.size();
                data.samples = (const sf::Int16*)audio_buffer.data();

                return true;
            }

            fprintf(stderr, "onGetData() before pop\n");
            if (audio_packet_queue.pop(packet, 1) < 0) {
                fprintf(stderr, "onGetData() pop false!");
                // data.sampleCount = 1;
                // data.samples = { 0 };
                return false;
            } 

            fprintf(stderr, "onGetData() after pop\n");
            for (int i = 0; i < packet->size; ++i) {
                packet_data.push_back(packet->data[i]);
            }
            fprintf(stderr, "onGetData() appended to packet_data\n");
            
        }

        return true;
    }

    void onSeek(sf::Time timeOffset) override {
        fprintf(stderr, "onSeek()\n");
        return;
    }
} ff_audio_stream;

static struct VideoPlayer {
    // VideoPlayer variables
    bool loaded = false;
    std::string name;

    // Libav variables
    // Libav loading stuff
    AVFormatContext* video_format_ctx;
    int video_stream_idx;
    int audio_stream_idx;
    int subtitle_stream_idx;
    // Redundant

    int i_frame_size;
    AVCodecContext* video_codec_ctx;
    AVCodecContext* audio_codec_ctx;
    AVCodec* video_codec;
    AVCodec* audio_codec;
    SwsContext* sws_ctx;

    // Video data
    sf::Texture image;
    sf::Sprite sprite;
    uint8_t* data;
    AVFrame* video_frame;
    AVFrame* video_frame_RGB;
    uint8_t* buffer;
    AVPacket packet;

    int load(const char* filename) {
        if (loaded) close();

        std::cout << "[EDITOR] Loading file " << filename << std::endl;

        av_register_all();

        video_format_ctx = avformat_alloc_context();

        if (avformat_open_input(&video_format_ctx, filename, nullptr, nullptr) != 0) {
            std::cerr << "Could not open file " << filename << "!" << std::endl;
            return -1;
        }

        if (avformat_find_stream_info(video_format_ctx, nullptr) < 0) {
            std::cerr << "Could not find stream info!" << std::endl;
            return -1;
        }

        // av_dump_format(video_format_ctx, 0, filename, 0);

        // video_format_ctx->streams contains streams

        video_stream_idx    = -1;
        audio_stream_idx    = -1;
        subtitle_stream_idx = -1;
        for (int i = 0; i < video_format_ctx->nb_streams; ++i) {
            if (video_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
                && video_stream_idx < 0) {
                video_stream_idx = i;
            }
            if (video_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO
                && audio_stream_idx < 0) {
                    audio_stream_idx = i;
            }
            if (video_format_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE
                && subtitle_stream_idx < 0) {
                    subtitle_stream_idx = i;
            }
        }

        if (video_stream_idx == -1) {
            std::cerr << "Video stream not found!" << std::endl;
            return -1;
        }
        if (audio_stream_idx == -1) {
            std::cerr << "Audio stream not found!" << std::endl;
            return -1;
        }
        
        video_codec_ctx = video_format_ctx->streams[video_stream_idx]->codec;
        audio_codec_ctx = video_format_ctx->streams[audio_stream_idx]->codec;
        audio_codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_S16;
        video_codec = avcodec_find_decoder(video_codec_ctx->codec_id);
        audio_codec = avcodec_find_decoder(audio_codec_ctx->codec_id);

        if (video_codec == nullptr) {
            std::cerr << "Unsupported video codec!" << std::endl;
            return -1;
        }
        if (audio_codec == nullptr) {
            std::cerr << "Unsupported audio codec!" << std::endl;
        }

        if (avcodec_open2(video_codec_ctx, video_codec, nullptr) < 0) {
            std::cerr << "Could not open video codec!" << std::endl;
            return -1;
        }
        if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0) {
            std::cerr << "Could not open audio codec!" << std::endl;
            return -1;
        }

        i_frame_size = video_codec_ctx->width*  video_codec_ctx->height*  3;

        video_frame = av_frame_alloc();
        video_frame_RGB = av_frame_alloc();
        if (video_frame_RGB == nullptr) {
            std::cerr << "Could not allocate frame!" << std::endl;
            return -1;
        }

        int numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, video_codec_ctx->width, video_codec_ctx->height);
        buffer = (uint8_t*)av_malloc(numBytes*sizeof(uint8_t));

        avpicture_fill((AVPicture*)video_frame_RGB, buffer, AV_PIX_FMT_RGB24, video_codec_ctx->width, video_codec_ctx->height);

        data = new sf::Uint8[video_codec_ctx->width*  video_codec_ctx->height*  4];

        sws_ctx = sws_getContext(
            video_codec_ctx->width, video_codec_ctx->height,
            video_codec_ctx->pix_fmt,
            video_codec_ctx->width, video_codec_ctx->height,
            AV_PIX_FMT_RGB24, SWS_BILINEAR,
            nullptr, nullptr, nullptr
        );

        image.create(video_codec_ctx->width, video_codec_ctx->height);
        sprite.setTexture(image);

        std::cout << "[EDITOR] Successfully loaded up video" << std::endl;
        std::cout << "[EDITOR] Video stream index: " << video_stream_idx << std::endl;
        std::cout << "[EDITOR] width: " << video_codec_ctx->width << " height: " << video_codec_ctx->height << std::endl;

        loaded = true;
        name = filename;

        return 0;
    }

    // int play() {
    //     // std::cerr << "audio stream being playing" << std::endl;
    //     return 0;
    // }

    int show() {
        if (!loaded) return 0;

        int frameFinished;

        if (av_read_frame(video_format_ctx, &packet) < 0) {
            // Put in different behavior here
            return close();
        }

        // std::cout << "[EDITOR] Video not over" << std::endl;
        // std::cout << "STREAM INDEX: " << packet.stream_index << std::endl;

        if (packet.stream_index == video_stream_idx) {
            avcodec_decode_video2(video_codec_ctx, video_frame, &frameFinished, &packet);

            // std::cout << (frameFinished ? "Framefinsihed" : "uh-oh") << std::endl; 

            if (frameFinished) {
                // std::cout << video_frame->display_picture_number << std::endl;

                // std::cout << "sws_scale" << std::endl;
                sws_scale(
                    sws_ctx, 
                    (uint8_t const* const*)video_frame->data,
                    video_frame->linesize, 0, video_codec_ctx->height,
                    video_frame_RGB->data, video_frame_RGB->linesize
                );

                for (int i = 0, j = 0; i < i_frame_size; i += 3, j += 4) {
                    data[j]   = video_frame_RGB->data[0][i];
                    data[j+1] = video_frame_RGB->data[0][i+1];
                    data[j+2] = video_frame_RGB->data[0][i+2];
                    data[j+3] = 255;
                }

                // std::cout << "update" << std::endl;
                        
                image.update(data);

                // std::cout << "[EDITOR] Image updated" << std::endl;
            }
        } else if (packet.stream_index == audio_stream_idx) {
            // fprintf(stderr, "VideoPlayer show() stream = audio, pushing...\n");
            audio_packet_queue.push(&packet);
        } else {
            // av_free_packet(&packet);
        }
        
        if (ff_audio_stream.getStatus() != sf::SoundSource::Playing) {
            ff_audio_stream.init(
                audio_codec_ctx->channels, 
                audio_codec_ctx->sample_rate,
                audio_codec_ctx
            );

            std::cerr << "audio stream initialized" << std::endl;
            ff_audio_stream.play();
        }

        window.draw(sprite);

        return 0;
    }

    int close() {
        std::cout << "[EDITOR] Closing file " << name << std::endl;

        av_free_packet(&packet);
        av_free(buffer);
        av_free(video_frame_RGB);
        av_free(video_frame);
        avcodec_close(video_codec_ctx);
        avformat_close_input(&video_format_ctx);

        loaded = false;
        name = "";

        return 0;
    }
} videoplayer;

static struct IGHandler {
    ImGuiIO* io;
    std::map<std::string, int> popups;

    IGHandler() {
        igs::Init(window);
        io = &ImGui::GetIO();

        io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }

    int update(sf::Time dt) {
        igs::Update(window, dt);

        ig::ShowDemoWindow();

        if (ig::BeginMainMenuBar()) { 
            // std::string s = std::to_string(video.video_frame->display_picture_number);
            // ig::Text(s.c_str());

            if (ig::BeginMenu("File")) {   
                if (ig::MenuItem("Save", "CTRL+S")) {
                    
                }
                if (ig::MenuItem("Load Video", "CTRL+F")) {
                    popups["Load Video"] = true;
                }

                ig::EndMenu();
            }

            if (ig::BeginMenu("Edit")) {
                ig::EndMenu();
            }

            if (ig::BeginMenu("View")) {
                ig::EndMenu();
            }

            ig::EndMainMenuBar();
        }

        for (auto iter = popups.begin(); iter != popups.end(); ++iter) {
            if (iter->second) ig::OpenPopup(iter->first.c_str());
        }

        if (ig::BeginPopupModal("Load Video")) {
            ig::Text("Select video file to open");
            if (ImGui::Button("OK", ImVec2(120, 0))) { 
                popups["Load Video"] = false;
                ImGui::CloseCurrentPopup(); 
            }
            ig::EndPopup();
        }

        return 0;
    }

    int show() {
        igs::Render(window);

        return 0;
    }

    int close() {
        igs::Shutdown();

        return 0;
    }
} igh;

int main()
{
    std::cout << "[EDITOR] Starting up..." << std::endl;

    window.setTitle("Video Editor");
    // window.setFramerateLimit(60);
    window.resetGLStates();

    videoplayer.load("data/episode_720.mkv");
    // videoplayer.play();
    // videoplayer.load("/mnt/c/Users/sussm/Videos/The Entire Bee Movie Sorted Alphabetically.mkv");

    sf::Clock clock;

    // std::string test_text = shell("tree");

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            igs::ProcessEvent(event);

            switch(event.type) {
                case sf::Event::Closed:
                    QUIT = 1;
                    window.close();
                    break;
                case sf::Event::KeyPressed:
                    break;
                // case sf::Event::KeyReleased:
                //     temp = false;
                //     break;
            }
        }
        
        sf::Time dt = clock.restart();

        igh.update(dt);

        int i = dt.asMilliseconds();
        std::string s = std::to_string(i);
        ig::Text(s.c_str());
        // ig::Text(test_text.c_str());

        window.clear(sf::Color::Black);

        videoplayer.show();
        igh.show();

        window.display();
    }

    videoplayer.close();
    igh.close();

    return 0;
}