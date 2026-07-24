// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 各フェーズの弾幕制御関数 (patternFunc)
// ============================================================

// 【フェーズ1：煙の解放】＆【3つ目の願い発動時の誘導化】
static void ShotSmoke(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // ボスを中心に、渦巻く怪しい煙（中楕円弾）を全方位に大量展開
        int num_bullets = 36;
        for (int i = 0; i < num_bullets; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 360度に均等に割り振り
            pEnemyShot->muki = (double)i / num_bullets * DX_PI * 2.0;
            // 画面に長く残るよう、非常にゆっくりと広がる速度に設定
            pEnemyShot->speed = 0.4 + (GetRand(40) / 100.0);

            // 煙の怪しさを表現するため、4:青 と 5:マゼンタ をランダムに選別
            int color = (GetRand(100) % 2 == 0) ? 4 : 5;
            pEnemyShot->kind = img_enemyShotMediumOval[color];

            // リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレームの移動更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (count < 660) {
            // 通常時：じわじわと外側に広がりつつ、わずかに回転させて渦巻き感を演出
            pEnemyShot->muki += 0.003;
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        else {
            // 3つ目の願い発動（660フレーム以降）：画面に漂っていた煙が一斉に自機をマイルドに追尾（誘導）
            double target_muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
            double diff = target_muki - pEnemyShot->muki;
            while (diff > DX_PI)  diff -= DX_PI * 2.0;
            while (diff < -DX_PI) diff += DX_PI * 2.0;

            // 旋回性能に制限をかけ、滑らかな誘導にする
            double rotate_speed = 0.012;
            if (diff > rotate_speed)  diff = rotate_speed;
            if (diff < -rotate_speed) diff = -rotate_speed;
            pEnemyShot->muki += diff;

            // 追尾に移行した煙はスピードが少し上がる
            pEnemyShot->speed = 1.2;
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 【1つ目の願い：富の豪雨】
static void ShotGoldRain(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_muki = pEnemyShotSet->muki;
        // 連射タイミング（kind）の奇数・偶数によって、Vの字の開き角度を交互に変えて揺さぶる
        double offset = (pEnemyShotSet->kind % 2 == 0) ? 0.18 : 0.32;

        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機を左右から挟み込むようなVの字の軌道
            pEnemyShot->muki = base_muki + (i == 0 ? -offset : offset);
            // 豪雨にふさわしい鋭い高速直線弾
            pEnemyShot->speed = 4.5 + (GetRand(100) / 100.0);

            // 金貨と宝石をイメージし、1:黄 と 0:赤 の菱形弾をブレンド
            int color = (GetRand(100) % 2 == 0) ? 1 : 0;
            pEnemyShot->kind = img_enemyShotDiamond[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 【2つ目の願い：逃れぬ戒め】
static void ShotPreach(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 魔法陣が展開される不気味な予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 生成された瞬間のプレイヤーの位置（pEnemyShotSetのx,y）を中心に円形に配置
        int num_bullets = 24;
        double radius = 150.0;
        for (int i = 0; i < num_bullets; i++) {
            pEnemyShot = new sEnemyShot;

            double angle = (double)i / num_bullets * DX_PI * 2.0;
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = angle + DX_PI; // 円の中心（かつての自機位置）を向く
            pEnemyShot->speed = 2.4;

            // 戒めの魔力を表現するため、3:シアン の鱗弾を使用
            pEnemyShot->kind = img_enemyShotScale[3];
            // param_i[0] を利用して、最初の60フレーム（約1秒）動かない静止予告タイマーにする
            pEnemyShot->param_i[0] = 60;
            pEnemyShot->margin = 200;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 予告期間が終わり、収束（発射）が始まるフレーム（セットのカウントが60に達した時）に重い銃声
    if (pEnemyShotSet->count == 60) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] > 0) {
            // 予告期間：中心点の周りを、魔法陣の歯車のように怪しく回転しながらプレイヤーを拘束
            double angle = atan2(pEnemyShot->y - pEnemyShotSet->y, pEnemyShot->x - pEnemyShotSet->x);
            angle += 0.03; // 回転速度
            double radius = 150.0;
            pEnemyShot->x = pEnemyShotSet->x + radius * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + radius * sin(angle);
            pEnemyShot->muki = angle + DX_PI; // 常に内側を向かせ続ける

            pEnemyShot->param_i[0]--;
        }
        else {
            // 収束：予告終了と同時に、中心点へ向かって一斉に収束突き刺し移動
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

// 【3つ目の願い：終焉の魔力】
static void ShotFinalLaser(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 画面横幅480のうち、ランダムで安全地帯（逃げ場）を設定し、残りの7割を埋め尽くすレーザーの爆撃
        // GetRand(2) は 0, 1, 2 の 3種類を返す
        int safe_zone = GetRand(2); // 0:左側が安全、1:中央が安全、2:右側が安全

        for (int x_pos = 15; x_pos < 480; x_pos += 24) {
            // 安全地帯に設定された範囲の座標なら、レーザーの生成をスキップして隙間を作る
            if (safe_zone == 0 && x_pos < 150) continue;
            if (safe_zone == 1 && x_pos > 160 && x_pos < 320) continue;
            if (safe_zone == 2 && x_pos > 330) continue;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (double)x_pos;
            pEnemyShot->y = -10.0; // 画面最上部（場外）から降らせる
            pEnemyShot->muki = DX_PI / 2.0; // 真下向き固定
            pEnemyShot->speed = 6.0; // 威圧感のある超高速

            // 圧倒的な魔力を誇示するため、6:白 のレーザーを使用
            pEnemyShot->kind = img_enemyShotLaser[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
// 敵本体のパターン・フェーズ管理メイン
// ============================================================
void EnemyPat_MagicLamp_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 中ボス・大ボス想定の初期位置・HP初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200固定仕様
        muki = 1;
        shot_count = 0;
    }
    else {
        // ボス本体の移動制御
        if (count < 660) {
            // フェーズ1〜2：プレイヤーを翻弄するように左右へゆらゆらと往復移動
            enemy.x += 0.98 * (double)muki;
            if (count % 120 == 60) muki *= -1;
        }
        else {
            // フェーズ3：魔人がランプの中へ戻る演出として、中央上部(240, 40)へじわじわと吸引・固定化
            enemy.x += (240.0 - enemy.x) * 0.05;
            enemy.y += (40.0 - enemy.y) * 0.05;
        }
    }

    // --------------------------------------------------------
    // タイムラインによる弾幕フェーズ遷移管理 (1秒=60フレーム想定)
    // --------------------------------------------------------
    const int T = 900;

    // 【フェーズ1：煙の解放】(1〜180フレーム / 開幕〜3秒)
    // 定期的にランプ（敵の位置）から怪しい不透明な煙をじわじわと吐き出させて画面に蓄積させる
    if (count % T >= 10 && count % T <= 160 && count % T % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSmoke;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 【フェーズ2：1つ目の願い『富の豪雨』】(181〜400フレーム / 約3秒〜6.5秒)
    // 魔人の両手から溢れ出る財宝。自機を鋭く狙う高速Vの字弾を激しく連射
    if (count % T >= 181 && count % T < 400 && count % T % 12 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGoldRain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 【フェーズ2：2つ目の願い『逃れぬ戒め』】(401〜650フレーム / 約6.5秒〜11秒)
    // プレイヤーの現在位置に周期的に魔法陣を先回り展開。一定時間回転したのち一斉に収束してプレイヤーを潰す
    if (count % T >= 410 && count % T < 630 && (count % T - 410) % 75 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPreach;
        // 包囲の中心を「生成された瞬間のプレイヤー座標」にロックする
        pEnemyShotSet->x = player.x;
        pEnemyShotSet->y = player.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 【フェーズ2：3つ目の願い『終焉の魔力』】(660フレーム〜終了まで / 約11秒以降)
    // 画面の7割を消し飛ばす極太レーザーの爆撃。
    // ※この発動と同時に、ステージ上にずっと残留していた「ShotSmokeの煙弾」が自機へ向け誘導を開始する複合耐久パート
    if (count % T >= 660 && (count % T - 660) % 45 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFinalLaser;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}