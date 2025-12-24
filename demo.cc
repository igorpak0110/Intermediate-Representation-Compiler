#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include "compiler.h"
#include "lexer.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

LexicalAnalyzer lexer;
unordered_map<string, int> var_location;

InstructionNode* parse_program();
void parse_var_section();
void parse_id_list();
int get_var_location(string name);
ArithmeticOperatorType parse_op();
int parse_primary();
InstructionNode* parse_assign_stmt();
ConditionalOperatorType parse_relop();
InstructionNode* parse_stmt();
InstructionNode* parse_stmt_list();
InstructionNode* parse_body();
InstructionNode* parse_if_stmt();
InstructionNode* parse_while_stmt();
InstructionNode* parse_switch_stmt();
InstructionNode* parse_for_stmt();
InstructionNode* parse_input_stmt();
InstructionNode* parse_output_stmt();
void parse_inputs();

InstructionNode* parse_program(){
    parse_var_section();
    InstructionNode* body = parse_body();
    parse_inputs();
    return body;
}

void parse_var_section(){
    parse_id_list();
    Token token = lexer.GetToken();
    if (token.token_type != SEMICOLON){
        cout << "Error: Missing semicolon at line " << token.line_no << "\n";
        exit(1);
    }
}

int get_var_location(string name){
    if (var_location.count(name) == 0){
        var_location[name] = next_available;
        next_available++;
    }
    return var_location[name];
}

void parse_id_list(){
    Token token = lexer.GetToken();
    if (token.token_type != ID) {
        cout << "Error: Expected identifier at line " << token.line_no << "\n";
        exit(1);
    }
    get_var_location(token.lexeme);
    Token next = lexer.peek(1);
    if (next.token_type == COMMA){
        lexer.GetToken();
        parse_id_list();
    }
}

ArithmeticOperatorType parse_op(){
    Token token = lexer.GetToken();
    switch (token.token_type){
        case PLUS:
            return OPERATOR_PLUS;
        case MINUS:
            return OPERATOR_MINUS;
        case MULT:
            return OPERATOR_MULT;
        case DIV:
            return OPERATOR_DIV;
        default:
            cout << "Error: Invalid arithmetic operator\n";
            exit(1);
    }
}

int parse_primary(){
    Token token = lexer.GetToken();
    if (token.token_type == ID){
        return get_var_location(token.lexeme);
    }
    else if (token.token_type == NUM){
        int address = next_available;
        mem[address] = stoi(token.lexeme);
        next_available++;
        return address;
    }
    else {
        cout << "Error: Expected identifier or number at line " << token.line_no << "\n";
        exit(1);
    }
}

InstructionNode* parse_assign_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != ID){
        cout << "Error: Expected identifier at line " << token.line_no << "\n";
        exit(1);
    }
    int leftHandSide = get_var_location(token.lexeme);
    token = lexer.GetToken();
    if (token.token_type != EQUAL) {
        cout << "Error: Expected '=' at line " << token.line_no << "\n";
        exit(1);
    }
    int op1 = parse_primary();
    ArithmeticOperatorType op = OPERATOR_NONE;
    int op2 = -1;

    token = lexer.peek(1);
    if (token.token_type == PLUS || token.token_type == MINUS ||
        token.token_type == MULT || token.token_type == DIV){
        
        op = parse_op();
        op2 = parse_primary();
    }

    token = lexer.GetToken();
    if (token.token_type != SEMICOLON){
        cout << "Error: Missing semicolon at line " << token.line_no << "\n";
        exit(1);
    }
    
    InstructionNode* node = new InstructionNode;
    node->type = ASSIGN;
    node->assign_inst.left_hand_side_index = leftHandSide;
    node->assign_inst.operand1_index = op1;
    node->assign_inst.operand2_index = op2;
    node->assign_inst.op = op;
    node->next = NULL;
    return node;
}

ConditionalOperatorType parse_relop(){
    Token token = lexer.GetToken();
    switch (token.token_type){
        case LESS:
            return CONDITION_LESS;
        case GREATER:
            return CONDITION_GREATER;
        case NOTEQUAL:
            return CONDITION_NOTEQUAL;
        default:
            cout << "Error: Invalid relational operator at line " << token.line_no << "\n";
            exit(1);
    }
}

