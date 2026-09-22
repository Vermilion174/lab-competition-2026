#include "ga.h"
#include "const.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>
#include <unordered_set>

namespace orienteering {

    // ============================================================
    // ランダムな染色体を生成
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
    // 親選択1：動的トーナメント選択（世代数に応じて k を変更）
    // ============================================================
    const Chromosome& tournament_select(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        int                            current_gen,
        RNG& rng)
    {
        // 世代数に応じてトーナメントサイズ k を3段階で決定
        int k = TOURNAMENT_SIZE_1;
        if (current_gen >= TOURNAMENT_SWITCH_GEN_2) {
            k = TOURNAMENT_SIZE_3;
        }
        else if (current_gen >= TOURNAMENT_SWITCH_GEN_1) {
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
    // 親選択2：確率に基づくランキング選択（比較・実験用に保持）
    // ============================================================
    const Chromosome& ranking_select(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        RNG& rng)
    {
        const int pop_size = static_cast<int>(population.size());

        std::vector<int> sorted_indices(pop_size);
        std::iota(sorted_indices.begin(), sorted_indices.end(), 0);
        std::sort(sorted_indices.begin(), sorted_indices.end(),
            [&fitnesses](int a, int b) {
                return fitnesses[a] < fitnesses[b];
            });

        const double total_rank_sum = static_cast<double>(pop_size * (pop_size + 1)) / 2.0;

        std::uniform_real_distribution<double> dist(0.0, total_rank_sum);
        double r = dist(rng);

        double current_sum = 0.0;
        for (int rank = 1; rank <= pop_size; ++rank) {
            int weight = pop_size - rank + 1;
            current_sum += weight;
            if (r <= current_sum) {
                int selected_idx = sorted_indices[rank - 1];
                return population[selected_idx];
            }
        }

        return population[sorted_indices[0]];
    }

    // ============================================================
    // 親選択メイン関数（世代数に応じて動的切り替え）
    // ============================================================
    const Chromosome& select_parent(
        const std::vector<Chromosome>& population,
        const std::vector<double>& fitnesses,
        int                            current_gen,
        RNG& rng)
    {
        // SELECT_METHOD_SWITCH_GEN = N_GEN * 1.0 のため、常にトーナメント選択を実行
        if (current_gen < SELECT_METHOD_SWITCH_GEN) {
            return tournament_select(population, fitnesses, current_gen, rng);
        }
        else {
            return ranking_select(population, fitnesses, rng);
        }
    }

    // ============================================================
    // 選択パート専用：1点交叉
    // ============================================================
    void one_point_crossover_selection(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        RNG& rng,
        Chromosome& child1,
        Chromosome& child2)
    {
        child1 = parent1;
        child2 = parent2;

        std::uniform_int_distribution<int> dist(1, N - 1);
        int cut_point = dist(rng);

        for (int i = cut_point; i < N; ++i) {
            child1[i] = parent2[i];
            child2[i] = parent1[i];
        }
    }

    // ============================================================
    // 順序パート専用：OX（順序交叉）
    // ============================================================
    void order_crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        RNG& rng,
        Chromosome& child1,
        Chromosome& child2)
    {
        child1 = parent1;
        child2 = parent2;

        std::uniform_int_distribution<int> dist_cut(0, MAX_CONTROLS);
        int a = dist_cut(rng);
        int b = dist_cut(rng);
        if (a > b) std::swap(a, b);

        auto ox_fill = [&](Chromosome& child,
            const Chromosome& parent_donor,
            const Chromosome& parent_filler) {
                std::vector<char> used(MAX_CONTROLS, 0);

                for (int i = a; i < b; ++i) {
                    int v = parent_donor[N + i];
                    child[N + i] = v;
                    used[v] = 1;
                }

                int pos = b % MAX_CONTROLS;
                for (int k = 0; k < MAX_CONTROLS; ++k) {
                    int v = parent_filler[N + (b + k) % MAX_CONTROLS];
                    if (used[v]) continue;
                    child[N + pos] = v;
                    used[v] = 1;
                    pos = (pos + 1) % MAX_CONTROLS;
                }
            };

        ox_fill(child1, parent1, parent2);
        ox_fill(child2, parent2, parent1);
    }

    // ============================================================
    // 順序パート専用：CX（循環交叉）
    // ============================================================
    void cycle_crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        Chromosome& child1,
        Chromosome& child2)
    {
        const int total_size = static_cast<int>(parent1.size());
        const int max_controls = total_size - N;

        child1 = parent1;
        child2 = parent2;

        std::vector<bool> visited(max_controls, false);
        std::vector<int>  val_to_pos2(max_controls);
        for (int i = 0; i < max_controls; ++i) {
            val_to_pos2[parent2[N + i]] = i;
        }

        int cycle_count = 0;

        for (int start_pos = 0; start_pos < max_controls; ++start_pos) {
            if (visited[start_pos]) continue;

            cycle_count++;
            int curr = start_pos;

            while (!visited[curr]) {
                visited[curr] = true;

                if (cycle_count % 2 == 0) {
                    child1[N + curr] = parent2[N + curr];
                    child2[N + curr] = parent1[N + curr];
                }

                curr = val_to_pos2[parent1[N + curr]];
            }
        }
    }

