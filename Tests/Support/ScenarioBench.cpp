#include "ScenarioBench.h"

#include <algorithm>
#include <cerrno>
#include <string>

#include "AddressFake.h"
#include "BlockDeviceFake.h"
#include "CounterFake.h"
#include "RamFileFake.h"
#include "ResolverFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockDevice.h"
#include "SolidSyslogFileBlockDevice.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogNullStore.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogSender.h"
#include "SolidSyslogStreamSender.h"
#include "SolidSyslogSwitchingSender.h"
#include "StreamFake.h"
#include "SyslogFieldParser.h"

namespace
{
/* Every record the bench logs carries its ordinal as MSG under one MSGID, so
 * a frame either parses to a record the bench wrote or is garbage. */
const char MESSAGE_ID[] = "SCN";

bool ParseDecimal(const std::string& text, unsigned* value)
{
    bool parsed = !text.empty();
    unsigned parsedValue = 0U;

    for (size_t i = 0U; parsed && (i < text.size()); i++)
    {
        parsed = (text[i] >= '0') && (text[i] <= '9');
        parsedValue = (parsedValue * 10U) + static_cast<unsigned>(text[i] - '0');
    }

    *value = parsedValue;
    return parsed;
}

/* The value of the quoted param that @p key opens, from a frame's SDATA. */
bool ParseQuoted(const std::string& sdata, const std::string& key, std::string* value)
{
    size_t start = sdata.find(key);
    bool found = start != std::string::npos;
    size_t valueStart = found ? (start + key.size()) : 0U;
    size_t end = found ? sdata.find('"', valueStart) : std::string::npos;
    found = found && (end != std::string::npos);
    *value = found ? sdata.substr(valueStart, end - valueStart) : std::string();
    return found;
}

/* The meta SD's sequenceId, from the frame's SDATA. */
bool ParseSequenceId(const std::string& sdata, unsigned* sequenceId)
{
    std::string text;
    return ParseQuoted(sdata, R"(sequenceId=")", &text) && ParseDecimal(text, sequenceId);
}

bool ParseFrame(const char* frame, unsigned* ordinal, unsigned* sequenceId)
{
    return (SyslogField(frame, SYSLOG_FIELD_MSGID) == MESSAGE_ID) && ParseDecimal(SyslogMsg(frame), ordinal) &&
           ParseSequenceId(SyslogField(frame, SYSLOG_FIELD_SDATA), sequenceId);
}

/* The library's element from a frame's SDATA, or empty when it has none. */
std::string LibraryElement(const std::string& sdata)
{
    size_t start = sdata.find("[solidsyslog@67478 ");
    size_t end = (start != std::string::npos) ? sdata.find(']', start) : std::string::npos;
    return (end != std::string::npos) ? sdata.substr(start, (end - start) + 1U) : std::string();
}

/* A report the logger made of its own event: the library's element is there,
 * its category param names the category the MSGID does, and the meta SD
 * numbers it as it does a record. */
bool ParseReport(const char* frame, ScenarioReport* report)
{
    std::string sdata = SyslogField(frame, SYSLOG_FIELD_SDATA);
    std::string element = LibraryElement(sdata);
    std::string named;
    bool parsed = !element.empty() && ParseQuoted(element, R"(category=")", &named) &&
                  ParseSequenceId(sdata, &report->SequenceId);
    report->Category = SyslogField(frame, SYSLOG_FIELD_MSGID);
    (void) ParseQuoted(element, R"(source=")", &report->SourceParam);
    (void) ParseQuoted(element, R"(detail=")", &report->DetailParam);
    (void) ParseQuoted(element, R"(native=")", &report->NativeParam);
    return parsed && (named == report->Category);
}

/* The params a report of the event carries, worked out as the library's
 * element writes them: no source without a name, a detail by name where its
 * source names it and by number otherwise, and a native code only where the
 * event has one. */
std::string SourceParamOf(const struct SolidSyslogErrorEvent* event)
{
    bool named = (event->Source != nullptr) && (event->Source->Name != nullptr);
    return named ? std::string(event->Source->Name) : std::string();
}

std::string DetailParamOf(const struct SolidSyslogErrorEvent* event)
{
    const char* name = SolidSyslogErrorEvent_DetailName(event);
    return (name != nullptr) ? std::string(name) : std::to_string(event->Detail);
}

std::string NativeParamOf(const struct SolidSyslogErrorEvent* event)
{
    return event->HasNativeCode ? std::to_string(event->NativeCode) : std::string();
}
} // namespace

