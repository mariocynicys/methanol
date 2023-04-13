#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include "quads.hpp"
#include "parse.tab.hpp"
using namespace std;

// Used for syntax error reporting.
extern int yylineno;
extern char *yytext;
int syntax_errors = 0;
#define log_syntax                                                                                                    \
    {                                                                                                                 \
        cerr << "STX(N#" << ++syntax_errors << "): Invalid syntax near '" << yytext << "' in L#" << yylineno << endl; \
    }
#define semantic_error(msg)                                     \
    {                                                           \
        cerr << "SEM-E(L#" << yylineno << "): " << msg << endl; \
        abort();                                                \
    }
#define semantic_warning(msg)                                   \
    {                                                           \
        cerr << "SEM-W(L#" << yylineno << "): " << msg << endl; \
    }

// Output files.
string fout;
ofstream symlog;
ofstream quadout;
// Clear the output files and exit(1).
void abort()
{
    symlog.close();
    symlog.open(fout + ".sym");
    quadout.close();
    quadout.open(fout + ".quad");
    exit(1);
}

// Symbol table: Access the scope first then the identifier by name.
vector<map<string, struct Identifier *>> symtable(1);
// Controls the scope of variables.
int current_scope = 0;
void enter_scope()
{
    current_scope++;
    symtable.push_back(map<string, Identifier *>());
}

// Wrappers around a list of some type to not expose these stuff to LEX's C interface.
struct StringList
{
    vector<string> list;

    StringList(string item)
    {
        this->append(item);
    }

    StringList *append(string item)
    {
        list.push_back(item);
        return this;
    }
};

struct TypeList
{
    vector<yytokentype> list;

    TypeList(yytokentype item)
    {
        this->append(item);
    }

    TypeList()
    {
    }

    TypeList *append(yytokentype item)
    {
        list.push_back(item);
        return this;
    }
};

template <typename... Args>
string format(const char *templ, Args... args)
{
    char buff[500];
    snprintf(buff, sizeof(buff), templ, args...);
    return string(buff);
}

const char *token_name(yytokentype type)
{
    switch (type)
    {
    /* Datatypes */
    case LOGICAL:
        return "a logical";
    case INTEGER:
        return "an integer";
    case DOUBLE:
        return "a float";
    case STRING:
        return "a string";
    case ENUM_TYPE_DECLARATION:
        return "an enum";
    /* Operations */
    case PLUS:
        return "addition";
    case MINUS:
        return "subtraction";
    case MULT:
        return "multiplication";
    case DIV:
        return "division";
    case LT:
        return "inequality";
    case GT:
        return "inequality";
    case LTE:
        return "inequality";
    case GTE:
        return "inequality";
    case EQ:
        return "equality";
    case NE:
        return "equality";
    case AND:
        return "logical and";
    case OR:
        return "logical or";
    default:
        return "UNKNOWN";
    }
}

// The computed value of an expression.
union Value
{
    bool logical;
    int integer;
    double real;
    string *str;

    Value() {}
    Value(bool logical) { this->logical = logical; }
    Value(int integer) { this->integer = integer; }
    Value(double real) { this->real = real; }
    Value(char *str) { this->str = new string(str); }
};

// A class for the expressions of our program.
struct Expression
{
    // The type of the expression.
    yytokentype type;
    // Whether the expression evaluates to a known constant value.
    bool is_const;
    // The value of the expression, if known.
    Value value;
    // The name of the enum type, if the expression is an enum value.
    string enum_type_name;

    Expression(yytokentype type, bool is_const, Value value)
    {
        this->type = type;
        this->is_const = is_const;
        this->value = value;
    }

    // This overload defines enums.
    Expression(string enum_type_name)
    {
        // We are using `ENUM_TYPE_DECLARATION` as a flag for enum expressions.
        this->type = ENUM_TYPE_DECLARATION;
        this->is_const = false;
        this->enum_type_name = enum_type_name;
    }

    void warn_const_cond(string stmt)
    {
        if (this->type != LOGICAL)
            semantic_error(format("%s statement's condition is %s but it must be %s.", stmt.c_str(), token_name(this->type), token_name(LOGICAL)));
        string value = this->value.logical ? "true" : "false";
        if (this->is_const)
            semantic_warning(format("%s statement's condition is always %s.", stmt.c_str(), value.c_str()));
    }

