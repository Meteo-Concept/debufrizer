#include <iostream>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <optional>
#include <tuple>
#include <ranges>

#include "section3and4.h"
#include "descriptor.h"
#include "code_table.h"

Section3And4::Section3And4(CodeTable table) : m_codeTable{table} {
}

void Section3And4::setCodeTable(CodeTable table) {
    m_codeTable = std::move(table);
}

std::istream &operator>>(std::istream &is, Section3And4 &s) {
    auto beginningOfSection3 = is.tellg();

    char buffer[3];
    // Size of section 3
    is.read(buffer, 3);
    if (is) {
        s.m_sizeSection3 = (uint8_t(buffer[0]) << 16) + (uint8_t(buffer[1]) << 8) + uint8_t(buffer[2]);
    }
    if (s.m_sizeSection3 < 5) {
        is.setstate(std::ios::failbit);
        return is;
    }

    // reserved octet
    is.read(buffer, 1);

    // number of data subsets
    is.read(buffer, 2);
    if (is) {
        s.m_numberOfSubsets = (uint8_t(buffer[0]) << 8) + uint8_t(buffer[1]);
    }

    // flags
    is.read(buffer, 1);
    if (is) {
        s.m_observedData = static_cast<uint8_t>(buffer[0]) & 0b1000'0000;
        s.m_compressedData = static_cast<uint8_t>(buffer[0]) & 0b0100'0000;
    }

    // descriptors
    s.m_descriptors.resize((s.m_sizeSection3 - 7) / 2);
    std::copy_n(std::istream_iterator<Descriptor>(is),
                s.m_descriptors.size(),
                std::begin(s.m_descriptors));


    is.seekg(beginningOfSection3);
    // There can be padding so jump to the real beginning of section 4 using the
    // size of section 3 as offset
    is.seekg(s.m_sizeSection3, std::ios_base::cur);

    // Size of section 4
    is.read(buffer, 3);
    if (is) {
        s.m_sizeSection4 = (uint8_t(buffer[0]) << 16) + (uint8_t(buffer[1]) << 8) + uint8_t(buffer[2]);
    }
    if (s.m_sizeSection4 < 4) {
        is.setstate(std::ios::failbit);
        return is;
    }

    // reserved octet
    is.read(buffer, 1);

    // slurp all descriptors
    s.m_section4Data.resize(s.m_sizeSection4 - 4);
    is.read(reinterpret_cast<char *>(s.m_section4Data.data()), s.m_section4Data.size());


    s.m_codeTree = s.makeCodeTree(s.m_descriptors);

    return is;
}

std::ostream &operator<<(std::ostream &os, const Section3And4 &s) {
    std::println(os, "size section 3: {}", s.m_sizeSection3);
    std::println(os, "size section 4: {}", s.m_sizeSection4);
    std::println(os, "observed data? : {}", s.m_observedData);
    std::println(os, "compressed data? : {}", s.m_compressedData);
    std::println(os, "number of subsets: {}", s.m_numberOfSubsets);
    std::println(os, "number of descriptors: {}", s.m_descriptors.size());
    s.displayDescriptors(os, s.m_descriptors);

    std::print(os, "\n");
    s.displayCodeTree(os, s.m_codeTree);

    return os;
}

void Section3And4::displayDescriptors(std::ostream &os, const std::vector<Descriptor> &descs) const {
    for (auto &&d: descs) {
        os << "\t" << d;
        switch (d.getF()) {
            case 0: // element descriptor
            {
                auto c = m_codeTable.getElementCode(d.getCode());
                if (c) {
                    std::print(os, " {}", c->description);
                }
                std::print(os, "\n");
            }
            break;
            case 1: // repetition
                if (d.getY() == 0) {
                    std::print(os, " delayed");
                }
                std::println(os, " repetition of the next {} descriptors", d.getX());
                break;
            case 2: // operator
                std::println(os, " operator");
                break;
            case 3: // sequence
            {
                auto seq = m_codeTable.getSequence(d.getCode());
                if (seq) {
                    std::print(os, " sequence of {} elements", seq->size());
                }
                std::print(os, "\n");
            }
            break;
        }
    }
}