InstructionNode* parse_stmt(){
    Token token = lexer.peek(1);
    if (token.token_type == ID)
        return parse_assign_stmt();
    else if (token.token_type == WHILE)
        return parse_while_stmt();
    else if (token.token_type == IF)
        return parse_if_stmt();
    else if (token.token_type == SWITCH)
        return parse_switch_stmt();
    else if (token.token_type == FOR)
        return parse_for_stmt();
    else if (token.token_type == OUTPUT)
        return parse_output_stmt();
    else if (token.token_type == INPUT)
        return parse_input_stmt();
    else {
        cout << "Error: Unexpected token at line " << token.line_no << "\n";
        exit(1);
    }
}

InstructionNode* parse_stmt_list(){
    InstructionNode* stmt = parse_stmt();
    Token token = lexer.peek(1);
    if (token.token_type == ID || token.token_type == WHILE ||
        token.token_type == IF || token.token_type == SWITCH ||
        token.token_type == FOR || token.token_type == OUTPUT ||
        token.token_type == INPUT){

        InstructionNode* rest = parse_stmt_list();
        InstructionNode* current = stmt;
        while(current->next != NULL){
            current = current->next;
        }
        current->next = rest;
    }
    return stmt;
}

InstructionNode* parse_body(){
    Token token = lexer.GetToken();
    if (token.token_type != LBRACE){
        cout << "Error: Expected '{' at line " << token.line_no << "\n";
        exit(1);
    }
    InstructionNode* stmt_list = parse_stmt_list();
    token = lexer.GetToken();
    if (token.token_type != RBRACE){
        cout << "Error: Expected '}' at line " << token.line_no << "\n";
        exit(1);
    }
    return stmt_list;
}

InstructionNode* parse_if_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != IF){
        cout << "Error: Expected 'if' at line " << token.line_no << "\n";
        exit(1);
    }
    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();
    InstructionNode* jump = new InstructionNode;
    jump->type = CJMP;
    jump->cjmp_inst.condition_op = relop;
    jump->cjmp_inst.operand1_index = op1;
    jump->cjmp_inst.operand2_index = op2;

    InstructionNode* body = parse_body();
    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = NULL;

    InstructionNode* current = body;
    while(current->next != NULL){
        current = current->next;
    }
    current->next = noop;

    jump->cjmp_inst.target = noop;
    jump->next = body;

    return jump;
}

InstructionNode* parse_while_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != WHILE){
        cout << "Error: Expected 'while' at line " << token.line_no << "\n";
        exit(1);
    }
    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();
    InstructionNode* cond = new InstructionNode;
    cond->type = CJMP;
    cond->cjmp_inst.condition_op = relop;
    cond->cjmp_inst.operand1_index = op1;
    cond->cjmp_inst.operand2_index = op2;

    InstructionNode* body = parse_body();
    InstructionNode* jump = new InstructionNode;
    jump->type = JMP;
    jump->jmp_inst.target = cond;

    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = NULL;

    InstructionNode* current = body;
    while(current->next != NULL){
        current = current->next;
    }
    current->next = jump;
    jump->next = noop;

    cond->next = body;
    cond->cjmp_inst.target = noop;

    return cond;
}

InstructionNode* parse_switch_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != SWITCH){
        cout << "Error: Expected 'switch' at line " << token.line_no << "\n";
        exit(1);
    }

    token = lexer.GetToken();
    if (token.token_type != ID){
        cout << "Error: Expected identifier at line " << token.line_no << "\n";
        exit(1);
    }
    int switch_var_loc = get_var_location(token.lexeme);

    token = lexer.GetToken();
    if (token.token_type != LBRACE){
        cout << "Error: Expected '{' at line " << token.line_no << "\n";
        exit(1);
    }

    vector<InstructionNode*> case_cjmps;
    vector<InstructionNode*> case_bodies;
    InstructionNode* defaultBody = NULL;
    token = lexer.peek(1);
    while (token.token_type == CASE){
        lexer.GetToken();
        token = lexer.GetToken();
        if (token.token_type != NUM){
            cout << "Error: Expected number at line " << token.line_no << "\n";
            exit(1);
        }
        int case_value_loc = next_available;
        mem[case_value_loc] = stoi(token.lexeme);
        next_available++;

        token = lexer.GetToken();
        if (token.token_type != COLON){
            cout << "Error: Expected ':' at line " << token.line_no << "\n";
            exit(1);
        }

        InstructionNode* body = parse_body();
        InstructionNode* cjmp = new InstructionNode;
        cjmp->type = CJMP;
        cjmp->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
        cjmp->cjmp_inst.operand1_index = switch_var_loc;
        cjmp->cjmp_inst.operand2_index = case_value_loc;
        cjmp->cjmp_inst.target = body;
        cjmp->next = NULL;
        case_cjmps.push_back(cjmp);
        case_bodies.push_back(body);
        token = lexer.peek(1);
    }

    token = lexer.peek(1);
    if (token.token_type == DEFAULT){
        lexer.GetToken();
        token = lexer.GetToken();
        if (token.token_type != COLON){
            cout << "Error: Expected ':' at line " << token.line_no << "\n";
            exit(1);
        }
        defaultBody = parse_body();
    }

    token = lexer.GetToken();
    if (token.token_type != RBRACE){
        cout << "Error: Expected '}' at line " << token.line_no << "\n";
        exit(1);
    }

    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = NULL;

    for (InstructionNode* body : case_bodies){
        InstructionNode* tail = body;
        while(tail->next != NULL){
            tail = tail->next;
        }
        InstructionNode* jump = new InstructionNode;
        jump->type = JMP;
        jump->jmp_inst.target = noop;
        jump->next = NULL;
        tail->next = jump;
    }

    if (defaultBody != NULL){
        InstructionNode* tail = defaultBody;
        while(tail->next != NULL){
            tail = tail->next;
        }
        tail->next = noop;
    }

    int n = case_cjmps.size();
    for (int i = 0; i < n; i++){
        if (i+1 < n){
            case_cjmps[i]->next = case_cjmps[i+1];
        }
        else {
            if (defaultBody != NULL){
                case_cjmps[i]->next = defaultBody;
            }
            else {
                case_cjmps[i]->next = noop;
            }
        }
    }

    if (n>0)
        return case_cjmps[0];
    else if (defaultBody != NULL)
        return defaultBody;
    else
        return noop;
}

