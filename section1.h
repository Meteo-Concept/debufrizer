#ifndef SECTION_1_H
#define SECTION_1_H

#include <iostream>
#include <string>
#include <chrono>
#include <cstdint>

#include <date/date.h>

class Section1
{
private:
	uint32_t m_size;
	uint8_t m_masterTable;
	uint16_t m_origCentre;
	uint16_t m_origSubcentre;
	uint8_t m_update;
	bool m_optionalSection2;
	uint8_t m_dataCategory;
	uint8_t m_dataSubcategory;
	uint8_t m_localDataSubcategory;
	uint8_t m_masterTableVersion;
	uint8_t m_localTableVersion;
	date::sys_seconds m_time;
	uint8_t m_optionalLocalField = 0;

public:
	inline uint32_t getSectionSize() const { return m_size; }
	inline uint8_t getMasterTable() const { return m_masterTable; }
	inline uint16_t getOrigCentre() const { return m_origCentre; }
	inline uint16_t getOrigSubcentre() const { return m_origSubcentre; }
	inline uint8_t getUpdate() const { return m_update; }
	inline bool hasSection2() const { return m_optionalSection2; }
	inline uint8_t getDataCategory() const { return m_dataCategory; }
	inline uint8_t getDataSubcategory() const { return m_dataSubcategory; }
	inline uint8_t getLocalDataSubcategory() const { return m_localDataSubcategory; }
	inline uint8_t getMasterTableVersion() const { return m_masterTableVersion; }
	inline uint8_t getLocalTableVersion() const { return m_localTableVersion; }
	inline date::sys_seconds getTime() const { return m_time; }
	inline uint8_t getLocalField() const { return m_optionalLocalField; }

	friend std::istream& operator>>(std::istream& is, Section1& s);
	friend std::ostream& operator<<(std::ostream& os, const Section1& s);
};

#endif
