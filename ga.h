#ifndef GA_H
#define GA_H

#include "csv_loader.h"
#include "graph.h"
#include "evaluate.h"

#include <random>
#include <utility>
#include <vector>
#include <cmath>

namespace orienteering {

using RNG = std::mt19937;

// ============================================================
// GA 操作（個体生成・選択・交叉・突然変異）
// ============================================================

// ランダムな染色体を生成（初期個体群用）
Chromosome create_random_chromosome(int N, RNG& rng);

// トーナメント選択
const Chromosome& tournament_select(
    const std::vector<Chromosome>& population,
    const std::vector<double>&     fitnesses,
    RNG&                           rng);

// 交叉（選択パート：一様交叉 / 順序パート：OX＝順序交叉）
std::pair<Chromosome, Chromosome> crossover(
    const Chromosome& parent1,
    const Chromosome& parent2,
    int               N,
    RNG&              rng);

// 突然変異（選択パート：ビット反転 / 順序パート：2点スワップ）
void mutate(Chromosome& chromosome, int N, RNG& rng);


// ============================================================
//  交叉関連
// ============================================================

// --- 選択パートの交叉アルゴリズム ---
void uniform_crossover_selection(
    const Chromosome& p1,
    const Chromosome& p2,
    int               N,
    RNG& rng,
    Chromosome& c1_out,
    Chromosome& c2_out);

void one_point_crossover_selection(
    const Chromosome& p1,
    const Chromosome& p2,
    int               N,
    RNG& rng,
    Chromosome& c1_out,
    Chromosome& c2_out);

// --- 順序パートの交叉アルゴリズム ---
void order_crossover(
    const Chromosome& p1,
    const Chromosome& p2,
    int               N,
    RNG& rng,
    Chromosome& c1_out,
    Chromosome& c2_out);

void cycle_crossover(
    const Chromosome& p1,
    const Chromosome& p2,
    int               N,
    Chromosome& c1_out,
    Chromosome& c2_out);

void subtour_exchange_crossover(
    const Chromosome& p1,
    const Chromosome& p2,
    int               N,
    RNG& rng,
    Chromosome& c1_out,
    Chromosome& c2_out);

// --- 交叉メイン関数 ---
std::pair<Chromosome, Chromosome> crossover(
    const Chromosome& parent1,
    const Chromosome& parent2,
    int               N,
    int               current_gen,
    int               select_switch_gen,
    int               order_switch_gen_1,
    int               order_switch_gen_2,
    RNG& rng);

// ============================================================
//  突然変異関連
// ============================================================

void mutate(
    Chromosome& chromosome,
    int         N,
    double      prob_bit,
    double      prob_swap,
    RNG& rng);


// ============================================================
// GA メインループ
// ============================================================

struct GAResult {
    Chromosome           best_chromosome;
    EvalResult           best_eval;
    std::vector<double>  best_fitness_history;
};

GAResult run_ga(
    const std::vector<Landmark>& landmarks,
    const PathCache&             path_cache,
    long long                    gate_node,
    RNG&                         rng);

} // namespace orienteering

#endif // GA_H
