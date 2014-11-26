#include "code_gen.h"
#include "utils.h"
#include "classes.h"
#include <vector>
#include <unordered_map>
#include <string>
#include "shared.h" 
#include <iostream>
#include "control_flow.h"
#include <tuple> /* will be of the form <offset, class>*/


#define MAIN_STARTING_OFFSET 12
#define CLASS_VARS_STARTING_OFFSET 0

using namespace std;

std::unordered_map<string, ClassNode*> cnode_table;
std::unordered_map<string, VarNode*> vnode_table;

void code_generation(struct program_t *program)
{
	int offset = 0;
	offset = initial_setup();
	convert_c_structs_to_classes(program->ph->id, program->cl->class_node_list, offset);
	//print_cpp_classes();
	class_list_t *classes = program->cl;
	while(strcmp(program->ph->id, classes->ci->id))
	{
		classes = classes->next;
	}

	std::vector<BasicBlock*> cfg = create_CFG(classes->cb->fdl->fd->fb->ss, program);

	gen_code_for_bbs(cfg);
}


void gen_code_for_bbs(std::vector<BasicBlock*> cfg)
{
	gen_code_for_bb(cfg[0]);
	cout << "  _ra_1: return 0;\n";
}

void gen_code_for_bb(BasicBlock* current_bb)
{
	cout << "  "<< current_bb->label << ":" << endl;

	gen_code_for_bb_stats(current_bb->statements);

	if(current_bb->children_ptrs.size() == 1)
	{
		Statement* last_stat = current_bb->statements[current_bb->statements.size() -1];
		if(!(last_stat->is_goto && last_stat->goto_ptr != NULL && 
			last_stat->goto_ptr->label.compare(current_bb->children_ptrs[0]->label)==0))
		{
			cout << "\tgoto " << current_bb->children_ptrs[0]->label << ";\n";
		}
	}
	current_bb->is_processed = true;

	for (int i=0; i<current_bb->children.size();i++)
	{
		if(!current_bb->children_ptrs[i]->is_processed)
		{
			gen_code_for_bb(current_bb->children_ptrs[i]);
		}
	}



}

void gen_code_for_bb_stats(std::vector<Statement*> statements)
{
	for(int i = 0; i< statements.size();i++)
	{
		if(statements[i]->is_goto)
		{
			gen_code_for_goto(statements[i]);
		}
		else if(statements[i]->is_print)
		{
			gen_code_for_print(statements[i]);
		}
		else
		{
			gen_code_for_assign(statements[i]);
		}
	}
}



void gen_code_for_goto(Statement* stat)
{
	if(stat->lhs != NULL)
		cout << "\tif(mem[" << retrieve_offset_for_va(stat->lhs) << "] == 1) goto " << stat->goto_ptr->label << ";\n";
	else if(stat->goto_ptr == NULL)
		cout << "\tgoto _ra_1;\n";
	else
		cout << "\tgoto " << stat->goto_ptr->label << ";" << endl;
}

void gen_code_for_print(Statement* stat)
{
	cout << "\tprintf(\"\%d\", mem[" << retrieve_offset_for_va(stat->lhs) << "]);\n";
}



std::vector<string> retrieve_offset_for_rhs(RHS* rhs)
{
	std::vector<string> offsets;
	tuple<string,string> term_tup1, term_tup2;

	if(rhs->t1->type == TERM_TYPE_CONST)
	{
		offsets.push_back(to_string(rhs->t1->data.constant));
	}
	else if(rhs->t1->type == TERM_TYPE_VAR)
	{
		term_tup1 = retrieve_offset_for_va(rhs->t1);
		offsets.push_back(get<0>(term_tup1));
	}

	if(rhs->t2 != NULL)
	{
		offsets.push_back(get_str_from_stat_op(rhs->op)); 

		if(rhs->t2->type == TERM_TYPE_CONST)
		{
			offsets.push_back(to_string(rhs->t2->data.constant));
		}
		else if(rhs->t2->type == TERM_TYPE_VAR)
		{
			term_tup2 = retrieve_offset_for_va(rhs->t2);
			offsets.push_back(get<0>(term_tup2));
		}
	}

	return offsets;
}


