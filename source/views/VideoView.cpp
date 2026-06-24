#include "VideoView.h"
#include "core/globals.h"
#include <nds.h>
#include <stdio.h>

void VideoView::init()
{
    videoCtrl.init(filename, 15.0f, nextView);
    setBrightness(2, -16);
}

ViewState VideoView::update()
{
    scanKeys();
    int keys = keysDown();

    if (keys)
    {
        musicCtrl.pause();
        for (int i = 0; i <= 16; i++)
        {
            setBrightness(3, -i);
            musicCtrl.update();
            swiWaitForVBlank();
        }

        return nextView;
    }

    return videoCtrl.update();
}

void VideoView::cleanup()
{
    // handles videoCtrl.cleanup()
    BaseView::cleanup();
}
