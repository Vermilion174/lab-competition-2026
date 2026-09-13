# 進化計算コンペ2026　配布プログラム

## ファイル構成

```
orienteering-cpp-test/
├── const.h            # 共通定数（コントロール数の制約・目標値・GAパラメータ等）
├── csv_loader.h/cpp   # CSV ロード（landmarks/nodes/edges）
├── graph.h/cpp        # 道路ネットワーク + Dijkstra + 経路キャッシュ
├── evaluate.h/cpp     # 染色体デコード + 4目的関数 + 評価関数
├── ga.h/cpp           # GA 操作（選択・交叉・突然変異・メインループ）
├── main.cpp           # エントリポイント・入出力
├── Makefile           # ビルド設定（コマンドラインから make で利用）
├── input/             # 入力 CSV ファイル
│   ├── landmarks.csv
│   ├── nodes.csv
│   ├── edges.csv
│   └── seimon.csv
├── output/            # 実行結果出力先
│   ├── best_course.json
│   └── fitness_history.csv
└── README.md
```

### 入力データの形式（`input/`）
**`landmarks.csv`** … コントロール候補となる地点の一覧

| 列 | 意味 |
|---|---|
| `id` | 候補の通し番号 |
| `name` | 地点名 |
| `feature` | 種別（例：`post_office`） |
| `lat`, `lon` | 緯度・経度 |
| `nearest_node` | 最寄りの道路ノード ID（`nodes.csv` の `node_id` と対応） |

**`nodes.csv`** … 道路ネットワークの地点（交差点など）

| 列 | 意味 |
|---|---|
| `node_id` | ノードの一意な ID |
| `lat`, `lon` | 緯度・経度 |
| `elevation` | 標高（m） |

**`edges.csv`** … ノード間を結ぶ道（有向エッジ）

| 列 | 意味 |
|---|---|
| `from_node`, `to_node` | 始点・終点のノード ID |
| `length_m` | 道の長さ（m） |
| `elevation_change` | 標高差（+ 登り / − 下り） |
| `elevation_gain` | 累積登り（m） |

**`seimon.csv`** … スタート・ゴール地点（正門）の座標

| 列 | 意味 |
|---|---|
| `lat`, `lon` | 緯度・経度 |

先頭の有効な 1 行のみを使用する。

---

### 出力データ（`output/`）
- 中身は空
- 実行後、best_course.jsonとfitness_history.csvが作られる。


`SetConsoleOutputCP(CP_UTF8)` で対策済み。それでも化ける場合は、実行前にコンソールで `chcp 65001` を実行する。
