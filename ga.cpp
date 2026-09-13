#include "ga.h"
#include "const.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <numeric>

namespace orienteering {

// ============================================================
// ランダムな染色体を生成
// ============================================================
Chromosome create_random_chromosome(int N, RNG& rng) {
    std::uniform_int_distribution<int> dist_n(MIN_CONTROLS, MAX_CONTROLS);
    int n_select = dist_n(rng);

    Chromosome chrom(N + MAX_CONTROLS, 0);

    // 選択パート：N個の候補からランダムに n_select 個を選ぶ
    std::vector<int> indices(N);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng);
    for (int i = 0; i < n_select; ++i) {
        chrom[indices[i]] = 1;
    }

    // 順序パート：0〜MAX_CONTROLS-1 のランダム順列
    std::vector<int> order(MAX_CONTROLS);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    for (int i = 0; i < MAX_CONTROLS; ++i) {
        chrom[N + i] = order[i];
    }
    return chrom;
}

// ============================================================
// トーナメント選択
// ============================================================
const Chromosome& tournament_select(
    const std::vector<Chromosome>& population,
    const std::vector<double>&     fitnesses,
    RNG&                           rng)
{
    std::uniform_int_distribution<int> dist(0, static_cast<int>(population.size()) - 1);
    int    best     = -1;
    double best_fit = std::numeric_limits<double>::max();
    for (int i = 0; i < TOURNAMENT_SIZE; ++i) {
        int idx = dist(rng);
        if (fitnesses[idx] <= best_fit) {
            best_fit = fitnesses[idx];
            best     = idx;
        }
    }
    return population[best];
}

// ============================================================
// 交叉
//   選択パート：一様交叉
//   順序パート：OX（順序交叉／順列を保存）
// ============================================================
std::pair<Chromosome, Chromosome> crossover(
    const Chromosome& parent1,
    const Chromosome& parent2,
    int               N,
    RNG&              rng)
{
    Chromosome c1(N + MAX_CONTROLS);
    Chromosome c2(N + MAX_CONTROLS);

    std::uniform_real_distribution<double> ureal(0.0, 1.0);

    // 選択パート：各ビットを 50% で入れ替え
    for (int i = 0; i < N; ++i) {
        if (ureal(rng) < 0.5) {
            c1[i] = parent1[i];
            c2[i] = parent2[i];
        } else {
            c1[i] = parent2[i];
            c2[i] = parent1[i];
        }
    }

    // 順序パート：OX（順序交叉）
    std::uniform_int_distribution<int> dist_cut(0, MAX_CONTROLS);
    int a = dist_cut(rng);
    int b = dist_cut(rng);
    if (a > b) std::swap(a, b);

    auto ox_fill = [&](Chromosome&       child,
                       const Chromosome& parent_donor,
                       const Chromosome& parent_filler) {
        std::vector<char> used(MAX_CONTROLS, 0);

        for (int i = a; i < b; ++i) {
            int v         = parent_donor[N + i];
            child[N + i]  = v;
            used[v]       = 1;
        }

        int pos = b % MAX_CONTROLS;
        for (int k = 0; k < MAX_CONTROLS; ++k) {
            int v = parent_filler[N + (b + k) % MAX_CONTROLS];
            if (used[v]) continue;
            child[N + pos] = v;
            used[v]        = 1;
            pos = (pos + 1) % MAX_CONTROLS;
        }
    };

    ox_fill(c1, parent1, parent2);
    ox_fill(c2, parent2, parent1);

    return {c1, c2};
}

// ============================================================
// 突然変異
//   選択パート：各ビットを PROB_BIT の確率で反転
//   順序パート：PROB_SWAP の確率で2点をスワップ
// ============================================================
void mutate(Chromosome& chromosome, int N, RNG& rng) {
    std::uniform_real_distribution<double> ureal(0.0, 1.0);

    // ビット反転
    for (int i = 0; i < N; ++i) {
        if (ureal(rng) < PROB_BIT) {
            chromosome[i] = 1 - chromosome[i];
        }
    }

    // 順序パートのスワップ
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
    const PathCache&             path_cache,
    long long                    gate_node,
    RNG&                         rng)
{
    const int N = static_cast<int>(landmarks.size());

    // 初期個体群
    std::vector<Chromosome> population;
    population.reserve(POP_SIZE);
    for (int i = 0; i < POP_SIZE; ++i) {
        population.push_back(create_random_chromosome(N, rng));
    }

    // 初期評価
    std::vector<double> fitnesses(POP_SIZE);
    for (int i = 0; i < POP_SIZE; ++i) {
        fitnesses[i] = evaluate(population[i], landmarks, path_cache, gate_node).fitness;
    }

    std::vector<double> best_history;
    best_history.reserve(N_GEN);

    for (int gen = 1; gen <= N_GEN; ++gen) {
        std::vector<Chromosome> next_pop;
        next_pop.reserve(POP_SIZE);

        // エリート保存：最良個体を1つそのまま次世代へ
        int elite_idx = static_cast<int>(
            std::min_element(fitnesses.begin(), fitnesses.end()) - fitnesses.begin());
        next_pop.push_back(population[elite_idx]);

        // 残りは選択・交叉・突然変異で生成
        while (static_cast<int>(next_pop.size()) < POP_SIZE) {
            const Chromosome& p1 = tournament_select(population, fitnesses, rng);
            const Chromosome& p2 = tournament_select(population, fitnesses, rng);
            auto children = crossover(p1, p2, N, rng);
            mutate(children.first,  N, rng);
            mutate(children.second, N, rng);
            next_pop.push_back(std::move(children.first));
            if (static_cast<int>(next_pop.size()) < POP_SIZE) {
                next_pop.push_back(std::move(children.second));
            }
        }

        population = std::move(next_pop);

        // 新世代を評価
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
    result.best_chromosome      = population[best_idx];
    result.best_eval            = evaluate(result.best_chromosome, landmarks, path_cache, gate_node);
    result.best_fitness_history = std::move(best_history);
    return result;
}

} // namespace orienteering
