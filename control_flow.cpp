extern "C"
{
	#include "shared.h"
	#include "symtab.h"
	#include "rulefuncs.h"
	#include "semantic.h"
	#include "error.h"
}
#include "control_flow.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <utility>
#include <algorithm>


using namespace std;

std::vector<BasicBlock*> cfg;
int current_bb = 0;
int condition_var_name = 0;
int IN3ADD_op = -1;
int extended_bb_label = 1;
int g_val_num = 0;

std::vector<int> statements_to_remove;

std::vector<std::unordered_map<string, int>> expr_tables;
std::vector<std::unordered_map<string, Val_obj*>> id_tables;
std::vector<std::unordered_map<int, int>> const_tables; /* constant will be first, valnum second */



statement_sequence_t * reverse_ss(statement_sequence_t* ss)
{
	statement_sequence_t *ss_temp1= ss, *ss_previous= NULL;

	while(ss != NULL && ss->next != NULL)
	{
		ss_temp1 = ss->next;
		ss->next = ss_previous;
		ss_previous = ss;
		ss = ss_temp1;
	}

	ss->next = ss_previous;
	return ss;
}

void print_statement_list(statement_sequence_t* ss)
{
	while(ss != NULL)
	{
		cout << ss->line_number<< endl;
		ss = ss->next;
	}
}

std::vector<BasicBlock*> create_CFG(statement_sequence_t *ss, program_t *program)
{
	BasicBlock *starting_block = new BasicBlock();
	cfg.push_back(starting_block);
	add_statements_to_cfg(ss);
	populate_par_child_ptrs();
	populate_goto_ptrs();
	remove_dummy_nodes();
	print_CFG();


	cout << endl;
	return cfg;
}

void populate_goto_ptrs()
{
	for(int i=0 ; i<cfg.size() ; i++)
	{
		for(int s=0; s< cfg[i]->statements.size(); s++)
		{
			Statement* stat = cfg[i]->statements[s];
			if(stat->is_goto)
			{
				BasicBlock* goto_block = cfg[stat->goto_index];
				if(goto_block->statements.size() > 0)
				{
					stat->goto_ptr = goto_block;
				}
				else
				{
					if(goto_block->children_ptrs.size() > 0)
					{
						cout << "DUMMY NODE HAS MORE THAN ONE CHILD\n";
					}

					int bb_index = stat->goto_index;
					while(cfg[bb_index]->statements.size() == 0)
					{
						bb_index++;
					}

					stat->goto_ptr = cfg[bb_index];
				}
			}
		}
	}
}

void populate_par_child_ptrs()
{
	int parent_index;
	int child_index;

	for(int i=0;i<cfg.size();i++)
	{
		for(int p=0;  p<cfg[i]->parents.size(); p++)
		{
			parent_index = cfg[i]->parents[p];
			cfg[i]->parents_ptrs.push_back(cfg[parent_index]);
		}

		for(int c=0 ; c < cfg[i]->children.size(); c++)
		{
			child_index = cfg[i]->children[c];
			cfg[i]->children_ptrs.push_back(cfg[child_index]);
		}
	}
}

void remove_dummy_nodes()
{
	for(int i=0 ; i < cfg.size()-1 ; i++)
	{
		if(cfg[i]->statements.size() == 0) //dummy node
		{
			for(int p=0 ; i<cfg[i]->parents_ptrs.size() ; p ++)
			{
				BasicBlock *parent = cfg[i]->parents_ptrs[p];

				parent->children_ptrs.erase(std::find(parent->children_ptrs.begin(), 
													  parent->children_ptrs.end(), 
													  cfg[i]));

				parent->children_ptrs.insert(parent->children_ptrs.end(), 
											 cfg[i]->children_ptrs.begin(),
											 cfg[i]->children_ptrs.end());

			}

			for(int c=0 ; c<cfg[i]->children_ptrs.size(); c++)
			{
				BasicBlock *child = cfg[i]->children_ptrs[c];

				child->parents_ptrs.erase(std::find(child->parents_ptrs.begin(),
													child->parents_ptrs.end(),
													cfg[i]));
			}

			cfg.erase(cfg.begin() + i);
			i--; 
		}
	}
}



