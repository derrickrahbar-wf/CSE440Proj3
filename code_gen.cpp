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
#include <sstream>


#define MAIN_STARTING_OFFSET 12
#define CLASS_VARS_STARTING_OFFSET 0

using namespace std;

std::unordered_map<string, ClassNode*> cnode_table;
std::unordered_map<string, VarNode*> vnode_table;
std::unordered_map<string, VarNode*> tmp_var_table;

int tmp_var_offset = 1;
int bool_label_num = 0;
int current_tmp_var_stack_count = 0;
std::vector<string> tmp_var_ids;


int if_statement_lock = 0;
int if_statement_var_size = 0;
string if_statement_true;

void code_generation(struct program_t *program)
{
	int offset = 0;
	offset = initial_setup();
	class_list_t *classes = program->cl;
	while(strcmp(program->ph->id, classes->ci->id))
	{
		classes = classes->next;
	}

	convert_c_structs_to_classes(program->ph->id, program->cl->class_node_list, offset);
	add_primitives_to_class_table();
	//print_cpp_classes();
	//print_global_var_table();


	std::vector<BasicBlock*> cfg = create_CFG(classes->cb->fdl->fd->fb->ss, program);

	gen_code_for_bbs(cfg);
	//print_CFG(cfg);
	cout << "}\n";
}


void gen_code_for_bbs(std::vector<BasicBlock*> cfg)
{
	gen_code_for_bb(cfg[0]);
	cout << "  _ra_1: return 0;\n";
}

void gen_code_for_bb(BasicBlock* current_bb)
{
	cout << "  "<< current_bb->label << ":" << endl;

	if(if_statement_lock)
	{
		if(current_bb->label.compare(if_statement_true) == 0)
		{
			cout << "\tmem[SP] = mem[SP] - " << if_statement_var_size << ";\n";
			if_statement_lock = 0;	
		}	
	}

	gen_code_for_bb_stats(current_bb->statements);

	if(current_tmp_var_stack_count > 0)
	{
		pop_tmp_vars_off_stack();
	}

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
		else if(statements[i]->rhs->is_new)
		{
			gen_code_for_new_stat(statements[i]);
		}
		else
		{
			gen_code_for_assign(statements[i]);
		}
	}
}

void gen_code_for_new_stat(Statement* stat)
{
	ClassNode *cl = look_up_class(stat->rhs->class_name);
	string lhs_pointer = std::get<0>(retrieve_offset_for_va(stat->lhs));
	cout << "\tmem[HP] = mem[HP] - " << cl->size << ";\n";
	if(stat->lhs->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
		cout << "\tmem[" << lhs_pointer << "] = mem[HP];\n";
	else
		cout << '\t' << lhs_pointer << " = mem[HP];\n";
}

void gen_code_for_goto(Statement* stat)
{
	
	if(stat->lhs != NULL)
	{
		cout << "\tif(mem[" << std::get<0>(retrieve_offset_for_va(stat->lhs)) << "] == 1) goto " << stat->goto_ptr->label << ";\n";
		if_statement_true = stat->goto_ptr->label;
		if_statement_lock = 1;
		if_statement_var_size = current_tmp_var_stack_count;
		pop_tmp_vars_off_stack();
	}
	else
	{
		if(stat->goto_ptr == NULL)
			cout << "\tgoto _ra_1;\n";
		else
		{
			if(current_tmp_var_stack_count > 0)
			{
				pop_tmp_vars_off_stack();	
			}
			cout << "\tgoto " << stat->goto_ptr->label << ";" << endl;
		}
	}

}

void gen_code_for_print(Statement* stat)
{
	if(stat->lhs->type == VARIABLE_ACCESS_T_IDENTIFIER && 
		(strcmp(stat->lhs->data.id, "True") == 0 || strcmp(stat->lhs->data.id, "False") == 0 ))
	{
		cout << "\tprintf(\"\%s\\n\", \"" << stat->lhs->data.id << "\");\n";
	}

	else
	{
		std::tuple<string,string> print_var = retrieve_offset_for_va(stat->lhs);
		
		if(std::get<1>(print_var).compare("boolean") == 0)
		{
			cout << "\tif(mem[" << std::get<0>(print_var) << "] == 1) goto t" << bool_label_num << ";\n";
			cout << "\tprintf(\"False\\n\");\n";
			cout << "\tgoto t" << bool_label_num+1 << ";\n";
			cout << "  t" << bool_label_num << ":\n";
			cout << "\tprintf(\"True\\n\");\n";
			bool_label_num++;
			cout << "  t" << bool_label_num << ":\n";
			bool_label_num++;
		}
		else if(!look_up_class(std::get<1>(print_var))->is_primitive && stat->lhs->type != VARIABLE_ACCESS_T_INDEXED_VARIABLE) /* a class so print the pointer value */
		{
			cout << "\tprintf(\"\%d\\n\", " << std::get<0>(print_var) << ");\n";
		}

		else
			cout << "\tprintf(\"\%d\\n\", mem[" << std::get<0>(print_var) << "]);\n";
	}
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
		term_tup1 = retrieve_offset_for_va(rhs->t1->data.var);
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
			term_tup2 = retrieve_offset_for_va(rhs->t2->data.var);
			offsets.push_back(get<0>(term_tup2));
		}
	}

	return offsets;
}

