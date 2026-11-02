#include "ScenarioBench.h"

#include <algorithm>
#include <string>

#include "AddressFake.h"
#include "BlockDeviceFake.h"
#include "CounterFake.h"
#include "ResolverFake.h"
#include "SolidSyslog.h"
#include "SolidSyslogBlockStore.h"
#include "SolidSyslogCircularBuffer.h"
#include "SolidSyslogConfig.h"
#include "SolidSyslogError.h"
#include "SolidSyslogMetaSd.h"
#include "SolidSyslogNullMutex.h"
#include "SolidSyslogStore.h"
#include "SolidSyslogStreamSender.h"
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

/* The meta SD's sequenceId, from the frame's SDATA. */
bool ParseSequenceId(const std::string& sdata, unsigned* sequenceId)
{
    const std::string key = "sequenceId=\"";
    size_t start = sdata.find(key);
    bool found = start != std::string::npos;
    size_t valueStart = found ? (start + key.size()) : 0U;
    size_t end = found ? sdata.find('"', valueStart) : std::string::npos;
    found = found && (end != std::string::npos);
    return found && ParseDecimal(sdata.substr(valueStart, end - valueStart), sequenceId);
}

bool ParseFrame(const char* frame, unsigned* ordinal, unsigned* sequenceId)
{
    return (SyslogField(frame, SYSLOG_FIELD_MSGID) == MESSAGE_ID) && ParseDecimal(SyslogMsg(frame), ordinal) &&
           ParseSequenceId(SyslogField(frame, SYSLOG_FIELD_SDATA), sequenceId);
}
} // namespace

ScenarioBench::ScenarioBench(const ScenarioBenchOptions& benchOptions)
    : options(benchOptions), ring(SOLIDSYSLOG_CIRCULAR_BUFFER_RING_BYTES(benchOptions.BufferMessages)),
      device(BlockDeviceFake_Create(benchOptions.BlockSize)), stream(StreamFake_Create()),
      counter(CounterFake_Create()), metaSd(nullptr), sds(), buffer(CreateBuffer(ring, this)), store(nullptr),
      sender(nullptr), logger(nullptr), logged(0U), passes(0U), garbage(0U), halts(0U), deviceFailed(false),
      framesSeen(0U), deviceOperationsInLastPass(0U), sendsInLastPass(0U), maxDeviceOperationsInOnePass(0U),
      maxSendsInOnePass(0U)
{
    SolidSyslog_SetErrorHandler(HandleError, this);

    struct SolidSyslogMetaSdConfig metaConfig = {};
    metaConfig.Counter = counter;
    metaSd = SolidSyslogMetaSd_Create(&metaConfig);
    sds[0] = metaSd;

    struct SolidSyslogBlockStoreConfig storeConfig = {};
    storeConfig.BlockDevice = device;
    storeConfig.MaxBlocks = options.MaxBlocks;
    storeConfig.GetDiscardPolicy = GetDiscardPolicy;
    storeConfig.DiscardPolicyContext = this;
    store = SolidSyslogBlockStore_Create(&storeConfig);

    struct SolidSyslogStreamSenderConfig senderConfig = {};
    senderConfig.Resolver = ResolverFake_Get();
    senderConfig.Stream = stream;
    senderConfig.Address = AddressFake_Get();
    sender = SolidSyslogStreamSender_Create(&senderConfig);

    struct SolidSyslogConfig config = {};
    config.Buffer = buffer;
    config.Sender = sender;
    config.Store = store;
    config.Sd = sds;
    config.SdCount = 1U;
    config.GetDiscardPolicy = GetDiscardPolicy;
    config.DiscardPolicyContext = this;
    config.OnHalt = OnHalt;
    config.HaltContext = this;
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
    SolidSyslog_Destroy(logger);
    SolidSyslogStreamSender_Destroy(sender);
    SolidSyslogBlockStore_Destroy(store);
    SolidSyslogCircularBuffer_Destroy(buffer);
    SolidSyslogMetaSd_Destroy(metaSd);
    CounterFake_Destroy(counter);
    StreamFake_Destroy(stream);
    SolidSyslog_SetErrorHandler(nullptr, nullptr);
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
    size_t operationsBefore = BlockDeviceFake_OperationCount();
    int sendsBefore = StreamFake_SendCallCount(stream);
    passes++;
    enum SolidSyslogServiceStatus status = SolidSyslog_Service(logger);
    deviceOperationsInLastPass = BlockDeviceFake_OperationCount() - operationsBefore;
    sendsInLastPass = static_cast<size_t>(StreamFake_SendCallCount(stream) - sendsBefore);
    maxDeviceOperationsInOnePass = std::max(maxDeviceOperationsInOnePass, deviceOperationsInLastPass);
    maxSendsInOnePass = std::max(maxSendsInOnePass, sendsInLastPass);
    TakeNewFrames();
    return status;
}

void ScenarioBench::TakeNewFrames()
{
    for (; framesSeen < StreamFake_FrameCount(stream); framesSeen++)
    {
        ScenarioDelivery delivery = {0U, 0U, passes};

        if (ParseFrame(StreamFake_Frame(stream, framesSeen), &delivery.Ordinal, &delivery.SequenceId))
        {
            delivered.push_back(delivery);
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

void ScenarioBench::FailDevice()
{
    deviceFailed = true;
    BlockDeviceFake_FailEveryOperation();
}

bool ScenarioBench::DeviceHasFailed() const
{
    return deviceFailed;
}

void ScenarioBench::SetDiscardPolicy(enum SolidSyslogDiscardPolicy policy)
{
    options.DiscardPolicy = policy;
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

size_t ScenarioBench::DeviceOperationsInLastPass() const
{
    return deviceOperationsInLastPass;
}

size_t ScenarioBench::SendsInLastPass() const
{
    return sendsInLastPass;
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
    ScenarioEvent recorded = {event->Severity, event->Source, event->Category, event->Detail, passes};
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