ScenarioBench::HandlerInstallation::HandlerInstallation(ScenarioBench* bench)
{
    SolidSyslog_SetErrorHandler(HandleError, bench);
}

ScenarioBench::HandlerInstallation::~HandlerInstallation()
{
    SolidSyslog_SetErrorHandler(nullptr, nullptr);
}

ScenarioBench::ScenarioBench(const ScenarioBenchOptions& benchOptions)
    : handler(this), options(benchOptions), ring(SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(benchOptions.BufferMessages)),
      device(BlockDeviceFake_Create(benchOptions.BlockSize)), file(RamFileFake_Create()), fileDevice(nullptr),
      countingDevice(), stream(StreamFake_Create()), streamVersion(0U), counter(CounterFake_Create()), metaSd(nullptr),
      sds(), buffer(CreateBuffer(ring, this)), securityPolicy(SecurityPolicyFake_Create()), store(nullptr),
      countingStore(), sender(nullptr), senders(), switchingSender(nullptr), watchedSender(), misrouted(false),
      logger(nullptr), logged(0U), passes(0U), garbage(0U), halts(0U), deviceFailed(false), framesSeen(0U),
      deviceOperationsInLastPass(0U), sendsInLastPass(0U), opensInLastPass(0U), maxDeviceOperationsInOnePass(0U),
      maxSendsInOnePass(0U), reporting(true), reportsMade(0U)
{
    struct SolidSyslogMetaSdConfig metaConfig = {};
    metaConfig.Counter = counter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    sds[0] = metaSd;

    countingDevice.Base.Acquire = CountingAcquire;
    countingDevice.Base.Dispose = CountingDispose;
    countingDevice.Base.Exists = CountingExists;
    countingDevice.Base.Read = CountingRead;
    countingDevice.Base.Append = CountingAppend;
    countingDevice.Base.WriteAt = CountingWriteAt;
    countingDevice.Base.Size = CountingSize;
    countingDevice.Base.GetBlockSize = CountingGetBlockSize;
    if (options.FileBacked)
    {
        fileDevice = SolidSyslogFileBlockDevice_Create(file, "scenario", options.BlockSize);
    }
    countingDevice.Device = options.FileBacked ? fileDevice : device;

    struct SolidSyslogBlockStoreConfig storeConfig = {};
    storeConfig.BlockDevice = &countingDevice.Base;
    storeConfig.MaxBlocks = options.MaxBlocks;
    storeConfig.SecurityPolicy = securityPolicy;
    storeConfig.GetDiscardPolicy = GetDiscardPolicy;
    storeConfig.DiscardPolicyContext = this;
    store = options.HasStore ? SolidSyslogBlockStore_Create(&storeConfig) : SolidSyslogNullStore_Get();
    countingStore.Base.Write = CountingWrite;
    countingStore.Base.ReadNextUnsent = CountingReadNextUnsent;
    countingStore.Base.MarkSent = CountingMarkSent;
    countingStore.Base.HasUnsent = CountingHasUnsent;
    countingStore.Base.GetTotalBytes = CountingGetTotalBytes;
    countingStore.Base.GetUsedBytes = CountingGetUsedBytes;
    countingStore.Base.IsTransient = CountingIsTransient;
    countingStore.Store = store;

    struct SolidSyslogStreamSenderConfig senderConfig = {};
    senderConfig.Resolver = ResolverFake_Get();
    senderConfig.Stream = stream;
    senderConfig.Address = AddressFake_Get();
    sender = SolidSyslogStreamSender_Create(&senderConfig);
    senders[0] = sender;

    struct SolidSyslogSwitchingSenderConfig switchingConfig = {};
    switchingConfig.Senders = senders;
    switchingConfig.SenderCount = 1U;
    switchingConfig.Selector = Select;
    switchingConfig.SelectorContext = this;
    switchingSender = SolidSyslogSwitchingSender_Create(&switchingConfig);
    watchedSender.Base.Send = WatchedSend;
    watchedSender.Base.Disconnect = WatchedDisconnect;
    watchedSender.Bench = this;

    struct SolidSyslogConfig config = {};
    config.Buffer = buffer;
    config.Sender = &watchedSender.Base;
    config.Store = &countingStore.Base;
    config.Sd = sds;
    config.SdCount = 1U;
    config.GetDiscardPolicy = GetDiscardPolicy;
    config.DiscardPolicyContext = this;
    config.OnHalt = OnHalt;
    config.HaltContext = this;
    config.IsEventReportingOn = IsReportingOn;
    config.EventReportingContext = this;
    logger = SolidSyslog_Create(&config);
}

