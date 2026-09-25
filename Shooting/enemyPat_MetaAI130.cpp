#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 漏斗の見た目だけ。動かない白弾でV字を作る
static void ShotFunnel(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // V字の壁
        const int N = 15;
        for (int i = 0; i < N; i++) {
            double t = (double)i / (N - 1); // 0->1
            // 左壁
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = pSet->x - 40.0 * (1.0 - t);
                p->y = pSet->y + 40.0 + 70.0 * t;
                p->muki = 0; p->speed = 0;
                p->kind = img_enemyShotSmallBall[6]; // 白・小
                p->margin = 100;
                p->param_i[0] = 0;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
            // 右壁
            {
                sEnemyShot* p = new sEnemyShot;
                p->x = pSet->x + 40.0 * (1.0 - t);
                p->y = pSet->y + 40.0 + 70.0 * t;
                p->muki = 0; p->speed = 0;
                p->kind = img_enemyShotSmallBall[6];
                p->margin = 100;
                p->param_i[0] = 0;
                p->prev = pSet->pEnemyShotHead->prev;
                p->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = p;
                pSet->pEnemyShotHead->prev = p;
            }
        }
        // 漏斗の口
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y + 110.0;
        p->muki = 0; p->speed = 0;
        p->kind = img_enemyShotMediumBall[6]; // 白・中
        p->margin = 100;
        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }
    // 完全固定なので移動処理なし
}