bool lhs_is_tmp_var(variable_access_t *lhs)
{
	if(lhs->type != VARIABLE_ACCESS_T_IDENTIFIER)
	{
		return false;
	}

	if(lhs->data.id[0] == '$')
	{
		return true;
	}
	
	return false;
}

string get_type_from_rhs(RHS *rhs)
{
	if(rhs->t1->type == TERM_TYPE_CONST)
		return "integer";
	
	return get_type_from_va(rhs->t1->data.var);
}

string get_type_from_va(variable_access_t *va)
{
	VarNode *va_node;
	string type;
	string parent_type;
	ClassNode *parent_cl;
	string tmp_type;
	switch(va->type)
	{
		case VARIABLE_ACCESS_T_IDENTIFIER:
			va_node = look_up_global_var(va->data.id);
			if (va_node != NULL)
				type = va_node->type;
			else
			{
				va_node = look_up_temp_var(va->data.id);
				if (va_node != NULL)
				{
					type = va_node->type;
				}	
			}
			break;
		case VARIABLE_ACCESS_T_INDEXED_VARIABLE:
			type = get_type_from_va(va->data.iv->va);
			break;
		case VARIABLE_ACCESS_T_ATTRIBUTE_DESIGNATOR:
			parent_type = get_type_from_va(va->data.ad->va);
			parent_cl = look_up_class(parent_type);
			if(parent_cl == NULL)
			{
				cout << "couldn't find parent class in get_type_from_va " << parent_type << endl;
			}
			else
			{
				tmp_type.assign(va->data.ad->id);
				for(int i = 0; i < parent_cl->attributes.size(); i++)
				{
					if(tmp_type.compare(parent_cl->attributes[i]->name) == 0)
					{
						type = parent_cl->attributes[i]->type;
					}
				}
			}
			break;
		// case VARIABLE_ACCESS_T_METHOD_DESIGNATOR:
		// 	break;
	}

	return type;
}

