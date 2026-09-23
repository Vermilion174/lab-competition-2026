#ifndef CONST_H
#define CONST_H

#include <cfloat>
#include <string>

namespace orienteering {

    // ファイル・ディレクトリ
    inline const std::string DATA_DIR = "input";
    inline const std::string OUTPUT_DIR = "output";

    // コントロール数制約
    constexpr int MIN_CONTROLS = 6;
    constexpr int MAX_CONTROLS = 12;

    // 目的関数パラメータ
    constexpr int    Q_TARGET = 8;        // 目標コントロール数
    constexpr double D_MIN = 150.0;    // 最低距離閾値
    constexpr double T_TARGET = 60.0;     // 目標時間
    constexpr double WALK_SPEED = 4020.0;   // 平地速度
    constexpr double CLIMB_SPEED = 300.0;    // 登り速度
    constexpr double ROUTE_TARGET = 50.0;     // 許容登り高低差
    constexpr double PENALTY = DBL_MAX;  // 無効解ペナルティ

    // 指標重み
    constexpr double W_MAP = 0.50;
    constexpr double W_DIST = 0.10;
    constexpr double W_TIME = 0.25;
    constexpr double W_ROUTE = 0.15;

    // GAパラメータ
    constexpr int          POP_SIZE = 200; // 個体数
    constexpr int          BASE_N_GEN = 100; // 基本世代数
    constexpr double       PROB_BIT = 0.02;// ビット反転確率
    constexpr double       PROB_SWAP = 0.10;// スワップ変異確率
    constexpr unsigned int RANDOM_SEED = 42;  // 乱数シード

    // 切り替え割合
    constexpr double RATIO_SELECT_SWITCH = 0.70; // 選択パート交叉切り替え
    constexpr double RATIO_ORDER_SWITCH = 0.80; // 順序パート交叉切り替え
    constexpr double RATIO_TOURNAMENT_SWITCH_1 = 0.40; // トーナメントサイズ切り替え1
    constexpr double RATIO_TOURNAMENT_SWITCH_2 = 0.70; // トーナメントサイズ切り替え2

    // トーナメントサイズ
    constexpr int TOURNAMENT_SIZE_1 = 2; // 序盤
    constexpr int TOURNAMENT_SIZE_2 = 4; // 中盤
    constexpr int TOURNAMENT_SIZE_3 = 5; // 終盤

    // 物理定数
    constexpr double PI = 3.14159265358979323846;
    constexpr double METERS_PER_DEGREE = 111320.0; // 緯度1度あたりの距離

} // namespace orienteering

#endif // CONST_H