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
        std::uniform_int_distribution<int> dist_n(MIN_CONTROLS, MAX_CONTROLS);
        int n_select = dist_n(rng);

        Chromosome chrom(N + MAX_CONTROLS, 0);

        std::vector<int> indices(N);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), rng);
        for (int i = 0; i < n_select; ++i) {
            chrom[indices[i]] = 1;
        }

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

    // 定常トーナメント選択（互換性のために保持）
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

    // 動的トーナメント選択（世代数・動的閾値に対応）
    const Chromosome& select_parent(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        int                            current_gen,
        int                            t_switch_1,
        int                            t_switch_2,
        RNG& rng)
    {
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
    // 交叉（Crossover）アルゴリズム群
    // ============================================================

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

    // 基本交叉（引数4つのオーバーロード / 一様交叉 + OX）
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

    // 動的切り替え交叉（メイン処理用）
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
    // 突然変異（Mutation）
    // ============================================================

    // 基本突然変異（引数3つのオーバーロード / 確率 PROB_BIT, PROB_SWAP 使用）
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

        // 1. 選択遺伝子のビット反転 (1/N の確率)
        for (int i = 0; i < N; ++i) {
            if (ureal(rng) < prob_bit) {
                chromosome[i] = 1 - chromosome[i];
            }
        }

        // 2. 順序遺伝子のスワップ（各位置において prob_swap の確率で他要素と交換）
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
    // ============================================================
    // GA メインループ
    // ============================================================
    GAResult run_ga(
        const std::vector<Landmark>& landmarks,
        const PathCache& path_cache,
        long long                    gate_node,
        RNG& rng)
    {
        const int N = static_cast<int>(landmarks.size());

        // ------------------------------------------------------------
        // ランドマーク数 N に応じた世代数 (n_gen) の設定
        // ------------------------------------------------------------
        const double SCALE_FACTOR = 30.0;
        const int n_gen = BASE_N_GEN + static_cast<int>(SCALE_FACTOR * std::sqrt(N));
        // ------------------------------------------------------------
        //  n_gen に応じた各手法の切り替え閾値の計算
        // ------------------------------------------------------------
        const int select_switch_gen = static_cast<int>(n_gen * RATIO_SELECT_SWITCH);
        const int order_switch_gen_1 = static_cast<int>(n_gen * RATIO_ORDER_SWITCH_1);
        const int order_switch_gen_2 = static_cast<int>(n_gen * RATIO_ORDER_SWITCH_2);

        const int t_switch_1 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_1);
        const int t_switch_2 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_2);

        // 突然変異率（ビット反転は 1/N に動的調整）
        const double prob_bit = 1.0 / static_cast<double>(N);
        const double prob_swap = 1.0 / static_cast<double>(MAX_CONTROLS);

        std::vector<Chromosome> population;
        population.reserve(POP_SIZE);
        for (int i = 0; i < POP_SIZE; ++i) {
            population.push_back(create_random_chromosome(N, rng));
        }

        std::vector<double> fitnesses(POP_SIZE);
        for (int i = 0; i < POP_SIZE; ++i) {
            fitnesses[i] = evaluate(population[i], landmarks, path_cache, gate_node).fitness;
        }

        std::vector<double> best_history;
        best_history.reserve(n_gen);

        for (int gen = 1; gen <= n_gen; ++gen) {
            std::vector<Chromosome> next_pop;
            next_pop.reserve(POP_SIZE);

            // エリート保存
            int elite_idx = static_cast<int>(
                std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin());
            next_pop.push_back(population[elite_idx]);

            while (static_cast<int>(next_pop.size()) < POP_SIZE) {
                // 親選択（動的閾値を渡す）
                const Chromosome& p1 = select_parent(population, fitnesses, gen, t_switch_1, t_switch_2, rng);
                const Chromosome& p2 = select_parent(population, fitnesses, gen, t_switch_1, t_switch_2, rng);

                // 交叉（動的閾値を渡す）
                auto children = crossover(p1, p2, N, gen,
                    select_switch_gen,
                    order_switch_gen_1,
                    order_switch_gen_2, rng);

                // 突然変異
                mutate(children.first, N, prob_bit, prob_swap, rng);
                mutate(children.second, N, prob_bit, prob_swap, rng);

                next_pop.push_back(std::move(children.first));
                if (static_cast<int>(next_pop.size()) < POP_SIZE) {
                    next_pop.push_back(std::move(children.second));
                }
            }

            population = std::move(next_pop);

            for (int i = 0; i < POP_SIZE; ++i) {
                fitnesses[i] = evaluate(population[i], landmarks, path_cache, gate_node).fitness;
            }

            double best = *std::min_element(fitnesses.begin(), fitnesses.end());
            best_history.push_back(best);

            std::cout << "  [世代 " << gen << "/" << n_gen << "]  best_fitness = " << best << std::endl;
        }

        int best_idx = static_cast<int>(
            std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin());

        GAResult result;
        result.best_chromosome = population[best_idx];
        result.best_eval = evaluate(result.best_chromosome, landmarks, path_cache, gate_node);
        result.best_fitness_history = std::move(best_history);
        return result;
    }

} // namespace orienteering