// 本命: ピッチドロップ本体
static void ShotPitchDrop(sEnemyShotSet* pSet)
{
    // pSet->param_i[0] : 0=膨張中, 1=落下中, 2=クールダウン
    // pSet->param_i[1] : クールダウンカウンタ
    // pSet->param_d[0] : たるみ量 sag
    // pSet->param_d[1] : 漏斗先端の基準Y
    // pSet->param_d[2] : 落下中の重力速度

    if (pSet->count == 0) {
        pSet->param_i[0] = 0;
        pSet->param_i[1] = 0;
        pSet->param_d[0] = 0.0;
        pSet->param_d[1] = pSet->y + 110.0; // 先端Y
        pSet->param_d[2] = 0.0;

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 1. ピッチの柱 12個 黒・中玉を数珠繋ぎ
        for (int j = 0; j < 12; j++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->y + 10.0 + j * 8.0;
            p->muki = 0; p->speed = 0;
            p->kind = img_enemyShotMediumBall[7]; // 黒・中
            p->margin = 80;
            p->param_i[0] = 1; // 柱
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        // 2. 一滴のコア 黒・中
        {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x;
            p->y = pSet->param_d[1];
            p->muki = 0; p->speed = 0;
            p->kind = img_enemyShotMediumBall[7];
            p->margin = 200; // 画面外削除を遅らせる
            p->param_i[0] = 99; // コア識別子
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        // 3. 表面張力を表現する白・小玉をコア周囲に6発
        for (int k = 0; k < 6; k++) {
            sEnemyShot* p = new sEnemyShot;
            double ang = k * DX_PI * 2.0 / 6.0;
            p->x = pSet->x + cos(ang) * 7.0;
            p->y = pSet->param_d[1] + sin(ang) * 7.0;
            p->muki = ang; p->speed = 0;
            p->kind = img_enemyShotSmallBall[6]; // 白・小
            p->margin = 80;
            p->param_i[0] = 2; // 張力
            p->param_d[0] = ang; // 角度保持
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
        return;
    }

    // コアを探す
    sEnemyShot* pCore = nullptr;
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        if (s->param_i[0] == 99) { pCore = s; break; }
    }
    if (!pCore) return;

    if (pSet->param_i[0] == 0) { // 膨張フェーズ
        pSet->param_d[0] += 0.022; // sag 極低速
        // 微振動で粘性を演出
        pCore->y = pSet->param_d[1] + pSet->param_d[0] + sin(pSet->count * 0.03) * 0.6;

        // 膨張表現: 45F毎に黒・小玉をコアに張り付けて大きく見せる
        // GetRand(x)は0..xの x+1通りなので注意
        if (pSet->count % 45 == 0 && pSet->count < 1100) {
            double r = 2.0 + pSet->param_d[0] * 0.35; // 成長する半径
            double ang = (GetRand(360) / 180.0) * DX_PI;
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotSmallBall[7]; // 黒・小
            p->margin = 200;
            p->param_i[0] = 3; // 付着ブロブ
            p->param_d[0] = cos(ang) * r * (GetRand(100) / 100.0);
            p->param_d[1] = sin(ang) * r * (GetRand(100) / 100.0);
            p->x = pCore->x + p->param_d[0];
            p->y = pCore->y + p->param_d[1];
            p->muki = 0; p->speed = 0;
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 埃: 白・小玉を漏斗からゆっくり漂わせて高密度化
        if (pSet->count % 18 == 0) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pSet->x + (GetRand(40) - 20);
            p->y = pSet->y + 60.0 + GetRand(20);
            p->muki = atan2(p->y - pCore->y, p->x - pCore->x) + (GetRand(60) - 30) / 180.0 * DX_PI;
            p->speed = 0.15 + GetRand(50) / 100.0;
            p->kind = img_enemyShotSmallBall[6]; // 白・小
            p->margin = 80;
            p->param_i[0] = 4; // 埃
            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }

        // 付着ブロブはコアに追従
        for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
            if (s->param_i[0] == 3) {
                s->x = pCore->x + s->param_d[0];
                s->y = pCore->y + s->param_d[1];
            }
            if (s->param_i[0] == 2) { // 表面張力リングが少し広がる
                double ang = s->param_d[0];
                double rad = 7.0 + pSet->param_d[0] * 0.18;
                s->x = pCore->x + cos(ang) * rad;
                s->y = pCore->y + sin(ang) * rad;
            }
        }

        // 1200F(20秒)で落下開始 -> ピッチドロップの遅さを表現
        if (pSet->count > 1200-40) {
            pSet->param_i[0] = 1; // 落下へ
            pCore->kind = img_enemyShotLargeBall[7]; // 黒・大玉に変身
            pCore->speed = 0.25;
            pCore->muki = DX_PI / 2.0;
            pSet->param_d[2] = pCore->speed;

            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 落下衝撃で周囲の弾を押し出す
            for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
                if (s == pCore) continue;
                if (s->param_i[0] == 1 || s->param_i[0] == 4) { // 柱と埃
                    double ang = atan2(s->y - pCore->y, s->x - pCore->x);
                    s->muki = ang;
                    s->speed = 0.8 + GetRand(120) / 100.0;
                }
                if (s->param_i[0] == 2) { // 張力リングは弾ける
                    double ang = s->param_d[0];
                    s->muki = ang;
                    s->speed = 1.2 + GetRand(80) / 100.0;
                }
            }
        }
    }
    else if (pSet->param_i[0] == 1) { // 落下フェーズ
        pSet->param_d[2] += 0.06; // 重力
        pCore->speed = pSet->param_d[2];
        // 付着ブロブも一緒に落ちる
        for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
            if (s->param_i[0] == 3) {
                s->muki = DX_PI / 2.0 + (GetRand(40) - 20) / 180.0 * DX_PI * 0.15;
                s->speed = pCore->speed * (0.85 + GetRand(30) / 100.0);
            }
        }
        if (pCore->y > 520.0) {
            pSet->param_i[0] = 2;
            pSet->param_i[1] = 0;
        }
    }
    else if (pSet->param_i[0] == 2) { // クールダウン後リセット
        pSet->param_i[1]++;
        if (pSet->param_i[1] > 90) {
            // 全弾削除して初期化し直し
            sEnemyShot* cur = pSet->pEnemyShotHead->next;
            while (cur != pSet->pEnemyShotHead) {
                sEnemyShot* nxt = cur->next;
                cur->prev->next = cur->next;
                cur->next->prev = cur->prev;
                delete cur;
                cur = nxt;
            }
            pSet->count = -1; // メインで+1されるので次フレーム0になる
            return;
        }
    }

    // 共通移動処理
    for (sEnemyShot* s = pSet->pEnemyShotHead->next; s != pSet->pEnemyShotHead; s = s->next) {
        if (pSet->param_i[0] == 0 && (s->param_i[0] == 3 || s->param_i[0] == 99 || s->param_i[0] == 2)) {
            continue; // 膨張中はコア追従で位置決め済み
        }
        s->x += s->speed * cos(s->muki);
        s->y += s->speed * sin(s->muki);
    }
}

void EnemyPat_PitchDrop_MetaAI()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        // 漏斗セット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotFunnel;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->muki = 0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        // ピッチ本体セット
        {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotPitchDrop;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = DX_PI / 2.0;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else {
        // 実験装置なので敵本体は微動のみ
        enemy.x = 240.0 + sin(count * 0.005) * 2.0;
    }
}