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

int function_labels = 0;
ClassNode* current_class_of_func = NULL;
FuncNode* current_func = NULL;
bool proccessing_function = false; /*used in lookup_global var 
									to know which table to look in*/

void code_generation(struct program_t *program)
{
	int offset = 12; /*if add more #defines uupdate*/
	
	class_list_t *classes = program->cl;

	while(strcmp(program->ph->id, classes->ci->id))
	{
		classes = classes->next;
	}

	func_declaration_list_t* main_class_func = classes->cb->fdl;
	while(strcmp(main_class_func->fd->fh->id, program->ph->id) != 0)
	{
		main_class_func = main_class_func->next;
	}
	
	statement_sequence_t* main_method = main_class_func->fd->fb->ss;	
	std::vector<BasicBlock*> cfg = create_CFG(main_method, program);
	
	function_labels = cfg.size(); /*set the next label we can use for function bb's*/
	

	class_list_t *in_order_class_list = reverse_class_list(program->cl);
	std::vector<VarNode*> v = convert_c_structs_to_classes(program->ph->id, program->cl->class_node_list, offset, in_order_class_list);
	add_primitives_to_class_table();
	initial_setup();
	init_global_region(v);

	//print_cpp_classes();
	//print_global_var_table();

	gen_code_for_bbs(cfg, true); /*this is the main gen_code*/
	//print_CFG(cfg);
	cout << "}\n";
}


void gen_code_for_bbs(std::vector<BasicBlock*> cfg, bool is_main)
{
	gen_code_for_bb(cfg[0], is_main);
	if(is_main)
		cout << "  _ra_1: return 0;\n";
}

void gen_code_for_bb(BasicBlock* current_bb, bool is_main)
{
	
	cout << "  "<< current_bb->label << ":" << endl;

	current_bb->is_processed = true; /*need to set before so functions can grab labels*/

	if(if_statement_lock)
	{
		if(current_bb->label.compare(if_statement_true) == 0)
		{
			cout << "\tmem[SP] = mem[SP] - " << if_statement_var_size << ";\n";
			if_statement_lock = 0;	
		}	
	}

	gen_code_for_bb_stats(current_bb->statements, current_bb);

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
			if(!current_bb->statements[current_bb->statements.size()-1]->is_function_call)
				cout << "\tgoto " << current_bb->children_ptrs[0]->label << ";\n";
		}
	}
	
	for (int i=0; i<current_bb->children.size();i++)
	{
		if(!current_bb->children_ptrs[i]->is_processed)
		{
			gen_code_for_bb(current_bb->children_ptrs[i], is_main);
		}
	}

}

void gen_code_for_return_from_func(Statement* stat)
{
	/*get back the initial fp*/
	cout << "\tmem[FP] = mem[mem[SP] - mem[mem[SP]]];\n"; /*mem[SP] stores the size of stack
														    to get to the old fp val location */
	cout << "\tmem[SP] = mem[SP] - mem[mem[SP]] - 1;\n"; /*clear the func call setup*/

	if(tmp_var_table.size() > 0)
		pop_tmp_vars_off_stack();

	string ret_type = get_type_from_va(stat->va);
	
	add_tmp_var_to_stack(stat->lhs->data.id, ret_type);
	tuple<string,string> lhs_offset = retrieve_offset_for_va(stat->lhs);

	bool is_primitive = look_up_class(ret_type)->is_primitive;
	
	if(is_primitive)
		cout << "\tmem[" << get<0>(lhs_offset) << "] = ";
	else
		cout << "\t" << get<0>(lhs_offset) << " = ";

	cout << "mem[R1]; /*get return val*/\n";

}

