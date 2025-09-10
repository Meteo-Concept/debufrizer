#ifndef SECTION_0_H
#define SECTION_0_H

#include <iostream>
#include <string>
#include <cstdint>

class Section0
{
private:
	std::string m_content = std::string(4, '\0');
	uint32_t m_size;
	uint8_t m_version;

public:
	inline uint32_t getSectionSize() const { return 8; }
	inline uint32_t getMessageSize() const { return m_size; }
	inline uint8_t getVersion() const { return m_version; }

	friend std::istream& operator>>(std::istream& is, Section0& s);
	friend std::ostream& operator<<(std::ostream& os, const Section0& s);
};

#endif