#ifndef SCENARIOBENCH_H
#define SCENARIOBENCH_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "SecurityPolicyFake.h"
#include "SolidSyslogBlockDeviceDefinition.h"
#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogSenderDefinition.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStoreDefinition.h"

struct SolidSyslog;
struct SolidSyslogAtomicCounter;
struct SolidSyslogBlockDevice;
struct SolidSyslogBuffer;
struct SolidSyslogErrorEvent;
struct SolidSyslogErrorSource;
struct SolidSyslogFile;
struct SolidSyslogSender;
struct SolidSyslogStore;
struct SolidSyslogStream;
struct SolidSyslogStructuredData;

/* The real Core wired end to end over fakes: a logger over a circular
 * buffer, a block store on the block device fake (or no store), and a stream
 * sender on the stream fake, with the meta SD numbering every record. The bench drives Log
 * and Service, injects faults into the fakes, and records what the stream
 * received and what the error handler was told; it never asserts, so the
 * scenario suite's invariants and the named tests each judge the record
 * their own way. */
struct ScenarioBenchOptions
{
    size_t BufferMessages; /* the ring holds this many worst-case records */
    size_t BlockSize;
    size_t MaxBlocks;
    enum SolidSyslogDiscardPolicy DiscardPolicy; /* the one policy the logger, buffer and store share */
    bool HasStore; /* false wires the Null store, so Service holds for the sender what it cannot send */
    bool FileBacked; /* the store's blocks are files, on a FileBlockDevice over a file held in memory */
};

/* One record the stream received whole and the bench recognised as its own. */
struct ScenarioDelivery
{
    unsigned Ordinal; /* the Log call that produced it, counting from 1 */
    unsigned SequenceId; /* the meta SD's sequenceId */
    size_t Pass; /* the Service pass that delivered it */
};

/* One report the logger made of its own event: a frame carrying the
 * library's solidsyslog element, whose MSGID names the same category as the
 * element's category param. */
struct ScenarioReport
{
    std::string Category;
    unsigned SequenceId; /* the meta SD's sequenceId, which a report takes from the same count as a record */
    size_t Pass; /* the Service pass that delivered it */
    /* The library element's params; empty for one it leaves out. */
    std::string SourceParam;
    std::string DetailParam;
    std::string NativeParam;
};

/* One event the error handler was told. */
struct ScenarioEvent
{
    enum SolidSyslogSeverity Severity;
    const struct SolidSyslogErrorSource* Source;
    uint16_t Category;
    int32_t Detail;
    size_t Pass; /* the Service pass it was raised in: 0 before any pass, or the last pass when raised from a Log */
    size_t RecordsKept; /* the records the store had kept when it was raised */
    unsigned LastSequenceId; /* the last sequenceId the meta SD had handed out when it was raised */
    /* The params a report of it carries, as the report writes them; empty for
     * one the report leaves out. */
    std::string SourceParam;
    std::string DetailParam;
    std::string NativeParam;
};

class ScenarioBench
{
  public:
    explicit ScenarioBench(const ScenarioBenchOptions& options);
    ~ScenarioBench();

    /* Logs the next record; answers its ordinal. */
    unsigned Log();

    /* One Service pass. */
    enum SolidSyslogServiceStatus Service();

    /* Faults, each in force until changed: the stream refuses every Send; the
     * block device fails every operation from now on; the policy every queue
     * asks answers differently. */
    void SetSenderDown(bool down);
    void FailDevice();
    void SetDiscardPolicy(enum SolidSyslogDiscardPolicy policy);

    /* Overwrites every byte of the ring, which is the bench's own storage, so
     * the length of the record at its head reads larger than any record it
     * holds. */
    void CorruptTheRing();

    /* What the store's security policy does with the next seal or open, which
     * a rig with no store never asks. */
    void SetSecurityPolicyFault(enum SecurityPolicyFakeFault fault);

    /* The stream's credentials change so that the collector's certificate is
     * refused, or accepted again. The stream's version moves with them, as the
     * stream contract asks, so the sender reconnects at its next send and meets
     * the change there. */
    void SetHandshakeRefused(bool refused);

    /* The switching sender in front of the stream asks a selector, which while
     * misrouted answers an index out of range: the switching sender then drops
     * whatever it is handed, records and reports alike, and reports the
     * loss. */
    void SetMisrouted(bool misrouted);

    /* The logger's switch for reporting its own events, on until changed. */
    void SetReporting(bool on);

    /* What happened. */
    unsigned Logged() const;
    size_t Passes() const;
    const std::vector<ScenarioDelivery>& Delivered() const;
    const std::vector<ScenarioReport>& Reports() const;
    const std::vector<ScenarioEvent>& Events() const;

    /* Reports and records the switching sender dropped while misrouted, which
     * the stream never saw; Pass is the pass that dropped them. */
    const std::vector<ScenarioReport>& DroppedReports() const;
    const std::vector<ScenarioDelivery>& DroppedRecords() const;

    /* Frames the stream received that parsed neither as a record of this
     * bench nor as a report. */
    size_t Garbage() const;

    /* Calls of the logger's halt callback. */
    size_t Halts() const;

    /* Times the switch let a report be made: Service asks it only when it is
     * about to make one. */
    size_t ReportsMade() const;

    /* Work the last pass did, and the most any pass did. */
    size_t DeviceOperationsInLastPass() const;
    size_t SendsInLastPass() const;
    size_t OpensInLastPass() const;
    size_t MaxDeviceOperationsInOnePass() const;
    size_t MaxSendsInOnePass() const;

