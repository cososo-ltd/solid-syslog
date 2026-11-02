#include "StoreBench.h"

#include <algorithm>
#include <cstring>

#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogError.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogTunables.h"

namespace
{
/* Every record is "SEQ=" and five decimal digits, padded to the record size
 * with a pattern drawn from the sequence, so a delivered record either parses
 * whole to its sequence or is garbage: one damaged byte anywhere in it, a
 * changed digit included, breaks the pattern. */
const char SEQUENCE_PREFIX[] = "SEQ=";
const size_t SEQUENCE_PREFIX_LENGTH = sizeof(SEQUENCE_PREFIX) - 1U;
const size_t SEQUENCE_DIGITS = 5U;
const size_t SEQUENCE_HEADER_LENGTH = SEQUENCE_PREFIX_LENGTH + SEQUENCE_DIGITS;

char PaddingByte(unsigned sequence, size_t index)
{
    return static_cast<char>('a' + (((static_cast<size_t>(sequence) * 7U) + index) % 26U));
}

void FormatRecord(char* record, size_t recordSize, unsigned sequence)
{
    for (size_t index = SEQUENCE_HEADER_LENGTH; index < recordSize; index++)
    {
        record[index] = PaddingByte(sequence, index);
    }
    std::memcpy(record, SEQUENCE_PREFIX, SEQUENCE_PREFIX_LENGTH);
    unsigned remaining = sequence;
    for (size_t digit = SEQUENCE_DIGITS; digit > 0U; digit--)
    {
        record[SEQUENCE_PREFIX_LENGTH + digit - 1U] = static_cast<char>('0' + (remaining % 10U));
        remaining /= 10U;
    }
}

bool ParseRecord(const char* record, size_t length, size_t recordSize, unsigned* sequence)
{
    bool parsed = (length == recordSize) && (std::memcmp(record, SEQUENCE_PREFIX, SEQUENCE_PREFIX_LENGTH) == 0);
    unsigned value = 0U;

    for (size_t digit = 0U; parsed && (digit < SEQUENCE_DIGITS); digit++)
    {
        char c = record[SEQUENCE_PREFIX_LENGTH + digit];
        parsed = (c >= '0') && (c <= '9');
        value = (value * 10U) + static_cast<unsigned>(c - '0');
    }

    for (size_t index = SEQUENCE_HEADER_LENGTH; parsed && (index < recordSize); index++)
    {
        parsed = record[index] == PaddingByte(value, index);
    }

    *sequence = value;
    return parsed;
}
} // namespace

StoreBench::StoreBench(const StoreBenchOptions& benchOptions)
    : options(benchOptions), device(BlockDeviceFake_Create(benchOptions.BlockSize)), store(nullptr),
      writesBeforeFault(0U), garbage(0U), lossReports(0U), freezeReports(0U), maxOperationsInOneCall(0U),
      operationsBeforeCall(0U)
{
    SolidSyslog_SetErrorHandler(HandleError, this);
    CreateStore();
}

void StoreBench::HandleError(void* context, const struct SolidSyslogErrorEvent* event)
{
    static_cast<StoreBench*>(context)->OnError(event);
}

void StoreBench::OnError(const struct SolidSyslogErrorEvent* event)
{
    bool loss = (event->Category == SOLIDSYSLOG_CAT_STORE_RECORD_DISCARDED) ||
                (event->Category == SOLIDSYSLOG_CAT_STORE_RECORDS_LOST) ||
                (event->Category == SOLIDSYSLOG_CAT_STORE_FROZEN);

    if (loss)
    {
        lossReports++;
    }

    if (event->Category == SOLIDSYSLOG_CAT_STORE_FROZEN)
    {
        freezeReports++;
    }
}

void StoreBench::CreateStore()
{
    struct SolidSyslogBlockStoreConfig config = {};
    config.BlockDevice = device;
    config.MaxBlocks = options.MaxBlocks;
    config.SecurityPolicy = options.Policy;
    config.GetDiscardPolicy = GetDiscardPolicy;
    config.DiscardPolicyContext = this;
    BeginCall();
    store = SolidSyslogBlockStore_Create(&config);
    EndCall();
}

enum SolidSyslogDiscardPolicy StoreBench::GetDiscardPolicy(void* context)
{
    return static_cast<StoreBench*>(context)->options.DiscardPolicy;
}

void StoreBench::BeginCall()
{
    operationsBeforeCall = BlockDeviceFake_OperationCount();
}

void StoreBench::EndCall()
{
    size_t operations = BlockDeviceFake_OperationCount() - operationsBeforeCall;
    maxOperationsInOneCall = std::max(maxOperationsInOneCall, operations);
}

