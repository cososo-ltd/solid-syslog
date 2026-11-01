#ifndef STOREBENCH_H
#define STOREBENCH_H

#include <cstddef>
#include <vector>

#include "BlockDeviceFake.h"
#include "SolidSyslogStore.h"

struct SolidSyslogSecurityPolicy;
struct SolidSyslogErrorEvent;

/* A block store over the BlockDeviceFake, with a record of everything the
 * store was told and said: what Write answered, what a drain delivered, and
 * what the error handler was told. The verdicts of the store sweep are read
 * from that record; the bench records and never asserts, so the sweep and
 * the named regression tests can each judge it their own way. */
struct StoreBenchOptions
{
    size_t BlockSize;
    size_t MaxBlocks;
    struct SolidSyslogSecurityPolicy* Policy; /* NULL for the Null policy */
    size_t RecordSize; /* every record is this many bytes */
};

class StoreBench
{
  public:
    explicit StoreBench(const StoreBenchOptions& options);
    ~StoreBench();

    /* Stores a record carrying @p sequence, padded to RecordSize. */
    enum SolidSyslogStoreWriteResult Write(unsigned sequence);

    /* Reads and marks sent until nothing is unsent, or @p maxPasses reads
     * have returned nothing while the store still says it holds something. */
    void Drain(size_t maxPasses);

    /* Reads and marks sent up to @p count records; answers how many it did. */
    size_t Deliver(size_t count);

    /* Destroys the store and creates it again over the same media: a power
     * cycle. */
    void Restart();

    /* One ReadNextUnsent with no MarkSent, so the record is still unsent on
     * the media: what a send that never completed leaves. */
    bool ReadWithoutMarking();

    struct SolidSyslogBlockDevice* Device() const;

    /* The sequences delivered, in delivery order. */
    const std::vector<unsigned>& Delivered() const;

    /* Sequences the store answered KEPT for and never delivered, when nothing
     * was said about them either: no loss report and no garbage delivered.
     * Coarse by design - a report stands for a run - so it names a record that
     * vanished with nothing said, not which record each report covered. */
    std::vector<unsigned> SilentlyLost() const;

    /* The same, for records appended to a block other than @p faultBlock:
     * for a fault that takes the block with it, what else went unspoken. */
    std::vector<unsigned> SilentlyLostOutsideBlock(size_t faultBlock) const;

    /* Sequences the store answered KEPT for and never delivered, whatever was
     * said about them. */
    std::vector<unsigned> Lost() const;

    /* Lost sequences whose record was appended to a block other than
     * @p faultBlock: loss one fault should not have reached. */
    std::vector<unsigned> LostOutsideBlock(size_t faultBlock) const;

    /* Those of LostOutsideBlock whose bytes are no longer intact on the media.
     * A store that has frozen leaves what it holds stranded, still readable,
     * for a start on repaired media; a record that is gone was disposed of or
     * overwritten, which no freeze excuses. */
    std::vector<unsigned> GoneOutsideBlock(size_t faultBlock) const;

    /* Reports under the store's discarded, lost and frozen categories: what
     * the store said when it could not deliver. */
    size_t LossReports() const;

    /* Reports under the store's frozen category alone: the store said it had
     * stopped using the media. */
    size_t FreezeReports() const;

    /* Called once the test has applied its fault; BytesWrittenAfterFault then
     * sums every write the device accepted from that point. */
    void MarkFaultApplied();
    size_t BytesWrittenAfterFault() const;

    /* Records handed back that did not parse as one the bench wrote. */
    size_t Garbage() const;

    /* Deliveries of a sequence already delivered. */
    size_t Duplicates() const;

    /* The most media operations any one store call made, Create included. */
    size_t MaxOperationsInOneCall() const;

    /* Writes the store made that it had no business making: a WriteAt that is
     * not one byte at the last byte of a record the log saw appended. Judged
     * by position, since what the store verified is not visible from here. */
    static std::vector<BlockDeviceFakeWrite> IllegalWrites();

    /* Where the records the log saw appended lie: a boundary is the first byte
     * of a record or the byte after the last one; the flag byte is a record's
     * last; the length bytes are the two after the two-byte magic. The sweep
     * uses these to name the faults no format, or no policy, can detect. */
    static bool IsRecordBoundary(size_t block, size_t offset);
    static bool IsRecordFlagByte(size_t block, size_t offset);
    static bool IsRecordLengthByte(size_t block, size_t offset);

  private:
    StoreBenchOptions options;
    struct SolidSyslogBlockDevice* device;
    struct SolidSyslogStore* store;

    struct Kept
    {
        unsigned Sequence;
        size_t Block;
        size_t Offset; /* of the Append that carried it */
        size_t Count;
    };

    std::vector<Kept> kept;
    std::vector<unsigned> delivered;
    size_t writesBeforeFault;
    size_t garbage;
    size_t lossReports;
    size_t freezeReports;
    size_t maxOperationsInOneCall;
    size_t operationsBeforeCall;

    void CreateStore();
    bool ReadOne();
    bool WasKept(unsigned sequence) const;
    bool WasDelivered(unsigned sequence) const;
    bool IsStillOnMedia(const Kept& record) const;
    void BeginCall();
    void EndCall();
    void OnError(const struct SolidSyslogErrorEvent* event);
    static void HandleError(void* context, const struct SolidSyslogErrorEvent* event);
};

#endif /* STOREBENCH_H */
