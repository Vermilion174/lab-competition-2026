#include "ga.h"
#include "const.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_set>

namespace orienteering {

    // ============================================================
    // 個体生成
    // ============================================================
    Chromosome create_random_chromosome(int N, RNG& rng) {
        // 選択するコントロール数の決定
        std::uniform_int_distribution<int> dist_n(MIN_CONTROLS, MAX_CONTROLS);
        int n_select = dist_n(rng);

        Chromosome chrom(N + MAX_CONTROLS, 0);

        // ランドマーク選択ビットのランダム設定
        std::vector<int> indices(N);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        for (int i = 0; i < n_select; ++i) {
            chrom[indices[i]] = 1;
        }

        // 訪問順序遺伝子のランダム設定
        std::vector<int> order(MAX_CONTROLS);
        std::iota(order.begin(), order.end(), 0);
        std::shuffle(order.begin(), order.end(), rng);
        for (int i = 0; i < MAX_CONTROLS; ++i) {
            chrom[N + i] = order[i];
        }
        return chrom;
    }

    // ============================================================
    // 親選択
    // ============================================================

    // 定常トーナメント選択
    const Chromosome& tournament_select(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        RNG& rng)
    {
        std::uniform_int_distribution<int> dist(0, static_cast<int>(population.size()) - 1);
        int    best = -1;
        double best_fit = std::numeric_limits<double>::max();
        for (int i = 0; i < TOURNAMENT_SIZE_1; ++i) {
            int idx = dist(rng);
            if (fitnesses[idx] <= best_fit) {
                best_fit = fitnesses[idx];
                best = idx;
            }
        }
        return population[best];
    }

    // 世代数に応じた動的トーナメント選択
    const Chromosome& select_parent(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        int                            current_gen,
        int                            t_switch_1,
        int                            t_switch_2,
        RNG& rng)
    {
        // 世代進行に伴うトーナメントサイズ設定
        int k = TOURNAMENT_SIZE_1;
        if (current_gen >= t_switch_2) {
            k = TOURNAMENT_SIZE_3;
        }
        else if (current_gen >= t_switch_1) {
            k = TOURNAMENT_SIZE_2;
        }

        std::uniform_int_distribution<int> dist(0, static_cast<int>(population.size()) - 1);
        int    best = -1;
        double best_fit = std::numeric_limits<double>::max();
        for (int i = 0; i < k; ++i) {
            int idx = dist(rng);
            if (fitnesses[idx] <= best_fit) {
                best_fit = fitnesses[idx];
                best = idx;
            }
        }
        return population[best];
    }

    // ============================================================
    // 交叉アルゴリズム群
    // ============================================================

    // 一様交叉
    void uniform_crossover_selection(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        RNG& rng,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        c1_out = p1;
        c2_out = p2;
        std::uniform_real_distribution<double> ureal(0.0, 1.0);
        for (int i = 0; i < N; ++i) {
            if (ureal(rng) < 0.5) {
                c1_out[i] = p1[i];
                c2_out[i] = p2[i];
            }
            else {
                c1_out[i] = p2[i];
                c2_out[i] = p1[i];
            }
        }
    }

    // 一点交叉
    void one_point_crossover_selection(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        RNG& rng,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        c1_out = p1;
        c2_out = p2;
        std::uniform_int_distribution<int> dist(1, N - 1);
        int cut_point = dist(rng);

        for (int i = cut_point; i < N; ++i) {
            c1_out[i] = p2[i];
            c2_out[i] = p1[i];
        }
    }

    // 順序交叉
    void order_crossover(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        RNG& rng,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        c1_out = p1;
        c2_out = p2;

        std::uniform_int_distribution<int> dist_cut(0, MAX_CONTROLS);
        int a = dist_cut(rng);
        int b = dist_cut(rng);
        if (a > b) std::swap(a, b);

        // 部分区間の複製と残りの順序充填
        auto ox_fill = [&](Chromosome& child, const Chromosome& p_donor, const Chromosome& p_filler) {
            std::vector<char> used(MAX_CONTROLS, 0);

            for (int i = a; i < b; ++i) {
                int v = p_donor[N + i];
                child[N + i] = v;
                used[v] = 1;
            }

            int pos = b % MAX_CONTROLS;
            for (int k = 0; k < MAX_CONTROLS; ++k) {
                int v = p_filler[N + (b + k) % MAX_CONTROLS];
                if (used[v]) continue;
                child[N + pos] = v;
                used[v] = 1;
                pos = (pos + 1) % MAX_CONTROLS;
            }
            };

        ox_fill(c1_out, p1, p2);
        ox_fill(c2_out, p2, p1);
    }

