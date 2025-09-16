#ifndef CODE_H
#define CODE_H

#include <string>
#include <variant>
#include <vector>
#include <optional>

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

	Code(int c = -1) : code{c} {}
};

struct SmallCode {
	int code = -1;
	int factor = 0;
	int offset = 0;
	int size = 0;
	std::variant<std::nullopt_t, long long, double, std::string> value = std::nullopt;
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
