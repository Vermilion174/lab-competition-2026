#pragma once

#include <string>
#include <vector>
#include <random>

// 生成するlandmarkの個数
constexpr int LANDMARK = 113;
constexpr int FEATURE = 17;
constexpr int NODE = 2268;

// 全体エリアの緯度・経度の最大/最小値
constexpr double MAX_LAT = 35.57160986;
constexpr double MIN_LAT = 35.55301084;
constexpr double MAX_LON = 139.5881599;
constexpr double MIN_LON = 139.5654312;

// グリッド分割数（例: 4×4 = 16ブロック）
constexpr int GRID_ROWS = 4; // 緯度方向の分割数
constexpr int GRID_COLS = 4; // 経度方向の分割数

struct landmark {
    int lm_id = 0;
    char name = 'a';
    std::string type = "default";
    double lm_lat = 0.0;
    double lm_lon = 0.0;
    double nearest_node_id = 0.0;
};

struct node {
    double nd_id = 0.0;
    double nd_lat = 0.0;
    double nd_lon = 0.0;
};

// 密集分布対応版の座標決定関数
void allocate_loc_grid(
    landmark& lm,
    std::mt19937& gen,
    std::discrete_distribution<int>& block_dist
);

void allocate_node(
    landmark& lm,
    const std::vector<node>& node_table,
    std::mt19937& gen
);