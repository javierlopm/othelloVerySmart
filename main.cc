// Game of Othello -- Example of main
// Universidad Simon Bolivar, 2012.
// Author: Blai Bonet
// Last Revision: 1/11/16
// Modified by: 

#include <iostream>
#include <climits>
#include "othello_cut.h"
#include "utils.h"

#include <unordered_map>

using namespace std;

unsigned expanded  = 0;
unsigned generated = 0;
int tt_threshold = 32; // threshold to save entries in TT

// Transposition table
struct stored_info_t {
    int value_;
    int type_;
    enum { EXACT, LOWER, UPPER };
    stored_info_t(int value = -100, int type = LOWER) : value_(value), type_(type) { }
};

struct hash_function_t {
    size_t operator()(const state_t &state) const {
        return state.hash();
    }
};

class hash_table_t : public unordered_map<state_t, stored_info_t, hash_function_t> {

};

hash_table_t TTable[2];

// int maxmin(state_t state, int depth, bool use_tt);
int minmax(state_t state, int depth, bool use_tt = false);
int maxmin(state_t state, int depth, bool use_tt = false);
int negamax(state_t state, int depth, int color, bool use_tt = false);
int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);
int scout(state_t state, int depth, int color, bool use_tt = false);
int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt = false);

int main(int argc, const char **argv) {
    state_t pv[128];
    int npv = 0;
    for( int i = 0; PV[i] != -1; ++i ) ++npv;

    int algorithm = 0;
    if( argc > 1 ) algorithm = atoi(argv[1]);
    bool use_tt = argc > 2;

    // Extract principal variation of the game
    state_t state;
    cout << "Extracting principal variation (PV) with " << npv << " plays ... " << flush;
    for( int i = 0; PV[i] != -1; ++i ) {
        bool player = i % 2 == 0; // black moves first!
        int pos = PV[i];
        pv[npv - i] = state;
        state = state.move(player, pos);
    }
    pv[0] = state;
    cout << "done!" << endl;

#if 0
    // print principal variation
    for( int i = 0; i <= npv; ++i )
        cout << pv[npv - i];
#endif

    // Print name of algorithm
    cout << "Algorithm: ";
    if( algorithm == 0 ) {
        cout << "Minmax-Maxmin";
    } else if( algorithm == 1 ) {
        cout << "Negamax (minmax version)";
    } else if( algorithm == 2 ) {
        cout << "Negamax (alpha-beta version)";
    } else if( algorithm == 3 ) {
        cout << "Scout";
    } else if( algorithm == 4 ) {
        cout << "Negascout";
    } else
        cout << "No-Algorithm";
    cout << (use_tt ? " w/ transposition table" : "") << endl;

    // Run algorithm along PV (bacwards)
    cout << "Moving along PV:" << endl;
    for( int i = 0; i <= npv; ++i ) {
        int value = 0;
        TTable[0].clear();
        TTable[1].clear();
        float start_time = Utils::read_time_in_seconds();
        expanded  = 0;
        generated = 0;
        int color = i % 2 == 1 ? 1 : -1;

        try {
            if( algorithm == 0 ) {
                value = color * (color == 1 ? maxmin(pv[i], i+1 , use_tt) : minmax(pv[i], i+1 , use_tt));
            } else if( algorithm == 1 ) {
                value = negamax(pv[i], i+1, color, use_tt);
            } else if( algorithm == 2 ) {
                value = negamax(pv[i], i+1, -200, 200, color, use_tt);
            } else if( algorithm == 3 ) {
                value = color * scout(pv[i], i+1, color, use_tt);
            } else if( algorithm == 4 ) {
                value = negascout(pv[i], 0, -200, 200, color, use_tt);
            }
        } catch( const bad_alloc &e ) {
            cout << "size TT[0]: size=" << TTable[0].size() << ", #buckets=" << TTable[0].bucket_count() << endl;
            cout << "size TT[1]: size=" << TTable[1].size() << ", #buckets=" << TTable[1].bucket_count() << endl;
            use_tt = false;
        }

        float elapsed_time = Utils::read_time_in_seconds() - start_time;

        cout << npv + 1 - i << ". " << (color == 1 ? "Black" : "White") << " moves: "
             << "value=" << color * value
             << ", #expanded=" << expanded
             << ", #generated=" << generated
             << ", seconds=" << elapsed_time
             << ", #generated/second=" << generated/elapsed_time
             << endl;
    }

    return 0;
}

int minmax(state_t state, int depth, bool use_tt){
    state_t aux_child;
    if (depth == 0 || state.terminal())
        return state.value();

    int score = INT_MAX;
    bool skip = true;
    for (int pos = 0; pos < DIM; ++pos) {
        if (state.is_white_move(pos)) {
            aux_child = state.white_move(pos);
            score     = min(score,maxmin(aux_child,depth-1,use_tt));
            skip = false;
            generated++;
        }
        
    }

    // No moves found, pass turn
    if (skip || score == INT_MAX ) {
        score = min(score,maxmin(state,depth-1,use_tt));
    }

    expanded++;
    return score;
}