void process_statement(statement_t *s)
{
	switch(s->type)
	{
		case STATEMENT_T_ASSIGNMENT:
			add_assignment_to_cfg(s->data.as);
			break;
		case STATEMENT_T_SEQUENCE:
			add_statements_to_cfg(s->data.ss);
			break;
		case STATEMENT_T_IF:
			add_if_statement_to_cfg(s->data.is);
			break;
		case STATEMENT_T_WHILE:
			add_while_statement_to_cfg(s->data.ws);
			break;
		case STATEMENT_T_PRINT:
			add_print_statement_to_cfg(s->data.ps);
			break;
	}
}

void add_while_statement_to_cfg(while_statement_t *ws)
{
	/* New BB for the condition of the ws */
	std::vector<int> parent;
	
	if(cfg[current_bb]->statements.size() != 0)
	{
		parent.push_back(current_bb);
		add_next_bb(parent);
	}
	int condition_index = current_bb;

	/* Add that condition to the newly created BB from above */
	add_condition_to_bb(ws->e);
	
	/* Update parent vector to hold current condition bb */
	parent.clear();
	parent.push_back(current_bb);

	/* Add new BB for ws body */
	add_next_bb(parent);
	process_statement(ws->s);
	
	/* Ending block of while body is set as a parent of the condition BB */
	cfg[condition_index]->parents.push_back(current_bb);
	cfg[current_bb]->children.push_back(condition_index);

	parent.clear();

	/*add goto statement at end of condition to go to node after ws*/
	add_goto_statement(condition_index, current_bb+1, NULL);

	/* Create BB that joins final BB of ws body and ws condition */
	parent.push_back(condition_index);
	add_next_bb(parent);

}

void add_statements_to_cfg(statement_sequence_t *ss)
{
	ss = reverse_ss(ss);

	while(ss != NULL)
	{
		process_statement(ss->s);
		ss = ss->next;
	}
}

void add_print_statement_to_cfg(print_statement_t *ps)
{
	Statement *stat = new Statement();
	stat->is_print = true;
	stat->lhs = ps->va;

	cfg[current_bb]->statements.push_back(stat);
}


void add_assignment_to_cfg(assignment_statement_t *as)
{
	Statement *stat = new Statement();
	stat->lhs = as->va;
	stat->rhs = get_rhs_from_expr(as->e);

	cfg[current_bb]->statements.push_back(stat);
}

void add_if_statement_to_cfg(if_statement_t *ifs)
{
	int parent = current_bb;
	add_condition_to_bb(ifs->e);

	
	/* Receive the starting block of the then statement */
	int if_st1_index = add_if_body_to_cfg(ifs->s1, parent);
	int end_st1_index = current_bb;

	/* same logic as above for the else stmt */
	int if_st2_index = add_if_body_to_cfg(ifs->s2, parent);
	int end_st2_index = current_bb;

	/*Add else go to from the condition node to start of else bb's*/
	add_goto_statement(parent, if_st2_index, NULL);
	
	cfg[parent]->children.push_back(if_st1_index);
	cfg[parent]->children.push_back(if_st2_index);

	std::vector<int> if_statements_index;
	if_statements_index.push_back(end_st1_index);
	if_statements_index.push_back(end_st2_index);

	add_next_bb(if_statements_index);
}

void add_condition_to_bb(expression_t *expr)
{
	Statement *stat = new Statement();
	RHS *rhs = get_rhs_from_expr(expr);


	stat->lhs = create_temp_id();
	stat->rhs = rhs;

	cfg[current_bb]->statements.push_back(stat);
	/* add goto statement to be the next bb */
	add_goto_statement(current_bb, current_bb+1, stat->lhs);
}

RHS* get_rhs_from_expr(expression_t *expr)
{
	RHS *rhs = new RHS();
	if(!is_3_address_code(expr))
	{
		rhs->t1 = gen_term_from_expr(expr);
		rhs->op = STAT_NONE;
		rhs->t2 = NULL;
	}
	else
	{
		rhs = IN3ADD_gen_rhs_from_3_add_expr(expr);
	}

	return rhs;
}

void add_next_bb(std::vector<int> parents)
{
	BasicBlock *new_block = new BasicBlock();
	current_bb++;
	for(int i=0; i<parents.size(); i++)
	{
		cfg[parents[i]]->children.push_back(current_bb);
		new_block->parents.push_back(parents[i]);
	}
	cfg.push_back(new_block);
}