    // 循環交叉
    void cycle_crossover(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        c1_out = p1;
        c2_out = p2;

        std::vector<bool> visited(MAX_CONTROLS, false);
        std::vector<int>  val_to_pos2(MAX_CONTROLS);
        for (int i = 0; i < MAX_CONTROLS; ++i) {
            val_to_pos2[p2[N + i]] = i;
        }

        // サイクルの抽出と交互割り当て
        int cycle_count = 0;
        for (int start_pos = 0; start_pos < MAX_CONTROLS; ++start_pos) {
            if (visited[start_pos]) continue;

            cycle_count++;
            int curr = start_pos;

            while (!visited[curr]) {
                visited[curr] = true;
                if (cycle_count % 2 == 0) {
                    c1_out[N + curr] = p2[N + curr];
                    c2_out[N + curr] = p1[N + curr];
                }
                curr = val_to_pos2[p1[N + curr]];
            }
        }
    }

    // 部分経路（サブツア）入れ替え交叉
    void subtour_exchange_crossover(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        RNG& rng,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        c1_out = p1;
        c2_out = p2;

        struct SubtourMatch {
            int p1_start, p1_len;
            int p2_start;
        };

        std::vector<SubtourMatch> matches;

        // 共通要素を持つ部分経路の検索
        for (int len = 2; len < MAX_CONTROLS; ++len) {
            for (int i = 0; i <= MAX_CONTROLS - len; ++i) {
                std::unordered_set<int> set1;
                for (int k = 0; k < len; ++k) {
                    set1.insert(p1[N + i + k]);
                }

                for (int j = 0; j <= MAX_CONTROLS - len; ++j) {
                    bool match = true;
                    for (int k = 0; k < len; ++k) {
                        if (set1.find(p2[N + j + k]) == set1.end()) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        bool identical = true;
                        for (int k = 0; k < len; ++k) {
                            if (p1[N + i + k] != p2[N + j + k]) {
                                identical = false;
                                break;
                            }
                        }
                        if (!identical) {
                            matches.push_back({ i, len, j });
                        }
                    }
                }
            }
        }

        // 一致部分の交換処理
        if (!matches.empty()) {
            std::uniform_int_distribution<int> dist_m(0, static_cast<int>(matches.size()) - 1);
            const auto& m = matches[dist_m(rng)];

            for (int k = 0; k < m.p1_len; ++k) {
                c1_out[N + m.p1_start + k] = p2[N + m.p2_start + k];
            }
            for (int k = 0; k < m.p1_len; ++k) {
                c2_out[N + m.p2_start + k] = p1[N + m.p1_start + k];
            }
        }
        else {
            order_crossover(p1, p2, N, rng, c1_out, c2_out);
        }
    }

