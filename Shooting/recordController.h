// recordController.h

#pragma once

enum class RecordStep {
    InitMenu,
    WaitBeforeR,
    PressR,
    WaitReplayEnd,
    PressQ,
    Done
};

class RecordController {
public:
    RecordController();

    // 毎フレーム呼ぶ。必要に応じて key[] を書き換える。
    void Update(int key[256]);

    bool IsDone() const { return m_step == RecordStep::Done; }

private:
    RecordStep m_step;
    int m_waitTimer;
    int m_replayCount;
    bool m_replayEnded;

    void changeStep(RecordStep next, int waitFrames = 0);
};


// 録画モード用フラグ
extern bool recordingMode;
extern int  replayLoopCount;