struct SolidSyslogBuffer* ScenarioBench::CreateBuffer(std::vector<uint8_t>& ring, void* policyContext)
{
    return SolidSyslogCircularBuffer_Create(
        SolidSyslogNullMutex_Get(),
        ring.data(),
        ring.size(),
        GetDiscardPolicy,
        policyContext
    );
}

ScenarioBench::~ScenarioBench()
{
    /* The logger goes first. Destroying it also forgets whatever the walk
     * left pending, so the next bench's logger reports none of it. */
    SolidSyslog_Destroy(logger);
    SolidSyslogSwitchingSender_Destroy(switchingSender);
    SolidSyslogStreamSender_Destroy(sender);
    if (options.HasStore)
    {
        SolidSyslogBlockStore_Destroy(store);
    }
    SecurityPolicyFake_Destroy(securityPolicy);
    if (options.FileBacked)
    {
        SolidSyslogFileBlockDevice_Destroy(fileDevice);
    }
    RamFileFake_Destroy(file);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogMetaSd_Destroy(metaSd);
    CounterFake_Destroy(counter);
    StreamFake_Destroy(stream);
}

unsigned ScenarioBench::Log()
{
    logged++;
    std::string text = std::to_string(logged);
    struct SolidSyslogMessage message =
        {SOLIDSYSLOG_FACILITY_USER, SOLIDSYSLOG_SEVERITY_INFORMATIONAL, MESSAGE_ID, text.c_str()};
    SolidSyslog_Log(logger, &message);
    return logged;
}

enum SolidSyslogServiceStatus ScenarioBench::Service()
{
    size_t operationsBefore = countingDevice.Operations;
    int sendsBefore = StreamFake_SendCallCount(stream);
    int opensBefore = StreamFake_OpenCallCount(stream);
    passes++;
    enum SolidSyslogServiceStatus status = SolidSyslog_Service(logger);
    deviceOperationsInLastPass = countingDevice.Operations - operationsBefore;
    sendsInLastPass = static_cast<size_t>(StreamFake_SendCallCount(stream) - sendsBefore);
    opensInLastPass = static_cast<size_t>(StreamFake_OpenCallCount(stream) - opensBefore);
    maxDeviceOperationsInOnePass = std::max(maxDeviceOperationsInOnePass, deviceOperationsInLastPass);
    maxSendsInOnePass = std::max(maxSendsInOnePass, sendsInLastPass);
    TakeNewFrames();
    return status;
}

void ScenarioBench::TakeNewFrames()
{
    for (; framesSeen < StreamFake_FrameCount(stream); framesSeen++)
    {
        const char* frame = StreamFake_Frame(stream, framesSeen);
        ScenarioDelivery delivery = {0U, 0U, passes};
        ScenarioReport report = {std::string(), 0U, passes, std::string(), std::string(), std::string()};

        if (ParseFrame(frame, &delivery.Ordinal, &delivery.SequenceId))
        {
            delivered.push_back(delivery);
        }
        else if (ParseReport(frame, &report))
        {
            reports.push_back(report);
        }
        else
        {
            garbage++;
        }
    }
}

void ScenarioBench::SetSenderDown(bool down)
{
    StreamFake_SetSendFails(stream, down);
}

/* A file-backed store's device fails through its file system, as a broken
 * disk's does, which gives EIO. */
void ScenarioBench::FailDevice()
{
    deviceFailed = true;
    if (options.FileBacked)
    {
        RamFileFake_FailEveryOperation(file, EIO);
    }
    else
    {
        BlockDeviceFake_FailEveryOperation();
    }
}

void ScenarioBench::SetSecurityPolicyFault(enum SecurityPolicyFakeFault fault)
{
    SecurityPolicyFake_SetFault(securityPolicy, fault);
}

/* The change is made before the version moves, as the stream contract asks. */
void ScenarioBench::SetHandshakeRefused(bool refused)
{
    StreamFake_RefuseHandshake(stream, refused);
    streamVersion++;
    StreamFake_SetVersion(stream, streamVersion);
}