std::vector<SmallCode> Section3And4::makeCodeTree(const std::vector<Descriptor> &descs) {
    std::vector<SmallCode> tree;

    using It = std::vector<Descriptor>::const_iterator;

    uint64_t pos = 0UL;
    std::vector<std::tuple<It, It, It, int> > sections{{descs.begin(), descs.begin(), descs.end(), 1}};
    std::vector<std::vector<SmallCode> *> blocks{&tree};
    std::map<Code, std::vector<Descriptor> > sequences;

    int addDataWidth = 0;

    while (!sections.empty()) {
        while (std::get<0>(sections.back()) != std::get<2>(sections.back())) {
            auto &[it, begin, end, repetitions] = sections.back();

            const Descriptor &d = *it;
            auto currentBlock = blocks.back();

            switch (d.getF()) {
                case 0: // element descriptor
                {
                    auto c = m_codeTable.getElementCode(d.getCode());
                    if (!c) {
                        std::println(std::cerr, "FATAL: Unknown element {}, cannot continue", d.getCode());
                        return {};
                    }

                    SmallCode cc{*c};
                    if (c->type == "long" || c->type == "double") {
                        //TODO other value types, scaling, offsetting, etc.
                        cc.size += addDataWidth;
                        cc.value = extractValue(pos, cc.size);
                    }

                    cc.pos = pos;
                    pos += cc.size;
                    currentBlock->push_back(std::move(cc));
                    ++it;
                }
                break;
                case 1: // repetition
                {
                    SmallCode &loop = currentBlock->emplace_back();
                    loop.code = d.getCode();
                    loop.pos = pos;

                    if (d.getY() == 0) {
                        // delayed repetition
                        // read the next element and hope it's a repetition count
                        ++it;
                        auto rep = m_codeTable.getElementCode(it->getCode());
                        if (!rep) {
                            std::println(std::cerr, "FATAL: Unknown element {}, cannot continue", d.getCode());
                            return {};
                        }
                        loop.repetitions = extractValue(pos, rep->size + addDataWidth);
                        pos += rep->size + addDataWidth;
                    } else {
                        loop.repetitions = d.getY();
                    }
                    blocks.push_back(&loop.block);
                    auto insideLoopBegin = it + 1;
                    auto insideLoopEnd = it + 1 + d.getX();
                    // Keep these operations in order, modifying sections invalidates it
                    it = insideLoopEnd;
                    sections.emplace_back(insideLoopBegin, insideLoopBegin, insideLoopEnd, loop.repetitions);
                }
                break;
                case 2: // operator
                {
                    Code op;
                    op.code = d.getCode();
                    op.name = "OPERATOR";
                    ++it;
                    if (d.getX() == 1) {
                        // change data width
                        if (d.getY() == 0) {
                            addDataWidth = 0;
                        } else {
                            addDataWidth += d.getY() - 128;
                        }
                    } else if (d.getX() == 2) {
                        // TODO change scale
                    } else if (d.getX() == 3) {
                        if (d.getY() == 0) {
                            // TODO cancel change of ref value
                        } else {
                            // Change the reference value
                            while (it->getCode() != 203255 && it != end) {
                                /* TODO record change of ref value */
                                const Descriptor &newValueElt = *it;
                                auto newValueCode = m_codeTable.getElementCode(newValueElt.getCode());
                                uint32_t value = extractValue(pos, d.getY());
                                std::println(std::cerr, "New reference value for {}: {}", newValueElt.getCode(), value);
                                pos += d.getY();
                                ++it;
                            }
                            if (it == end) {
                                std::println(
                                    std::cerr,
                                    "FATAL: Malformed message, operator 203YYY unterminated, cannot continue");
                            } else {
                                ++it;
                            }
                        }
                    } else {
                        std::println(std::cerr, "Operator {} not handled yet, cannot continue.", d.getCode());
                        return {};
                    }
                }
                break;
                case 3: // sequence
                {
                    auto seq = m_codeTable.getSequence(d.getCode());
                    if (!seq) {
                        std::println(std::cerr, "FATAL: Unknown sequence {}, cannot continue", d.getCode());
                        return {};
                    }
                    auto [newSequenceIt, inserted] = sequences.emplace(d.getCode(), std::move(*seq));
                    // Same here, keep these operations in order, modifying sections invalidates it
                    ++it;
                    sections.emplace_back(newSequenceIt->second.begin(), newSequenceIt->second.begin(),
                                          newSequenceIt->second.end(), 1);
                    // we keep injecting in the same current block
                    blocks.push_back(currentBlock);
                }
                break;
            }
        }

        auto &[it, begin, end, repetitions] = sections.back();
        --repetitions;
        if (repetitions <= 0) {
            sections.pop_back();
            blocks.pop_back();
        } else {
            it = begin;
        }
    }

    return tree;
}

