#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <random>
#include <cfloat>
#include <iomanip>
#include "autogen.h"

// グリッドの確率分布に基づいて座標を生成する関数
void allocate_loc_grid(
    landmark& lm,
    std::mt19937& gen,
    std::discrete_distribution<int>& block_dist
) {
    // 1. 確率分布に従って配置先ブロック（0 ? GRID_ROWS*GRID_COLS - 1）を選択
    int block_index = block_dist(gen);

    int row = block_index / GRID_COLS; // 緯度方向のブロック位置
    int col = block_index % GRID_COLS; // 経度方向のブロック位置

    // 2. ブロックごとの緯度・経度の幅を計算
    double lat_step = (MAX_LAT - MIN_LAT) / GRID_ROWS;
    double lon_step = (MAX_LON - MIN_LON) / GRID_COLS;

    double block_min_lat = MIN_LAT + row * lat_step;
    double block_max_lat = block_min_lat + lat_step;

    double block_min_lon = MIN_LON + col * lon_step;
    double block_max_lon = block_min_lon + lon_step;

    // 3. 選択されたブロック内で一様乱数を生成
    std::uniform_real_distribution<> dis_lat(block_min_lat, block_max_lat);
    std::uniform_real_distribution<> dis_lon(block_min_lon, block_max_lon);

    lm.lm_lat = dis_lat(gen);
    lm.lm_lon = dis_lon(gen);
}

void allocate_node(
    landmark& lm,
    const std::vector<node>& node_table,
    std::mt19937& gen
) {
    double min_dis = DBL_MAX;
    std::vector<double> candidates;

    // 緯度1度 ≒ 111320.0m
    constexpr double LAT_METERS = 111320.0;
    // 緯度35.56度付近での経度1度 ≒ 111320.0 * cos(35.56°) ≒ 90558.0m
    constexpr double LON_METERS = 111320.0 * 0.81352; 

    for (const auto& nd : node_table) {
        double dx = (lm.lm_lat - nd.nd_lat) * LAT_METERS; // メートル換算
        double dy = (lm.lm_lon - nd.nd_lon) * LON_METERS; // メートル換算
        double dis = (dx * dx) + (dy * dy);              // 平方メートル表記の距離

        if (dis < min_dis) {
            min_dis = dis;
            candidates.clear();
            candidates.push_back(nd.nd_id);
        } else if (dis == min_dis) {
            candidates.push_back(nd.nd_id);
        }
    }

    if (!candidates.empty()) {
        if (candidates.size() == 1) {
            lm.nearest_node_id = candidates[0];
        } else {
            std::uniform_int_distribution<size_t> dis_cand(0, candidates.size() - 1);
            lm.nearest_node_id = candidates[dis_cand(gen)];
        }
    }
}
int main() {
    std::random_device rd;
    std::mt19937 gen(rd());

/*
    // latlon.xlsx の実データ分布に基づいた各ブロックの出現重み（4x4 グリッド）
    // 行 index (0:南端 ? 3:北端) / 列 index (0:西端 ? 3:東端)
    */
    std::vector<double> weights = {
        //  col0,  col1,  col2,  col3
            4.0,   5.0,  34.0,   1.0,   // row 0 (南側: col2に最大密集エリア)
            6.0,   6.0,   7.0,   3.0,   // row 1 (中央南)
           15.0,   8.0,   6.0,   6.0,   // row 2 (中央北: col0にサブ密集エリア)
            3.0,   4.0,   3.0,   2.0    // row 3 (北側)
    };

    // 重みに対応したインデックス（0~15）を返す離散分布オブジェクト
    std::discrete_distribution<int> block_dist(weights.begin(), weights.end());
    std::vector<landmark> dummy(LANDMARK);
    std::vector<node> node_table;
    node_table.reserve(NODE);

    /*
    // nodes.csv の読み込み
    */
    std::ifstream fp_node("nodes.csv");
    if (!fp_node.is_open()) {
        std::cerr << "ファイル nodes.csv が開けませんでした" << std::endl;
        return EXIT_FAILURE;
    }

    std::string line;
    if (!std::getline(fp_node, line)) {
        std::cerr << "ファイル nodes.csv が空か、ヘッダーの読み込みに失敗しました" << std::endl;
        return EXIT_FAILURE;
    }

    while (std::getline(fp_node, line) && node_table.size() < NODE) {
        std::stringstream ss(line);
        std::string token;
        node nd;

        if (std::getline(ss, token, ',')) nd.nd_id = std::stod(token);
        if (std::getline(ss, token, ',')) nd.nd_lat = std::stod(token);
        if (std::getline(ss, token, ',')) nd.nd_lon = std::stod(token);

        node_table.push_back(nd);
    }
    fp_node.close();

    std::cout << "nodes.csvのデータを " << node_table.size() << " 件読み込みました" << std::endl;

    /*
       // 密集度を反映した autogen.csv の生成
       */
    std::ofstream fp_autogen("autogen.csv");
    if (!fp_autogen.is_open()) {
        std::cerr << "ファイル autogen.csv が開けませんでした" << std::endl;
        return EXIT_FAILURE;
    }

    fp_autogen << "id,name,feature,lat,lon,nearest_node\n";

    for (int i = 0; i < LANDMARK; ++i) {
        dummy[i].lm_id = i;
        dummy[i].name = 'a';

        // 密集確率付きで位置決め
        allocate_loc_grid(dummy[i], gen, block_dist);
        allocate_node(dummy[i], node_table, gen);

        fp_autogen << dummy[i].lm_id << ","
            << dummy[i].name << ","
            << dummy[i].type << ","
            << std::fixed << std::setprecision(6)
            << dummy[i].lm_lat << ","
            << dummy[i].lm_lon << ","
            << std::defaultfloat // 精度指定をクリアして整数表記に戻す
            << static_cast<long long>(dummy[i].nearest_node_id) << "\n";
    }

    fp_autogen.close();
    std::cout << "密集指定版 autogen.csv の出力が完了しました" << std::endl;

    return 0;
}