void ScenarioBench::SetMisrouted(bool isMisrouted)
{
    misrouted = isMisrouted;
}

const std::vector<ScenarioReport>& ScenarioBench::DroppedReports() const
{
    return droppedReports;
}

const std::vector<ScenarioDelivery>& ScenarioBench::DroppedRecords() const
{
    return droppedRecords;
}

/* The one sender is index 0, so 1 is out of range. */
uint8_t ScenarioBench::Select(void* context)
{
    return static_cast<ScenarioBench*>(context)->misrouted ? 1U : 0U;
}

/* The switching sender asks the selector on every send, so a report or a
 * record handed to it while misrouted is one it drops. */
bool ScenarioBench::WatchedSend(struct SolidSyslogSender* base, const void* buffer, size_t size)
{
    ScenarioBench* bench = reinterpret_cast<WatchedSender*>(base)->Bench;
    ScenarioReport report = {std::string(), 0U, bench->passes, std::string(), std::string(), std::string()};
    ScenarioDelivery record = {0U, 0U, bench->passes};
    std::string frame(static_cast<const char*>(buffer), size);
    if (bench->misrouted && ParseReport(frame.c_str(), &report))
    {
        bench->droppedReports.push_back(report);
    }
    else if (bench->misrouted && ParseFrame(frame.c_str(), &record.Ordinal, &record.SequenceId))
    {
        bench->droppedRecords.push_back(record);
    }
    return SolidSyslogSender_Send(bench->switchingSender, buffer, size);
}

void ScenarioBench::WatchedDisconnect(struct SolidSyslogSender* base)
{
    SolidSyslogSender_Disconnect(reinterpret_cast<WatchedSender*>(base)->Bench->switchingSender);
}

void ScenarioBench::CorruptTheRing()
{
    std::fill(ring.begin(), ring.end(), static_cast<uint8_t>(0xFFU));
}

bool ScenarioBench::DeviceHasFailed() const
{
    return deviceFailed;
}

size_t ScenarioBench::RingBytes() const
{
    return ring.size();
}

void ScenarioBench::SetDiscardPolicy(enum SolidSyslogDiscardPolicy policy)
{
    options.DiscardPolicy = policy;
}

void ScenarioBench::SetReporting(bool on)
{
    reporting = on;
}

unsigned ScenarioBench::Logged() const
{
    return logged;
}

size_t ScenarioBench::Passes() const
{
    return passes;
}

const std::vector<ScenarioDelivery>& ScenarioBench::Delivered() const
{
    return delivered;
}

const std::vector<ScenarioReport>& ScenarioBench::Reports() const
{
    return reports;
}

const std::vector<ScenarioEvent>& ScenarioBench::Events() const
{
    return events;
}

size_t ScenarioBench::Garbage() const
{
    return garbage;
}

size_t ScenarioBench::Halts() const
{
    return halts;
}

size_t ScenarioBench::ReportsMade() const
{
    return reportsMade;
}

size_t ScenarioBench::DeviceOperationsInLastPass() const
{
    return deviceOperationsInLastPass;
}

size_t ScenarioBench::SendsInLastPass() const
{
    return sendsInLastPass;
}

size_t ScenarioBench::OpensInLastPass() const
{
    return opensInLastPass;
}

size_t ScenarioBench::MaxDeviceOperationsInOnePass() const
{
    return maxDeviceOperationsInOnePass;
}

size_t ScenarioBench::MaxSendsInOnePass() const
{
    return maxSendsInOnePass;
}

bool ScenarioBench::StoreHasUnsent()
{
    return SolidSyslogStore_HasUnsent(store);
}

bool ScenarioBench::StoreIsFrozen()
{
    return SolidSyslogStore_IsTransient(store);
}

void ScenarioBench::HandleError(void* context, const struct SolidSyslogErrorEvent* event)
{
    static_cast<ScenarioBench*>(context)->OnError(event);
}

void ScenarioBench::OnError(const struct SolidSyslogErrorEvent* event)
{
    ScenarioEvent recorded = {
        event->Severity,
        event->Source,
        event->Category,
        event->Detail,
        passes,
        countingStore.Kept,
        CounterFake_LastHandedOut(counter),
        SourceParamOf(event),
        DetailParamOf(event),
        NativeParamOf(event)
    };
    events.push_back(recorded);
}

