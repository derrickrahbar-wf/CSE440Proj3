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
void gen_code_for_goto(Statement* stat);
void gen_code_for_print(Statement* stat);
void gen_code_for_assign(Statement* stat);
string gen_code_for_va(variable_access_t *va);
void convert_c_structs_to_classes(char *id, ClassNode_c *cnode_list, int offset);
ClassNode* _convert_to_class(ClassNode_c *cnode, int starting_offset, bool is_global);
std::vector<VarNode*> convert_attribute_structs(VarNode_c *attr_structs, int starting_offset, bool is_global);
int get_class_size(string _class);
ClassNode *find_classmap(string parent);
void convert_c_structs_for_main(ClassNode_c *cnode_list);
void init_var_table(std::vector<VarNode*> vnodes);
void init_global_region(std::vector<VarNode*> vnodes);
int initial_setup();
void print_cpp_classes();



#endif