void gen_code_for_assign(Statement* stat)
{
	string rhs_type;
	string lhs_type = get_type_from_va(stat->lhs);
	if(!lhs_type.empty())
	{
		rhs_type = get_type_from_rhs(stat->rhs);
	}
	else
	{
		lhs_type = get_type_from_rhs(stat->rhs);
		rhs_type = lhs_type;
	}

	if(stat->rhs != NULL)
	{		
		int op = stat->rhs->op;
		if(op == STAT_EQUAL || op == STAT_NOTEQUAL || op == STAT_LT || op == STAT_GT
			|| op == STAT_LE || op == STAT_GE || op == STAT_AND || op == STAT_OR)
		{
			lhs_type = "boolean";
		}
	}

	if(lhs_is_tmp_var(stat->lhs))
	{

		add_tmp_var_to_stack(stat->lhs->data.id, lhs_type);
	}

	tuple<string,string> lhs_offset = retrieve_offset_for_va(stat->lhs);

	std::vector<string> rhs_offsets = retrieve_offset_for_rhs(stat->rhs);

	bool is_primitive = look_up_class(lhs_type)->is_primitive;
	if(stat->lhs->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
		is_primitive = true;

	if(is_primitive)
		cout << "\tmem[" << get<0>(lhs_offset) << "] = ";
	else
		cout << "\t" << get<0>(lhs_offset) << " = ";
	

	if(stat->rhs->t1->type == TERM_TYPE_VAR)
	{
		if(look_up_class(rhs_type)->is_primitive || stat->rhs->t1->data.var->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
		{
			cout << "mem[" << rhs_offsets[0] << "]";	
		}
		else
		{
			cout << rhs_offsets[0]; /*We want to set the classes pointer value */
		}	
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
			if(look_up_class(rhs_type)->is_primitive || stat->rhs->t2->data.var->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
			{
				cout << "mem[" << rhs_offsets[2] << "];\n";	
			}
			else
			{
				cout << rhs_offsets[2] << ";\n"; /*We want to set the classes pointer value */
			}
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
	
	bool no_tmps = !lhs_is_tmp_var(stat->lhs);

	no_tmps &= ((stat->rhs->t1->type == TERM_TYPE_CONST) || (stat->rhs->t1->type == TERM_TYPE_VAR && !lhs_is_tmp_var(stat->rhs->t1->data.var)));

	if(stat->rhs->t2 != NULL)
	{
		no_tmps &= ((stat->rhs->t2->type == TERM_TYPE_CONST) || (stat->rhs->t2->type == TERM_TYPE_VAR && !lhs_is_tmp_var(stat->rhs->t2->data.var)));
	}

	if(no_tmps && (current_tmp_var_stack_count > 0))
	{
		pop_tmp_vars_off_stack();
	}
	
}

void pop_tmp_vars_off_stack()
{
	cout << "\tmem[SP] = mem[SP] - " << current_tmp_var_stack_count << ";\n";
	current_tmp_var_stack_count = 0;

	tmp_var_table.clear();
	tmp_var_offset = 1;
}

void check_and_pop_tmp_var(Term *tmp, int cl_size)
{
	if(tmp->type == TERM_TYPE_VAR)
	{
		if(tmp->data.var->type == VARIABLE_ACCESS_T_IDENTIFIER)
		{
			if(tmp->data.var->data.id[0] == '$')
			{
				tmp_var_offset -= cl_size;
				cout << "\tmem[SP] = mem[SP] - " << cl_size << ";\n";
			}
			string id(tmp->data.var->data.id);
			tmp_var_table.erase(id);
		}
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
			op = "&";
			break;
		case STAT_OR:
			op = "|";
			break;
		case STAT_NONE:
			op = "----";
			break;
		}

		return op;
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
				offset = "mem[FP] + " + to_string(va_node->offset);
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

tuple<string,string> get_offset_and_class_for_attr_des(attribute_designator_t *attr)
{
	std::tuple<string, string> va_tuple = retrieve_offset_for_va(attr->va);
	string tmp_id(attr->id);
	VarNode *id_var_node = get_var_node_from_class(std::get<1>(va_tuple), tmp_id);

	string final_offset;
	if(id_var_node->offset != 0)
		final_offset = std::get<0>(va_tuple) + "+" + std::to_string(id_var_node->offset);
	else
		final_offset = std::get<0>(va_tuple);

	if (!id_var_node->is_primitive)
	{
		final_offset = "mem[" + final_offset + "]"; 	
	}

	return std::make_tuple (final_offset, id_var_node->type);
}

VarNode* get_var_node_from_class(string class_name, string attr_name)
{
	ClassNode *cl = look_up_class(class_name);
	for(int i = 0; i < cl->attributes.size(); i++)
	{
		if(attr_name.compare(cl->attributes[i]->name) == 0)
		{
			return cl->attributes[i];
		}
	}
	cout << "inside of get_var_node_from_class. Couldn't find " << attr_name << "in the class " << class_name << endl;

	return NULL;
}

ClassNode* look_up_class(string class_name)
{
	std::unordered_map<std::string,ClassNode*>::const_iterator got = cnode_table.find (class_name);
  	
  	if ( got == cnode_table.end() )
    	cout << class_name << " not found in the class hash table. Inside of look_up_class" << endl;
  	else
  		return got->second;
  	return NULL;
}

VarNode* look_up_global_var(string var_name)
{
	std::unordered_map<std::string, VarNode*>::const_iterator got = vnode_table.find (var_name);
  	
  	if ( got != vnode_table.end() )
  		return got->second;
  	return NULL;
}

VarNode* look_up_temp_var(string var_name)
{
	std::unordered_map<std::string, VarNode*>::const_iterator got = tmp_var_table.find (var_name);
  	
  	if ( got != tmp_var_table.end() )
  		return got->second;
  	else
  		return NULL;
}

VarNode* add_tmp_var_to_stack(string var_name, string type)
{
	ClassNode *cl = look_up_class(type);
	VarNode *tmp_var = new VarNode();
	tmp_var->name = var_name;
	tmp_var->is_global = false;
	tmp_var->offset = tmp_var_offset;
	tmp_var_offset += cl->size;
	tmp_var->size = cl->size;
	tmp_var->type = type;
	tmp_var->is_primitive = cl->is_primitive;

	tmp_var_table[var_name] = tmp_var;

	cout << "\tmem[SP]++;\n";
	cout << "\tmem[mem[SP]] = 0;\n";

	current_tmp_var_stack_count++;
	tmp_var_ids.push_back(var_name);

	return tmp_var;
}

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

string get_offset_for_expr(indexed_variable_t *iv, range_list *r, int current_size, string offset)
{
	primary_t *p = iv->iel->e->se1->t->f->data.p;

	if(p->type == PRIMARY_T_VARIABLE_ACCESS)
	{
		if(p->data.va->type != VARIABLE_ACCESS_T_IDENTIFIER)
		{
			cout << "error get_offset_and_class_for_index_var va should be identifier\n";
		}

		string var_name;
		stringstream ss;
		ss << p->data.va->data.id;
		ss >> var_name;
		/*this is either a temp or global var */
		VarNode* var = look_up_global_var(var_name);
		string var_offset;
		if(var == NULL)
		{
			/* need to add the fp for relative offset of temp var */
			var = look_up_temp_var(var_name);
			var_offset = "mem[FP] + " + std::to_string(var->offset);
		}
		else
			var_offset = std::to_string(var->offset);
			
		if(r->r->min->ui != 0)
			offset += " + (mem[" + var_offset + "] - " + std::to_string(r->r->min->ui) + ")";
		else
			offset += " + mem[" + var_offset + "]";

		if(current_size != 1)
			offset += " * " + std::to_string(current_size);
		
	}
	else if(p->type == PRIMARY_T_UNSIGNED_CONSTANT)
	{
		/*this is a constant*/
		int new_offset = (p->data.un->ui - r->r->min->ui) * current_size;
		if(is_number(offset))
		{
			new_offset += std::stoi(offset);
			offset = std::to_string(new_offset);
		}
		else
		{
			if(new_offset != 0)
				offset += " + " + std::to_string(new_offset);
		}
	}
	else
	{
		cout << "error in get_offset_and_class_for_index_var the index wasnt a var or constant\n";
	}

	return offset;
}

tuple<string,string> get_offset_and_class_for_index_var(indexed_variable_t *iv, range_list *r, int current_size)
{
	tuple<string,string> iv_offset_and_class;
	string offset;

	if(iv->va->type != VARIABLE_ACCESS_T_INDEXED_VARIABLE)
	{
		iv_offset_and_class = retrieve_offset_for_va(iv->va);
		if(!find_classmap(get_type_from_va(iv->va))->is_primitive)
		{
			
			/* we want to remove the mem[] surrounding the offset of the pointer */
			string offset = std::get<0>(iv_offset_and_class);
			offset.replace(0, 4, "");
			offset = offset.substr(0, offset.size()-1);
			std::get<0>(iv_offset_and_class) = offset;
		}
	}
	else
	{
		int current_array_size = r->r->max->ui - r->r->min->ui + 1;
		iv_offset_and_class = get_offset_and_class_for_index_var(iv->va->data.iv, r->next, current_array_size*current_size);
	}

	std::get<0>(iv_offset_and_class) = get_offset_for_expr(iv, r, current_size, std::get<0>(iv_offset_and_class));
	
	return iv_offset_and_class;
}

void print_range_list(range_list *range)
{
	while(range != NULL)
	{
		cout << "[" << range->r->min->ui << ".." << range->r->max->ui << "]\n";
		range = range->next;
	}
}

/*need to reverse the order of the ranges before the last index is analyzed first 
 for indexed variables and the ranges are currently in order of first to last */
range_list *create_range_list(array_type_t* array)
{
	range_list *range = new range_list();
	range->r = array->r;
	range_list *tmp;

	type_denoter_t *td = array->td;

	while(td->type == TYPE_DENOTER_T_ARRAY_TYPE)
	{
		tmp = new range_list();
		tmp->r = td->data.at->r;
		tmp->next = range;
		range = tmp;
		td = td->data.at->td;
	}

	//print_range_list(range);
	return range;
}

string get_var_name_from_iv(indexed_variable_t* iv)
{
	string name;
	while(iv->va->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
	{
		iv = iv->va->data.iv;
	}

	if(iv->va->type == VARIABLE_ACCESS_T_IDENTIFIER)
	{
		name = iv->va->data.id;
	}

	return name;
}

/* used for cases such as a.d[0] will return the d so we
   can find that attribure array in the class that a is */
string find_attr_name_of_indexed_var(indexed_variable_t *iv)
{
	string name;
	while(iv->va->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
	{
		iv = iv->va->data.iv;
	}

	if(iv->va->type == VARIABLE_ACCESS_T_ATTRIBUTE_DESIGNATOR)
	{
		name = iv->va->data.ad->id;
	}

	return name;
}

string find_class_of_indexed_var(indexed_variable_t* iv)
{
	string class_type;
	while(iv->va->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
	{
		iv = iv->va->data.iv;
	}

	if(iv->va->type == VARIABLE_ACCESS_T_ATTRIBUTE_DESIGNATOR)
	{
		class_type = get_type_from_va(iv->va->data.ad->va);
	}
	else
	{
		cout << "expecting to have an attr designator here\n";
	}

	return class_type;
}

array_type_t *find_array_for_iv(indexed_variable_t* iv)
{
	string va_var_name = get_var_name_from_iv(iv);
	array_type_t *array = NULL;

	if(va_var_name.empty()) /*this must be an attribute of a class*/
	{
		string va_type = find_class_of_indexed_var(iv);
		va_var_name = find_attr_name_of_indexed_var(iv);

		ClassNode *cl = look_up_class(va_type);

		for(int i=0 ; i<cl->attributes.size();i++)
		{
			if(va_var_name.compare(cl->attributes[i]->name) == 0)
			{
				array = cl->attributes[i]->array;
			}
		}
	}
	else /*this was an id thus is a global var */
	{
		VarNode *iv_var = look_up_global_var(va_var_name);
		array = iv_var->array;
	}

	return array;
}
	 

/*will return the value of the location associated with the va */
/* NOTE: always call mem[<returnval>] for this va actual value*/
tuple<string,string> retrieve_offset_for_va(variable_access_t *va)
{
	tuple<string,string> va_offset_and_class;
	range_list *r_list;
	switch(va->type)
	{
		case VARIABLE_ACCESS_T_IDENTIFIER:
			va_offset_and_class = get_offset_and_class_for_va_id(va->data.id);
			break;
		case VARIABLE_ACCESS_T_INDEXED_VARIABLE:
			r_list = create_range_list(find_array_for_iv(va->data.iv));
			va_offset_and_class = get_offset_and_class_for_index_var(va->data.iv, r_list, 1);
			break;
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
	cn->size = cnode->size;
	cn->attributes = convert_attribute_structs(cnode->attributes, starting_offset, is_global);
	if(cnode->parent != NULL)
	{
		cn->parent = find_classmap(char_to_str(cnode->parent->name));
		int current_attr_size = cn->attributes.size();
		std::vector<VarNode*> parent_attrs = copy_var_nodes(cn->parent->attributes); 
		cn->attributes.insert(cn->attributes.end(), parent_attrs.begin(), parent_attrs.end());

		for(int i=current_attr_size; i<cn->attributes.size();i++)
		{
			cn->attributes[i]->offset = i;
		}
		cn->size += cn->parent->size;
	}
	cnode_table[cn->name] = cn;

	return cn;
}

std::vector<VarNode*> copy_var_nodes(std::vector<VarNode*> attr_nodes)
{
	std::vector<VarNode*> copy_vec;
	VarNode *copy;
	for(int i=0; i<attr_nodes.size(); i++)
	{
		copy = new VarNode();
		copy->name = attr_nodes[i]->name;
		copy->is_global = attr_nodes[i]->is_global;
		copy->is_primitive = attr_nodes[i]->is_primitive;
		copy->offset = attr_nodes[i]->offset;
		copy->size = attr_nodes[i]->size;
		copy->type = attr_nodes[i]->type;

		copy_vec.push_back(copy);
	}

	return copy_vec;
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
		vn->size = attr_structs->size; 
		vn->array = attr_structs->array;
		vn->offset = starting_offset;

		starting_offset+=vn->size;

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
	printf("\tmem[FP] = %d;\n", vnodes[vnodes.size()-1]->offset + vnodes[vnodes.size()-1]->size -1 );
	printf("\tmem[SP] = %d; //start of sp after global vars are placed\n", vnodes[vnodes.size()-1]->offset + vnodes[vnodes.size()-1]->size -1 );
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

void add_primitives_to_class_table()
{
	ClassNode *boolean = new ClassNode();
	boolean->name = "boolean";
	boolean->size = 1;
	boolean->is_primitive = true;

	ClassNode *integer = new ClassNode();
	integer->name = "integer";
	integer->size = 1;
	integer->is_primitive = true;

	cnode_table["integer"] = integer;
	cnode_table["boolean"] = boolean;
}

void print_global_var_table()
{
	cout << "****************************************************\n";
	VarNode *tmp;
	for ( auto it = vnode_table.begin(); it != vnode_table.end(); ++it )
	{
		tmp = it->second;
		cout << "VAR NAME: " << tmp->name << " TYPE: " << tmp->type << endl;
	}

	cout << "****************************************************\n";
}

void print_array(array_type_t *array, string type)
{
	cout << "ARRAY [" << array->r->min->ui << ".." << array->r->max->ui << "] of ";

	switch(array->td->type)
	{
		case TYPE_DENOTER_T_IDENTIFIER:
        case TYPE_DENOTER_T_CLASS_TYPE:
            cout << type << endl;
            break;
        case TYPE_DENOTER_T_ARRAY_TYPE:
            print_array(array->td->data.at, type);
            break;
	}
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
			if(tmp->attributes[i]->array != NULL)
			{
				cout << '\t';
				print_array(tmp->attributes[i]->array, tmp->attributes[i]->type);
			}
		}
		cout << endl << endl;
	}
}

void print_CFG(std::vector<BasicBlock*> cfg)
{
	for(int i = 0; i < cfg.size(); i++)
	{
		printf("\n \nCURRENT BB PTR: %s\n", cfg[i]->label.c_str());

		printf("Parents: ");
	
		for(int x=0 ; x <cfg[i]->parents_ptrs.size() ; x++)
		{
			printf("%s, ", cfg[i]->parents_ptrs[x]->label.c_str());
		}
	
		printf("\nChildren: ");
	
		for(int j=0 ;j < cfg[i]->children_ptrs.size(); j++)
		{
			printf("%s, ", cfg[i]->children_ptrs[j]->label.c_str());
		}
		
		printf("\nStatements: \n");
		
		for(int k=0 ; k < cfg[i]->statements.size(); k++)
		{
			Statement *stmt = cfg[i]->statements[k];
			if(stmt->is_goto)
			{
				cout << "\t";
				if(stmt->lhs)
				{
					cout << "if ";
					cout << print_var_access(stmt->lhs)<< " ";
				}
				
				cout << "GO TO: " << stmt->goto_ptr->label<< endl;
				
			}
			else if(stmt->is_print)
			{
				cout << "\tPRINT " << print_var_access(stmt->lhs) << endl;
			}
			else if(stmt->rhs->is_new)
			{
				cout << "\tASSIGNMENT: "<< print_var_access(stmt->lhs) << " = new " << stmt->rhs->class_name << ";\n";
			}
			else
			{

				string op;

				switch(stmt->rhs->op)
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
				char *t1, *t2;
				string str;

				if(stmt->rhs->t1->type == TERM_TYPE_CONST)
				{
					
					stringstream ss;
					ss << stmt->rhs->t1->data.constant;
					str = ss.str();
					t1 = new char [str.length()+1];
					strcpy(t1, str.c_str());

				}
				else
				{
					t1 = print_var_access(stmt->rhs->t1->data.var);
				}
				if(stmt->rhs->t2 != NULL)
				{
					if(stmt->rhs->t2->type == TERM_TYPE_CONST)
					{
						stringstream ss;
						ss << stmt->rhs->t2->data.constant;
						str = ss.str();
						t2 = new char [str.length()+1];
						strcpy(t2, str.c_str());
					}
					else
					{
						t2 = print_var_access(stmt->rhs->t2->data.var);
					}
				}

				printf("\tASSIGNMENT: ");

				printf("%s = ", print_var_access(stmt->lhs));
				if(stmt->rhs->t1->sign == STAT_SIGN_NEGATIVE)
				{
					printf("-");
				}
				printf("%s", t1);
				if(stmt->rhs->t2 != NULL)
				{
					printf(" %s ", op.c_str());
					if(stmt->rhs->t2->sign == STAT_SIGN_NEGATIVE)
					{
						printf("-");
					}
					printf("%s", t2);
				}
				cout << endl;
			}

		}

		printf("-------------------------------------------------------------");
}
}