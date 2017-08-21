#include <assert.h>
#include <stdlib.h>

#include "jkiss/jkiss.h"

#include "puyobot/bottom.h"
#include "puyobot/template/pattern.h"
#include "puyobot/template/bottom.h"

void test_chain_of_fours() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    print_bottom_template(template);
    int chain = resolve_bottom(template->floor, template->num_colors, NULL);
    assert(chain == 5);
    for (int i = 0; i < template->num_colors; ++i) {
        assert(!template->floor[i]);
    }
    free_bottom_template(template);
}

void test_extend_chain() {
    BottomTemplate *template = calloc(1, sizeof(BottomTemplate));
    extend_bottom_chain(template, 0, 0);
    extend_bottom_chain(template, 0, 0);
    extend_bottom_chain(template, 0, 1);
    print_bottom_template(template);
    int chain = resolve_bottom(template->floor, template->num_colors, NULL);
    assert(template->trigger_front || chain == 3);
    free_bottom_template(template);
}

void test_tail_chain() {
    BottomTemplate *template = calloc(1, sizeof(BottomTemplate));
    tail_bottom_chain(template);
    tail_bottom_chain(template);
    tail_bottom_chain(template);
    print_bottom_template(template);
    int chain = resolve_bottom(template->floor, template->num_colors, NULL);
    assert(chain == 3);
    free_bottom_template(template);
}

void test_spam() {
    BottomTemplate *template = bottom_chain_of_fours(4);
    spam_bottom(template);
    print_bottom_template(template);
    puyos_t all = 0;
    for (int i = 0; i < template->num_colors; ++i) {
        all |= template->floor[i];
    }
    assert(all == FULL);
    free_bottom_template(template);
}

void test_sprinkle() {
    BottomTemplate *template = bottom_chain_of_fours(4);
    sprinkle_bottom(template);
    print_bottom_template(template);
    int chain = resolve_bottom(template->floor, template->num_colors, NULL);
    assert(chain == 4);
    free_bottom_template(template);
}

void test_min_state() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    calculate_bottom_conflicts(template);
    State *state = min_state_from_bottom(template);
    print_state(state);
    int chain;
    resolve(state, &chain);
    assert(chain == 5);
    free_bottom_template(template);
    free(state);
}

void test_any_state() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    calculate_bottom_conflicts(template);
    State *state = any_state_from_bottom(template, 5, 10000);
    print_state(state);
    int chain;
    resolve(state, &chain);
    assert(chain == 5);
    free_bottom_template(template);
    free(state);
}

void test_cut_trigger() {
    BottomTemplate *template = bottom_chain_of_fours(5);
    puyos_t cut = cut_bottom_trigger(template);
    print_bottom_template(template);
    print_puyos(cut);
    int chain = resolve_bottom(template->floor, template->num_colors, NULL);
    assert(!!cut ^ !!chain);
    free_bottom_template(template);
}

void test_conflicts() {
    BottomTemplate *template;
    while (1) {
        template = bottom_chain_of_fours(4);
        if(cut_bottom_trigger(template)) {
            break;
        }
        free_bottom_template(template);
    }
    spam_bottom_aura(template);
    calculate_bottom_conflicts(template);
    print_bottom_template(template);
    State *state = min_state_from_bottom(template);
    print_state(state);
    assert(!resolve(state, NULL));
    free_bottom_template(template);
    free(state);
}

int main() {
    jkiss_init();
    init_tetrominoes();

    test_chain_of_fours();
    test_extend_chain();
    test_tail_chain();
    test_spam();
    test_min_state();
    test_cut_trigger();
    test_conflicts();
    test_sprinkle();
    test_any_state();

    return 0;
}