InstructionNode* parse_for_stmt(){
    lexer.GetToken();

    if (lexer.GetToken().token_type != LPAREN){
        cout << "Error: Expected '('\n";
        exit(1);
    }

    InstructionNode* assign_stmt1 = parse_assign_stmt();

    int op1 = parse_primary();
    ConditionalOperatorType relop = parse_relop();
    int op2 = parse_primary();

    if (lexer.GetToken().token_type != SEMICOLON) {
        cout << "Error: Expected ';' after condition\n";
        exit(1);
    }

    InstructionNode* cond = new InstructionNode;
    cond->type = CJMP;
    cond->cjmp_inst.operand1_index = op1;
    cond->cjmp_inst.operand2_index = op2;
    cond->cjmp_inst.condition_op = relop;

    InstructionNode* assign_stmt2 = parse_assign_stmt();

    if (lexer.GetToken().token_type != RPAREN){
        cout << "Error: Expected ')'\n";
        exit(1);
    }

    InstructionNode* body = parse_body();
    InstructionNode* noop = new InstructionNode;
    noop->type = NOOP;
    noop->next = NULL;

    InstructionNode* last = assign_stmt1;
    while (last->next){
        last = last->next;
    }
    last->next = cond;
    cond->next = body;
    cond->cjmp_inst.target = noop;

    InstructionNode* lastBody = body;
    while(lastBody->next){
        lastBody = lastBody->next;
    }
    lastBody->next = assign_stmt2;

    InstructionNode* lastAssignStmt2 = assign_stmt2;
    while(lastAssignStmt2->next){
        lastAssignStmt2 = lastAssignStmt2->next;
    }

    InstructionNode* jumpBack = new InstructionNode;
    jumpBack->type = JMP;
    jumpBack->jmp_inst.target = cond;
    jumpBack->next = noop;
    lastAssignStmt2->next = jumpBack;
    return assign_stmt1;
}

InstructionNode* parse_input_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != INPUT){
        cout << "Error: Expected 'input' at line " << token.line_no << "\n";
        exit(1);
    }
    token = lexer.GetToken();
    if (token.token_type != ID){
        cout << "Error: Expected identifier at line " << token.line_no << "\n";
        exit(1);
    }
    int loc = get_var_location(token.lexeme);
    token = lexer.GetToken();
    if (token.token_type != SEMICOLON){
        cout << "Error: Missing semicolon at line " << token.line_no << "\n";
        exit(1);
    }
    InstructionNode* node = new InstructionNode;
    node->type = IN;
    node->input_inst.var_index = loc;
    node->next = NULL;
    return node;
}

InstructionNode* parse_output_stmt(){
    Token token = lexer.GetToken();
    if (token.token_type != OUTPUT){
        cout << "Error: Expected 'output' at line " << token.line_no << "\n";
        exit(1);
    }
    token = lexer.GetToken();
    if (token.token_type != ID){
        cout << "Error: Expected identifier at line " << token.line_no << "\n";
        exit(1);
    }
    int loc = get_var_location(token.lexeme);
    token = lexer.GetToken();
    if (token.token_type != SEMICOLON){
        cout << "Error: Missing semicolon at line " << token.line_no << "\n";
        exit(1);
    }
    InstructionNode* node = new InstructionNode;
    node->type = OUT;
    node->output_inst.var_index = loc;
    node->next = NULL;
    return node;
}

