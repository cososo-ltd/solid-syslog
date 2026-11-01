#include "StoreBench.h"

#include <cstring>

#include "SolidSyslogBlockStore.h"
#include "SolidSyslogError.h"
#include "SolidSyslogStoreCategories.h"
#include "SolidSyslogTunables.h"

namespace
{
/* Every record is "SEQ=" and five decimal digits, padded with 'x' to the
 * record size, so a delivered record either parses to its sequence or is
 * garbage. */
const char SEQUENCE_PREFIX[] = "SEQ=";
const size_t SEQUENCE_PREFIX_LENGTH = sizeof(SEQUENCE_PREFIX) - 1U;
const size_t SEQUENCE_DIGITS = 5U;
const size_t SEQUENCE_HEADER_LENGTH = SEQUENCE_PREFIX_LENGTH + SEQUENCE_DIGITS;

void FormatRecord(char* record, size_t recordSize, unsigned sequence)
{
    std::memset(record, 'x', recordSize);
    std::memcpy(record, SEQUENCE_PREFIX, SEQUENCE_PREFIX_LENGTH);
    unsigned remaining = sequence;
    for (size_t digit = SEQUENCE_DIGITS; digit > 0U; digit--)
    {
        record[SEQUENCE_PREFIX_LENGTH + digit - 1U] = static_cast<char>('0' + (remaining % 10U));
        remaining /= 10U;
    }
}

bool ParseRecord(const char* record, size_t length, unsigned* sequence)
{
    bool parsed =
        (length >= SEQUENCE_HEADER_LENGTH) && (std::memcmp(record, SEQUENCE_PREFIX, SEQUENCE_PREFIX_LENGTH) == 0);
    unsigned value = 0U;

    for (size_t digit = 0U; parsed && (digit < SEQUENCE_DIGITS); digit++)
    {
        char c = record[SEQUENCE_PREFIX_LENGTH + digit];
        parsed = (c >= '0') && (c <= '9');
        value = (value * 10U) + static_cast<unsigned>(c - '0');
    }

    *sequence = value;
    return parsed;
}
} // namespace

StoreBench::StoreBench(const StoreBenchOptions& benchOptions)
    : options(benchOptions), device(nullptr), store(nullptr), writesBeforeFault(0U), garbage(0U), lossReports(0U),
      maxOperationsInOneCall(0U), operationsBeforeCall(0U)
{
    SolidSyslog_SetErrorHandler(HandleError, this);
    device = BlockDeviceFake_Create(benchOptions.BlockSize);
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
}

void StoreBench::CreateStore()
{
    struct SolidSyslogBlockStoreConfig config = {};
    config.BlockDevice = device;
    config.MaxBlocks = options.MaxBlocks;
    config.SecurityPolicy = options.Policy;
    BeginCall();
    store = SolidSyslogBlockStore_Create(&config);
    EndCall();
}

void StoreBench::BeginCall()
{
    operationsBeforeCall = BlockDeviceFake_OperationCount();
}

void StoreBench::EndCall()
{
    size_t operations = BlockDeviceFake_OperationCount() - operationsBeforeCall;

    if (operations > maxOperationsInOneCall)
    {
        maxOperationsInOneCall = operations;
    }
}

bool StoreBench::ReadWithoutMarking()
{
    return ReadOne();
}

struct SolidSyslogBlockDevice* StoreBench::Device() const
{
    return device;
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
        Kept keptRecord = {sequence, BlockDeviceFake_Write(BlockDeviceFake_WriteCount() - 1U).Block};
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

std::vector<unsigned> StoreBench::Lost() const
{
    std::vector<unsigned> lost;

    for (size_t i = 0U; i < kept.size(); i++)
    {
        if (!WasDelivered(kept[i].Sequence))
        {
            lost.push_back(kept[i].Sequence);
        }
    }

    return lost;
}

std::vector<unsigned> StoreBench::LostOutsideBlock(size_t faultBlock) const
{
    std::vector<unsigned> lost;

    for (size_t i = 0U; i < kept.size(); i++)
    {
        if ((kept[i].Block != faultBlock) && !WasDelivered(kept[i].Sequence))
        {
            lost.push_back(kept[i].Sequence);
        }
    }

    return lost;
}

bool StoreBench::WasDelivered(unsigned sequence) const
{
    bool wasDelivered = false;

    for (size_t j = 0U; j < delivered.size(); j++)
    {
        wasDelivered = wasDelivered || (delivered[j] == sequence);
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

std::vector<BlockDeviceFakeWrite> StoreBench::IllegalWrites() const
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
        unsigned sequence = 0U;
        if (ParseRecord(record, length, &sequence))
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