void gen_code_for_assign(Statement* stat)
{
	tuple<string,string> lhs_offset = retrieve_offset_for_va(stat->lhs);

	std::vector<string> rhs_offsets = retrieve_offsets_for_rhs(stat->rhs);

	cout << "mem[" << get<0>(lhs_offset) << "] = ";

	if(stat->rhs->t1->type == TERM_TYPE_VAR)
	{
		cout << "mem[" << rhs_offsets[0] << "]";
	}
	else if(stat->rhs->t1->type == TERM_TYPE_CONST)
	{
		cout << rhs_offsets[0];
	}

	if(rhs_offsets.size() == 3)
	{
		cout << " " << rhs_offsets[1]; /* this is the operation */

		if(stat->rhs->t2->type == TERM_TYPE_VAR)
		{
			cout << " mem[" << rhs_offsets[2] << "];\n";
		}
		else if(stat->rhs->t2->type == TERM_TYPE_CONST)
		{
			cout << rhs_offsets[2] << ";\n";
		} 
	}
	else if(rhs_offsets.size() == 1)
	{
		cout << ";\n";
	}
	else
	{
		cout << "gen_code_for_assign has not 1 or 3 strings\n";
	}
	
}


string get_str_from_stat_op(int op_num)
{
	string op;
	switch(op_num)
	{
		case STAT_PLUS:
			op = "+";
			break;
		case STAT_MINUS:
			op = "-";
			break;
		case STAT_STAR:
			op = "*";
			break;
		case STAT_SLASH: 
			op = "/";
			break;
		case STAT_MOD:
			op = "\%";
			break;
		case STAT_EQUAL:
			op = "==";
			break;
		case STAT_NOTEQUAL:
			op = "!=";
			break;
		case STAT_LT: 
			op = "<";
			break;
		case STAT_GT: 
			op = ">";
			break;
		case STAT_LE:
			op = "<=";
			break;
		case STAT_GE :
			op = ">=";
			break;
		case STAT_AND:
			op = "AND";
			break;
		case STAT_OR:
			op = "OR";
			break;
		case STAT_NONE:
			op = "----";
			break;
		}

		return string;
}

tuple<string,string> get_offset_and_class_for_va_id(char *va)
{
	string va_id(va);
	tuple<string, string> va_id_tuple;
	string offset, class_name;
	VarNode *va_node = look_up_global_var(va_id); 

	if(va_node != NULL) /* its a global var take absolute offset */
	{
		if(va_node->is_primitive)
		{
			offset = to_string(va_node->offset);
		}
		else
		{
			offset = "mem[" + to_string(va_node->offset) + "]";
		}

		class_name = va_node->type;
	}
	else 
	{
		va_node = look_up_temp_var(va_id);
		if(va_node != NULL)
		{
			if(va_node->is_primitive)
			{
				offset = "mem[FP] + " to_string(va_node->offset);
			}
			else
			{
				offset = "mem[mem[FP] + " + to_string(va_node->offset) + "]";
			}

			class_name = va_node->type;
		}
		else 
		{
			cout << "get_offset_and_class_for_va_id couldnt find var " << va_id << endl;
		}
	}

	va_id_tuple = std::make_tuple(offset, class_name);

	return va_id_tuple;
}

tuple<string,string> get_offset_and_class_for_attr_des(attribute_designator_t *va)
{

}


/*will return the value of the location associated with the va */
/* NOTE: always call mem[<returnval>] for this va actual value*/
tuple<string,string> retrieve_offset_for_va(variable_access_t *va)
{
	tuple<string,string> va_offset_and_class;

	switch(va->type)
	{
		case VARIABLE_ACCESS_T_IDENTIFIER:
			va_offset_and_class = get_offset_and_class_for_va_id(va->data.id);
			break;
		// case VARIABLE_ACCESS_T_INDEXED_VARIABLE:
		// 	va_offset_and_class = get_offset_and_class_for_index_var(va);
		// 	break;
		case VARIABLE_ACCESS_T_ATTRIBUTE_DESIGNATOR:
			va_offset_and_class = get_offset_and_class_for_attr_des(va->data.ad);
			break;
		// case VARIABLE_ACCESS_T_METHOD_DESIGNATOR:
		// 	va_offset_and_class = get_offset_and_class_for_method_des(va);
		// 	break;
		default:
			cout << "ERROR retrieve_offset_for_va\n";
			break;
	}

	return va_offset_and_class;
}

/* Iterates through the classnode list  */
/* If it is the main class, all its vars will be stored in global section, and its offset */
/* will be its position in the global area. All vars that are not part of the main class */
/* won't be put into memory yet. And their offsets will be set according to the size of */
/* preceeding variables in that class */
void convert_c_structs_to_classes(char *id, ClassNode_c *cnode_list, int offset)
{
	while(cnode_list != NULL)
	{
		if(strcmp(cnode_list->name, id) == 0)
		{
			convert_c_structs_for_main(cnode_list);
		}
		else
		{
			_convert_to_class(cnode_list, CLASS_VARS_STARTING_OFFSET, false);
		}
		cnode_list = cnode_list->next;
	}
}



 /* Converts struct to class and adds to the cnode_table */
