#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"

void EnemyPat_Spout_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 50;
    }
}