#include "SDL.h"
#include "SDL_thread.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavutil/file.h"
};

#include "Gemini.h"

int main(int argc, char *argv[])
{
    char *fileName = "assets/Sample.mkv";
    Gemini *gemini = new Gemini();

    int streamIndex = -1;

    // Get AVFormatContext, and print detailed info about file 
    gemini->initFFmpeg(fileName);

    // Initialize the AVCodecContext
    gemini->openCodecContext(&streamIndex);


    gemini->initSDL();

    // Convert 
    gemini->convertPixel(streamIndex, 6);
    gemini->clean();
    return 0;
}