void gen_code_for_bb_stats(std::vector<Statement*> statements, BasicBlock* current_bb)
{
	for(int i = 0; i< statements.size();i++)
	{
		if(statements[i]->is_function_call)
		{
			gen_code_for_class_function_call(statements[i], current_bb);
		}
		else if(statements[i]->is_goto)
		{
			gen_code_for_goto(statements[i]);
		}
		else if(statements[i]->is_return_assign)
		{
			gen_code_for_return_from_func(statements[i]);
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

void push_stack()
{
	cout << "\tmem[SP]++;\n";
	cout << "\tmem[mem[SP]] = 0;\n";
}

void add_ra_code_and_to_stack(BasicBlock* bb)
{
	if(bb->children_ptrs.size() > 1)
		cout << "unexpected bb of function call has more than one child\n";

	string label = bb->children_ptrs[0]->label;
	push_stack();

	label = label.substr(2, label.size()-2);
	cout << "\tmem[mem[SP]] = " << label << ";\n";
}

void add_object_location_to_stack(variable_access_t *va)
{
	push_stack();
	string class_offset = std::get<0>(retrieve_offset_for_va(va->data.md->va)); /* get offset for 
																		class of caller */
	
	/* if this was from an indexed_var we need to add an extra mem[]
		because the above function would have only returns a rhs val
		of the location */
	if(va->data.md->va->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
		class_offset = "mem[" + class_offset + "]";

	cout << "\tmem[mem[SP]] = " << class_offset << ";\n";

}

string get_num_from_primary(primary_t* p, int sign)
{
	int num_val=0;
	string num, id;

	switch(p->type)
	{
		case PRIMARY_T_UNSIGNED_CONSTANT:
			num_val = sign * p->data.un->ui;
			num = std::to_string(num_val);
			break;
		case PRIMARY_T_VARIABLE_ACCESS:
			id = char_to_str(p->data.va->data.id);
			if(id.compare("True") == 0)
			{
				if(sign == 1) /*sign was not a NOT */
					num_val = 1;
				else
					num_val = 0;

				num = std::to_string(num_val);
			}
			else if(id.compare("False") == 0)
			{
				if(sign == 1)
					num_val = 0;
				else 
					num_val = 1;

				num = std::to_string(num_val);
			}
			break;
		case PRIMARY_T_PRIMARY:
			num = get_num_from_primary(p->data.next, -1*sign); /*For the NOT*/
			break;
	}

	return num;
}

string get_num_from_factor(factor_t *f)
{
	string num;
	int sign = 1;
	int num_val = 0;
	primary_t *p;
	string id;
	switch(f->type)
	{
		case FACTOR_T_SIGNFACTOR:
			if(f->data.f->sign == SIGN_MINUS)
				sign *= -1;
			num = get_num_from_factor(f->data.f->next);
			break;
		case FACTOR_T_PRIMARY:
			num = get_num_from_primary(f->data.p, sign);
			break;
	}

	return num;
}

string get_num_from_expr(expression_t* e)
{
	return get_num_from_factor(e->se1->t->f);
}

/* will return the location of the pointer for a class or the location
   of a value for a primitive type */
string retrieve_offset_for_param_expr(expression_t *e, string param_type)
{
	/*the expression should be variable for pass by ref */
	string num_val = get_num_from_expr(e);
	if(!num_val.empty())
	{	
		/*probably an error if this executes so add comment for debugging*/
		num_val = num_val + "/* This was a constant passed in a pass by var location */";
		return num_val; 
	}

	
	variable_access_t *va = e->se1->t->f->data.p->data.va;
	string offset = std::get<0>(retrieve_offset_for_va(va));

	if(!lhs_is_tmp_var(va) && !look_up_class(param_type)->is_primitive) 
	{
		/*this is a class variable, thus offest will hold the location
		  associated with the heap object, we want to strip to get 
		  the location associated with the pointer value */
		  offset = strip_outer_mem(offset);
	}

	return offset;
}



string retrieve_value_for_param_expr(expression_t *e, string param_type)
{
	string num_val = get_num_from_expr(e);
	if(!num_val.empty())
		return num_val; 
	
	string offset = retrieve_offset_for_param_expr(e, param_type); /*this will return the location to ptr
														 or location to value if prim */

	offset = "mem[" + offset + "]"; /*get ptr val or prim val*/

	return offset;
}


int add_params_to_stack(actual_parameter_list_t *apl, FuncNode* func)
{
	std::vector<VarNode*> params = func->params;
	expression_t *param_expr;
	int param_size = params.size();

	for(int i=0; i<param_size; i++)
	{
		param_expr = apl->ap->e1;
		if(expr_not_constant(param_expr))
			cout << "unexpected expression should be var or constant: "<< print_expr(param_expr) <<" \n";

		push_stack();


		if(params[i]->is_var)
			cout << "\tmem[mem[SP]] = " << retrieve_offset_for_param_expr(param_expr, params[i]->type) << ";\n";
		else
			cout << "\tmem[mem[SP]] = " << retrieve_value_for_param_expr(param_expr, params[i]->type) << ";\n";


		apl = apl->next;
	}

	return param_size;
}

void prepare_stack_for_function_call(variable_access_t* va, BasicBlock* current_bb, FuncNode* func)
{
	push_stack();
	cout << "\tmem[mem[SP]] = mem[FP];\n"; /*store the callers FP*/

	add_ra_code_and_to_stack(current_bb); /*adds code to store the child label pt
												on the stack*/

	add_object_location_to_stack(va); /* stores the heap location of the object of function */

	int param_size = add_params_to_stack(va->data.md->fd->apl, func); /*adds code to add_param 
																		vals returns size of param 
																		section on stack */
	push_stack();
	cout << "\tmem[mem[SP]] = " << param_size + 3 << ";\n"; /*store the size of the 
															caller setup to it
															can push the stack back up when done*/
}

std::tuple<ClassNode*, FuncNode*> get_class_and_func_node_for_func_call(variable_access_t* va)
{
	std::tuple<ClassNode*, FuncNode*> class_and_func; 
	string function_name = char_to_str(va->data.md->fd->id);
	string class_type = get_type_from_va(va->data.md->va);
	ClassNode* cl = look_up_class(class_type);
	FuncNode* function;

	for(int i=0; i<cl->functions.size();i++)
	{
		if(function_name.compare(cl->functions[i]->name) == 0)
			function = cl->functions[i];
	}

	return std::make_tuple(cl, function);
}

void init_local_vars(FuncNode *function)
{
	cout << "\t/*function var section for " << function->vars.size() << " */\n";
	for(int i=0; i< function->vars.size(); i++)
	{
		if(function->vars[i]->size > 1)
			cout << "\tmem[SP] = mem[SP] + " << function->vars[i]->size << ";\n";
		else
			push_stack();

		vnode_table[function->vars[i]->name] = function->vars[i];
	}
	cout << "\t/* end of func var init */\n";
}

void reset_stack_from_function_call(FuncNode* function)
{
	int param_size = function->params.size();
	cout << "\tmem[SP] = mem[FP]; //reset stack\n";
	/* the val will be above params and object start location
	    if ra = 1 and ob = 2 and params = 3-5 fp = 6, 
	    so fp - (param_size + 2) = 6 - (3 + 2) = 1 = ra */
	cout << "\tgoto *label[mem[mem[FP] - " << param_size + 2 << "]];\n"; 
} 


void gen_code_for_class_function_call(Statement *stat, BasicBlock* current_bb)
{
	std::tuple<ClassNode*,FuncNode*> class_and_func;
	variable_access_t* method_va = stat->rhs->t1->data.var;
	FuncNode* function;

	class_and_func = get_class_and_func_node_for_func_call(method_va);
	function = std::get<1>(class_and_func);

	prepare_stack_for_function_call(method_va, current_bb, function);
	cout << "\tmem[FP] = mem[SP];\n"; /*set the functions FP*/
	cout << "\t/* end of stack setup for call to " << function->name << "*/\n";


	if(!function->is_processed)
	{
		cout << "\tgoto " << function->label << ";\n";
		cout << "  " << function->label << ":\n";
		cout << "\t/* FUNCTION: " << function->name << " */\n";

		function->is_processed = true; /*we are writting code for this function*/
		
		/********************* STORING STATS *******************************/
		/* we need to store the global vars so we can restore when finished
			from a func that will be dependent on this current state */
		bool current_proc_function_status = proccessing_function;
		ClassNode * callers_class = current_class_of_func; /* store caller's class */
		FuncNode * callers_func = current_func;
		std::unordered_map<string, VarNode*> caller_vnode_table = vnode_table;
		std::unordered_map<string, VarNode*> caller_tmp_table = tmp_var_table;
		int caller_tmp_var_offset = tmp_var_offset; 
		int caller_cur_tmp_var_stack_count = current_tmp_var_stack_count;
		std::vector<string> caller_tmp_var_ids = tmp_var_ids;
		int caller_if_statement_lock = if_statement_lock;
		int caller_if_statement_var_size = if_statement_var_size;
		string caller_if_statement_true = if_statement_true;

		proccessing_function = true;
		current_class_of_func = std::get<0>(class_and_func); /*set the current class are in*/
		current_func = function;
		vnode_table.clear(); /*clear the table for new function code */
		add_primitives_to_class_table();
		tmp_var_table.clear();
		tmp_var_offset = 1; 
		current_tmp_var_stack_count = 0;
		tmp_var_ids.clear();
		if_statement_lock = 0;
		if_statement_var_size = 0;
		if_statement_true = "";
		/********************************************************************/		

		
		init_local_vars(function);

		gen_code_for_bbs(function->cfg, false); /*not main method*/

		reset_stack_from_function_call(function); 

		/********************REPLACING STATS********************************************/
		current_class_of_func = callers_class;
		current_func = callers_func;
		proccessing_function = current_proc_function_status; 
		vnode_table = caller_vnode_table;
		tmp_var_table = caller_tmp_table;

		tmp_var_offset = caller_tmp_var_offset;
		current_tmp_var_stack_count = caller_cur_tmp_var_stack_count;
		tmp_var_ids = caller_tmp_var_ids;
		if_statement_lock = caller_if_statement_lock;
		if_statement_var_size = caller_if_statement_var_size;
		if_statement_true = caller_if_statement_true;							         
		/***********************************************************************************/

	}
	else
	{
		cout << "\tgoto " << function->label << ";\n";
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
		{
			if(tmp_var_table.size() > 0)
				pop_tmp_vars_off_stack();

			if(stat->is_return_assign)
				int tmp = 1; /*ignore this statement TODO remove*/
			else
				cout << "\tgoto _ra_1;\n"; /*The actual end of the main method*/

		}
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
			va_node = look_up_global_var(va->data.id); //processed
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
		case VARIABLE_ACCESS_T_METHOD_DESIGNATOR:
			parent_type = get_type_from_va(va->data.md->va);
			parent_cl = look_up_class(parent_type);
			if(parent_cl == NULL)
			{
				cout << "couldnt find parent class in get_type_from_va " << parent_type << endl;
			}
			else
			{
				tmp_type.assign(va->data.md->fd->id);
				
				for(int i=0; i<parent_cl->functions.size();i++)
				{
					if(tmp_type.compare(parent_cl->functions[i]->name) == 0)
					{
						type = parent_cl->functions[i]->return_type;
					}
				}
			}

			break;
	}

	return type;
}

bool lhs_is_function_name(variable_access_t* va)
{
	string func_name = current_func->name;

	if(va->type == VARIABLE_ACCESS_T_IDENTIFIER)
	{
		string var_name = char_to_str(va->data.id);
		if(func_name.compare(var_name) == 0)
			return true;
	}

	return false;

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
	tuple<string,string> lhs_offset;

	if(!(current_class_of_func != NULL && lhs_is_function_name(stat->lhs)))
	{
		lhs_offset = retrieve_offset_for_va(stat->lhs);	
	}
	
	std::vector<string> rhs_offsets = retrieve_offset_for_rhs(stat->rhs);

	bool is_primitive = look_up_class(lhs_type)->is_primitive;
	if(stat->lhs->type == VARIABLE_ACCESS_T_INDEXED_VARIABLE)
		is_primitive = true;

	if(current_class_of_func != NULL && lhs_is_function_name(stat->lhs))
		cout << "\tmem[R1] = ";	
	else if(is_primitive)
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
	VarNode *va_node = look_up_global_var(va_id); //processed

	if(va_node != NULL) /* its a global var take absolute offset */
	{
		if(va_node->is_primitive)
		{
			offset = gen_right_var_offset_based_on_scope(va_node);
		}
		else
		{
			offset = "mem[" + gen_right_var_offset_based_on_scope(va_node) + "]";
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
  	else if(current_class_of_func != NULL) /*if not in var table, see if it is in params list, or a class var*/
  	{
  		VarNode* class_var = NULL;
  		for(int i=0;i<current_func->params.size();i++) /*check in params list*/
  		{
  			if(var_name.compare(current_func->params[i]->name) == 0)
			{
				class_var = current_func->params[i];	
			}
  		}

  		if(class_var == NULL) /*check and see if its a class attr*/
  		{
  			for(int i=0;i<current_class_of_func->attributes.size(); i++)
  			{
  				if(var_name.compare(current_class_of_func->attributes[i]->name) == 0)
  				{
  					class_var = current_class_of_func->attributes[i];
  					class_var->is_class_attr = true; /*make sure the caller knows its a class attr*/	
  				}			
  			}
  		}

  		return class_var;
  	}

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

string gen_right_var_offset_based_on_scope(VarNode* var)
{
	if(current_class_of_func != NULL)
	{
		string offset;
		int param_size = current_func->params.size();

		if(var->is_class_attr) /*class attr, we need to return the location of the var on heap*/
		{
			/* accesses the val at the location where the object ptr is stored*/ 
			string class_ptr_val = "mem[mem[FP] - " + std::to_string((param_size+ 1)) + "]";

			offset = class_ptr_val + std::to_string(var->offset); /* add the relative offset for class var*/
		}
		else if(var->is_param) /* we need to access it above the FP*/
		{
			offset = "mem[FP] - " + std::to_string((param_size - var->offset)); /*first val will be at mem[FP] - size*/
		}
		else /*func var, we need to access it below the FP*/
		{
			offset = "mem[FP] + " + std::to_string(var->offset);
		}

		return offset;
	}
	else
	{	
		return std::to_string(var->offset); /*in global method, this has absolute offset*/
	}
}

string get_offset_for_expr(indexed_variable_t *iv, range_list *r, int current_size, string offset)
{
	primary_t *p = iv->iel->e->se1->t->f->data.p;

	if(p->type == PRIMARY_T_VARIABLE_ACCESS)
	{
		if(p->data.va->type != VARIABLE_ACCESS_T_IDENTIFIER)
		{
			cout << "error get_offset_for_expr va should be identifier\n";
		}

		string var_name;
		stringstream ss;
		ss << p->data.va->data.id;
		ss >> var_name;
		/*this is either a temp or global var */
		VarNode* var = look_up_global_var(var_name); //processed
		string var_offset;
		if(var == NULL)
		{
			/* need to add the fp for relative offset of temp var */
			var = look_up_temp_var(var_name);
			var_offset = "mem[FP] + " + std::to_string(var->offset);
		}
		else
			var_offset = gen_right_var_offset_based_on_scope(var);
			
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
		cout << "error in get_offset_for_expr the index wasnt a var or constant\n";
	}

	return offset;
}

string strip_outer_mem(string offset)
{
	offset.replace(0, 4, "");
	offset = offset.substr(0, offset.size()-1);

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
			std::get<0>(iv_offset_and_class) = strip_outer_mem(std::get<0>(iv_offset_and_class));
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
		VarNode *iv_var = look_up_global_var(va_var_name); //processed
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
		case VARIABLE_ACCESS_T_METHOD_DESIGNATOR:
			cout << "Error retrieve_offset_for_va trying to get offset for method!\n";
			break;
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
std::vector<VarNode*> convert_c_structs_to_classes(char *id, ClassNode_c *cnode_list, int offset, class_list_t *class_list)
{
	std::vector<VarNode*> attr;
	while(cnode_list != NULL)
	{
		if(strcmp(cnode_list->name, id) == 0)
		{
			attr = convert_c_structs_for_main(cnode_list, class_list);
		}
		else
		{
			_convert_to_class(cnode_list, CLASS_VARS_STARTING_OFFSET, false, class_list);
		}
		cnode_list = cnode_list->next;
		class_list = class_list->next;
	}

	return attr;
}



 /* Converts struct to class and adds to the cnode_table */
ClassNode* _convert_to_class(ClassNode_c *cnode, int starting_offset, bool is_global, class_list_t* cl)
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
	
	cn->functions = create_function_nodes(cl->cb->fdl, is_global, cn->name);
	cnode_table[cn->name] = cn;

	return cn;
}


std::vector<VarNode*> create_param_var_nodes(formal_parameter_section_list_t *fpsl)
{
	std::vector<VarNode*> params;
	int offset = 0; /*the new FP will be pointing to the size 
						of the stack for this func, first param
						is at mem[FP] - param_size + offset */
	string type;
	identifier_list_t* id_list;
	VarNode* vnode;

	fpsl = reverse_fpsl(fpsl);

	while(fpsl != NULL)
	{
		type = char_to_str(fpsl->fps->id);
		id_list = fpsl->fps->il;
		id_list = reverse_id_list(id_list);
		while(id_list != NULL)
		{
			vnode = new VarNode();
			vnode->is_global = false;
			vnode->is_var = fpsl->fps->is_var;
			vnode->offset = offset;
			offset++;
			vnode->size = 1;
			vnode->type = type;
			vnode->is_primitive = is_primitive(type);
			vnode->name = char_to_str(id_list->id);
			vnode->is_param = true;

			params.push_back(vnode);
			id_list = id_list->next;
		}

		fpsl = fpsl->next;
	}

	return params;
}


/*if the class is global that means it is the main class and we want to avoid created the cfg 
  main method which we already do */
std::vector<FuncNode*> create_function_nodes(func_declaration_list_t *fdl, bool is_global, string name)
{
	std::vector<FuncNode*> functions;
	FuncNode* fnode;
	function_declaration_t *func;
	string func_name;


	while(fdl != NULL)
	{
		func = fdl->fd;
		func_name = char_to_str(func->fh->id);
		if((name.compare(func_name)) != 0 || !is_global) /*if its not the main method of the main class */
		{
			fnode = new FuncNode();
			fnode->name = func_name;
			
			fnode->label = "bb" + std::to_string(function_labels);
			function_labels++;
			
			fnode->return_type = char_to_str(func->fh->res); 

			fnode->cfg = create_FuncCFG(fdl->fd->fb->ss, function_labels, fnode->return_type);
			function_labels += fnode->cfg.size()-1; /*update the functional label count*/
			
			if(func->fh->fpsl != NULL)
				fnode->params = create_param_var_nodes(func->fh->fpsl);
			
			if(func->fb->vdl != NULL)
				fnode->vars = convert_attribute_structs(func->fb->vdl->vd->var_list, 1, false); 
			

			functions.push_back(fnode);
		}

		fdl = fdl->next;
	}

	return functions;
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
		vn->is_param = false;

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
std::vector<VarNode*> convert_c_structs_for_main(ClassNode_c *cnode_list, class_list_t *cl)
{
	ClassNode *main_class = _convert_to_class(cnode_list, MAIN_STARTING_OFFSET, true, cl);
	
	std::vector<VarNode*> attr = main_class->attributes;
	init_var_table(main_class->attributes);

	return attr;
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

void populate_label_array(int bb_num)
{
	for(int i=0;i<bb_num;i++)
	{
		cout << "\tlabel["<< i <<"] = &&bb" << i << ";\n"; 
	}
}

void initial_setup()
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
	cout << "void *label[" << function_labels << "];\n";
	printf("\nint main() {\n");
	printf("\tmem[HP] = MEM_MAX-1;\n");
	populate_label_array(function_labels);
	printf("\t/* end of static initial setup */\n");
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

identifier_list_t* reverse_id_list(identifier_list_t* fpsl)
{
	identifier_list_t *fpsl_temp1= fpsl, *fpsl_previous= NULL;

	while(fpsl != NULL && fpsl->next != NULL)
	{
		fpsl_temp1 = fpsl->next;
		fpsl->next = fpsl_previous;
		fpsl_previous = fpsl;
		fpsl = fpsl_temp1;
	}

	fpsl->next = fpsl_previous;
	return fpsl;
}

formal_parameter_section_list_t* reverse_fpsl(formal_parameter_section_list_t *fpsl)
{
	formal_parameter_section_list_t *fpsl_temp1= fpsl, *fpsl_previous= NULL;

	while(fpsl != NULL && fpsl->next != NULL)
	{
		fpsl_temp1 = fpsl->next;
		fpsl->next = fpsl_previous;
		fpsl_previous = fpsl;
		fpsl = fpsl_temp1;
	}

	fpsl->next = fpsl_previous;
	return fpsl;	
}

class_list_t * reverse_class_list(class_list_t* cl_orig)
{
	class_list_t *cl_temp1= cl_orig, *cl_previous= NULL, *cl = cl_orig;

	while(cl != NULL && cl->next != NULL)
	{
		cl_temp1 = cl->next;
		cl->next = cl_previous;
		cl_previous = cl;
		cl = cl_temp1;
	}

	cl->next = cl_previous;
	return cl;
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
	FuncNode* func;
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
		for(int i=0;i<tmp->functions.size(); i++)
		{
			func = tmp->functions[i];
			cout << "\nFUNCTION: " << func->return_type << " " << func->name;
			if(func->params.size() != 0)
			{
				cout << "(" << func->params[i]->type << " " << func->params[i]->name << "(" << func->params[i]->offset << ")";

				for(int i=1; i<func->params.size();i++)
				{
					cout << ", " << func->params[i]->type << " " << func->params[i]->name<< "(" << func->params[i]->offset << ")";
				}

				cout << ")";
			}
			cout << "\nVARS:\n";
			for(int i =0; i < func->vars.size(); i++)
			{
				cout << '\t' << func->vars[i]->name << ", TYPE: " << func->vars[i]->type << ", IS GLOBAL: " << func->vars[i]->is_global 
				<< ", SIZE: " << func->vars[i]->size << ", IS PRIMITIVE: " << func->vars[i]->is_primitive << ", OFFSET: " << func->vars[i]->offset << endl;
				if(func->vars[i]->array != NULL)
				{
					cout << '\t';
					print_array(func->vars[i]->array, func->vars[i]->type);
				}
			}
			cout << endl << "CFG *****************************************************************\n";
			print_CFG(func->cfg);
			cout << endl << "***********************************************************************\n"; 
		}
		cout << endl << endl;
	}
}
