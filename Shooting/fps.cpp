#include "DxLib.h"
#include "fps.h"

int frameDurationMs;

void fpsTimeFunction()
{
    static int fpsTime[2] = { 0, }, fpsTime_i = 0;
    static double fps = 0.0;

    if (fpsTime_i == 0)
        fpsTime[0] = GetNowCount();
    if (fpsTime_i == 49) {
        fpsTime[1] = GetNowCount();
        fps = 1000.0f / ((fpsTime[1] - fpsTime[0]) / 50.0f);
        fpsTime_i = 0;
    }
    else
        fpsTime_i++;
    if (fps != 0)
        DrawFormatString(5, 460, GetColor(255, 255, 255), "FPS %.1f", fps);
    return;
}
