#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/bottom.h"
#include "puyobot/template/pattern.h"
#include "puyobot/template/bottom.h"
#include "puyobot/template/bottom_match.h"

void test_chain_of_fours() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    calculate_bottom_conflicts(template);
    print_bottom_template(template);
    State *state = min_state_from_bottom(template);
    print_state(state);
    BottomMatchResult result = match_bottom(state, template);
    print_bottom_match_result(result);
    assert(result.on_chain == result.all);
    free(state);
    free_bottom_template(template);
}

void test_match_result() {
    int num_colors = 3;
    puyos_t *floor = calloc(num_colors, sizeof(puyos_t));
    floor[0] = 3 | (3 << V_SHIFT);
    handle_bottom_gravity(floor, num_colors);
    floor[1] = 15 << 1;
    handle_bottom_gravity(floor, num_colors);
    floor[2] = 15 << 2;
    handle_bottom_gravity(floor, num_colors);
    BottomTemplate *template = template_from_floor(floor, num_colors);
    prepare_bottom_template(template, 0);
    print_bottom_template(template);
    State *state = any_state_from_bottom(template, 5, 10000);
    print_state(state);
    BottomMatchResult result = match_bottom(state, template);
    print_bottom_match_result(result);
    double score = bottom_match_score(template, result);
    printf("score = %f\n", score);

    assert(score > 0);

    free(state);
    free_bottom_template(template);
}

void test_random_match_result() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    prepare_bottom_template(template, 0);
    print_bottom_template(template);
    State *state = calloc(1, sizeof(State));
    blast_state(state, NUM_COLORS, 6 * CLEAR_THRESHOLD);
    resolve(state, NULL);
    print_state(state);
    BottomMatchResult result = match_bottom(state, template);
    print_bottom_match_result(result);
    double score = bottom_match_score(template, result);
    printf("score = %f\n", score);
    if (result.num_color_conflicts) {
        assert(score < 0);
    }
}

int main() {
    jkiss_init();
    init_tetrominoes();

    test_chain_of_fours();
    test_match_result();
    test_random_match_result();

    return 0;
}
