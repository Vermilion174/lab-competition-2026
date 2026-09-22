#ifndef CONST_H
#define CONST_H

#include <cfloat>
#include <string>

namespace orienteering {

// ============================================================
// ファイル・ディレクトリ
// ============================================================
inline const std::string DATA_DIR   = "input";    // 入力データのフォルダ
inline const std::string OUTPUT_DIR = "output";   // 出力ファイルのフォルダ

// ============================================================
// コントロール数の制約（コンペ課題で指定）
// ============================================================
constexpr int MIN_CONTROLS = 6;
constexpr int MAX_CONTROLS = 12;

// ============================================================
// 目的関数パラメータ（コンペ課題 表1）
// ============================================================
constexpr int    Q_TARGET     = 8;        // 目標コントロール数
constexpr double D_MIN        = 150.0;    // 近すぎる距離の閾値（m）
constexpr double T_TARGET     = 60.0;     // 目標所要時間（分）
constexpr double WALK_SPEED   = 4020.0;   // 平地の歩行速度（m/時）
constexpr double CLIMB_SPEED  = 300.0;    // 登り坂の速度換算値（m/時）
constexpr double ROUTE_TARGET = 50.0;     // 累積登り高低差の許容値（m）
constexpr double PENALTY      = DBL_MAX;   // 無効解へのペナルティ（実値では到達し得ないセンチネル）

// ============================================================
// 重み設定（4指標を重み付き和で集約。合計 1.0）
// ============================================================
constexpr double W_MAP   = 0.50;
constexpr double W_DIST  = 0.10;
constexpr double W_TIME  = 0.25;
constexpr double W_ROUTE = 0.15;

// ============================================================
// GA のハイパーパラメータ
// ============================================================
constexpr int    POP_SIZE        = 200;   // 個体数
constexpr int    N_GEN           = 500;   // 世代数
constexpr double PROB_BIT        = 0.02;  // 選択パートのビット反転確率
constexpr double PROB_SWAP       = 0.10;  // 順序パートのスワップ確率

constexpr unsigned int RANDOM_SEED = 42;

// ============================================================
// 切り替えタイミング用定数（事前計算用パラメータ）
// ============================================================
// 親選択手法の切り替え閾値
constexpr int SELECT_METHOD_SWITCH_GEN = static_cast<int>(N_GEN * 1.0);

// 選択パートの交叉手法切り替え閾値
constexpr int SELECT_SWITCH_GEN = static_cast<int>(N_GEN * 0.60);

// 順序パートの交叉手法切り替え閾値
constexpr int ORDER_SWITCH_GEN_1 = static_cast<int>(N_GEN * 5.0);
constexpr int ORDER_SWITCH_GEN_2 = static_cast<int>(N_GEN * 1.0);

// 動的トーナメントサイズの切り替え閾値とサイズ定義 (3段階)
constexpr int TOURNAMENT_SWITCH_GEN_1 = static_cast<int>(N_GEN * 0.40); // 段階1 -> 2 (例: 40%)
constexpr int TOURNAMENT_SWITCH_GEN_2 = static_cast<int>(N_GEN * 0.80); // 段階2 -> 3 (例: 80%)

constexpr int TOURNAMENT_SIZE_1 = 2; // 序盤：低淘汰圧（多様性保持）
constexpr int TOURNAMENT_SIZE_2 = 3; // 中盤：中淘汰圧（バランス）
constexpr int TOURNAMENT_SIZE_3 = 5; // 終盤：高淘汰圧（高速収束）

// ============================================================
// 物理定数
// ============================================================
constexpr double PI                = 3.14159265358979323846;
constexpr double METERS_PER_DEGREE = 111320.0;  // 緯度1度あたりのメートル

} // namespace orienteering

#endif // CONST_H
