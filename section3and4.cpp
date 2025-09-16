#include <iostream>
#include <chrono>
#include <algorithm>
#include <iterator>
#include <optional>
#include <tuple>
#include <ranges>

#include <gdal/gdal.h>
#include <gdal/gdal_priv.h>

#include "section3and4.h"
#include "descriptor.h"
#include "code_table.h"

Section3And4::Section3And4(CodeTable table)
    :
    m_codeTable{table} {}

void Section3And4::setCodeTable(CodeTable table)
{
    m_codeTable = std::move(table);
}

std::istream& operator>>(std::istream& is, Section3And4& s)
{
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
    std::copy_n(
        std::istream_iterator<Descriptor>(is),
        s.m_descriptors.size(),
        std::begin(s.m_descriptors)
    );


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
    is.read(reinterpret_cast<char*>(s.m_section4Data.data()), s.m_section4Data.size());

    s.m_codeTree = s.makeCodeTree(s.m_descriptors);

    return is;
}

std::ostream& operator<<(std::ostream& os, const Section3And4& s)
{
    std::println(os, "size section 3: {}", s.m_sizeSection3);
    std::println(os, "size section 4: {}", s.m_sizeSection4);
    std::println(os, "observed data? : {}", s.m_observedData);
    std::println(os, "compressed data? : {}", s.m_compressedData);
    std::println(os, "number of subsets: {}", s.m_numberOfSubsets);
    std::println(os, "number of descriptors: {}", s.m_descriptors.size());
    s.displayDescriptors(os, s.m_descriptors);
    std::print(os, "\n");

    return os;
}

std::pair<int, int> Section3And4::getGridSize() const
{
    return {0, 0};
}