void parse_inputs() {
    Token token = lexer.GetToken();
    if (token.token_type != NUM) {
        cout << "Error: Expected NUM in input list\n";
        exit(1);
    }
    inputs.push_back(stoi(token.lexeme));

    Token next = lexer.peek(1);
    while (next.token_type == NUM) {
        token = lexer.GetToken();
        inputs.push_back(stoi(token.lexeme));
        next = lexer.peek(1);
    }
}

InstructionNode* parse_generate_intermediate_representation(){
    return parse_program();
}

// struct InstructionNode *parse_generate_intermediate_representation()
// {
//      // Sample program for demonstration purpose only
//      // Replace the following with a call to a parser that reads the
//      // program from stdin & creates appropriate data structures to be
//      // executed by execute_program()
//      // This is the supposed input for the following construction:

//      // a, b, c, d;
//      // {
//      //     input a;
//      //     input b;
//      //     c = 10;
//      //
//      //     IF c <> a
//      //     {
//      //         output b;
//      //     }
//      //
//      //     IF c > 1
//      //     {
//      //         a = b + 900;
//      //         input d;
//      //         IF a > 10
//      //         {
//      //             output d;
//      //         }
//      //     }
//      //
//      //     d = 0;
//      //     WHILE d < 4
//      //     {
//      //         c = a + d;
//      //         IF d > 1
//      //         {
//      //             output d;
//      //         }
//      //         d = d + 1;
//      //     }
//      // }
//      // 1 2 3 4 5 6

//      // Assigning location for variable "a"
//      int address_a = next_available;
//      mem[next_available] = 0; // next_available is a global variable that is decalred in
//      next_available++;        // execute.cc

//      // Assigning location for variable "b"
//      int address_b = next_available;
//      mem[next_available] = 0;
//      next_available++;

//      // Assigning location for variable "c"
//      int address_c = next_available;
//      mem[next_available] = 0;
//      next_available++;

//      // Assigning location for variable "d"
//      int address_d = next_available;
//      mem[next_available] = 0;
//      next_available++;

//      // Assigning location for constant 10
//      int address_ten = next_available;
//      mem[next_available] = 10;
//      next_available++;

//      // Assigning location for constant 1
//      int address_one = next_available;
//      mem[next_available] = 1;
//      next_available++;

//      // Assigning location for constant 900
//      int address_ninehundred = next_available;
//      mem[next_available] = 900;
//      next_available++;

//      // Assigning location for constant 3
//      int address_three = next_available;
//      mem[next_available] = 3;
//      next_available++;

//      // Assigning location for constant 0
//      int address_zero = next_available;
//      mem[next_available] = 0;
//      next_available++;

//      // Assigning location for constant 4
//      int address_four = next_available;
//      mem[next_available] = 4;
//      next_available++;

//      struct InstructionNode *i1 = new InstructionNode;
//      struct InstructionNode *i2 = new InstructionNode;
//      struct InstructionNode *i3 = new InstructionNode;
//      struct InstructionNode *i4 = new InstructionNode;
//      struct InstructionNode *i5 = new InstructionNode;
//      struct InstructionNode *i6 = new InstructionNode;
//      struct InstructionNode *i7 = new InstructionNode;
//      struct InstructionNode *i8 = new InstructionNode;
//      struct InstructionNode *i9 = new InstructionNode;
//      struct InstructionNode *i10 = new InstructionNode;
//      struct InstructionNode *i11 = new InstructionNode;
//      struct InstructionNode *i12 = new InstructionNode;
//      struct InstructionNode *i13 = new InstructionNode;
//      struct InstructionNode *i14 = new InstructionNode;
//      struct InstructionNode *i15 = new InstructionNode;
//      struct InstructionNode *i16 = new InstructionNode;
//      struct InstructionNode *i17 = new InstructionNode;
//      struct InstructionNode *i18 = new InstructionNode;
//      struct InstructionNode *i19 = new InstructionNode;
//      struct InstructionNode *i20 = new InstructionNode;
//      struct InstructionNode *i21 = new InstructionNode;
//      struct InstructionNode *i22 = new InstructionNode;

//      i1->type = IN; // input a
//      i1->input_inst.var_index = address_a;
//      i1->next = i2;

//      i2->type = IN; // input b
//      i2->input_inst.var_index = address_b;
//      i2->next = i3;