int maxmin(state_t state, int depth, bool use_tt){
    state_t aux_child;
    if (depth == 0 || state.terminal())
        return state.value();

    int score = INT_MIN;
    bool skip = true;
    for (int pos = 0; pos < DIM; ++pos) {
        if (state.is_black_move(pos)) {
            aux_child = state.black_move(pos);
            score     = max(score,minmax(aux_child,depth-1,use_tt));
            skip = false;
            generated++;
        }
    }

    // No moves found, pass turn
    if (skip || score == INT_MIN ) {
        score = max(score,minmax(state,depth-1,use_tt));
    }
    expanded++;

    return score;
}

int negamax(state_t state, int depth, int color, bool use_tt){
    bool skip = true;
    state_t aux_child;
    if (depth == 0 || state.terminal())
        return color * state.value();

    int score = INT_MIN;
    bool isBlack = color==1;
    for (int pos = 0; pos < DIM; ++pos) {
        if ( (isBlack && state.is_black_move(pos))||(!isBlack && state.is_white_move(pos))) {
            aux_child = state.move(isBlack,pos);
            score     = max(score,-negamax(aux_child,depth-1,-color,use_tt));
            skip = false;
            generated++;
        }
    }

    // No moves found, pass turn
    if (skip || score == INT_MIN ) {
        score = max(score,-negamax(state,depth-1,-color,use_tt));
    }

    expanded++;
    return score;
}

int negamax(state_t state, int depth, int alpha, int beta, int color, bool use_tt){
    bool skip = true;
    state_t aux_child;
    bool isBlack = color==1;

    if (depth == 0 || state.terminal())
        return color * state.value();

    int score = INT_MIN;

    for (int pos = 0; pos < DIM; ++pos) {
        if ( (isBlack && state.is_black_move(pos))||(!isBlack && state.is_white_move(pos))) {
            aux_child = state.move(isBlack,pos);

            int val = -negamax(aux_child,depth-1,-beta,-alpha,-color,use_tt);

            score  = max(score,val);
            alpha  = max(alpha,val);
            skip = false;
            generated++;

            if (alpha >= beta) break;
        }
    }

    // No moves found, pass turn
    if (skip || score == INT_MIN ) {
        score = max(score,-negamax(state,depth-1,-beta,-alpha,-color,use_tt));
    }

    expanded++;
    return score;
}




int test(state_t state, int depth, int color, int score, bool gt){
    state_t aux_child;
    if (state.terminal() || depth == 0){
        if (gt) return state.value() > score;
        else    return state.value() >= score;
    }
    bool isBlack  = color==1;
    bool visited  = false;

    for (int pos = 0; pos < DIM; ++pos) {
        if ( (isBlack && state.is_black_move(pos))||(!isBlack && state.is_white_move(pos))) {
        	generated++;
            aux_child = state.move(color==1,pos);

            visited = true;

            if (isBlack  && test(aux_child,depth-1,-color,score,gt)) 
                return true;
            if(!isBlack && !test(aux_child,depth-1,-color,score,gt)) 
                return false;
        }
    }

    if (!visited){
        expanded++;
        return test(state,depth-1,-color,score,gt);
    }
    expanded++;
    return (!isBlack); 
}



int scout(state_t state, int depth, int color, bool use_tt){
    if (depth == 0 || state.terminal())
        return state.value();
    state_t aux_child;
    bool isBlack = color==1;
    int score = 0;
    bool is_first = true;
    bool visited  = false;

    for (int pos = 0; pos < DIM; ++pos) {
        if ( (isBlack && state.is_black_move(pos))||(!isBlack && state.is_white_move(pos))) {
            visited = true;
            generated++;
            aux_child = state.move(isBlack,pos);
            if (is_first) {
                score = scout(aux_child,depth-1,-color,use_tt);
                is_first = false;
                continue;
            }


            if (isBlack && test(aux_child,depth-1,-color,score,true)) 
                score = scout(aux_child,depth-1,-color,use_tt);
            if(!isBlack && !test(aux_child,depth-1,-color,score,false)) 
                score = scout(aux_child,depth-1,-color,use_tt);
        }
    }

    if (!visited ) 
        score = scout(state,depth-1,-color,use_tt);
    
    expanded++;
    return score; 
}


int negascout(state_t state, int depth, int alpha, int beta, int color, bool use_tt){
    if (state.terminal())
        return color * state.value();
    state_t aux_child;
    bool isBlack = color==1;
    bool is_first = true;
    bool visited  = false;
    int  score    = 0;

    for (int pos = 0; pos < DIM; ++pos) {
        if ( (isBlack && state.is_black_move(pos))||(!isBlack && state.is_white_move(pos))) {
            visited = true;
            generated++;
            aux_child = state.move(isBlack,pos);

            if (is_first) {
                score = -negascout(aux_child,depth-1,-beta,-alpha,-color,use_tt);
                is_first = false;
            }
            else {
                score = -negascout(aux_child,depth-1,-alpha-1,-alpha,-color,use_tt);

                if (alpha < score && score < beta) {
                    score = -negascout(aux_child,depth-1,-beta,-score,-color,use_tt);
                }
            }
            alpha = max(alpha,score);
            if (alpha >= beta)
                break;
        }
    }

    if (!visited )
        alpha = -negascout(state,depth-1,-beta,-alpha,-color,use_tt);

    expanded++;
    return alpha;

}