void Section3And4::displayDescriptors(std::ostream& os, const std::vector<Descriptor>& descs) const
{
    for (auto&& d: descs) {
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

SmallCode Section3And4::extractSingleElement(uint64_t pos, std::map<int, long long>& dataOffsets, int addDataWidth, int addDataScale, const Code& c)
{
    SmallCode cc{c};
    // Try to find a custom offset or insert the default one for future calls
    auto [offsetIt, alreadyPresent] = dataOffsets.emplace(cc.code, cc.offset);
    cc.offset = offsetIt->second;
    if (c.type == "long" || c.type == "double") {
        cc.size += addDataWidth;
        uint32_t value = extractValue(pos, cc.size);
        if (std::popcount(value) < cc.size) {
            long long v = static_cast<long long>(value) + offsetIt->second;
            int scale = cc.factor + addDataScale;
            if (c.type == "long" && scale <= 0) {
                cc.value = static_cast<long long>(v * std::pow(10, -scale));
            } else {
                cc.value = static_cast<double>(v) * std::pow(10.0, -scale);
            }
        }
    } else if (c.type == "table") {
        cc.value = static_cast<long long>(extractValue(pos, cc.size));
    } else if (c.type == "string") {
        if (pos % 8 == 0) {
            // we only bother with aligned strings for now
            cc.value = std::string(
                m_section4Data.begin() + (pos / 8),
                m_section4Data.begin() + (pos + cc.size) / 8 + 1
            );
        }
    }
    cc.pos = pos;

    return cc;
}

std::vector<SmallCode> Section3And4::makeCodeTree(const std::vector<Descriptor>& descs)
{
    std::vector<SmallCode> tree;

    using It = std::vector<Descriptor>::const_iterator;

    uint64_t pos = 0UL;
    std::vector<std::tuple<It, It, It, int>> sections{{descs.begin(), descs.begin(), descs.end(), 1}};
    std::vector<std::vector<SmallCode>*> blocks{&tree};
    std::map<Code, std::vector<Descriptor>> sequences;

    std::map<int, long long> dataOffsets;
    int addDataWidth = 0;
    int addDataScale = 0;

    while (!sections.empty()) {
        while (std::get<0>(sections.back()) != std::get<2>(sections.back())) {
            auto& [it, begin, end, repetitions] = sections.back();

            const Descriptor& d = *it;
            auto currentBlock = blocks.back();

            switch (d.getF()) {
                case 0: // element descriptor
                {
                    auto c = m_codeTable.getElementCode(d.getCode());
                    if (!c) {
                        std::println(std::cerr, "FATAL: Unknown element {}, cannot continue", d.getCode());
                        return {};
                    }

                    SmallCode cc = extractSingleElement(pos, dataOffsets, addDataWidth, addDataScale, *c);
                    pos += cc.size;
                    currentBlock->push_back(std::move(cc));
                    ++it;
                }
                break;
                case 1: // repetition
                {
                    SmallCode& loop = currentBlock->emplace_back();
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
                        if (d.getY() == 0) {
                            addDataScale = 0;
                        } else {
                            addDataScale += d.getY() - 128;
                        }
                    } else if (d.getX() == 3) {
                        if (d.getY() == 0) {
                            dataOffsets.clear();
                        } else {
                            // Change the reference value
                            while (it->getCode() != 203255 && it != end) {
                                const Descriptor& newValueElt = *it;
                                auto newValueCode = m_codeTable.getElementCode(newValueElt.getCode());
                                uint32_t value = extractValue(pos, d.getY());
                                uint32_t signBit = 0b1 << (d.getY() - 1);
                                // 1-complement negative number
                                long long computedOffset = (value & signBit)
                                                               ? -static_cast<long long>(value & ~signBit)
                                                               : value;
                                std::println(
                                    std::cerr, "New reference value for {}: {}", newValueCode->code, computedOffset
                                );
                                dataOffsets[newValueCode->code] = computedOffset;
                                pos += d.getY();
                                ++it;
                            }
                            if (it == end) {
                                std::println(
                                    std::cerr,
                                    "FATAL: Malformed message, operator 203YYY unterminated, cannot continue"
                                );
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
                    sections.emplace_back(
                        newSequenceIt->second.begin(), newSequenceIt->second.begin(),
                        newSequenceIt->second.end(), 1
                    );
                    // we keep injecting in the same current block
                    blocks.push_back(currentBlock);
                }
                break;
            }
        }

        auto& [it, begin, end, repetitions] = sections.back();
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

uint32_t Section3And4::extractValue(unsigned long pos, int size) const
{
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
        for (int i = octetLeft + 1 ; i <= octetRight ; ++i) {
            parsed += static_cast<uint64_t>(m_section4Data[i]) << ((octetRight - i) * 8);
        }

        parsed >>= extraBitsOnTheRight;
    }

    // Truncate to 32 bits (max size supported)
    return static_cast<uint32_t>(parsed);
}


void Section3And4::dumpCodeTree(std::ostream& os, const std::vector<SmallCode>& codes) const
{
    std::vector<std::tuple<std::vector<SmallCode>::const_iterator, std::vector<SmallCode>::const_iterator, int>> pos{
        {codes.begin(), codes.end(), 0}
    };
    while (!pos.empty()) {
        while (std::get<0>(pos.back()) != std::get<1>(pos.back())) {
            auto& [it, end, indentation] = pos.back();
            const SmallCode& c = *it;
            int f = c.code / 100000;
            if (f == 0) {
                auto&& details = m_codeTable.getElementCode(c.code);
                if (details) {
                    std::string prefix(indentation, '\t');
                    std::print(os, "{}{:06d} - {} - {} - {}", prefix, c.code, details->name, c.size, c.pos);
                    if (std::holds_alternative<long long>(c.value)) {
                        std::println(os, " : {}", std::get<long long>(c.value));
                    } else if (std::holds_alternative<long long>(c.value)) {
                        std::println(os, " : {}", std::get<double>(c.value));
                    } else if (std::holds_alternative<std::string>(c.value)) {
                        std::println(os, " : {}", std::get<std::string>(c.value).data());
                    } else if (std::holds_alternative<std::nullopt_t>(c.value)) {
                        std::println(os, " : missing");
                    }
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

void Section3And4::buildTiff(const std::string& filename, Descriptor target)
{
    struct GridInformation
    {
        int gridX{1000};
        int gridY{1000};
        double origLongitude{-9.965};
        double origLatitude{53.670};
        double origX{-619652.07406};
        double origY{-3526818.3379};
        int sizeX{1536};
        int sizeY{1536};
        int year = -1;
        int month = -1;
        int day = -1;
        int hour = -1;
        int minute = -1;
        int second = -1;
    } grid;

    for (auto it = m_codeTree.begin(); it != m_codeTree.end(); ++it) {
        if (it->code == 30021) {
            grid.sizeX = std::get<long long>(it->value);
        } else if (it->code == 30022) {
            grid.sizeY = std::get<long long>(it->value);
        } else if (it->code == 5001) {
            grid.origLatitude = std::get<double>(it->value);
        } else if (it->code == 6001) {
            grid.origLongitude = std::get<double>(it->value);
        } else if (it->code == 5033) {
            grid.gridX = std::get<long long>(it->value);
        } else if (it->code == 6033) {
            grid.gridY = std::get<long long>(it->value);
        } else if (it->code == 29001) {
            if (std::get<long long>(it->value) != 1) {
                std::println(std::cerr, "FATAL: Projection used is not polar stereographic (only supported projection for now), cannot continue");
                return;
            }
        } else if (it->code == 4001) {
            grid.year = std::get<long long>(it->value);
        } else if (it->code == 4002) {
            grid.month = std::get<long long>(it->value);
        } else if (it->code == 4003) {
            grid.day = std::get<long long>(it->value);
        } else if (it->code == 4004) {
            grid.hour = std::get<long long>(it->value);
        } else if (it->code == 4005) {
            grid.minute = std::get<long long>(it->value);
        } else if (it->code == 4006) {
            grid.second = std::get<long long>(it->value);
        }
    }

    // Find the relevant data to export as a raster band
    std::optional<SmallCode> targetLoop;
    for (auto it = m_codeTree.begin(); it != m_codeTree.end(); ++it) {
        if (it->code == 105000 && it->block.size() == grid.sizeX * grid.sizeY) {
            if (it->block[0].code == target.getCode()) { // Found ya!
                targetLoop = *it;
            }
        }
    }
    if (!targetLoop) {
        std::println(std::cerr, "FATAL: Unable to find the data to export among the data descriptors, cannot continue");
        return;
    }

    const char* pszFormat = "GTiff";
    GDALAllRegister();
    GDALDriver* tifDriver = GetGDALDriverManager()->GetDriverByName(pszFormat);
    if (tifDriver == nullptr) {
        std::println(std::cerr, "Driver {} not found", pszFormat);
        return;
    }

    GDALDataset* radarDataset = tifDriver->Create(
        filename.c_str(), grid.sizeX, grid.sizeY, 1, GDT_Float64, nullptr
    );

    OGRSpatialReference projection;
    char* projectionAsWKT = nullptr;
    projection.SetProjCS("Polar stereographic BUFR projection");
    projection.SetWellKnownGeogCS("WGS84");
    projection.SetPS(45., 0., 1., 0., 0.);
    projection.exportToWkt(&projectionAsWKT);

    OGRSpatialReference lonlat;
    lonlat.importFromEPSG(4326);
    auto converter = OGRCreateCoordinateTransformation(&lonlat, &projection);
    double projFirstAxis = grid.origLatitude;
    double projSecondAxis = grid.origLongitude;
    // EPSG:4326 uses latitude as the first axis, longitude as the second axis,
    // meanwhile the polar stereographic as easting as the first axis, northing as the second one
    if (converter && converter->Transform(1, &projFirstAxis, &projSecondAxis)) {
        grid.origX = projFirstAxis;
        grid.origY = projSecondAxis;
    }
    double projOriginAndPixelSizes[6] = {grid.origX, double(grid.gridX), 0., grid.origY, 0, double(-grid.gridY)};
    radarDataset->SetGeoTransform(projOriginAndPixelSizes);
    radarDataset->SetProjection(projectionAsWKT);
    CPLFree(projectionAsWKT);

    auto getDouble = [](const SmallCode& val) -> double {
        if (std::holds_alternative<double>(val.value)) {
            return std::get<double>(val.value);
        } else if (std::holds_alternative<long long>(val.value)) {
            return double(std::get<long long>(val.value));
        } else {
            return NAN;
        }
    };
    std::vector<double> raster(grid.sizeX * grid.sizeY);
    std::transform(targetLoop->block.begin(), targetLoop->block.end(), raster.begin(), getDouble);
    GDALRasterBand* poBand = radarDataset->GetRasterBand(1);
    auto err = poBand->RasterIO(
        GF_Write, 0, 0, grid.sizeX, grid.sizeY,
        raster.data(), grid.sizeX, grid.sizeY, GDT_Float64, 0, 0
    );
    if (err != CE_None) {
        std::println(std::cerr, "Error writing raster band");
    }

    if (grid.year >= 0 && grid.month >= 0 && grid.day >= 0) {
        std::chrono::sys_days d{std::chrono::year_month_day{std::chrono::year{grid.year}/grid.month/grid.day}};
        if (grid.hour >= 0 && grid.minute >= 0 && grid.second >= 0) {
            std::chrono::sys_seconds t = d + std::chrono::hours{grid.hour} + std::chrono::minutes{grid.minute} + std::chrono::seconds{grid.second};
            radarDataset->SetMetadataItem("TIFFTAG_DATETIME", std::format("{0:%F}T{0:%T}Z", t).c_str());
        }
    }
    radarDataset->SetMetadataItem("TIFFTAG_IMAGEDESCRIPTION", "Radar");
    radarDataset->SetMetadataItem("TIFFTAG_ARTIST", "Météo Concept <contact@meteo-concept.fr>");

    // Once we're done, close properly the dataset
    GDALClose((GDALDatasetH)radarDataset);
}
