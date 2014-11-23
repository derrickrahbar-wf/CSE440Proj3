#ifndef CLASSES_H
#define CLASSES_H

#include <vector>
#include <iostream>

using namespace std;

class VarNode {
public:
	string name;
	bool is_global; /* needed for part 2*/
	bool is_primitive;
	int offset;
	int size;
	string type; /* the class its from */
};

class ClassNode {
public:
	string name;
	ClassNode *parent;
	std::vector<VarNode*> attributes;
	int size;
};

#endif /* CLASSES_H */
