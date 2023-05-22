// This file contains macros for writing to the quad file.

// General.
#define q_push(arg) quadout << "\tPUSH " << arg << endl
#define q_pop() quadout << "\tPOP" << endl
#define q_pushs(str) quadout << "\tPUSH " << quoted(str) << endl
#define q_pushv(name) quadout << "\tPUSH v_" << name << get_scope(name) << endl
#define q_popv(name) quadout << "\tPOP v_" << name << get_scope(name) << endl
#define q_popt() quadout << "\tPOP " << "tmp" << endl
#define q_pusht() quadout << "\tPUSH " << "tmp" << endl
#define q_dupexpr() quadout << "\tDUP" << endl

#define q_int2real() quadout << "\tINT2REAL" << endl
#define q_real2int() quadout << "\tREAL2INT" << endl

#define q_funcdef(name, scp) quadout << "\tJMP fend_" << name << scp << endl << "DEF f_" << name << scp << ":" << endl
#define q_funcall(name) quadout << "\tCALL f_" << name << get_scope(name) << endl
#define q_ret() quadout << "\tRET" << endl
#define q_endfunc(name) quadout << "LABEL fend_" << name << get_scope(name) << ":" << endl

#define q_print() quadout << "\tPRINT" << endl

// Operations.
#define q_neg() quadout << "\tNEG" << endl
#define q_plus() quadout << "\tPLUS" << endl
#define q_minus() quadout << "\tMINUS" << endl
#define q_mult() quadout << "\tMULT" << endl
#define q_div() quadout << "\tDIV" << endl

#define q_lt() quadout << "\tLT" << endl
#define q_gt() quadout << "\tGT" << endl
#define q_lte() quadout << "\tLTEQ" << endl
#define q_gte() quadout << "\tGTEQ" << endl
#define q_eq() quadout << "\tEQ" << endl
#define q_ne() quadout << "\tNEQ" << endl

#define q_and() quadout << "\tAND" << endl
#define q_or() quadout << "\tOR" << endl
#define q_not() quadout << "\tNOT" << endl

// A label counter to control the jumps (branches, loops, etc...).
// It's a map to account for scopes (each scope has its own label counter).
std::map<int, int> lbls;

#define lbl lbls[current_scope]
#define print_lbl " s" << current_scope << "_l" << lbl

#define q_if() lbl++; quadout << "\tJZ" << print_lbl << endl
#define q_else() lbl++; quadout << "\tJMP" << print_lbl << endl << "LABEL" << print_lbl - 1 << ":" << endl
#define q_endif() quadout << "LABEL" << print_lbl << ":" << endl; q_end("if")

#define q_while() lbl++; quadout << "LABEL" << print_lbl << ":" << endl
#define q_checkwhile() lbl++; quadout << "\tJZ" << print_lbl << endl
#define q_endwhile() quadout << "\tJMP" << print_lbl - 1 << endl << "LABEL" << print_lbl << ":" << endl; q_end("while")

#define q_repeat() lbl++; quadout << "LABEL" << print_lbl << ":" << endl
#define q_endrepeat() quadout << "\tJZ" << print_lbl << endl; q_end("repeat")

#define q_for() lbl++; quadout << "LABEL" << print_lbl << ":" << endl
#define q_checkfor() quadout << "\tJZ" << print_lbl + 3 << endl << "\tJMP" << print_lbl + 2 << endl << "LABEL" << print_lbl + 1 << ":" << endl
#define q_forback() quadout << "\tJMP" << print_lbl << endl << "LABEL" << print_lbl + 2 << ":" << endl
#define q_endfor() quadout << "\tJMP" << print_lbl + 1 << endl << "LABEL" << print_lbl + 3 << ":" << endl; lbl += 3; q_end("for")

// A stack of labels to get out of a switch statement.
// This is necessary because we don't know how many cases a switch statement has.
std::vector<std::string> switch_stack;
#define last_switch_lbl switch_stack[switch_stack.size() - 1]

#define q_switch() lbl++; switch_stack.push_back(" s" + std::to_string(current_scope) + "_l" + std::to_string(lbl))
#define q_casecheck() lbl++; quadout << "\tEQ" << endl << "\tJZ" << print_lbl << endl
#define q_endcase() quadout << "\tJMP" << last_switch_lbl << endl << "LABEL" << print_lbl << ":" << endl
#define q_endswitch() quadout << "LABEL" << last_switch_lbl << ":" << endl; switch_stack.pop_back(); q_pop(); q_end("switch")


#define q_start(name) quadout << endl << endl << "/* " << name << " statement */" << endl
#define q_end(name) quadout << "/* " << name << " statement */" << endl << endl