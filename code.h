#ifndef CODE_H
#define CODE_H

#include <vector>

//example: 006196|maxObliqueRange|long|Distance oblique maximal d'utilisation des donnees|m|-3|0|16
struct Code {
	int code = -1;
	std::string name;
	std::string type;
	std::string description;
	std::string unit;
	int factor = 0;
	int offset = 0;
	int size = 0;
};

struct SmallCode {
	int code = -1;
	int factor = 0;
	int offset = 0;
	int size = 0;
	long long value = 0;
	unsigned long pos = -1;
	unsigned long repetitions = 0;
	std::vector<SmallCode> block;

	SmallCode() = default;
	explicit SmallCode(Code code) :
		code{code.code},
		factor{code.factor},
		offset{code.offset},
		size{code.size}
	{}
};

inline bool operator<(const Code& left, const Code& right) {
	return left.code < right.code;
}

inline bool operator<(const SmallCode& left, const SmallCode& right) {
	return left.code < right.code;
}

#endif