uint32_t Section3And4::extractValue(unsigned long pos, int size) const {
    unsigned long octetLeft = pos / 8;
    unsigned long octetRight = (pos + size) / 8 - ((pos + size) % 8 == 0);

    /*
     *               size = 17
     *               pos = 12
     *
     * 0        8       16       24       32       40
     * v        v        v        v        v        v
     * |--------|-----***|********|******--|--------|
     *          ^                 ^
     *          |                 |
     *      octetLeft = 1      octetRight = 3
     *
     *
     * Expected result:
     *
     *          |--------|-------*|********|********|
     *
     *               size = 16
     *               pos = 16
     *
     * 0        8       16       24       32       40
     * v        v        v        v        v        v
     * |--------|--------|********|********|--------|
     *                   ^        ^
     *                   |        |
     *         octetLeft = 2   octetRight = 3
     *
     * Expected result:
     *
     *          |--------|--------|********|********|
     *
     *
     * We end up with correct endianness for the platform, from
     * big endian data.
     * This code cannot assume there are extra octets in data
     * beyond the last one we need so there is an edge case to
     * consider if (pos + size) % 8 == 0.
     */

    int extraBitsOnTheLeft = pos % 8;
    int extraBitsOnTheRight = (8 - ((pos + size) % 8)) % 8;

    uint64_t parsed = 0;

    // Clean up the first octet
    uint8_t firstOctet = m_section4Data[octetLeft];
    firstOctet <<= extraBitsOnTheLeft;

    // Put it on the front
    if (octetRight == octetLeft) {
        parsed = firstOctet >> (extraBitsOnTheLeft + extraBitsOnTheRight);
    } else {
        parsed = static_cast<uint64_t>(firstOctet) << ((octetRight - octetLeft) * 8 - extraBitsOnTheLeft);

        // Add the other octets
        for (int i = octetLeft + 1; i <= octetRight; ++i) {
            parsed += static_cast<uint64_t>(m_section4Data[i]) << ((octetRight - i) * 8);
        }

        parsed >>= extraBitsOnTheRight;
    }

    // Truncate to 32 bits (max size supported)
    return static_cast<uint32_t>(parsed);
}


void Section3And4::displayCodeTree(std::ostream &os, const std::vector<SmallCode> &codes) const {
    int currentIndentation = 0;
    std::vector<std::tuple<std::vector<SmallCode>::const_iterator, std::vector<SmallCode>::const_iterator, int> > pos{
        {codes.begin(), codes.end(), 0}
    };
    while (!pos.empty()) {
        while (std::get<0>(pos.back()) != std::get<1>(pos.back())) {
            auto &[it, end, indentation] = pos.back();
            const SmallCode &c = *it;
            int f = c.code / 100000;
            if (f == 0) {
                auto &&details = m_codeTable.getElementCode(c.code);
                if (details) {
                    std::string prefix(indentation, '\t');
                    std::println(os, "{}{:06d} {} {} {} : {}", prefix, c.code, details->name, c.size, c.pos, c.value);
                    ++it;
                }
            } else if (f == 1) {
                std::string prefix(indentation, '\t');
                std::println(os, "{}LOOP of {} repetitions of {} elements", prefix, c.repetitions, c.block.size());
                ++it;
                pos.emplace_back(c.block.begin(), c.block.end(), indentation + 1);
            }
        }
        pos.pop_back();
    }
}