bool StoreBench::ReadWithoutMarking()
{
    return ReadOne();
}

struct SolidSyslogBlockDevice* StoreBench::Device() const
{
    return device;
}

bool StoreBench::HasUnsent() const
{
    return SolidSyslogStore_HasUnsent(store);
}

void StoreBench::Restart()
{
    SolidSyslogBlockStore_Destroy(store);
    CreateStore();
}

StoreBench::~StoreBench()
{
    SolidSyslogBlockStore_Destroy(store);
    SolidSyslog_SetErrorHandler(nullptr, nullptr);
}

const std::vector<unsigned>& StoreBench::Delivered() const
{
    return delivered;
}

enum SolidSyslogStoreWriteResult StoreBench::Write(unsigned sequence)
{
    char record[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    FormatRecord(record, options.RecordSize, sequence);
    BeginCall();
    enum SolidSyslogStoreWriteResult result = SolidSyslogStore_Write(store, record, options.RecordSize);
    EndCall();

    if (result == SOLIDSYSLOG_STORE_WRITE_KEPT)
    {
        /* The record is the Append the log saw last. */
        BlockDeviceFakeWrite append = BlockDeviceFake_Write(BlockDeviceFake_WriteCount() - 1U);
        Kept keptRecord = {sequence, append.Block, append.Offset, append.Count};
        kept.push_back(keptRecord);
    }

    return result;
}

std::vector<unsigned> StoreBench::SilentlyLost() const
{
    std::vector<unsigned> silent;

    if ((lossReports == 0U) && (garbage == 0U))
    {
        silent = Lost();
    }

    return silent;
}

std::vector<unsigned> StoreBench::SilentlyLostOutsideBlock(size_t faultBlock) const
{
    std::vector<unsigned> silent;

    if ((lossReports == 0U) && (garbage == 0U))
    {
        silent = LostOutsideBlock(faultBlock);
    }

    return silent;
}

std::vector<unsigned> StoreBench::Lost() const
{
    std::vector<unsigned> lost;

    for (const Kept& record : kept)
    {
        if (!WasDelivered(record.Sequence))
        {
            lost.push_back(record.Sequence);
        }
    }

    return lost;
}

std::vector<unsigned> StoreBench::LostOutsideBlock(size_t faultBlock) const
{
    std::vector<unsigned> lost;

    for (const Kept& record : kept)
    {
        if ((record.Block != faultBlock) && !WasDelivered(record.Sequence))
        {
            lost.push_back(record.Sequence);
        }
    }

    return lost;
}

std::vector<unsigned> StoreBench::GoneOutsideBlock(size_t faultBlock) const
{
    std::vector<unsigned> gone;

    for (const Kept& record : kept)
    {
        if ((record.Block != faultBlock) && !WasDelivered(record.Sequence) && !IsStillOnMedia(record))
        {
            gone.push_back(record.Sequence);
        }
    }

    return gone;
}

/* Whether the bytes the Append carried still read back with the record in
 * them: not disposed of with the block, not overwritten by a later one. */
bool StoreBench::IsStillOnMedia(const Kept& record) const
{
    std::vector<char> onMedia(record.Count);
    char expected[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    FormatRecord(expected, options.RecordSize, record.Sequence);
    bool found = false;

    if (SolidSyslogBlockDevice_Read(device, record.Block, record.Offset, onMedia.data(), record.Count))
    {
        for (size_t at = 0U; !found && ((at + options.RecordSize) <= record.Count); at++)
        {
            found = std::memcmp(&onMedia[at], expected, options.RecordSize) == 0;
        }
    }

    return found;
}

bool StoreBench::WasKept(unsigned sequence) const
{
    bool wasKept = false;

    for (const Kept& record : kept)
    {
        wasKept = wasKept || (record.Sequence == sequence);
    }

    return wasKept;
}

bool StoreBench::WasDelivered(unsigned sequence) const
{
    bool wasDelivered = false;

    for (unsigned sent : delivered)
    {
        wasDelivered = wasDelivered || (sent == sequence);
    }

    return wasDelivered;
}

void StoreBench::MarkFaultApplied()
{
    writesBeforeFault = BlockDeviceFake_WriteCount();
}

size_t StoreBench::BytesWrittenAfterFault() const
{
    size_t bytes = 0U;
    size_t writeCount = BlockDeviceFake_WriteCount();

    for (size_t i = writesBeforeFault; i < writeCount; i++)
    {
        bytes += BlockDeviceFake_Write(i).Count;
    }

    return bytes;
}

size_t StoreBench::LossReports() const
{
    return lossReports;
}

size_t StoreBench::FreezeReports() const
{
    return freezeReports;
}

size_t StoreBench::Garbage() const
{
    return garbage;
}

size_t StoreBench::Duplicates() const
{
    size_t duplicates = 0U;

    for (size_t i = 0U; i < delivered.size(); i++)
    {
        for (size_t j = 0U; j < i; j++)
        {
            if (delivered[j] == delivered[i])
            {
                duplicates++;
                break;
            }
        }
    }

    return duplicates;
}

size_t StoreBench::MaxOperationsInOneCall() const
{
    return maxOperationsInOneCall;
}

namespace
{
bool IsLastByteOfAnAppend(const BlockDeviceFakeWrite& write, size_t writeCount)
{
    bool found = false;

    for (size_t i = 0U; !found && (i < writeCount); i++)
    {
        BlockDeviceFakeWrite append = BlockDeviceFake_Write(i);
        found = append.Appended && (append.Block == write.Block) && (append.Count > 0U) &&
                ((append.Offset + append.Count - 1U) == write.Offset);
    }

    return found;
}
} // namespace

bool StoreBench::IsRecordFlagByte(size_t block, size_t offset)
{
    BlockDeviceFakeWrite at = {block, offset, 1U, false};
    return IsLastByteOfAnAppend(at, BlockDeviceFake_WriteCount());
}

bool StoreBench::IsRecordBoundary(size_t block, size_t offset)
{
    return (offset == 0U) || IsRecordFlagByte(block, offset - 1U);
}

namespace
{
/* The record store's cleartext header: the magic, then the length. */
const size_t RECORD_MAGIC_SIZE = 2U;
const size_t RECORD_LENGTH_SIZE = 2U;

bool IsFirstByteOfAnAppend(size_t block, size_t offset)
{
    bool found = false;
    size_t writeCount = BlockDeviceFake_WriteCount();

    for (size_t i = 0U; !found && (i < writeCount); i++)
    {
        BlockDeviceFakeWrite append = BlockDeviceFake_Write(i);
        found = append.Appended && (append.Block == block) && (append.Count > 0U) && (append.Offset == offset);
    }

    return found;
}
} // namespace

bool StoreBench::IsRecordLengthByte(size_t block, size_t offset)
{
    bool found = false;

    for (size_t field = 0U; !found && (field < RECORD_LENGTH_SIZE); field++)
    {
        size_t start = RECORD_MAGIC_SIZE + field;
        found = (offset >= start) && IsFirstByteOfAnAppend(block, offset - start);
    }

    return found;
}

std::vector<BlockDeviceFakeWrite> StoreBench::IllegalWrites()
{
    std::vector<BlockDeviceFakeWrite> illegal;
    size_t writeCount = BlockDeviceFake_WriteCount();

    for (size_t i = 0U; i < writeCount; i++)
    {
        BlockDeviceFakeWrite write = BlockDeviceFake_Write(i);
        bool flagByte = !write.Appended && (write.Count == 1U) && IsLastByteOfAnAppend(write, writeCount);

        if (!write.Appended && !flagByte)
        {
            illegal.push_back(write);
        }
    }

    return illegal;
}

void StoreBench::Drain(size_t maxPasses)
{
    size_t emptyPasses = 0U;

    while ((emptyPasses < maxPasses) && SolidSyslogStore_HasUnsent(store))
    {
        if (ReadOne())
        {
            BeginCall();
            SolidSyslogStore_MarkSent(store);
            EndCall();
        }
        else
        {
            emptyPasses++;
        }
    }
}

size_t StoreBench::Deliver(size_t count)
{
    size_t sent = 0U;

    while ((sent < count) && ReadOne())
    {
        BeginCall();
        SolidSyslogStore_MarkSent(store);
        EndCall();
        sent++;
    }

    return sent;
}

/* One ReadNextUnsent, with what it handed back recorded as a sequence or as
 * garbage. */
bool StoreBench::ReadOne()
{
    char record[SOLIDSYSLOG_MAX_MESSAGE_SIZE];
    size_t length = 0U;
    BeginCall();
    bool read = SolidSyslogStore_ReadNextUnsent(store, record, sizeof(record), &length);
    EndCall();

    if (read)
    {
        /* A sequence the bench never wrote is a damaged record that still
         * parses, not a delivery. */
        unsigned sequence = 0U;
        if (ParseRecord(record, length, options.RecordSize, &sequence) && WasKept(sequence))
        {
            delivered.push_back(sequence);
        }
        else
        {
            garbage++;
        }
    }

    return read;
}
