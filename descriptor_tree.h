#ifndef DESCRIPTOR_TREE_H
#define DESCRIPTOR_TREE_H

#include <iostream>
#include <vector>
#include <ranges>
#include <utility>

#include "code_table.h"


struct DescriptorTreeNode
{
	Descriptor descriptor;
	std::vector<DescriptorTreeNode> elements;
	int level;

	DescriptorTreeNode() = default;
};

#endif