    // ============================================================
    // 順序パート専用：サブツアー交換交叉
    // ============================================================
    void subtour_exchange_crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        RNG& rng,
        Chromosome& child1,
        Chromosome& child2)
    {
        child1 = parent1;
        child2 = parent2;

        struct SubtourMatch {
            int p1_start, p1_len;
            int p2_start;
        };

        std::vector<SubtourMatch> matches;

        for (int len = 2; len < MAX_CONTROLS; ++len) {
            for (int i = 0; i <= MAX_CONTROLS - len; ++i) {
                std::unordered_set<int> set1;
                for (int k = 0; k < len; ++k) {
                    set1.insert(parent1[N + i + k]);
                }

                for (int j = 0; j <= MAX_CONTROLS - len; ++j) {
                    bool match = true;
                    for (int k = 0; k < len; ++k) {
                        if (set1.find(parent2[N + j + k]) == set1.end()) {
                            match = false;
                            break;
                        }
                    }

                    if (match) {
                        bool identical = true;
                        for (int k = 0; k < len; ++k) {
                            if (parent1[N + i + k] != parent2[N + j + k]) {
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
                child1[N + m.p1_start + k] = parent2[N + m.p2_start + k];
            }
            for (int k = 0; k < m.p1_len; ++k) {
                child2[N + m.p2_start + k] = parent1[N + m.p1_start + k];
            }
        }
        else {
            order_crossover(parent1, parent2, N, rng, child1, child2);
        }
    }

    // ============================================================
    // 交叉メイン関数（世代数 current_gen に応じて切り替え）
    // ============================================================
    std::pair<Chromosome, Chromosome> crossover(
        const Chromosome& parent1,
        const Chromosome& parent2,
        int               N,
        int               current_gen,
        RNG& rng)
    {
        Chromosome c1(N + MAX_CONTROLS);
        Chromosome c2(N + MAX_CONTROLS);

        // 1. 選択パート（0 ~ N-1）の動的切り替え
        if (current_gen < SELECT_SWITCH_GEN) {
            std::uniform_real_distribution<double> ureal(0.0, 1.0);
            for (int i = 0; i < N; ++i) {
                if (ureal(rng) < 0.5) {
                    c1[i] = parent1[i];
                    c2[i] = parent2[i];
                }
                else {
                    c1[i] = parent2[i];
                    c2[i] = parent1[i];
                }
            }
        }
        else {
            one_point_crossover_selection(parent1, parent2, N, rng, c1, c2);
        }

        // 2. 順序パート（N ~ N+MAX_CONTROLS-1）の動的切り替え
        Chromosome c1_order, c2_order;

        if (current_gen < ORDER_SWITCH_GEN_1) {
            order_crossover(parent1, parent2, N, rng, c1_order, c2_order);
        }
        else if (current_gen < ORDER_SWITCH_GEN_2) {
            cycle_crossover(parent1, parent2, N, c1_order, c2_order);
        }
        else {
            subtour_exchange_crossover(parent1, parent2, N, rng, c1_order, c2_order);
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
    void mutate(Chromosome& chromosome, int N, RNG& rng) {
        std::uniform_real_distribution<double> ureal(0.0, 1.0);

        for (int i = 0; i < N; ++i) {
            if (ureal(rng) < PROB_BIT) {
                chromosome[i] = 1 - chromosome[i];
            }
        }

        if (ureal(rng) < PROB_SWAP) {
            std::uniform_int_distribution<int> dist(N, N + MAX_CONTROLS - 1);
            int a = dist(rng);
            int b = dist(rng);
            while (b == a) b = dist(rng);
            std::swap(chromosome[a], chromosome[b]);
        }
    }

    // ============================================================
    // GA メインループ
    // ============================================================
    GAResult run_ga(
        const std::vector<Landmark>& landmarks,
        const PathCache& path_cache,
        long long                     gate_node,
        RNG& rng)
    {
        const int N = static_cast<int>(landmarks.size());

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
        best_history.reserve(N_GEN);

        for (int gen = 1; gen <= N_GEN; ++gen) {
            std::vector<Chromosome> next_pop;
            next_pop.reserve(POP_SIZE);

            int elite_idx = static_cast<int>(
                std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin());
            next_pop.push_back(population[elite_idx]);

            while (static_cast<int>(next_pop.size()) < POP_SIZE) {
                const Chromosome& p1 = select_parent(population, fitnesses, gen, rng);
                const Chromosome& p2 = select_parent(population, fitnesses, gen, rng);

                auto children = crossover(p1, p2, N, gen, rng);

                mutate(children.first, N, rng);
                mutate(children.second, N, rng);
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

            std::cout << "  [世代 " << gen << "]  best_fitness = " << best << std::endl;
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