enum SolidSyslogDiscardPolicy ScenarioBench::GetDiscardPolicy(void* context)
{
    return static_cast<ScenarioBench*>(context)->options.DiscardPolicy;
}

void ScenarioBench::OnHalt(void* context)
{
    static_cast<ScenarioBench*>(context)->halts++;
}

bool ScenarioBench::IsReportingOn(void* context)
{
    return static_cast<ScenarioBench*>(context)->OnReportingAsked();
}

bool ScenarioBench::OnReportingAsked()
{
    if (reporting)
    {
        reportsMade++;
    }
    return reporting;
}

/* Base is the first member, so the store's device is the counting device. */
ScenarioBench::CountingBlockDevice* ScenarioBench::CountingDeviceFrom(struct SolidSyslogBlockDevice* base)
{
    return reinterpret_cast<CountingBlockDevice*>(base);
}

bool ScenarioBench::CountingAcquire(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Acquire(self->Device, blockIndex);
}

bool ScenarioBench::CountingDispose(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Dispose(self->Device, blockIndex);
}

bool ScenarioBench::CountingExists(struct SolidSyslogBlockDevice* base, size_t blockIndex)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Exists(self->Device, blockIndex);
}

bool ScenarioBench::CountingRead(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    void* buf,
    size_t count
)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Read(self->Device, blockIndex, offset, buf, count);
}

bool ScenarioBench::CountingAppend(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    const void* buf,
    size_t count
)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Append(self->Device, blockIndex, buf, count);
}

bool ScenarioBench::CountingWriteAt(
    struct SolidSyslogBlockDevice* base,
    size_t blockIndex,
    size_t offset,
    const void* buf,
    size_t count
)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_WriteAt(self->Device, blockIndex, offset, buf, count);
}

bool ScenarioBench::CountingSize(struct SolidSyslogBlockDevice* base, size_t blockIndex, size_t* size)
{
    CountingBlockDevice* self = CountingDeviceFrom(base);
    self->Operations++;
    return SolidSyslogBlockDevice_Size(self->Device, blockIndex, size);
}

size_t ScenarioBench::CountingGetBlockSize(struct SolidSyslogBlockDevice* base)
{
    return SolidSyslogBlockDevice_GetBlockSize(CountingDeviceFrom(base)->Device);
}

/* Base is the first member, so the logger's store is the counting store. */
ScenarioBench::CountingStore* ScenarioBench::CountingStoreFrom(struct SolidSyslogStore* base)
{
    return reinterpret_cast<CountingStore*>(base);
}

enum SolidSyslogStoreWriteResult ScenarioBench::CountingWrite(
    struct SolidSyslogStore* base,
    const void* data,
    size_t size
)
{
    CountingStore* self = CountingStoreFrom(base);
    enum SolidSyslogStoreWriteResult result = SolidSyslogStore_Write(self->Store, data, size);
    if (result == SOLIDSYSLOG_STORE_WRITE_KEPT)
    {
        self->Kept++;
    }
    return result;
}

bool ScenarioBench::CountingReadNextUnsent(struct SolidSyslogStore* base, void* data, size_t maxSize, size_t* bytesRead)
{
    return SolidSyslogStore_ReadNextUnsent(CountingStoreFrom(base)->Store, data, maxSize, bytesRead);
}

void ScenarioBench::CountingMarkSent(struct SolidSyslogStore* base)
{
    SolidSyslogStore_MarkSent(CountingStoreFrom(base)->Store);
}

bool ScenarioBench::CountingHasUnsent(struct SolidSyslogStore* base)
{
    return SolidSyslogStore_HasUnsent(CountingStoreFrom(base)->Store);
}

size_t ScenarioBench::CountingGetTotalBytes(struct SolidSyslogStore* base)
{
    return SolidSyslogStore_GetTotalBytes(CountingStoreFrom(base)->Store);
}

size_t ScenarioBench::CountingGetUsedBytes(struct SolidSyslogStore* base)
{
    return SolidSyslogStore_GetUsedBytes(CountingStoreFrom(base)->Store);
}

bool ScenarioBench::CountingIsTransient(struct SolidSyslogStore* base)
{
    return SolidSyslogStore_IsTransient(CountingStoreFrom(base)->Store);
}
