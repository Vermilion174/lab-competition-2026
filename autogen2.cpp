/*
// landmarkを任意個記述したcsvを出力するプログラム
// landmarkの個数による適応度の変化をみるために用いる
*/

#include "autogen2.h"

int main(void) {

    srand((unsigned int)time(NULL));

    landmark dummy[LANDMARK];
    node     node_table[NODE];

    FILE *fp_node;
    FILE *fp_autogen;

    /*
    // 最寄りnode_ID割り当てのために、nodes.csvのlatとlonを保持
    */
    fp_node = fopen("nodes.csv", "r");
    if (fp_node == NULL) {
        printf("ファイル nodes.csv が開けませんでした\n");
        exit(EXIT_FAILURE);
    }

    // 1行目のヘッダー（列名）を読み捨て
    char header_buf[256];
    if (fgets(header_buf, sizeof(header_buf), fp_node) == NULL) {
        printf("ファイル nodes.csv が空か、ヘッダーの読み込みに失敗しました\n");
        fclose(fp_node);
        exit(EXIT_FAILURE);
    }

    int i = 0;

    // nodes.csv の読み込み
    while (fscanf(fp_node, "%d,%lf,%lf", &node_table[i].nd_id, &node_table[i].nd_lat, &node_table[i].nd_lon) == 3) {
        i++;
        if (i >= NODE) {
            break;
        }
    }
    fclose(fp_node);

    printf("nodes.csvのデータを %d 件読み込みました\n", i);

    /*
    // ダミーのlandmarkを生成し、autogen.csvを出力
    */
    fp_autogen = fopen("autogen.csv", "w");
    if (fp_autogen == NULL) {
        printf("ファイル autogen.csv が開けませんでした\n");
        exit(EXIT_FAILURE);
    }

    // 列名書き込み
    fprintf(fp_autogen, "id,name,feature,lat,lon,nearest_node\n");

    for (i = 0; i < LANDMARK; i++) {
        dummy[i].lm_id = i; // ID割り当て
        sprintf(dummy[i].name, "dummy_%d", i); // dummy_iで一意の名前を割り当て

        allocate_feature(&dummy[i]);
        allocate_loc(&dummy[i]);
        allocate_nodes(&dummy[i], node_table);

        fprintf(fp_autogen, "%d,%s,%s,%.6f,%.6f,%d\n",
                dummy[i].lm_id,
                dummy[i].name,
                dummy[i].type,
                dummy[i].lm_lat,
                dummy[i].lm_lon,
                dummy[i].nearest_node_id);
    }

    fclose(fp_autogen);
    printf("autogen.csvの出力が完了しました\n");

    return 0;
}