    void warn_const_switch()
    {
        string value;
        if (this->is_num())
            value = this->type == INTEGER ? to_string((int)this->get_num()) : to_string(this->get_num());
        else if (this->type == STRING)
            value = *this->value.str;
        else
            semantic_error(format("Switch statement's condition is %s but it must be %s, %s or %.s.", token_name(this->type), token_name(INTEGER), token_name(DOUBLE), token_name(STRING)));
        if (this->is_const)
            semantic_warning(format("Switch statement's condition is always %s.", value.c_str()));
    }

    bool is_num()
    {
        return type == INTEGER || type == DOUBLE;
    }

    double get_num()
    {
        if (type == INTEGER)
            return value.integer;
        else if (type == DOUBLE)
            return value.real;
        else
            semantic_error(format("Cannot convert %s to number.", token_name(type)));
    }

    bool is_enum()
    {
        return type == ENUM_TYPE_DECLARATION;
    }

    Expression *neg()
    {
        if (type == INTEGER)
            this->value.integer = -this->value.integer;
        else if (type == DOUBLE)
            this->value.real = -this->value.real;
        else
            semantic_error(format("Cannot negate %s.", token_name(type)));
        return this;
    }

    Expression *complement()
    {
        if (type == LOGICAL)
            this->value.logical = !this->value.logical;
        else
            semantic_error(format("Cannot logically complement %s.", token_name(type)));
        return this;
    }

    Expression *oper(Expression *other, yytokentype op)
    {
        // If both expressions are constants, we can compute the result.
        // Otherwise, the result is garbage.
        this->is_const &= other->is_const;
        bool failed = false;
        if (false)
            ;
        else if (op == PLUS)
        {
            if (this->type == INTEGER && other->type == INTEGER) // No conversion needed.
                this->value.integer += other->value.integer;
            else if (this->type == DOUBLE && other->type == DOUBLE) // No conversion needed.
                this->value.real += other->value.real;
            else if (this->type == INTEGER && other->type == DOUBLE) // Convert the first to double.
            {
                q_popt();
                q_int2real();
                q_pusht();
                this->type = DOUBLE;
                this->value.real = this->value.integer + other->value.real;
            }
            else if (this->type == DOUBLE && other->type == INTEGER) // Convert the second to double.
            {
                q_int2real();
                this->value.real += other->value.integer;
            }
            else
                failed = true;
        }
        else if (op == MINUS)
        {
            if (this->type == INTEGER && other->type == INTEGER) // No conversion needed.
                this->value.integer -= other->value.integer;
            else if (this->type == DOUBLE && other->type == DOUBLE) // No conversion needed.
                this->value.real -= other->value.real;
            else if (this->type == INTEGER && other->type == DOUBLE) // Convert the first to double.
            {
                q_popt();
                q_int2real();
                q_pusht();
                this->type = DOUBLE;
                this->value.real = this->value.integer - other->value.real;
            }
            else if (this->type == DOUBLE && other->type == INTEGER) // Convert the second to double.
            {
                q_int2real();
                this->value.real -= other->value.integer;
            }
            else
                failed = true;
        }
        else if (op == MULT)
        {
            if (this->type == INTEGER && other->type == INTEGER) // No conversion needed.
                this->value.integer *= other->value.integer;
            else if (this->type == DOUBLE && other->type == DOUBLE) // No conversion needed.
                this->value.real *= other->value.real;
            else if (this->type == INTEGER && other->type == DOUBLE) // Convert the first to double.
            {
                q_popt();
                q_int2real();
                q_pusht();
                this->type = DOUBLE;
                this->value.real = this->value.integer * other->value.real;
            }
            else if (this->type == DOUBLE && other->type == INTEGER) // Convert the second to double.
            {
                q_int2real();
                this->value.real *= other->value.integer;
            }
            else
                failed = true;
        }
        else if (op == DIV)
        {
            if (this->type == INTEGER && other->type == INTEGER) // No conversion needed.
                this->value.integer /= other->value.integer;
            else if (this->type == DOUBLE && other->type == DOUBLE) // No conversion needed.
                this->value.real /= other->value.real;
            else if (this->type == INTEGER && other->type == DOUBLE) // Convert the first to double.
            {
                q_popt();
                q_int2real();
                q_pusht();
                this->type = DOUBLE;
                this->value.real = this->value.integer / other->value.real;
            }
            else if (this->type == DOUBLE && other->type == INTEGER) // Convert the second to double.
            {
                q_int2real();
                this->value.real /= other->value.integer;
            }
            else
                failed = true;
        }
        else
        {
            /* All the ones below will produce logical. */
            if (false)
                ;
            else if (op == LT)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() < other->get_num();
                else
                    failed = true;
            }
            else if (op == GT)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() > other->get_num();
                else
                    failed = true;
            }
            else if (op == LTE)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() <= other->get_num();
                else
                    failed = true;
            }
            else if (op == GTE)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() >= other->get_num();
                else
                    failed = true;
            }
            else if (op == EQ)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() == other->get_num();
                else if (this->type == STRING && other->type == STRING)
                    this->value.logical = *(this->value.str) == *(other->value.str);
                else if (this->is_enum() && other->is_enum())
                {
                    if (this->enum_type_name != other->enum_type_name)
                        semantic_error(format("Enum '%s' and '%s' are incompatible for comparison.", this->enum_type_name.c_str(), other->enum_type_name.c_str()));
                }
                else
                    failed = true;
            }
            else if (op == NE)
            {
                if (this->is_num() && other->is_num())
                    this->value.logical = this->get_num() != other->get_num();
                else if (this->type == STRING && other->type == STRING)
                    this->value.logical = *(this->value.str) != *(other->value.str);
                else if (this->is_enum() && other->is_enum())
                {
                    if (this->enum_type_name != other->enum_type_name)
                        semantic_error(format("Enum '%s' and '%s' are incompatible for comparison.", this->enum_type_name.c_str(), other->enum_type_name.c_str()));
                }
                else
                    failed = true;
            }
            else if (op == AND)
            {
                if (this->type == LOGICAL && other->type == LOGICAL)
                    this->value.logical = this->value.logical && other->value.logical;
                else
                    failed = true;
            }
            else if (op == OR)
            {
                if (this->type == LOGICAL && other->type == LOGICAL)
                    this->value.logical = this->value.logical || other->value.logical;
                else
                    failed = true;
            }
            if (!failed)
                this->type = LOGICAL;
        }
        if (failed)
            semantic_error(format("Operation %s cannot be performed between %s and %s.", token_name(op), token_name(this->type), token_name(other->type)));
        return this;
    }
};

