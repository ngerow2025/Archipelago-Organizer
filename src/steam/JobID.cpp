#include "JobID.h"
#include <cassert>

JobID::JobID() : m_jobID(std::numeric_limits<uint64_t>::max()) {}
JobID::JobID(uint64_t jobID) : m_jobID(jobID) {}

uint64_t JobID::getRawJobID() const { return m_jobID; }

uint64_t JobID::getSequenceNumber() const { return (m_jobID & sequenceMask) >> sequenceOffset; }
void JobID::setSequenceNumber(uint64_t sequenceNumber) {
    assert(sequenceNumber < (1ULL << sequenceBits) && "Sequence number does not fit in 20 bits");
    m_jobID = (m_jobID & ~sequenceMask) | (sequenceNumber & sequenceMask);
}

uint64_t JobID::getStartTime() const { return (m_jobID & startTimeMask) >> startTimeOffset; }
void JobID::setStartTime(uint64_t startTime) {
    assert(startTime < (1ULL << startTimeBits) && "Start time does not fit in 30 bits");
    m_jobID = (m_jobID & ~startTimeMask) | ((startTime << startTimeOffset) & startTimeMask);
}

uint64_t JobID::getProcessID() const { return (m_jobID & processIDMask) >> processIDOffset; }
void JobID::setProcessID(uint64_t processID) {
    assert(processID < (1ULL << processIDBits) && "Process ID does not fit in 4 bits");
    m_jobID = (m_jobID & ~processIDMask) | ((processID << processIDOffset) & processIDMask);
}

uint64_t JobID::getBoxID() const { return (m_jobID & boxIDMask) >> boxIDOffset; }
void JobID::setBoxID(uint64_t boxID) {
    assert(boxID < (1ULL << boxIDBits) && "Box ID does not fit in 10 bits");
    m_jobID = (m_jobID & ~boxIDMask) | ((boxID << boxIDOffset) & boxIDMask);
}

std::atomic<uint64_t> JobID::s_nextSequenceNumber = 0;

JobID JobID::createClientJobID(uint64_t processTime){
    uint64_t sequenceNumber = s_nextSequenceNumber.fetch_add(1);
    JobID jobID;
    jobID.setBoxID(0); // boxID is 0 for client jobs
    jobID.setProcessID(0); // processID is 0 for client jobs
    jobID.setStartTime(processTime);
    jobID.setSequenceNumber(sequenceNumber);
    return jobID;
}