void add_goto_statement(int current_index, int goto_index, variable_access_t *if_var)
{
	Statement* st = new Statement();
	st->is_goto = true;
	st->goto_index = goto_index;
	if(if_var != NULL)
	{
		st->lhs = if_var;
	}	
	cfg[current_index]->statements.push_back(st);
}

/* Adds a statement of an if statement (i.e. adds st1 or st2 of an if statement) */
int add_if_body_to_cfg(statement_t *st, int parent)
{
	current_bb++;
	int my_index = current_bb;
	BasicBlock *next_block = new BasicBlock();
	next_block->parents.push_back(parent);

	cfg.push_back(next_block);

	process_statement(st);
	
	return my_index;
}

variable_access_t* create_temp_id()
{
	char s[32];
	int num_len = sprintf(s, "%d", condition_var_name);

	char actual_id[num_len + 1];
	sprintf(actual_id, "$%d", condition_var_name);

	condition_var_name++;

	return create_id(actual_id);
}

/* Creates a variable_access node*/
variable_access_t* create_id(char* id)
{

	variable_access_t *new_id = (variable_access_t*)malloc(sizeof(variable_access_t));
	new_id->type = VARIABLE_ACCESS_T_IDENTIFIER;
	new_id->data.id = (char*)malloc(sizeof(char)*strlen(id)+1);
	strncpy(new_id->data.id, id, strlen(id) +1);
	return new_id;
}

bool is_3_address_code(expression_t *expr)
{
	if(expr_term_count(expr) > 2)
	{
		return false;
	}
	return true;
}

Term* gen_term_from_expr(expression_t *expr)
{
	Term *se1_term = gen_term_from_se(expr->se1);
	
	if(expr->se2 == NULL)
	{
		return se1_term;
	}

	RHS *rhs = new RHS();

	rhs->t1 = se1_term;
	rhs->op = relop_to_statop(expr->relop);
	rhs->t2 = gen_term_from_se(expr->se2);
	
	variable_access_t *lhs = create_and_insert_stat(rhs);
	
	return create_temp_term(lhs);
}


Term* gen_term_from_se(simple_expression_t *se)
{
	Term *t_term = gen_term_from_term(se->t);

	if(se->next == NULL)
	{
		return t_term;
	}

	RHS *rhs = new RHS();

	rhs->t1 = gen_term_from_se(se->next);
	rhs->op = addop_to_statop(se->addop);
	rhs->t2 = t_term;

	variable_access_t *lhs = create_and_insert_stat(rhs);

	return create_temp_term(lhs);
}

variable_access_t* create_and_insert_stat(RHS *rhs)
{
	Statement *stat = new Statement();
	stat->lhs = create_temp_id();
	stat->rhs = rhs;

	cfg[current_bb]->statements.push_back(stat); 
	return stat->lhs;
}

Term* gen_term_from_primary(primary_t *p)
{	
	Term *t;
	switch(p->type)
	{
		case PRIMARY_T_VARIABLE_ACCESS:
			t = new Term();
			t->type = TERM_TYPE_VAR;
			t->data.var = create_id(p->data.va->data.id);
			return t;
			break;
		
		case PRIMARY_T_UNSIGNED_CONSTANT:
			t = new Term();
			t->type = TERM_TYPE_CONST;
			t->data.constant = p->data.un->ui;
			return t;	
			break;
		
		case PRIMARY_T_EXPRESSION:
			return gen_term_from_expr(p->data.e);
			break;
	}

	cout << "gen_term_from_primary\n";
	error_unknown(-1);
	return NULL;
}

Term* create_temp_term(variable_access_t* id)
{
	Term *t = new Term();
	t->type = TERM_TYPE_VAR;
	t->data.var = id;

	return t;
}


/* This expression is in 3 address code, parse through to create rhs */
RHS* IN3ADD_gen_rhs_from_3_add_expr(expression_t *expr)
{
	RHS *rhs = new RHS();
	
	std::vector<Term*> terms = IN3ADD_get_terms_from_expr(expr);

	rhs->t1 = terms[0];
	rhs->t2 = terms[1];
	rhs->op = IN3ADD_op;
	IN3ADD_op = -1;

	return rhs;
}