// A class for the variables and functions of our program.
struct Identifier
{
    // The name of the identifier.
    string name;
    // The type of the variable, or return type for a function.
    yytokentype type;
    // The scope of the identifier.
    int scope;
    // The line number where the identifier was declared.
    int line;
    // Whether the identifier is a function.
    bool is_func;
    vector<yytokentype> func_params;
    // Whether the identifier is an enum type.
    bool is_enum_type;
    vector<string> enum_variants;
    // Whether the identifier is an enum value.
    bool is_enum_variant;
    string enum_type;
    // Whether the identifier has been used in the program.
    bool is_used;
    //  Whether the identifier has been initialized.
    bool is_initialized;
    // Whether the identifier is a constant.
    bool is_const;
    // We are only interested in the value of the identifier if it is a constant.
    Value value;

    // A constructor for variables identifiers.
    Identifier(
        string name,
        yytokentype type,
        bool is_initialized,
        bool is_const, Value value)
    {
        this->name = name;
        this->type = type;
        this->scope = current_scope;
        this->line = yylineno;
        this->is_used = false;

        this->is_initialized = is_initialized;
        this->is_const = is_const;
        this->value = value;

        // Identifier is not an enum.
        this->is_func = false;
        this->is_enum_type = false;
        this->is_enum_variant = false;
    }

    // This overload defines functions.
    Identifier(string name, yytokentype type, vector<yytokentype> func_params)
    {
        this->name = name;
        this->type = type;
        this->scope = current_scope;
        this->line = yylineno;
        this->is_used = false;

        this->is_func = true;
        this->func_params = func_params;

        this->is_enum_type = false;
        this->is_enum_variant = false;
    }

    // This overload defines enum types and enum identifiers.
    Identifier(string name, bool is_enum_type, vector<string> enum_variants, bool is_enum_variant, string enum_type)
    {
        this->name = name;
        this->type = ENUM_TYPE_DECLARATION;
        this->scope = current_scope;
        this->line = yylineno;
        this->is_used = false;

        this->is_func = false;

        this->is_enum_type = is_enum_type;
        this->enum_variants = enum_variants;

        this->is_enum_variant = is_enum_variant;
        this->enum_type = enum_type;
    }

