#include "Gemini.h"

bool Gemini::initFFmpeg(char * file)
{
    av_register_all();

    if ((avformat_open_input(&mFormatCtx, file, 0, 0)) < 0) {
        printf("Failed to open input file");
        return true;
    }

    if ((avformat_find_stream_info(mFormatCtx, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        return true;
    }

    av_dump_format(mFormatCtx, 0, file, 0);

    return false;
}

bool Gemini::initSDL()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Failed to initialize SDL - %s\n", SDL_GetError());
        return true;
    }

    mWindow = SDL_CreateWindow("Gemini Player v1.0", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        mCodecCtx->width, mCodecCtx->height, 0);
    if (!mWindow) {
        printf("Failed to create windows - %s\n", SDL_GetError());
        return true;
    }

    mRenderer = SDL_CreateRenderer(mWindow, -1, 0);
    if (!mRenderer) {
        printf("Failed to create renderer - %s\n", SDL_GetError());
        return true;
    }

    mTexture = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_YV12,
        SDL_TEXTUREACCESS_STREAMING, mCodecCtx->width, mCodecCtx->height);
    if (!mTexture) {
        printf("Failed to create texture - %s\n", SDL_GetError());
        return true;
    }

    mRectangle.x = 0;
    mRectangle.y = 0;
    mRectangle.w = mCodecCtx->width;
    mRectangle.h = mCodecCtx->height;

    mRunning = true;
    mPause = false;
    return false;
}

bool Gemini::openCodecContext(int *index)
{
    // Find video stream in the file
    int streamIndex = av_find_best_stream(mFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (streamIndex < 0) {
        printf("Failed to find stream in input file\n");
        return true;
    }

    // Find a decoder with a matching codec ID
    AVCodec *dec = avcodec_find_decoder(mFormatCtx->streams[streamIndex]->codecpar->codec_id);
    if (!dec) {
        printf("Failed to find codec!\n");
        return true;
    }

    // Allocate a codec context for the decoder
    mCodecCtx = avcodec_alloc_context3(dec);
    if (!mCodecCtx) {
        printf("Failed to allocate the codec context\n");
    }

    if (avcodec_parameters_to_context(mCodecCtx, mFormatCtx->streams[streamIndex]->codecpar) < 0) {
        printf("Failed to copy codec parameters to decoder context!\n");
        return true;
    }

    // Initialize the AVCodecContext to use the given Codec
    if (avcodec_open2(mCodecCtx, dec, NULL) < 0) {
        printf("Failed to open codec\n");
        return true;
    }

    *index = streamIndex;

    return false;
}

bool Gemini::convertPixel(int index, int num)
{
    int time = 1000 * mFormatCtx->streams[index]->avg_frame_rate.den 
        / mFormatCtx->streams[index]->avg_frame_rate.num;

    AVPacket packet;
    AVFrame *srcFrame = av_frame_alloc();
    AVFrame *dstFrame = av_frame_alloc();;
    allocImage(dstFrame);

    // Initialize an SwsContext for software scaling
    struct SwsContext *sws_ctx = sws_getContext(
        mCodecCtx->width, mCodecCtx->height, mCodecCtx->pix_fmt,
        mCodecCtx->width, mCodecCtx->height, AV_PIX_FMT_YUV420P,
        SWS_BILINEAR, NULL, NULL, NULL);

    while (av_read_frame(mFormatCtx, &packet) >= 0 && mRunning ) {
        
        handleEvents();
        while (mPause) {
            handleEvents();
            SDL_Delay(time);
        }

        // Is this a packet from the video stream?
        if (packet.stream_index == index) {

            // Decode the video frame
            avcodec_send_packet(mCodecCtx, &packet);
            int ret = avcodec_receive_frame(mCodecCtx, srcFrame);
            if (ret) continue;

            // Convert the image from its native format to RGB
            sws_scale(sws_ctx, (uint8_t const * const *)srcFrame->data,
                srcFrame->linesize, 0, mCodecCtx->height,
                dstFrame->data, dstFrame->linesize);

            SDL_UpdateYUVTexture(mTexture, &mRectangle,
                dstFrame->data[0], dstFrame->linesize[0],
                dstFrame->data[1], dstFrame->linesize[1],
                dstFrame->data[2], dstFrame->linesize[2]
            );

            render();
            SDL_Delay(time);

            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
        }
    }
    av_freep(&dstFrame->data[0]);
    av_frame_free(&srcFrame);
    av_frame_free(&dstFrame);

    return false;
}

void Gemini::handleEvents()
{
    SDL_Event event;
    if (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            mRunning = false;
            break;
        case SDL_MOUSEBUTTONUP:
            mPause = !mPause;
            break;
        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_SPACE)
                mPause = !mPause;
            break;
        default:
            break;
        }
    }
}

bool Gemini::allocImage(AVFrame * image)
{
    image->format = AV_PIX_FMT_YUV420P;
    image->width = mCodecCtx->width;
    image->height = mCodecCtx->height;

    // Allocate an image, and fill pointers and linesizes accordingly.
    av_image_alloc(image->data, image->linesize,
        image->width, image->height, (AVPixelFormat)image->format, 32);

    return false;
}

void Gemini::render()
{
    SDL_RenderClear(mRenderer);

    SDL_RenderCopy(mRenderer, mTexture, NULL, &mRectangle);

    // Set apative size image
    // SDL_RenderCopy(mRenderer, mTexture, NULL, NULL);

    SDL_RenderPresent(mRenderer);
}

void Gemini::clean()
{
    avcodec_close(mCodecCtx);
    avformat_close_input(&mFormatCtx);

    SDL_DestroyWindow(mWindow);
    SDL_DestroyRenderer(mRenderer);
    SDL_Quit();
}