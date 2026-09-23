#include "ga.h"
#include "const.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_set>

namespace orienteering {

    // 個体と評価結果を保持する構造体
    struct EvaluatedIndividual {
        Chromosome chrom;
        EvalResult eval;
        double     fitness;
    };

    // ============================================================
    //  個体生成
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
    //  親選択
    // ============================================================
    const EvaluatedIndividual& select_parent(
        const std::vector<EvaluatedIndividual>& population,
        int                                     current_gen,
        int                                     t_switch_1,
        int                                     t_switch_2,
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
            if (population[idx].fitness <= best_fit) {
                best_fit = population[idx].fitness;
                best = idx;
            }
        }
        return population[best];
    }

    // ============================================================
    //  交叉アルゴリズム群
    // ============================================================
    void uniform_crossover_selection(
        const Chromosome& p1,
        const Chromosome& p2,
        int               N,
        RNG& rng,
        Chromosome& c1_out,
        Chromosome& c2_out)
    {
        std::uniform_real_distribution<double> ureal(0.0, 1.0);
        for (int i = 0; i < N; ++i) {
            if (ureal(rng) >= 0.5) {
                std::swap(c1_out[i], c2_out[i]);
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

    std::pair<Chromosome, Chromosome> crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        int               current_gen,
        int               select_switch_gen,
        int               order_switch_gen,
        RNG& rng)
    {
        Chromosome c1_select = parent1;
        Chromosome c2_select = parent2;
        Chromosome c1_order = parent1;
        Chromosome c2_order = parent2;

        if (current_gen < select_switch_gen) {
            uniform_crossover_selection(parent1, parent2, N, rng, c1_select, c2_select);
        }
        else {
            one_point_crossover_selection(parent1, parent2, N, rng, c1_select, c2_select);
        }

        if (current_gen < order_switch_gen) {
            order_crossover(parent1, parent2, N, rng, c1_order, c2_order);
        }
        else {
            cycle_crossover(parent1, parent2, N, c1_order, c2_order);
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
    //  突然変異
    // ============================================================
    void mutate(
        Chromosome& chromosome,
        int         N,
        double      prob_bit,
        double      prob_swap,
        RNG& rng)
    {
        std::uniform_real_distribution<double> ureal(0.0, 1.0);

        for (int i = 0; i < N; ++i) {
            if (ureal(rng) < prob_bit) {
                chromosome[i] = 1 - chromosome[i];
            }
        }

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
    //  GAメイン処理
    // ============================================================
    GAResult run_ga(
        const std::vector<Landmark>& landmarks,
        const PathCache& path_cache,
        long long         gate_node,
        RNG& rng)
    {
        const int N = static_cast<int>(landmarks.size());

        const double SCALE_FACTOR = 30.0;
        const int n_gen = BASE_N_GEN + static_cast<int>(SCALE_FACTOR * std::sqrt(N));

        const int select_switch_gen = static_cast<int>(n_gen * RATIO_SELECT_SWITCH);
        const int order_switch_gen = static_cast<int>(n_gen * RATIO_ORDER_SWITCH);

        const int t_switch_1 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_1);
        const int t_switch_2 = static_cast<int>(n_gen * RATIO_TOURNAMENT_SWITCH_2);

        const double prob_bit = 1.0 / static_cast<double>(N);
        const double prob_swap = 1.0 / static_cast<double>(MAX_CONTROLS);

        // 初期集団の生成と評価
        std::vector<EvaluatedIndividual> population(POP_SIZE);
        for (int i = 0; i < POP_SIZE; ++i) {
            population[i].chrom = create_random_chromosome(N, rng);
            population[i].eval = evaluate(population[i].chrom, landmarks, path_cache, gate_node);
            population[i].fitness = population[i].eval.fitness;
        }

        std::vector<double> best_history;
        best_history.reserve(n_gen);

        for (int gen = 1; gen <= n_gen; ++gen) {
            std::vector<EvaluatedIndividual> next_pop;
            next_pop.reserve(POP_SIZE);

            // エリート個体の抽出
            int elite_count = std::max(1, static_cast<int>(POP_SIZE * 0.01));
            std::vector<int> sorted_indices(POP_SIZE);
            std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
            std::partial_sort(sorted_indices.begin(),
                sorted_indices.begin() + elite_count,
                sorted_indices.end(),
                [&population](int a, int b) { return population[a].fitness < population[b].fitness; });

            // エリート個体の保存
            for (int e = 0; e < elite_count; ++e) {
                next_pop.push_back(population[sorted_indices[e]]);
            }

            // 次世代個体の生成
            while (static_cast<int>(next_pop.size()) < POP_SIZE) {
                const auto& p1 = select_parent(population, gen, t_switch_1, t_switch_2, rng);
                const auto& p2 = select_parent(population, gen, t_switch_1, t_switch_2, rng);

                auto children = crossover(p1.chrom, p2.chrom, N, gen, select_switch_gen, order_switch_gen, rng);

                mutate(children.first, N, prob_bit, prob_swap, rng);
                mutate(children.second, N, prob_bit, prob_swap, rng);

                // 子個体の評価
                EvaluatedIndividual c1;
                c1.chrom = std::move(children.first);
                c1.eval = evaluate(c1.chrom, landmarks, path_cache, gate_node);
                c1.fitness = c1.eval.fitness;
                next_pop.push_back(std::move(c1));

                if (static_cast<int>(next_pop.size()) < POP_SIZE) {
                    EvaluatedIndividual c2;
                    c2.chrom = std::move(children.second);
                    c2.eval = evaluate(c2.chrom, landmarks, path_cache, gate_node);
                    c2.fitness = c2.eval.fitness;
                    next_pop.push_back(std::move(c2));
                }
            }

            population = std::move(next_pop);

            // 最良評価値の記録
            double best = population[0].fitness;
            for (int i = 1; i < POP_SIZE; ++i) {
                if (population[i].fitness < best) {
                    best = population[i].fitness;
                }
            }
            best_history.push_back(best);

            std::cout << "  [世代 " << gen << "/" << n_gen << "]  best_fitness = " << best << std::endl;
        }

        int best_idx = 0;
        for (int i = 1; i < POP_SIZE; ++i) {
            if (population[i].fitness < population[best_idx].fitness) {
                best_idx = i;
            }
        }

        GAResult result;
        result.best_chromosome = population[best_idx].chrom;
        result.best_eval = population[best_idx].eval;
        result.best_fitness_history = std::move(best_history);
        return result;
    }

} // namespace orienteering