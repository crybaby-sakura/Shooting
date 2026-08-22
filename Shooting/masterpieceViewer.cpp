// masterpieceViewer.cpp

// main.cpp の先頭にインクルード追加
#include "masterpieceViewer.h"
#include "DxLib.h"
#include "gv.h"
#include "initial.h"
#include "fps.h"
#include "gameScreen.h"
#include "stageData.h"
#include "menu.h"
#include "imgSoundLoad.h"
#include "player.h"
#include "playerShot.h"
#include "enemy.h"
#include "enemyShot.h"
#include "fileOpenClose.h"
#include "getHitKeyStateAll2.h"
#include "replay.h"
#include "stateManager.h"   // StateManager, Joutai
#include "recordController.h"
#include <vector>


// 傑作選録画モード
bool masterpieceMode = true;

std::vector<MasterpieceEntry> masterpieceList = {
    { "Gemini47",   "専用素材を全く用意していないにも関わらず既存の弾だけでへにょりレーザーを上手く表現してくれました。単なる正弦波ではなく2つ重ねてへにょり度を上げているのも良い感じ。" },
    { "Grok48",     "大半のAIがクジラの潮吹きを実装してくれる中勘違いしちゃう子が若干名。KimiやZ.aiがそういうのはちょっと……と実装を拒否する中、Grokだけはノリノリで実装してくれました。" },
    { "Zai49",      "ライフゲームの世界に自機を放り込む実にシンプルな作品。変化が長く続くよう初期化時にR-ペントミノを使うという気配りも見せてくれました。" },
    { "Gemini50",   "弾で魚を上手く形作ってくれました。自機ショットとの当たり判定も勝手に実装し、もはやゲームに本来存在しないザコ敵を自力で生み出しちゃったようなものです。凄すぎるって……" },
    { "Kimi51",     "これでもかってくらい大玉を敷き詰めてその隙間でイライラ棒を表現してくれました。疑似超低速移動を使わせる極細地帯と高速移動を入れないと間に合わない回転レーザー地帯が鬼門。" },
    { "Gemini52",   "弾幕シューで\"蜂\"といえば……に期待していたもののまたも空振り。針を避ける位置を蜜で邪魔されないようしっかり誘導して隙間を作る必要があります。" },
    { "Zai53",      "「だいもんじ」ではなく「おおもじ」弾幕を実装してくるAIがいることは予想していましたが、Z.aiがここぞとばかりに自己アピールしてきました。避けてても中々楽しめます。" },
    { "Gemini54",   "この日にチャンネル登録者数が1000人を超えたので、AIたちにビールかけで祝ってもらいました。かけ始める前にシャカシャカシャカ、ポン！の演出が入っているのがお気に入りです。" },
    { "DeepSeek55", "これからは再生数を意識していかないとということで、AIたちにサムネを考えてもらいました。レーザー以外全部飾りという、サムネ撮り終わったら捨てといてええで！感が好きです。" },
    { "Kimi56",     "錯視のせいで大きさを見誤って被弾したら面白いなと思ったものの、周りの弾にも当たり判定はあるので全く意味がありませんでした。個人的にはKimiの弾幕が一番錯視を感じました。" },
    { "Gemini57",   "カオスなパターンを生み出すルール30ですが、セルが少ないとすぐ安定化しちゃいます。Geminiはそれを理解しており、レーザーでランダムに生死を切り替えるギミックが入っていました。" },
    { "DeepSeek58", "動画では伝わりづらいと思いますが自機は常にボス方向への弱い引力を受けています。力の方向に粒子を飛ばして表現するこの実装もDeepSeekがやってくれました。裏方としても優秀です。" },
    { "Gemini59",   "他のAIたちのリズム感が壊滅している中完璧な三三七拍子で弾を撃ってきてくれました。おまけにリズムに乗って移動することで撃ち込みやすくもなります。いやあ素晴らしい。" },
    { "Gemini60",   "撃ち続けると画面全体に散った弾が一斉に自機に向かって飛んできてキツくなります。とはいえ動画の尺的に撃たないわけにもいかず……回避パターンを考えるのがとても楽しい弾幕でした。" },
    { "DeepSeek61", "仮想の3D空間に正十二面体を構築した上それをグリグリ回してきてくれました。DeepSeekによる説明の通り原案ではランダム弾も撃っていたのですが、あまりに難しかったのでナーフしました。" },
    { "Zai62",      "「カオスすぎたｗｗｗ」系のタイトルをAskStudioが勧めてくるので乗っかってみました。ビットの影に隠れたままだと裏から刺されるので勇気を出して撃ち込みにいく必要があります。激ムズ！" },
    { "Zai63",      "\"床屋の前に置いてあるやつ\"のつもりがまさかの支柱に付いた標識に！「潮吹き」や「大文字」とは違い、本当にこんな弾幕になることは予想していませんでした。" },
    { "Claude64",   "他の多くのAIが串の表現に苦戦したり諦めたりする中、串も含めて三色団子をしっかり表現した上で丁度いい難易度の弾幕に仕上げてくれました。" },
    { "ChatGPT65",  "時間をかけすぎると長く伸びた首で自機をしばかれるので、なるべく正面を取って撃ち込めるパターンを組む必要があります。弾源の位置が刻一刻と変わるのも厄介ポイント。" },
    { "Claude66",   "他の多くのAIが自機に届く前に超巨大弾を崩してしまう中、しっかり画面最下部まで撃ち込んでくれました。しかし\"超巨大弾\"から\"大玉転がし\"を連想するのは完全に予想外でした。" },
    { "Grok67",     "これでもかなりナーフしてます。原案ではメモリプールを食い潰すまで弾を分裂増殖させてきました。やり過ぎだって……" },
    { "Qwen68",     "自機方向に意図的に隙間を作るアイデアが光っており、おかげで思い切ってリングを5重にするというアレンジを加えることができました。大迫力になって満足です。" },
    { "Gemini69",   "原作を完全再現するとスイカまで育たないことを察したのか簡略化して実装してくれました。自機の近くでメロンが合体してスイカが誕生し爆発することもあるので気が抜けません。" },
    { "Gemini70",   "ばら撒かれた無数の小玉が周波数パラメータの変化に伴い次々と模様を描き出していきます。弾幕としてはひどい運ゲーなのですが、あまりに綺麗だったのでそのまま享受しました。" },
    { "ChatGPT71",  "切れかけの白熱電球がチカチカする様子に、ゲームにありがちな電球がONのときだけオブジェクトが動く演出を取り入れてくれました。OFF→ON時に弾が微妙に曲がるので結構危ないです。" },
    { "Claude72",   "2chネタだと把握できていたAIは少数派でした。予告無しで画面全体に弾を出してくる演出がお気に入り。安全回廊は向きがランダムなので全く期待できません。" },
    { "Grok73",     "ワープ間隔を徐々に短くして最後に締めの一発！という演出が光っていました。中玉は全て自機狙いで切り返しの時間も十分あるので避けるのはそんなに難しくありません。" },
    { "Gemini74",   "ワインダー＋ばら撒き弾は弾幕STGによくある組み合わせですが、たらこスパゲッティってまさにこれだ！と気付いてモチーフに選んでみました。おまけで刻み海苔までふってきて高難易度に。" },
};
const char* g_masterpieceComment = nullptr;

