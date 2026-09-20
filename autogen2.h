/* =========================================================================
 *  landmarkを任意個記述したcsvを出力するプログラムのヘッダファイル
 *  landmarkの個数による適応度の変化をみるために用いる
 * ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <float.h>

#define LANDMARK 100  // 生成するlandmarkの個数
#define FEATURE 18    // 種別数 enumで定義した種別の個数と一致させる
#define NODE 2268     // nodes.csvの行数

// 緯度・経度の最大/最小値
#define MAX_LAT  35.57160986
#define MIN_LAT  35.55301084
#define MAX_LON 139.5881599
#define MIN_LON 139.5654312

// 種別（feature）の定義（Xマクロ）
#define FEATURE_LIST(X)\
    X(artwork)         \
    X(attraction)      \
    X(books)           \
    X(cafe)            \
    X(clotes)          \
    X(electronics)     \
    X(hospital)        \
    X(library)         \
    X(mail)            \
    X(park)            \
    X(playground)      \
    X(police)          \
    X(post_office)     \
    X(restaurant)      \
    X(school)          \
    X(supermarket)     \
    X(university)

//種別enumの定義
#define DEFINE_ENUM(name) name,
typedef enum{
    FEATURE_LIST(DEFINE_ENUM)
}feature;
#undef DEFINE_ENUM

//種別を文字列に変換するためのテーブル
#define DEFINE_STRING(name) #name,
const char *feature_name[] = {
    FEATURE_LIST(DEFINE_STRING)
};
#undef DEFINE_STRING

// landmark構造体の定義 landmarks.csvの列名に対応
typedef struct {
    int     lm_id;
    char    name[20];
    char    type[100];
    double  lm_lat;
    double  lm_lon;
    double  nearest_node_id;
} landmark;

// nodes.csvのデータを保持する構造体
typedef struct {
    double nd_id;
    double nd_lat;
    double nd_lon;
} node;

// 種別をランダムに割り当てる関数
void allocate_feature(landmark *p_lm) {
    int n;

    n = rand() % (FEATURE);

    strcpy( p_lm -> type, feature_name[n]);

    return;
}

// 緯度・経度をランダムに決める関数
void allocate_loc(landmark *p_lm) {

    double lat;
    double lon;

    lat = MIN_LAT + ((double)rand() / RAND_MAX) * (MAX_LAT - MIN_LAT);
    lon = MIN_LON + ((double)rand() / RAND_MAX) * (MAX_LON - MIN_LON);

    p_lm->lm_lat = lat;
    p_lm->lm_lon = lon;

    return;
}

// 最寄りのnode_IDをランダムに割り当てる関数
void allocate_node(landmark *p_lm, const node *p_nd) {

    int column = -1;
    int *cand = NULL;
    int num_cand = 2;
    double min_dis = DBL_MAX;

    for (int i = 0; i < NODE; i++) {
        double dx  = p_lm->lm_lat - p_nd[i].nd_lat;
        double dy  = p_lm->lm_lon - p_nd[i].nd_lon;
        double dis = (dx * dx) + (dy * dy);

        if (dis < min_dis) {
            min_dis = dis;
            column  = i;
            if (cand != NULL) {
                free(cand);
                cand = NULL;
                num_cand = 2;
            }
        }
        // 最寄りのnodeが複数あった場合の処理
        else if (dis == min_dis) {
            if (cand == NULL) {
                cand = (int *)malloc(sizeof(int) * num_cand);
                if (cand == NULL) exit(EXIT_FAILURE);
                cand[0] = p_nd[column].id;
                cand[1] = p_nd[i].id;
            }
            else {
                num_cand++;
                int *tmp = (int *)realloc(cand, sizeof(int) * num_cand);
                if (tmp == NULL) {
                    free(cand);
                    exit(EXIT_FAILURE);
                }
                cand = tmp;
                cand[num_cand - 1] = p_nd[i].id;
            }
        }
    }

    if (cand != NULL) {
        int nearest_node = rand() % num_cand;
        p_lm->nearest_node = cand[nearest_node];
        free(cand);
    }
    else if (column != -1) {
        p_lm->nearest_node = p_nd[column].id;
    }

    return;
}