ClassNode* _convert_to_class(ClassNode_c *cnode, int starting_offset, bool is_global)
{
	ClassNode *cn  = new ClassNode();
	cn->name = char_to_str(cnode->name);
	if(cnode->parent != NULL)
	{
		cn->parent = find_classmap(char_to_str(cnode->parent->name));
	}
	cn->size = cnode->size;
	cn->attributes = convert_attribute_structs(cnode->attributes, starting_offset, is_global);
	// cout<< "size of " << cn->name << "," << cn->attributes.size() << endl;
	cnode_table[cn->name] = cn;

	return cn;
}

std::vector<VarNode*> convert_attribute_structs(VarNode_c *attr_structs, int starting_offset, bool is_global)
{
	std::vector<VarNode*> attrs;
	while(attr_structs != NULL)
	{
		VarNode *vn = new VarNode();
		vn->name = char_to_str(attr_structs->name);
		vn->type = char_to_str(attr_structs->type);
		vn->is_global = is_global;
		vn->is_primitive = is_primitive(vn->type) ? true : false;
		vn->size = get_class_size(vn->type);
		vn->offset = starting_offset;
		if(is_global)
		{
			starting_offset++;
		}
		else
		{
			starting_offset+= vn->size;
		}

		attrs.push_back(vn);

		attr_structs = attr_structs->next;
	}

	return attrs;
}

int get_class_size(string _class)
{
	if (is_primitive(_class))
	{
		return 1;
	}

	return find_classmap(_class)->size;
}

ClassNode *find_classmap(string parent)
{
	std::unordered_map<std::string,ClassNode*>::const_iterator got = cnode_table.find (parent);
	if(got == cnode_table.end())
	{
		printf("ERROR PARENT NOT FOUND IN find_classmap\n");
		return NULL;
	}

	return got->second;
}

/* Convert struct to class */
/* Initalize global region for vars of main (set to 0)*/
void convert_c_structs_for_main(ClassNode_c *cnode_list)
{
	ClassNode *main_class = _convert_to_class(cnode_list, MAIN_STARTING_OFFSET, true);

	init_global_region(main_class->attributes);
	init_var_table(main_class->attributes);
}

void init_var_table(std::vector<VarNode*> vnodes)
{
	for(int i=0;i<vnodes.size();i++)
	{
		vnode_table[vnodes[i]->name] = vnodes[i];
	}
}

/* This function will initializes all global vars to 0 
    and initializes fp and sp to start of the stack */
void init_global_region(std::vector<VarNode*> vnodes)
{
	if (vnodes.size() == 0)
	{
		return;	
	}

	for(int i=0; i <vnodes.size(); i++)
	{
		printf("\tmem[%d] = 0;\n", vnodes[i]->offset);
	}
	printf("\tmem[FP] = %d;\n", vnodes[vnodes.size()-1]->offset);
	printf("\tmem[SP] = %d; //start of sp after global vars are placed\n", vnodes[vnodes.size()-1]->offset);
}

int initial_setup()
{
	printf("#include <stdio.h>\n");
	printf("#define MEM_MAX 65536\n");
	printf("#define NUM_REG 9\n");
	printf("#define GLOBAL_REGION (NUM_REG+3)\n");
	printf("#define FP NUM_REG\n");
	printf("#define SP (NUM_REG + 1)\n");
	printf("#define HP (NUM_REG + 2)\n");
	printf("#define R1 0\n");
	printf("#define R2 1\n");
	printf("#define R3 2\n");
	printf("#define R4 3\n");
	printf("#define R5 4\n");
	printf("#define R6 5\n");
	printf("#define R7 6\n");
	printf("#define R8 7\n");
	printf("#define R9 8\n");
	printf("int mem[MEM_MAX]; \n");
	printf("\nint main() {\n");
	printf("\tmem[HP] = MEM_MAX-1;\n");
	printf("\t/* end of static initial setup */\n");

	return 12; /*Start of offset to place main class vars */
}

void print_cpp_classes()
{
	ClassNode *tmp;
	for ( auto it = cnode_table.begin(); it != cnode_table.end(); ++it )
	{
		tmp = it->second;
		cout << "------------------------\n";
		cout << "CLASS: " << it->first << endl;
		if(tmp->parent != NULL)
		{
			cout << "PARENT: " << tmp->parent->name << endl;
		}
		cout <<"SIZE: " << tmp->size << endl;
		cout <<"ATTRIBUTES:\n";
		for(int i =0; i < tmp->attributes.size(); i++)
		{
			cout << '\t' << tmp->attributes[i]->name << ", TYPE: " << tmp->attributes[i]->type << ", IS GLOBAL: " << tmp->attributes[i]->is_global 
			<< ", SIZE: " << tmp->attributes[i]->size << ", IS PRIMITIVE: " << tmp->attributes[i]->is_primitive << ", OFFSET: " << tmp->attributes[i]->offset << endl;
		}
		cout << endl << endl;
	}
}