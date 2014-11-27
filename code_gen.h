#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <vector>
#include "classes.h"
#include "shared.h"
#include "control_flow.h"

void code_generation(struct program_t *program);
void gen_code_for_bbs(std::vector<BasicBlock*> cfg);
void gen_code_for_bb(BasicBlock* current_bb);
void gen_code_for_bb_stats(std::vector<Statement*> statements);
void gen_code_for_new_stat(Statement* stat);
void gen_code_for_goto(Statement* stat);
void gen_code_for_print(Statement* stat);
std::vector<string> retrieve_offset_for_rhs(RHS* rhs);
bool lhs_is_tmp_var(variable_access_t *lhs);
string get_type_from_rhs(RHS *rhs);
string get_type_from_va(variable_access_t *va);
void gen_code_for_assign(Statement* stat);
void pop_tmp_vars_off_stack(RHS *rhs, string type);
void check_and_pop_tmp_var(Term *tmp, int cl_size);
string get_str_from_stat_op(int op_num);
tuple<string,string> get_offset_and_class_for_va_id(char *va);
tuple<string,string> get_offset_and_class_for_attr_des(attribute_designator_t *attr);
VarNode* get_var_node_from_class(string class_name, string attr_name);
ClassNode* look_up_class(string class_name);
VarNode* look_up_global_var(string var_name);
VarNode* look_up_temp_var(string var_name);
VarNode* add_tmp_var_to_stack(string var_name, string type);
tuple<string,string> retrieve_offset_for_va(variable_access_t *va);
void convert_c_structs_to_classes(char *id, ClassNode_c *cnode_list, int offset);
ClassNode* _convert_to_class(ClassNode_c *cnode, int starting_offset, bool is_global);
std::vector<VarNode*> convert_attribute_structs(VarNode_c *attr_structs, int starting_offset, bool is_global);
int get_class_size(string _class);
ClassNode *find_classmap(string parent);
void convert_c_structs_for_main(ClassNode_c *cnode_list);
void init_var_table(std::vector<VarNode*> vnodes);
void init_global_region(std::vector<VarNode*> vnodes);
int initial_setup();
void add_primitives_to_class_table();
void print_cpp_classes();




#endif