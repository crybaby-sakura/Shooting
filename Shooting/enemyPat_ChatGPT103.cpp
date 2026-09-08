// enemyPat_Tmp.cpp
//
// 弾幕：くるくる回転迷宮・高難度版
//
// 「くるくるくるりん」の「回転する棒の先端を避けながら
// 狭い通路を抜ける」感覚を重視した弾幕。
// 
// 画面上に複数の回転ゲートを縦に配置し、各ゲートは
// 中玉を一直線に連結した長い棒で構成する。
// 棒はゆっくり回転するため、画面を横断する棒の向きが
// 刻々と変化し、左右の端に安全な通過口が現れる。
//
// ・count / pEnemyShotSet->count / pEnemyShot->count の
//   インクリメントは行わない。
// ・画面外の弾の消去も行わない。
// ・使用素材：img_enemyShotMediumBall[] のみ。
//   中玉を連結することで「回転する棒」を表現する。
//


// ============================================================
// 回転ゲート
// ============================================================
//
// param_i[0] : 棒を構成する弾数
// param_i[1] : 中玉の色
// param_i[2] : ゲート番号
//
// param_d[0] : 回転速度（rad/frame）
//
// ShotSet 自体の x / y がゲート中心。
// 各弾の param_d[0] に棒中心からの距離を保存する。
//
static void ShotRotateGate(sEnemyShotSet* pEnemyShotSet)
{
    // --------------------------------------------------------
    // 初回生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        const int BAR_COUNT = pEnemyShotSet->param_i[0];
        const int color = pEnemyShotSet->param_i[1];

        // 弾同士が少し重なるようにして、見た目が連続した
        // 「巨大な棒」になるようにする。
        const double BAR_STEP = 11.0;
        const double HALF = (BAR_COUNT - 1) * 0.5;

        for (int i = 0; i < BAR_COUNT; ++i) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 棒の中心からの距離。
            pEnemyShot->param_d[0] = (i - HALF) * BAR_STEP;

            // 回転する障害物なので、個々の弾は自力移動しない。
            pEnemyShot->speed = 0.0;

            // 棒を表現するため中玉を使用。
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // 初期位置はゲート中心。
            // 次の更新で正しい位置へ配置される。
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 棒の向き。
            pEnemyShot->muki = pEnemyShotSet->muki;

            // 弾リストへ追加。
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --------------------------------------------------------
    // 回転
    // --------------------------------------------------------
    pEnemyShotSet->muki += pEnemyShotSet->param_d[0];

    // 大きく回り続けても精度が落ちないように角度を折り返す。
    if (pEnemyShotSet->muki > DX_PI * 2.0) {
        pEnemyShotSet->muki -= DX_PI * 2.0;
    }
    else if (pEnemyShotSet->muki < -DX_PI * 2.0) {
        pEnemyShotSet->muki += DX_PI * 2.0;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;

    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double dist = pShot->param_d[0];

        // 棒をゲート中心の周囲で回転させる。
        pShot->x = pEnemyShotSet->x + cos(pEnemyShotSet->muki) * dist;
        pShot->y = pEnemyShotSet->y + sin(pEnemyShotSet->muki) * dist;

        // 描画側で向きを参照できるように保存。
        pShot->muki = pEnemyShotSet->muki;

        pShot = pShot->next;
    }
}


// ============================================================
// 回転ゲート生成
// ============================================================
//
// 1つのゲートを「横長の棒」に近い初期角度で配置する。
// ゲートごとに回転方向・速度・中心位置を変えることで、
// 同じタイミングで開口部が揃わないようにする。
//
static void CreateRotateGate(
    double x,
    double y,
    double angle,
    double rotateSpeed,
    int barCount,
    int color,
    int gateIndex)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotRotateGate;

    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;

    pEnemyShotSet->muki = angle;
    pEnemyShotSet->param_d[0] = rotateSpeed;

    pEnemyShotSet->param_i[0] = barCount;
    pEnemyShotSet->param_i[1] = color;
    pEnemyShotSet->param_i[2] = gateIndex;

    // ShotSet の番兵。
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    // ShotSet リストへ追加。
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}


// ============================================================
// 敵本体
// ============================================================
void EnemyPat_KuruKuruKururin_ChatGPT()
{
    // 敵は上部でゆっくり左右移動。
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 45.0;

        enemy.maxHp = enemy.hp = 200;

        muki = 1;
    }
    else {
        enemy.x += 0.70 * (double)muki;

        if (enemy.x < 100.0) {
            enemy.x = 100.0;
            muki = 1;
        }
        else if (enemy.x > 380.0) {
            enemy.x = 380.0;
            muki = -1;
        }
    }

    // ========================================================
    // 序盤：回転ゲートを1枚ずつ配置
    // ========================================================
    //
    // 各ゲートを縦に離して置く。
    // プレイヤーは「今いるゲートの開口部を抜ける」
    // →「次のゲートの開口部へ移動する」
    // という操作を繰り返す。
    //
    if (count == 5) {
        // 1枚目
        CreateRotateGate(
            240.0,
            155.0,
            0.0,
            0.0105,
            33,
            0,
            0
        );

        // 2枚目
        CreateRotateGate(
            190.0,
            275.0,
            DX_PI,
            -0.0130,
            37,
            1,
            1
        );

        // 3枚目
        CreateRotateGate(
            290.0,
            395.0,
            0.0,
            0.0160,
            41,
            2,
            2
        );

        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // ========================================================
    // 中盤以降：4枚目・5枚目を追加
    // ========================================================
    //
    // すべてのゲートを最初から出すと序盤から難しすぎるため、
    // 時間経過後にさらに2枚を追加する。
    //
    // 追加ゲートは中心を左右にずらして、単純な上下移動だけでは
    // 抜けられないようにする。
    //
    if (count == 300) {
        CreateRotateGate(
            350.0,
            215.0,
            DX_PI * 0.5,
            -0.0180,
            29,
            3,
            3
        );

        if (CheckSoundMem(sound_enemyShot_light)) {
            StopSoundMem(sound_enemyShot_light);
        }
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    if (count == 480) {
        CreateRotateGate(
            125.0,
            335.0,
            DX_PI * 0.5,
            0.0200,
            31,
            4,
            4
        );

        if (CheckSoundMem(sound_enemyShot_light)) {
            StopSoundMem(sound_enemyShot_light);
        }
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // ========================================================
    // 終盤：ゲートの回転速度を上げるような構成
    // ========================================================
    //
    // 既存の ShotSet の param_d[0] を直接変更する。
    // 生成済みのゲートはすべて enemyShotSetHead に登録されている。
    //
    // これにより「同じ隙間がずっと待っていれば現れる」状態を崩し、
    // プレイヤーが実際にゲートを見て追従する必要が出る。
    //
    if (count == 720) {
        sEnemyShotSet* pSet = enemyShotSetHead.next;

        while (pSet != &enemyShotSetHead) {
            // 既存ゲートのみを対象。
            // 速度の符号は維持し、回転だけ少し速くする。
            if (pSet->patternFunc == ShotRotateGate) {
                pSet->param_d[0] *= 1.20;
            }

            pSet = pSet->next;
        }
    }

    // ========================================================
    // 最終段階：さらに難しい回転ゲート
    // ========================================================
    if (count == 900) {
        CreateRotateGate(
            240.0,
            455.0,
            DX_PI * 0.25,
            -0.0230,
            35,
            4,
            5
        );

        if (CheckSoundMem(sound_enemyShot_medium)) {
            StopSoundMem(sound_enemyShot_medium);
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}
