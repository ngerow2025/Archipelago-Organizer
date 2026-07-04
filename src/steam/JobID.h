#pragma once

#include <cstdint>
#include <limits>
#include <cassert>
#include <atomic>

namespace {
    constexpr uint64_t makeMask(uint64_t bits, uint64_t offset) {
        return ((1ULL << bits) - 1ULL) << offset;
    }
}

class JobID {
public:
    explicit JobID();
    explicit JobID(uint64_t jobID);

    uint64_t getRawJobID() const;

    uint64_t getSequenceNumber() const;
    void setSequenceNumber(uint64_t sequenceNumber);

    uint64_t getStartTime() const;
    void setStartTime(uint64_t startTime);

    uint64_t getProcessID() const;
    void setProcessID(uint64_t processID);

    uint64_t getBoxID() const;
    void setBoxID(uint64_t boxID);

    static JobID createClientJobID(uint64_t processTime);

private:

    static constexpr uint64_t sequenceBits  = 20;
    static constexpr uint64_t startTimeBits = 30;
    static constexpr uint64_t processIDBits = 4;
    static constexpr uint64_t boxIDBits     = 10;

    static constexpr uint64_t sequenceOffset  = 0;
    static constexpr uint64_t startTimeOffset = sequenceOffset  + sequenceBits;
    static constexpr uint64_t processIDOffset = startTimeOffset + startTimeBits;
    static constexpr uint64_t boxIDOffset     = processIDOffset + processIDBits;

    static constexpr uint64_t sequenceMask  = makeMask(sequenceBits,  sequenceOffset);
    static constexpr uint64_t startTimeMask = makeMask(startTimeBits, startTimeOffset);
    static constexpr uint64_t processIDMask = makeMask(processIDBits, processIDOffset);
    static constexpr uint64_t boxIDMask     = makeMask(boxIDBits,     boxIDOffset);

    uint64_t m_jobID;
    static std::atomic<uint64_t> s_nextSequenceNumber;
};