extern std::vector<std::pair<double, double>>& GetOffsets();


// ゲームループの本体部分を関数化（既存の WinMain の中身をほぼコピー）
// 傑作選モード用の専用メインループ
int masterpieceMain() {
    // ウィンドウ初期化などは WinMain と共通なので、WinMain の先頭で masterpieceMode を判定し、
    // 初期化後に masterpieceMain() を呼ぶ形にする
    // ここではゲームループ部分のみを実装する

    // --- ゲーム画面の準備（WinMainからコピー） ---
    int gameScreen = MakeScreen(GAME_W, GAME_H, TRUE);
    SetDrawScreen(gameScreen);

    StateManager::ChangeState(Joutai::Menu);

    playerShotHead.prev = &playerShotHead;
    playerShotHead.next = &playerShotHead;
    enemyShotSetHead.prev = &enemyShotSetHead;
    enemyShotSetHead.next = &enemyShotSetHead;

    GetOffsets();

    // 傑作選リストの現在位置
    size_t masterpieceIndex = 0;
    int wait = 120; // メニュー画面で 2 秒待機

    // 最初の傑作選ステージの stageNum を探す
    for (size_t i = 0; i < stageData.size(); i++) {
        if (stageData[i].stageId == masterpieceList[masterpieceIndex].stageId) {
            stageNum = (int)i;
            cursor.page = stageNum / 100;
            int rem = stageNum % 100;
            cursor.y = rem / 10;
            cursor.x = rem % 10;
            break;
        }
    }
    g_masterpieceComment = masterpieceList[masterpieceIndex].comment.c_str();

    while (!ProcessMessage() && !ClearDrawScreen()) {
        int frameStart = GetNowCount();

        // キー入力（通常モードと同じ）
        if (StateManager::GetState() == Joutai::Replay) {
            updateReplayInput();
        }
        else {
            getHitKeyStateAll_2(key);
            if (StateManager::GetState() == Joutai::Game) {
                replayKeyHistory.push_back(packReplayKey(key));
            }
        }

        // 状態別処理（既存の WinMain と同じ）
        if (StateManager::GetState() == Joutai::Menu) {
            moveCursor();
            menuDraw();
            wait--;
            if (wait <= 0) {
                StateManager::ChangeState(Joutai::Replay);
            }
        }
        else if (StateManager::GetState() == Joutai::Replay) {
            stageData[stageNum].patternFunc();
            enemyControl();
            enemyShotControl();
            enemyShotCalc();
            playerControl();
            playerShotControl();
            playerShotCalc();
            playerShotHit();
            enemyShotHit();
            enemyHit();

            backGround();
            playerDisp();
            playerShotDisp();
            enemyDisp();
            enemyShotDisp();
            drawSidePanel();
            foreGround();

            wait = 150; // リプレイ終了後は 2.5 秒待機
        }
        else if (StateManager::GetState() == Joutai::Win || StateManager::GetState() == Joutai::Lose) {
            backGround();
            playerDisp();
            playerShotDisp();
            enemyDisp();
            enemyShotDisp();
            drawSidePanel();
            foreGround();
            drawGameOverlay();
            count--;

            wait--;
            if (wait <= 0) {
                // リプレイデータ終了 → 次の傑作選ステージへ
                masterpieceIndex++;
                if (masterpieceIndex >= masterpieceList.size()) {
                    StateManager::ChangeState(Joutai::Menu);
                    wait = 999999;
                }
                else {
                    // 次の傑作選ステージの stageNum を探す
                    for (size_t i = 0; i < stageData.size(); i++) {
                        if (stageData[i].stageId == masterpieceList[masterpieceIndex].stageId) {
                            stageNum = (int)i;
                            cursor.page = stageNum / 100;
                            int rem = stageNum % 100;
                            cursor.y = rem / 10;
                            cursor.x = rem % 10;
                            break;
                        }
                    }
                    g_masterpieceComment = masterpieceList[masterpieceIndex].comment.c_str();

                    isMuteki = (masterpieceList[masterpieceIndex].stageId == "Grok67");

                    StateManager::ChangeState(Joutai::Replay);
                }
            }
        }

        fpsTimeFunction();
        count++;

        // リサイズ対応
        SetDrawScreen(DX_SCREEN_BACK);
        ClearDrawScreen();
        int winW, winH;
        GetDrawScreenSize(&winW, &winH);
        double scaleX = (double)winW / GAME_W;
        double scaleY = (double)winH / GAME_H;
        double scale = (scaleX < scaleY) ? scaleX : scaleY;
        int drawW = (int)(GAME_W * scale);
        int drawH = (int)(GAME_H * scale);
        int offsetX = (winW - drawW) / 2;
        int offsetY = (winH - drawH) / 2;
        DrawExtendGraph(offsetX, offsetY, offsetX + drawW, offsetY + drawH, gameScreen, TRUE);
        ScreenFlip();
        SetDrawScreen(gameScreen);

        int elapsed = GetNowCount() - frameStart;
        if (elapsed < 17) WaitTimer(17 - elapsed);
    }

    return 0;
}
