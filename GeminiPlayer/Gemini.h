#pragma once

#include "SDL.h"
#include "SDL_thread.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class Gemini
{
public:
    Gemini(): mFormatCtx(NULL), mCodecCtx(NULL), 
        mWindow(NULL), mRenderer(NULL),mTexture(NULL){};

    bool initFFmpeg(char *file);
    bool openCodecContext(int *index);
    bool initSDL();
    bool convertPixel(int index, int num);
    void handleEvents();
    void clean();

private:
    
    bool allocImage(AVFrame *dstFrame);
    void render();
    AVFormatContext     *mFormatCtx;
    AVCodecContext      *mCodecCtx;

    SDL_Window      *mWindow;
    SDL_Renderer    *mRenderer;
    SDL_Texture     *mTexture;
    SDL_Rect        mRectangle;
    bool mRunning;
    bool mPause;
};

