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
bool masterpieceMode = false;

std::vector<MasterpieceEntry> masterpieceList = {
    { "Claude", "全ての始まり。まさか「幾何学的に美しい弾幕を実装して」だけのプロンプトでこれを撃ってくるとは思いませんでした。" }, // 20.47
    { "Gemini2", "誘導を失敗すると一瞬で逃げ場が無くなります。避け方を考えるのも実際に避けるのも楽しい弾幕でした。" }, // 20.38
    { "DeepSeek3", "見せつけるように一回転した後、壁を成してゆっくりと自機を追い詰めてきます。意外と隙間はあります。" }, // 20.33
    { "ChatGPT4", "ゆらゆら舞う桜の花びらを避けつつ大玉で作られた道を進んでいきます。大玉は自機外しなので誘導ゲーです。" }, // 34.95
    { "Gemini5", "圧倒的な迫力！とはいえ大半は目眩ましなので、落ち着いて自機周辺だけを見れば避けるのは簡単です。" }, // 20.58
    { "Grok6", "Grokだけはインターネット検索を指示していないのに自主的にWebで調査し、波粒の実装記事を発見してくれていました。" }, // 20.23
    { "Claude7", "敵機自身が彗星となり、尾を引きながら画面中を動き回る……コンセプトも難易度も言う事なしの見事な弾幕でした。" }, // 23.38
    { "Sakana8", "雷を模した弾の列が一瞬で現れ、視認する暇もないまま崩れて自機へ襲いかかります。なるべく早めに上部脱出を図るのがコツ。" }, // 24.46
    { "Gemini9", "猛烈な風に舞う雪と氷の粒が自機に襲いかかります。画面外の弾消し判定がなければ自分では回避不可能でした。" }, // 67.73
    { "Grok10", "大味ながら、竜の強烈なブレス攻撃を良く表現できていると思います。弾がうねるので見た目ほど余裕は無いです。" }, // 28.62
    { "Gemini11", "溜めるエフェクトから大量の弾を一列に放ってきます。次弾装填にちょっと時間がかかってるのもそれっぽい。" }, // 20.80
    { "Qwen12", "太陽の輝きと降り注ぐ太陽光線がよく表現できていました。ボーっと避けてるとどんどん流されていくので正面維持を意識。" }, // 23.18
    { "DeepSeek13", "鱗弾を円形に組み合わせることでカッターを表現してくれました。基本は大きい弾と一緒ですが、たまに無傷ですり抜けられることも。" }, // 27.30
    { "Claude14", "緑と紫の低速弾を中心に毒のようにじわじわとプレイヤーを追い詰めてきます。撃ち込みに行けないので長期戦必至。" }, // 43.32
    { "Gemini15", "弾幕シューで洗濯機といえば……に期待していたものの空振り。でも渦の向きが途中で変わったりガタガタ稼働してたり素晴らしい！" }, // 20.22
    { "Gemini16", "格子点上に配置された黒石と白石が一斉に動き出します。端の隙間がカツカツで、タイムは配置運にかなり左右されます。" }, // 46.75
    { "Gemini17", "「こうよう」派と「もみじ」派でほぼ差は出なかったです。ほとんど弾を避けなくていいのに揺れるせいで意外に高難易度でした。" }, // 32.72
    { "Gemini18", "クモの巣を展開して自機を捕まえ、自機狙いに転じて一斉に襲いかかってきます。縦糸に沿うように避けると楽です。" }, // 24.53
    { "Qwen19", "弾が画面外から現れ双曲線軌道を描いて去っていきます。向きも色もバグりまくってるんですが、それもまた味ということで。" }, // 22.66
    { "Gemini20", "この回から敵弾が自由パラメータを持てるようにしました。砂時計の形を模すだけでなく、それをひっくり返すような演出まで入れてくれました。" }, // 20.17
    { "Gemini21", "大玉は砕けて小玉として降り注ぎ、たまに中玉や鱗弾がそのまま高速で降ってきます。後者が本当に厄介です。" }, // 20.18
    { "DeepSeek22", "レーンに沿ってノーツが流れてくるシンプルな音ゲーを再現してくれました。運ゲーなので、階段パターンがくると大幅なタイムロスに。" }, // 22.87
    { "Gemini23", "ゆったり回転しているように見えますが、(速度)=(半径)x(角速度)なので思った以上に高速です。大きく誘導して隙間を作ります。" }, // 34.57
    { "DeepSeek24", "撃ち込みを意識すると難易度が急上昇します。調整が終わってから実は中央の隙間に潜り込めることに気付いてしまいました……。" }, // 22.85
    { "ChatGPT25", "扇風機の羽を大玉で表現してくれました。高速で回転しているおかげか意外とそれっぽく見えます。なるべく近寄らないように避けていきます。" }, // 28.27
    { "Sakana26", "他のAIがいきなり文字を書く中で、一旦バラバラに撒いた弾を集めて文字を作るというアイデアが光っていました。" }, // 23.30
    { "Gemini27", "この回からプロンプトを「アイデア出し」→「実装」の二段階にしました。アニメや漫画の世界のクソデカ手裏剣好き。" }, // 20.85
    { "Claude28", "Webで調査させ、それを再現するよう指示しました。上部左右から降ってくる天井＋段階的に増える中玉と、かなり似せてくれました。" }, // 26.37
    { "Gemini29", "ゲームには本来存在しない機能である「敵弾と自機ショットの当たり判定」を自前で実装して割れるシャボン玉を表現してくれました。凄すぎて怖い。" }, // 51.72
    { "Gemini30", "自機近くまで達してから炸裂するので、どんどん前に出ないと潰されます。敵機が反動で後ろに下がるところが良い感じ。" }, // 20.30
    { "Kimi31", "密度はそんなに高くないものの、前後から挟み撃ちにされるとキツいです。大玉だけは別方向に飛ばしてしまうと楽です。" }, // 20.91
    { "Gemini32", "ゲームには本来存在しない機能である「敵弾と敵弾の当たり判定」を自前で実装して爆弾への引火を表現してくれました。凄すぎてマジ怖い。" }, // 21.66
    { "Claude33", "「絶対に回避できない弾幕」を要求したので完璧な答えではあるのですが……あまりの無慈悲さに笑ってしまいました。" }, // --.--
    { "ChatGPT34", "着火後火の粉を撒き散らしながら自機に迫ってきます。火の粉に押しつぶされないよう積極的に前に出て避けていきます。" }, // 20.50
    { "Claude35", "ただのワインダーで終わりかと思いきやぐるっと囲んできて収束します。撃ち込めるタイミングを作るのに苦労しました。" }, // 36.48
    { "Claude36", "全く予想できないタイミングで豆が弾けてポップコーンが飛び散ります。かなり運ゲ気味です。ちなみに菱形弾は「塩」だそうです。" }, // 21.67
    { "Claude37", "どんな立体かは指示していないのに自主的にトーラスを選択して見事に描ききってくれました！グリグリ回転して見せつけてきます。" }, // 22.55
    { "Gemini38", "この回から新素材としてレーザーを用意しました。黄レーザーは予告線のつもりだそうですが、そんな機能は無いので当たり判定はしっかりあります。" }, // 20.27
    { "ChatGPT39", "まさかドット絵でインベーダーを描いてくるとは！そんなに似てないから許してください……。速攻しないと一気にキツくなります。" }, // 21.00
    { "Gemini40", "二重振り子っぽく見えますが物理演算は一切していないただのインチキです。でもそう見えるし何より避けてて楽しいからヨシ！" }, // 22.82
    { "DeepSeek41", "モチーフにするよう依頼しただけなのにブロック崩しそのものを実装しちゃいました。頭がめちゃくちゃこんがらがります。" }, // 24.23
    { "Gemini42", "ちゃんと後光の差すピラミッドに見えるし、何より避けててとても楽しい！非の打ち所のない弾幕でした。" }, // 25.80
    { "Zai43", "体当たりの速さを軌跡に流れ込んでくる弾で表現してくれました。行きと帰りで揺れ方が変わるんで、かなり余裕を見てないと被弾します。" }, // 34.30
    { "Zai44", "矢印のもつ「指し示す」という要素を抽象化して拾ってくれました。ただの超大玉と思って避ければいいだけなんですが感覚が狂う……" },
    { "Kimi45", "実はHP60以下で第三形態に移行し、とんでもない難易度の弾幕を撃ってきます。撃ちすぎないようHP調整するのが大変でした。" }, // 31.43
    { "DeepSeek46", "実は30秒経過すると第三形態に移行し、ボスが中央にワープしてえげつない弾幕を撃ってきます。速攻一択。" }, // 29.80
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