    /* Whether FailDevice has been called. */
    bool DeviceHasFailed() const;

    /* The ring's size, which bounds what one pass can drain. */
    size_t RingBytes() const;

    /* What the queues say now. */
    bool StoreHasUnsent();
    bool StoreIsFrozen();

    ScenarioBench(const ScenarioBench&) = delete;
    ScenarioBench& operator=(const ScenarioBench&) = delete;

  private:
    /* Installed before any Core object is created, so a Create that reports
     * is on the record; the first member, so that it is. */
    struct HandlerInstallation
    {
        explicit HandlerInstallation(ScenarioBench* bench);
        ~HandlerInstallation();
        HandlerInstallation(const HandlerInstallation&) = delete;
        HandlerInstallation& operator=(const HandlerInstallation&) = delete;
    };

    /* The store the logger is given: the bench's own, counting the records it
     * keeps, since a record kept is what ends some of the store's runs of
     * loss. */
    struct CountingStore
    {
        struct SolidSyslogStore Base;
        struct SolidSyslogStore* Store;
        size_t Kept;
    };

    /* The device the store is given: the bench's own, counting the operations
     * asked of it, whatever stands behind it. GetBlockSize is a property, not
     * an operation, and is not counted. */
    struct CountingBlockDevice
    {
        struct SolidSyslogBlockDevice Base;
        struct SolidSyslogBlockDevice* Device;
        size_t Operations;
    };

    /* The sender the logger is given: the switching sender, watched, so a
     * report it drops while misrouted is on the record. */
    struct WatchedSender
    {
        struct SolidSyslogSender Base;
        ScenarioBench* Bench;
    };

    HandlerInstallation handler;
    ScenarioBenchOptions options;
    std::vector<uint8_t> ring;
    struct SolidSyslogBlockDevice* device;
    struct SolidSyslogFile* file;
    struct SolidSyslogBlockDevice* fileDevice;
    CountingBlockDevice countingDevice;
    struct SolidSyslogStream* stream;
    uint32_t streamVersion;
    struct SolidSyslogAtomicCounter* counter;
    struct SolidSyslogStructuredData* metaSd;
    struct SolidSyslogStructuredData* sds[1];
    struct SolidSyslogBuffer* buffer;
    struct SolidSyslogSecurityPolicy* securityPolicy;
    struct SolidSyslogStore* store;
    CountingStore countingStore;
    struct SolidSyslogSender* sender;
    struct SolidSyslogSender* senders[1];
    struct SolidSyslogSender* switchingSender;
    WatchedSender watchedSender;
    bool misrouted;
    std::vector<ScenarioReport> droppedReports;
    std::vector<ScenarioDelivery> droppedRecords;
    struct SolidSyslog* logger;

    unsigned logged;
    size_t passes;
    std::vector<ScenarioDelivery> delivered;
    std::vector<ScenarioReport> reports;
    std::vector<ScenarioEvent> events;
    size_t garbage;
    size_t halts;
    bool deviceFailed;
    size_t framesSeen;
    size_t deviceOperationsInLastPass;
    size_t sendsInLastPass;
    size_t opensInLastPass;
    size_t maxDeviceOperationsInOnePass;
    size_t maxSendsInOnePass;
    bool reporting;
    size_t reportsMade;

    void TakeNewFrames();
    static struct SolidSyslogBuffer* CreateBuffer(std::vector<uint8_t>& ring, void* policyContext);
    void OnError(const struct SolidSyslogErrorEvent* event);
    static void HandleError(void* context, const struct SolidSyslogErrorEvent* event);
    static enum SolidSyslogDiscardPolicy GetDiscardPolicy(void* context);
    static void OnHalt(void* context);
    bool OnReportingAsked();
    static bool IsReportingOn(void* context);
    static uint8_t Select(void* context);
    static bool WatchedSend(struct SolidSyslogSender* base, const void* buffer, size_t size);
    static void WatchedDisconnect(struct SolidSyslogSender* base);
    static CountingBlockDevice* CountingDeviceFrom(struct SolidSyslogBlockDevice* base);
    static bool CountingAcquire(struct SolidSyslogBlockDevice* base, size_t blockIndex);
    static bool CountingDispose(struct SolidSyslogBlockDevice* base, size_t blockIndex);
    static bool CountingExists(struct SolidSyslogBlockDevice* base, size_t blockIndex);
    static bool CountingRead(
        struct SolidSyslogBlockDevice* base,
        size_t blockIndex,
        size_t offset,
        void* buf,
        size_t count
    );
    static bool CountingAppend(struct SolidSyslogBlockDevice* base, size_t blockIndex, const void* buf, size_t count);
    static bool CountingWriteAt(
        struct SolidSyslogBlockDevice* base,
        size_t blockIndex,
        size_t offset,
        const void* buf,
        size_t count
    );
    static bool CountingSize(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size);
    static size_t CountingGetBlockSize(struct SolidSyslogBlockDevice* base);
    static CountingStore* CountingStoreFrom(struct SolidSyslogStore* base);
    static enum SolidSyslogStoreWriteResult CountingWrite(struct SolidSyslogStore* base, const void* data, size_t size);
    static bool CountingReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead);
    static void CountingMarkSent(struct SolidSyslogStore* base);
    static bool CountingHasUnsent(struct SolidSyslogStore* base);
    static size_t CountingGetTotalBytes(struct SolidSyslogStore* base);
    static size_t CountingGetUsedBytes(struct SolidSyslogStore* base);
    static bool CountingIsTransient(struct SolidSyslogStore* base);
};

#endif /* SCENARIOBENCH_H */