Term* gen_term_from_term(term_t *t)
{
	Term *f_term = gen_term_from_factor(t->f);

	if(t->next == NULL)
	{
		return f_term;
	}

	RHS *rhs = new RHS();

	rhs->t1 = gen_term_from_term(t->next);
	rhs->op = mulop_to_statop(t->mulop);
	rhs->t2 = f_term;
	
	variable_access_t *lhs = create_and_insert_stat(rhs);

	return create_temp_term(lhs);
}

Term* gen_term_from_factor(factor_t *f)
{
	factor_data_t *f_data;
	switch(f->type)
	{
		case FACTOR_T_SIGNFACTOR:
			f_data = f->data.f;
			if(f_data->sign == SIGN_PLUS)
			{
				return gen_term_from_factor(f_data->next);
			}
			else
			{
				return create_negative_factor_term(f_data->next);
			}
			break;

		case FACTOR_T_PRIMARY:
			return gen_term_from_primary(f->data.p);
			break;
	}

	cout << "gen_term_from_factor\n";
	error_unknown(-1);
	return NULL;
}

Term* create_negative_factor_term(factor_t *f)
{
	Term *t1 = new Term();
	t1->sign = STAT_SIGN_NEGATIVE;
	t1->data.constant = 1;
	t1->type = TERM_TYPE_CONST;

	RHS *rhs = new RHS();
	rhs->t1 = t1;
	rhs->op = STAT_STAR;
	rhs->t2 = gen_term_from_factor(f);

	variable_access_t *lhs = create_and_insert_stat(rhs);

	return create_temp_term(lhs);
}


std::vector<Term*> IN3ADD_get_terms_from_expr(expression_t *expr)
{
	std::vector<Term*> terms;
	terms = IN3ADD_get_terms_from_se(expr->se1);
	
	if(expr->se2 != NULL)
	{
		IN3ADD_op = relop_to_statop(expr->relop);
		terms.push_back(IN3ADD_get_terms_from_se(expr->se2)[0]);
	}

	return terms;
}

std::vector<Term*> IN3ADD_get_terms_from_se(simple_expression_t *se)
{
	std::vector<Term*> terms;
	std::vector<Term*> se_term = IN3ADD_get_terms_from_term(se->t);
	

	if(se->next != NULL)
	{
		IN3ADD_op = addop_to_statop(se->addop);
		terms = IN3ADD_get_terms_from_se(se->next);
	}

	terms.insert(terms.end(), se_term.begin(), se_term.end());

	return terms;
}

std::vector<Term*> IN3ADD_get_terms_from_term(term_t *t)
{
	std::vector<Term*> terms;
	std::vector<Term*> term_factor = IN3ADD_get_terms_from_factor(t->f);
	
	if(t->next != NULL)	
	{
		IN3ADD_op = mulop_to_statop(t->mulop);
		terms = IN3ADD_get_terms_from_term(t->next);
	}

	terms.insert(terms.end(), term_factor.begin(), term_factor.end());

	return terms;
}

std::vector<Term*> IN3ADD_get_terms_from_factor(factor_t *f)
{
	std::vector<Term*> terms;

	switch(f->type)
	{
		case FACTOR_T_SIGNFACTOR:
			terms = IN3ADD_get_terms_from_factor(f->data.f->next);
			terms[0]->sign = ((f->data.f->sign == SIGN_PLUS) ? STAT_SIGN_POSITIVE : STAT_SIGN_NEGATIVE);
			break;

		case FACTOR_T_PRIMARY:
			terms = IN3ADD_get_terms_from_primary(f->data.p);
			break;
	}

	return terms;
}

std::vector<Term*> IN3ADD_get_terms_from_primary(primary_t *p)
{
	std::vector<Term*> terms;
	Term *t;
	switch(p->type)
	{
		case PRIMARY_T_VARIABLE_ACCESS:
			t = new Term();
			t->type = TERM_TYPE_VAR;
			t->data.var = create_id(p->data.va->data.id);
			terms.push_back(t);
			break;

		case PRIMARY_T_UNSIGNED_CONSTANT:
			t = new Term();
			t->type = TERM_TYPE_CONST;
			t->data.constant = p->data.un->ui;
			terms.push_back(t);
			break;

		case PRIMARY_T_EXPRESSION:
			terms = IN3ADD_get_terms_from_expr(p->data.e);
	}

	return terms;
}

