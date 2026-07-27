#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ========================================================
// 敵本体のパターン
// ========================================================
void EnemyPat_Tmp()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 1; // 耐久は高めを想定
    }

    enemy.x = player.x;
    //enemy.y = player.y - 200;
}