//      i3->type = ASSIGN; // c = 10
//      i3->assign_inst.left_hand_side_index = address_c;
//      i3->assign_inst.op = OPERATOR_NONE;
//      i3->assign_inst.operand1_index = address_ten;
//      i3->next = i4;

//      i4->type = CJMP; // if c <> a
//      i4->cjmp_inst.condition_op = CONDITION_NOTEQUAL;
//      i4->cjmp_inst.operand1_index = address_c;
//      i4->cjmp_inst.operand2_index = address_a;
//      i4->cjmp_inst.target = i6; // if not (c <> a) skip forward to NOOP
//      i4->next = i5;

//      i5->type = OUT; // output b
//      i5->output_inst.var_index = address_b;
//      i5->next = i6;

//      i6->type = NOOP; // NOOP after IF
//      i6->next = i7;

//      i7->type = CJMP; // if c > 1
//      i7->cjmp_inst.condition_op = CONDITION_GREATER;
//      i7->cjmp_inst.operand1_index = address_c;
//      i7->cjmp_inst.operand2_index = address_one;
//      i7->cjmp_inst.target = i13; // if not (c > 1) skip forward to NOOP (way down)
//      i7->next = i8;

//      i8->type = ASSIGN; // a = b + 900
//      i8->assign_inst.left_hand_side_index = address_a;
//      i8->assign_inst.op = OPERATOR_PLUS;
//      i8->assign_inst.operand1_index = address_b;
//      i8->assign_inst.operand2_index = address_ninehundred;
//      i8->next = i9;

//      i9->type = IN; // input d
//      i9->input_inst.var_index = address_d;
//      i9->next = i10;

//      i10->type = CJMP; // if a > 10
//      i10->cjmp_inst.condition_op = CONDITION_GREATER;
//      i10->cjmp_inst.operand1_index = address_a;
//      i10->cjmp_inst.operand2_index = address_ten;
//      i10->cjmp_inst.target = i12; // if not (a > 10) skipp forward to NOOP
//      i10->next = i11;

//      i11->type = OUT; // output d
//      i11->output_inst.var_index = address_d;
//      i11->next = i12;

//      i12->type = NOOP; // NOOP after inner IF
//      i12->next = i13;

//      i13->type = NOOP; // NOOP after outer IF
//      i13->next = i14;

//      i14->type = ASSIGN; // d = 0
//      i14->assign_inst.left_hand_side_index = address_d;
//      i14->assign_inst.op = OPERATOR_NONE;
//      i14->assign_inst.operand1_index = address_zero;
//      i14->next = i15;

//      i15->type = CJMP; // if d < 4
//      i15->cjmp_inst.condition_op = CONDITION_LESS;
//      i15->cjmp_inst.operand1_index = address_d;
//      i15->cjmp_inst.operand2_index = address_four;
//      i15->cjmp_inst.target = i22; // if not (d < 4) skip whole WHILE body
//      i15->next = i16;

//      i16->type = ASSIGN; // c = a + d
//      i16->assign_inst.left_hand_side_index = address_c;
//      i16->assign_inst.op = OPERATOR_PLUS;
//      i16->assign_inst.operand1_index = address_a;
//      i16->assign_inst.operand2_index = address_d;
//      i16->next = i17;

//      i17->type = CJMP; // if d > 1
//      i17->cjmp_inst.condition_op = CONDITION_GREATER;
//      i17->cjmp_inst.operand1_index = address_d;
//      i17->cjmp_inst.operand2_index = address_one;
//      i17->cjmp_inst.target = i19; // if not (d > 1) skip body of IF
//      i17->next = i18;

//      i18->type = OUT; // output d
//      i18->output_inst.var_index = address_d;
//      i18->next = i19;

//      i19->type = NOOP; // NOOP after body of IF
//      i19->next = i20;

//      i20->type = ASSIGN; // d = d + 1
//      i20->assign_inst.left_hand_side_index = address_d;
//      i20->assign_inst.op = OPERATOR_PLUS;
//      i20->assign_inst.operand1_index = address_d;
//      i20->assign_inst.operand2_index = address_one;
//      i20->next = i21;

//      i21->type = JMP;
//      i21->jmp_inst.target = i15;
//      i21->next = i22;

//      i22->type = NOOP; // NOOP after body of WHILE
//      i22->next = NULL;

//      // Inputs
//      inputs.push_back(1);
//      inputs.push_back(2);
//      inputs.push_back(3);
//      inputs.push_back(4);
//      inputs.push_back(5);
//      inputs.push_back(6);

//      return i1;
//  }