int relop_to_statop(int relop)
{
	switch(relop)
	{
		case RELOP_EQUAL:
			return STAT_EQUAL;
			break;
		case RELOP_NOTEQUAL:
			return STAT_NOTEQUAL;
			break;
		case RELOP_LT:
			return STAT_LT;
			break;
		case RELOP_GT:
			return STAT_GT;
			break;
		case RELOP_LE:
			return STAT_LE;
			break;
		case RELOP_GE:
			return STAT_GE;
			break;	
	}

	return RELOP_NONE;
}

int mulop_to_statop(int mulop)
{
	switch(mulop)
	{
		case MULOP_STAR:
			return STAT_STAR;
			break;
		case MULOP_SLASH:
			return STAT_SLASH;
			break;
		case MULOP_MOD:
			return STAT_MOD;
			break;
	}

	return MULOP_NONE;
}

int addop_to_statop(int addop)
{
	switch(addop)
	{
		case ADDOP_PLUS:
			return STAT_PLUS;
			break;
		case ADDOP_MINUS:
			return STAT_MINUS;
			break;
	}

	return ADDOP_NONE;
}

int expr_term_count(expression_t *expr)
{
	if(expr == NULL)
	{
		return 0;
	}
	return se_term_count(expr->se1) + se_term_count(expr->se2);
}

int se_term_count(simple_expression_t *se)
{
	if(se == NULL)
	{
		return 0;
	}

	return se_term_count(se->next) + term_term_count(se->t);
}

int term_term_count(term_t *t)
{
	if(t == NULL)
	{
		return 0;
	}
	return factor_term_count(t->f) + term_term_count(t->next);
}

int factor_term_count(factor_t *f)
{
	if(f == NULL)
	{
		return 0;
	}

	switch(f->type)
	{
		case FACTOR_T_SIGNFACTOR:
			return factor_term_count(f->data.f->next);
			break;
		case FACTOR_T_PRIMARY:
			return primary_term_count(f->data.p);
			break;
	}

	cout << "factor_term_count\n";
	error_unknown(-1);
	return -1;
}

int primary_term_count(primary_t *p)
{
	if(p == NULL)
	{
		return 0;
	}

	switch(p->type)
	{
		case PRIMARY_T_VARIABLE_ACCESS:
		case PRIMARY_T_UNSIGNED_CONSTANT:
			return 1;
			break;
		case PRIMARY_T_EXPRESSION:
			return expr_term_count(p->data.e);
			break;
	}
	
	cout << "primary_term_count\n";
	error_unknown(-1);
	return -1;
}



char * print_var_access(variable_access_t* va)
{
	char* left;
	char* index_va;
	switch(va->type)
	{
		case VARIABLE_ACCESS_T_IDENTIFIER:
			return va->data.id;
			break;
		case VARIABLE_ACCESS_T_INDEXED_VARIABLE:
			index_va = print_var_access(va->data.iv->va);
			return strcat(index_va, "[]");
			break;
		case VARIABLE_ACCESS_T_ATTRIBUTE_DESIGNATOR:
			left =  strcat(print_var_access(va->data.ad->va), ".");
			return strcat(left, va->data.ad->id);
			break;
		case VARIABLE_ACCESS_T_METHOD_DESIGNATOR:

			break;
	}
	cout << "This shouldn't happen, in print_var_access" << endl;
	return NULL;
}


void print_CFG()
{
	for(int i = 0; i < cfg.size(); i++)
	{
		printf("\n \nCURRENT BB PTR: %p\n", cfg[i]);

		printf("Parents: ");
	
		for(int x=0 ; x <cfg[i]->parents_ptrs.size() ; x++)
		{
			printf("%p, ", cfg[i]->parents_ptrs[x]);
		}
	
		printf("\nChildren: ");
	
		for(int j=0 ;j < cfg[i]->children_ptrs.size(); j++)
		{
			printf("%p, ", cfg[i]->children_ptrs[j]);
		}
		
		printf("\nStatements: \n");
		
		for(int k=0 ; k< cfg[i]->statements.size(); k++)
		{
			Statement *stmt = cfg[i]->statements[k];
			
			if(stmt->is_goto)
			{
				cout << "\t";
				if(stmt->lhs)
				{
					cout << print_var_access(stmt->lhs) << endl;
				}
				
				cout << "GO TO: " << stmt->goto_ptr<< endl;
				
			}
			
			else
			{
				string op;
				switch(stmt->rhs->op)
				{
					case STAT_PLUS :
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