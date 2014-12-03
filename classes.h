#ifndef CLASSES_H
#define CLASSES_H

#include <vector>
#include <iostream>
#include "shared.h"

using namespace std;

class VarNode {
public:
	string name;
	bool is_global = false; /* needed for part 2*/
	bool is_primitive = false;
	int offset = -1;
	int size = -1;
    array_type_t *array = NULL;
	string type; /* the class its from */
};

class ClassNode {
public:
	string name;
	ClassNode *parent;
	std::vector<VarNode*> attributes;
	int size;
	bool is_primitive = false;
};


 class ListNode  {
    public:
    int val;
    ListNode * next;
 
};
#endif /* CLASSES_H */
