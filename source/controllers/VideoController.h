#pragma once
#include "core/enums.h"
#include <nds.h>
#include <string>

#define FRAMES_TO_BUFFER 15
#define READS_PER_UPDATE 3

class VideoController
{
  public:
    VideoController() {};
    void init(std::string iFileName, float iFps, ViewState iNextState);
    ViewState update();
    void cleanup();

  private:
    ViewState nextState;
    float fps;

    FILE* videoFile;
    bool fileEOF;
    int currentFrame;
    int bg;

    u8* ramBuffer;
    int readIndex;
    int writeIndex;
    int framesAvailable;

    // dynamic video variables
    u16 frameW;
    u16 frameH;
    u8 bpp;
    u32 frameSize;
    u32 bufferSize;

    u8 audioBuf[16384];

    void refillBuffer();
};