    Expression *get_expr()
    {
        if (this->is_enum_variant)
        {
            return new Expression(this->enum_type);
        }
        return new Expression(this->type, this->is_const, this->value);
    }
};

Identifier *get_ident(string name, string expect)
{
    int scp = current_scope;
    do
        if (symtable[scp].find(name) != symtable[scp].end())
        {
            Identifier *id = symtable[scp][name];
            bool is_variable = !id->is_func && !id->is_enum_type;

            if (expect == "Function" && !id->is_func)
            {
                semantic_error(format("'%s' is not a function.", name.c_str()));
            }
            else if (expect == "Enum" && !id->is_enum_type)
            {
                semantic_error(format("'%s' is not a enum type.", name.c_str()));
            }
            else if (expect == "Variable" && !is_variable)
            {
                semantic_error(format("'%s' is not a variable.", name.c_str()));
            }
            return id;
        }
    while (scp--);
    semantic_error(format("%s '%s' has not been declared before.", expect.c_str(), name.c_str()));
}

int get_scope(string name)
{
    int scp = current_scope;
    do
        if (symtable[scp].find(name) != symtable[scp].end())
            return scp;
    while (scp--);
    return -1;
}

Identifier *func_identifier(string name, yytokentype type, TypeList *params)
{
    return new Identifier(name, type, params->list);
}

Identifier *var_identifier(string name, yytokentype type)
{
    return new Identifier(name, type, false, false, Value());
}

// Same as the one above but marks the variable as initialized.
Identifier *func_param_identifier(string name, yytokentype type)
{
    return new Identifier(name, type, true, false, Value());
}

Identifier *const_var_identifier(string name, yytokentype type, Expression *expr)
{
    if (expr->type != type)
        semantic_error(format("Type mismatch in constant declaration. Expected %s but got %s.", token_name(type), token_name(expr->type)));
    if (!expr->is_const)
        semantic_error(format("A non-constant expression doesn't have a compile-time known value."));
    return new Identifier(name, type, true, true, expr->value);
}

Identifier *enum_typ_identifier(string name, StringList *variants)
{
    return new Identifier(name, true, variants->list, false, "");
}

Identifier *enum_var_identifier(string name, string type)
{
    get_ident(type, "Enum"); // Make sure that this type has been declared before.
    return new Identifier(name, false, vector<string>(), true, type);
}

Expression *get_expr_for_variable(string name)
{
    Identifier *id = get_ident(name, "Variable");
    if (id->is_initialized == false)
        semantic_warning(format("Variable '%s' is being used without being initialized", name.c_str()));
    id->is_used = true;
    return id->get_expr();
}

Expression *get_expr_for_func_invocation(string name, struct TypeList *args)
{
    vector<yytokentype> arg_types = args->list;
    Identifier *id = get_ident(name, "Function");

    if (id->func_params.size() != arg_types.size())
        semantic_error(format("Function '%s' expects %d arguments, but %d were provided.", name.c_str(), id->func_params.size(), arg_types.size()));

    for (int i = 0; i < arg_types.size(); i++)
        if (id->func_params[i] != arg_types[i])
            semantic_error(format("Argument N#%d of function '%s' is %s, but %s was provided.", i + 1, name.c_str(), token_name(id->func_params[i]), token_name(arg_types[i])));

    id->is_used = true;
    return id->get_expr();
}

void assign_expr_to_variable(Expression *expr, string name)
{
    Identifier *id = get_ident(name, "Variable");
    if (id->is_const)
        semantic_error(format("Cannot assign to constant '%s'.", name.c_str()));

    // For enum variable assignment.
    if (id->get_expr()->is_enum() && expr->is_enum())
    {
        // Make sure they are of the same type.
        if (id->enum_type != expr->enum_type_name)
            semantic_error(format("'%s' of enum type '%s' cannot be assigned an expression of enum type '%s'.", id->name.c_str(), id->enum_type.c_str(), expr->enum_type_name.c_str()));
    }
    // Other assignments must be of the same type as well.
    else if (id->type != expr->type)
    {
        // But integers and reals are an exception.
        if (id->get_expr()->is_num() && expr->is_num())
        {
            if (id->type == DOUBLE) // convert expr to real
                q_int2real();
            else // convert expr to int
                q_real2int();
        }
        else
            semantic_error(format("Variable '%s' declared in L#%d of type %s can't be assigned %s.", id->name.c_str(), id->line, token_name(id->type), token_name(expr->type)));
    }
    q_popv(id->name);
    id->is_initialized = true;
    id->value = expr->value;
}