    // 基本交叉
    std::pair<Chromosome, Chromosome> crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        RNG& rng)
    {
        Chromosome c1_select, c2_select;
        Chromosome c1_order, c2_order;

        uniform_crossover_selection(parent1, parent2, N, rng, c1_select, c2_select);
        order_crossover(parent1, parent2, N, rng, c1_order, c2_order);

        Chromosome c1(N + MAX_CONTROLS);
        Chromosome c2(N + MAX_CONTROLS);

        for (int i = 0; i < N; ++i) {
            c1[i] = c1_select[i];
            c2[i] = c2_select[i];
        }
        for (int i = 0; i < MAX_CONTROLS; ++i) {
            c1[N + i] = c1_order[N + i];
            c2[N + i] = c2_order[N + i];
        }

        return { c1, c2 };
    }

    // 動的切り替え交叉
    std::pair<Chromosome, Chromosome> crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        int               current_gen,
        int               select_switch_gen,
        int               order_switch_gen_1,
        int               order_switch_gen_2,
        RNG& rng)
    {
        Chromosome c1_select, c2_select;
        if (current_gen < select_switch_gen) {
            uniform_crossover_selection(parent1, parent2, N, rng, c1_select, c2_select);
        }
        else {
            one_point_crossover_selection(parent1, parent2, N, rng, c1_select, c2_select);
        }

        Chromosome c1_order, c2_order;
        if (current_gen < order_switch_gen_1) {
            order_crossover(parent1, parent2, N, rng, c1_order, c2_order);
        }
        else if (current_gen < order_switch_gen_2) {
            cycle_crossover(parent1, parent2, N, c1_order, c2_order);
        }
        else {
            subtour_exchange_crossover(parent1, parent2, N, rng, c1_order, c2_order);
        }

        Chromosome c1(N + MAX_CONTROLS);
        Chromosome c2(N + MAX_CONTROLS);

        for (int i = 0; i < N; ++i) {
            c1[i] = c1_select[i];
            c2[i] = c2_select[i];
        }
        for (int i = 0; i < MAX_CONTROLS; ++i) {
            c1[N + i] = c1_order[N + i];
            c2[N + i] = c2_order[N + i];
        }

        return { c1, c2 };
    }

    // ============================================================
    // 突然変異
    // ============================================================

    // 基本突然変異
    void mutate(Chromosome& chromosome, int N, RNG& rng) {
        mutate(chromosome, N, PROB_BIT, PROB_SWAP, rng);
    }

    // 確率指定突然変異
    void mutate(
        Chromosome& chromosome,
        int         N,
        double      prob_bit,
        double      prob_swap,
        RNG& rng)
    {
        std::uniform_real_distribution<double> ureal(0.0, 1.0);

        // 選択ビットの反転
        for (int i = 0; i < N; ++i) {
            if (ureal(rng) < prob_bit) {
                chromosome[i] = 1 - chromosome[i];
            }
        }

        // 順序遺伝子の入れ替え
        std::uniform_int_distribution<int> dist(N, N + MAX_CONTROLS - 1);
        for (int i = N; i < N + MAX_CONTROLS; ++i) {
            if (ureal(rng) < prob_swap) {
                int target = dist(rng);
                while (target == i && MAX_CONTROLS > 1) {
                    target = dist(rng);
                }
                std::swap(chromosome[i], chromosome[target]);
            }
        }
    }
    /*
    // ============================================================
    // 局所探索：山登り法
    // ============================================================
    Chromosome hill_climbing(
        Chromosome chrom,
        const std::vector<Landmark>& landmarks,
        const PathCache& path_cache,
        long long gate_node,
        int max_evals)
    {
        const int N = static_cast<int>(landmarks.size());

        // 有効な順序長の算出
        int active_len = 0;
        for (int i = 0; i < N; ++i) {
            if (chrom[i] == 1) active_len++;
        }

        if (active_len < 2) return chrom;

        active_len = std::min(active_len, MAX_CONTROLS);

        double current_fit = evaluate(chrom, landmarks, path_cache, gate_node).fitness;
        int evals = 0;
        bool improved = true;

        // 2-opt近傍探索による最適化
        while (improved && evals < max_evals) {
            improved = false;

            for (int i = 0; i < active_len - 1 && !improved && evals < max_evals; ++i) {
                for (int j = i + 1; j < active_len && !improved && evals < max_evals; ++j) {
                    Chromosome neighbor = chrom;

                    std::reverse(neighbor.begin() + N + i, neighbor.begin() + N + j + 1);

                    double neighbor_fit = evaluate(neighbor, landmarks, path_cache, gate_node).fitness;
                    evals++;

                    if (neighbor_fit < current_fit) {
                        chrom = std::move(neighbor);
                        current_fit = neighbor_fit;
                        improved = true;
                    }
                }
            }
        }

        return chrom;
    }
    */

    // ============================================================
    // GA メインループ
    // ============================================================
    GAResult run_ga(
        const std::vector<Landmark>& landmarks,
        const PathCache& path_cache,
        long long gate_node,
        RNG& rng)
    {
        const int N = static_cast<int>(landmarks.size());

        // 総世代数の算出
        const double SCALE_FACTOR = 30.0;
        const int n_gen = BASE_N_GEN + static_cast<int>(SCALE_FACTOR * std::sqrt(N));

        // 手法切り替え世代の算出
        const int select_switch_gen = static_cast<int>(n_gen * RATIO_SELECT_SWITCH);
        const int order_switch_gen_1 = static_cast<int>(n_gen * RATIO_ORDER_SWITCH_1);
        const int order_switch_gen_2 = static_cast<int>(n_gen * RATIO_ORDER_SWITCH_2);

        const int t_switch_1 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_1);
        const int t_switch_2 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_2);

        // 突然変異率の設定
        const double prob_bit = 1.0 / static_cast<double>(N);
        const double prob_swap = 1.0 / static_cast<double>(MAX_CONTROLS);

        /*
        // 山登り法の設定値
        const double TOP_RATIO_HC = 0.02;
        const int TOP_K_HC = std::max(1, static_cast<int>(POP_SIZE * TOP_RATIO_HC));
        const int HC_INTERVAL = 10;
        const int HC_MAX_EVALS = 150;
        */

        // 初期集団の生成
        std::vector<Chromosome> population;
        population.reserve(POP_SIZE);
        for (int i = 0; i < POP_SIZE; ++i) {
            population.push_back(create_random_chromosome(N, rng));
        }

        // 初期個体の評価
        std::vector<double> fitnesses(POP_SIZE);
        for (int i = 0; i < POP_SIZE; ++i) {
            fitnesses[i] = evaluate(population[i], landmarks, path_cache, gate_node).fitness;
        }

        std::vector<double> best_history;
        best_history.reserve(n_gen);

        // 世代交代ループ
        for (int gen = 1; gen <= n_gen; ++gen) {
            std::vector<Chromosome> next_pop;
            next_pop.reserve(POP_SIZE);

            // エリート数の計算（POP_SIZE の 1% 、最低でも 1 個体）
            int elite_count = std::max(1, static_cast<int>(POP_SIZE * 0.01));

            // 適応度順にインデックスをソート
            std::vector<int> sorted_indices(POP_SIZE);
            std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
            std::partial_sort(sorted_indices.begin(),
                sorted_indices.begin() + elite_count,
                sorted_indices.end(),
                [&fitnesses](int a, int b) { return fitnesses[a] < fitnesses[b]; });

            // 上位 1% のエリート個体を無条件で次世代へコピー
            for (int e = 0; e < elite_count; ++e) {
                next_pop.push_back(population[sorted_indices[e]]);
            }
            // 次世代個体の生成
            while (static_cast<int>(next_pop.size()) < POP_SIZE) {
                const Chromosome& p1 = select_parent(population, fitnesses, gen, t_switch_1, t_switch_2, rng);
                const Chromosome& p2 = select_parent(population, fitnesses, gen, t_switch_1, t_switch_2, rng);

                auto children = crossover(p1, p2, N, gen,
                    select_switch_gen,
                    order_switch_gen_1,
                    order_switch_gen_2, rng);

                mutate(children.first, N, prob_bit, prob_swap, rng);
                mutate(children.second, N, prob_bit, prob_swap, rng);

                next_pop.push_back(std::move(children.first));
                if (static_cast<int>(next_pop.size()) < POP_SIZE) {
                    next_pop.push_back(std::move(children.second));
                }
            }

            population = std::move(next_pop);

            // 全個体の評価
            for (int i = 0; i < POP_SIZE; ++i) {
                fitnesses[i] = evaluate(population[i], landmarks, path_cache, gate_node).fitness;
            }
            /*
            // 山登り法の実行領域
            if (gen % HC_INTERVAL == 0 || gen == n_gen) {
                std::vector<int> sorted_indices(POP_SIZE);
                std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
                std::sort(sorted_indices.begin(), sorted_indices.end(),
                    [&fitnesses](int a, int b) { return fitnesses[a] < fitnesses[b]; });

                for (int k = 0; k < TOP_K_HC; ++k) {
                    int idx = sorted_indices[k];
                    population[idx] = hill_climbing(population[idx], landmarks, path_cache, gate_node, HC_MAX_EVALS);
                    fitnesses[idx] = evaluate(population[idx], landmarks, path_cache, gate_node).fitness;
                }
            }
            */
            // 最良適応度の記録と出力
            double best = *std::min_element(fitnesses.begin(), fitnesses.end());
            best_history.push_back(best);

            std::cout << "  [世代 " << gen << "/" << n_gen << "]  best_fitness = " << best << std::endl;
        }

        // 最終結果の抽出
        int best_idx = static_cast<int>(
            std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin());

        GAResult result;
        result.best_chromosome = population[best_idx];
        result.best_eval = evaluate(result.best_chromosome, landmarks, path_cache, gate_node);
        result.best_fitness_history = std::move(best_history);
        return result;
    }

} // namespace orienteering