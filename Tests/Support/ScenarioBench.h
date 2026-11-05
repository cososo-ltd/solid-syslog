#ifndef SCENARIOBENCH_H
#define SCENARIOBENCH_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "SolidSyslogDiscardPolicy.h"
#include "SolidSyslogPrival.h"
#include "SolidSyslogServiceStatus.h"
#include "SolidSyslogStoreDefinition.h"

struct SolidSyslog;
struct SolidSyslogAtomicCounter;
struct SolidSyslogBlockDevice;
struct SolidSyslogBuffer;
struct SolidSyslogErrorEvent;
struct SolidSyslogErrorSource;
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

    /* The logger's switch for reporting its own events, on until changed. */
    void SetReporting(bool on);

    /* What happened. */
    unsigned Logged() const;
    size_t Passes() const;
    const std::vector<ScenarioDelivery>& Delivered() const;
    const std::vector<ScenarioReport>& Reports() const;
    const std::vector<ScenarioEvent>& Events() const;

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

    HandlerInstallation handler;
    ScenarioBenchOptions options;
    std::vector<uint8_t> ring;
    struct SolidSyslogBlockDevice* device;
    struct SolidSyslogStream* stream;
    struct SolidSyslogAtomicCounter* counter;
    struct SolidSyslogStructuredData* metaSd;
    struct SolidSyslogStructuredData* sds[1];
    struct SolidSyslogBuffer* buffer;
    struct SolidSyslogStore* store;
    CountingStore countingStore;
    struct SolidSyslogSender* sender;
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