void declare_identifier(Identifier *id)
{
    if (symtable[current_scope].find(id->name) != symtable[current_scope].end())
        semantic_error(format("Identifier '%s' has already been declared in this scope in L#%d.", id->name.c_str(), symtable[current_scope][id->name]->line));
    symtable[current_scope][id->name] = id;
}

string check_and_get_static_enum_code(string enum_type, string enum_variant)
{
    Identifier *id = get_ident(enum_type, "Enum");
    for (string variant : id->enum_variants)
        if (variant == enum_variant)
        {
            id->is_used = true;
            return enum_type + "." + enum_variant;
        }
    semantic_error(format("Enum '%s' does not contain variant '%s'.", enum_type.c_str(), enum_variant.c_str()));
}

// Stores a stack of function return types to check them against return statements.
vector<pair<yytokentype, bool>> func_return_types_stack;
void push_func_ret_type(yytokentype type)
{
    func_return_types_stack.push_back({type, false});
}
void validate_return_type(Expression *expr)
{
    yytokentype curr_ret_type = func_return_types_stack[func_return_types_stack.size() - 1].first;
    if (curr_ret_type != expr->type)
        semantic_error(format("Return type mismatch. Expected %s, got %s.", token_name(curr_ret_type), token_name(expr->type)));
    func_return_types_stack[func_return_types_stack.size() - 1].second = true;
}
void check_return_included(string name)
{
    auto top = func_return_types_stack[func_return_types_stack.size() - 1];
    func_return_types_stack.pop_back();
    if (top.second == false)
        semantic_warning(format("Function '%s' doesn't return anything.", name.c_str(), token_name(top.first)))

    // Also add a return just for safety.
    if (top.first == INTEGER || top.first == LOGICAL)
        q_push(0);
    else if (top.first == DOUBLE)
        q_push(0.0);
    else if (top.first == STRING)
        q_pushs("");
    q_ret();
    q_newline();
}

// Like the above, but for switch statements.
vector<yytokentype> switch_cases_stack;
void push_switch_type(yytokentype type)
{
    switch_cases_stack.push_back(type);
}
void validate_case_type(Expression *expr)
{
    yytokentype case_type = switch_cases_stack[switch_cases_stack.size() - 1];
    if (case_type != expr->type)
        semantic_error(format("Case type mismatch. Expected %s, got %s.", token_name(case_type), token_name(expr->type)))
}
void pop_switch_type()
{
    switch_cases_stack.pop_back();
}

string padn(string s, int n)
{
    if (s.length() > n)
        return s.substr(0, n - 3) + "...";
    while (s.length() < n)
        s += " ";
    return s;
}
void log_symtable()
{
    symlog << "\t\t\t\t\t\t\t==================" << endl;
    symlog << "L#" << yylineno << ":" << endl;
    symlog << "Id. Name\t\tScope\tDec. Line\tIs Used\t\tIs Init.\tIs Const.\tValue" << endl;
    for (auto scope_map : symtable)
    {
        for (auto ident : scope_map)
        {
            auto id = ident.second;
            symlog << padn(id->name, 15) << "\t";
            symlog << id->scope << "\t\t\t";
            symlog << id->line << "\t\t\t";
            symlog << id->is_used << "\t\t\t";
            symlog << id->is_initialized << "\t\t\t";
            symlog << id->is_const << "\t\t";
            if (id->is_const)
            {
                if (id->type == INTEGER)
                    symlog << id->value.integer;
                else if (id->type == DOUBLE)
                    symlog << id->value.real;
                else if (id->type == STRING)
                    symlog << '"' << *id->value.str << '"';
                else if (id->type == LOGICAL)
                    symlog << (id->value.logical ? "true" : "false");
            }
            else
                symlog << "-";
            symlog << endl;
        }
    }
}
void leave_scope()
{
    map<string, struct Identifier *> top = symtable[current_scope];
    for (auto iden : top)
    {
        Identifier *id = iden.second;
        if (!id->is_used)
            semantic_warning(format("Identifier '%s' defined in L#%d has never been used.", id->name.c_str(), id->line));
    }
    current_scope--;
    symtable.pop_back();
}
