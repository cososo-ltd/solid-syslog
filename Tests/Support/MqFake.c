#include "MqFake.h"

#include <errno.h>
#include <fcntl.h>
#include <mqueue.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>

#include "SafeString.h"
#include "SolidSyslogTunables.h"

enum
{
    MQFAKE_MAX_QUEUES = 4,
    /* Storage cap for the in-memory ring per queue. Per-call enforcement
     * uses the caller's `mq_attr.mq_maxmsg` instead, so this value just
     * needs to be at least as large as the largest `maxMessages` any test
     * passes to `mq_open`. Bumping it is cheap (BSS only). */
    MQFAKE_MAX_MESSAGES_PER_QUEUE = 16,
    MQFAKE_MAX_OPEN_HISTORY = 8,
    MQFAKE_MAX_NAME_LEN = 64,
    MQFAKE_HANDLE_BASE = 100,
    MQFAKE_SEND_BUF_SIZE = SOLIDSYSLOG_MAX_MESSAGE_SIZE + 1U
};

struct MqFakeQueue
{
    long maxMessages;
    size_t maxMessageSize;
    size_t messageLens[MQFAKE_MAX_MESSAGES_PER_QUEUE];
    int head;
    int tail;
    int count;
    bool inUse;
    char messages[MQFAKE_MAX_MESSAGES_PER_QUEUE][SOLIDSYSLOG_MAX_MESSAGE_SIZE];
};

static struct MqFakeQueue queues[MQFAKE_MAX_QUEUES];

static bool nextOpenShouldFail;
static int nextOpenErrno;
static int openCallCount;
static char openNameHistory[MQFAKE_MAX_OPEN_HISTORY][MQFAKE_MAX_NAME_LEN];
static char lastOpenName[MQFAKE_MAX_NAME_LEN];
static int lastOpenOflag;
static long lastOpenMaxMessages;
static size_t lastOpenMaxMessageSize;

static bool nextSendShouldFail;
static int nextSendErrno;
static int sendCallCount;
static mqd_t lastSendMqd;
static char lastSendBufCopy[MQFAKE_SEND_BUF_SIZE];
static size_t lastSendLen;

static bool nextReceiveShouldFail;
static int nextReceiveErrno;
static int receiveCallCount;
static mqd_t lastReceiveMqd;
static size_t lastReceiveMaxLen;

static int closeCallCount;
static mqd_t lastClosedMqd;

static int unlinkCallCount;
static char lastUnlinkName[MQFAKE_MAX_NAME_LEN];

void MqFake_Reset(void)
{
    /* Reset errno so a stale value from a prior test cannot leak. */
    errno = 0;

    for (size_t i = 0; i < (size_t) MQFAKE_MAX_QUEUES; i++)
    {
        queues[i].inUse = false;
        queues[i].maxMessages = 0;
        queues[i].maxMessageSize = 0;
        queues[i].head = 0;
        queues[i].tail = 0;
        queues[i].count = 0;
        for (size_t m = 0; m < (size_t) MQFAKE_MAX_MESSAGES_PER_QUEUE; m++)
        {
            queues[i].messageLens[m] = 0;
        }
    }

    nextOpenShouldFail = false;
    nextOpenErrno = 0;
    openCallCount = 0;
    for (size_t i = 0; i < (size_t) MQFAKE_MAX_OPEN_HISTORY; i++)
    {
        openNameHistory[i][0] = '\0';
    }
    lastOpenName[0] = '\0';
    lastOpenOflag = 0;
    lastOpenMaxMessages = 0;
    lastOpenMaxMessageSize = 0;

    nextSendShouldFail = false;
    nextSendErrno = 0;
    sendCallCount = 0;
    lastSendMqd = (mqd_t) -1;
    lastSendBufCopy[0] = '\0';
    lastSendLen = 0;

    nextReceiveShouldFail = false;
    nextReceiveErrno = 0;
    receiveCallCount = 0;
    lastReceiveMqd = (mqd_t) -1;
    lastReceiveMaxLen = 0;

    closeCallCount = 0;
    lastClosedMqd = (mqd_t) -1;

    unlinkCallCount = 0;
    lastUnlinkName[0] = '\0';
}

void MqFake_FailNextOpen(int errnoValue)
{
    nextOpenShouldFail = true;
    nextOpenErrno = errnoValue;
}

int MqFake_OpenCallCount(void)
{
    return openCallCount;
}

const char* MqFake_LastOpenName(void)
{
    return lastOpenName;
}

int MqFake_LastOpenOflag(void)
{
    return lastOpenOflag;
}

long MqFake_LastOpenMaxMessages(void)
{
    return lastOpenMaxMessages;
}

size_t MqFake_LastOpenMaxMessageSize(void)
{
    return lastOpenMaxMessageSize;
}

const char* MqFake_OpenNameAt(int callIndex)
{
    const char* result = "";
    if ((callIndex >= 0) && (callIndex < MQFAKE_MAX_OPEN_HISTORY))
    {
        result = openNameHistory[callIndex];
    }
    return result;
}

void MqFake_FailNextSend(int errnoValue)
{
    nextSendShouldFail = true;
    nextSendErrno = errnoValue;
}

int MqFake_SendCallCount(void)
{
    return sendCallCount;
}

mqd_t MqFake_LastSendMqd(void)
{
    return lastSendMqd;
}

const char* MqFake_LastSendBufAsString(void)
{
    return lastSendBufCopy;
}

size_t MqFake_LastSendLen(void)
{
    return lastSendLen;
}

void MqFake_FailNextReceive(int errnoValue)
{
    nextReceiveShouldFail = true;
    nextReceiveErrno = errnoValue;
}

int MqFake_ReceiveCallCount(void)
{
    return receiveCallCount;
}

mqd_t MqFake_LastReceiveMqd(void)
{
    return lastReceiveMqd;
}

size_t MqFake_LastReceiveMaxLen(void)
{
    return lastReceiveMaxLen;
}

int MqFake_CloseCallCount(void)
{
    return closeCallCount;
}

mqd_t MqFake_LastClosedMqd(void)
{
    return lastClosedMqd;
}

int MqFake_UnlinkCallCount(void)
{
    return unlinkCallCount;
}

const char* MqFake_LastUnlinkName(void)
{
    return lastUnlinkName;
}

static int MqFake_AllocateQueue(void)
{
    int result = -1;
    for (int i = 0; i < MQFAKE_MAX_QUEUES; i++)
    {
        if (!queues[i].inUse)
        {
            queues[i].inUse = true;
            queues[i].head = 0;
            queues[i].tail = 0;
            queues[i].count = 0;
            result = i;
            break;
        }
    }
    return result;
}

static struct MqFakeQueue* MqFake_QueueFromMqd(mqd_t mqd)
{
    struct MqFakeQueue* result = NULL;
    int index = (int) mqd - MQFAKE_HANDLE_BASE;
    if ((index >= 0) && (index < MQFAKE_MAX_QUEUES) && queues[index].inUse)
    {
        result = &queues[index];
    }
    return result;
}

/* NOLINTNEXTLINE(cert-dcl50-cpp,readability-inconsistent-declaration-parameter-name) -- POSIX API; varargs and parameter names differ from glibc internal names */
mqd_t mq_open(const char* name, int oflag, ...)
{
    mqd_t result = (mqd_t) -1;

    if (nextOpenShouldFail)
    {
        nextOpenShouldFail = false;
        errno = nextOpenErrno;
    }
    else
    {
        long requestedMaxMessages = 0;
        size_t requestedMaxMessageSize = 0;
        if ((oflag & O_CREAT) != 0)
        {
            va_list ap;
            va_start(ap, oflag);
            (void) va_arg(ap, mode_t); /* mode — unused by fake */
            const struct mq_attr* attr = va_arg(ap, struct mq_attr*);
            va_end(ap);
            if (attr != NULL)
            {
                requestedMaxMessages = attr->mq_maxmsg;
                requestedMaxMessageSize = (size_t) attr->mq_msgsize;
            }
        }

        int index = MqFake_AllocateQueue();
        if (index >= 0)
        {
            queues[index].maxMessages = requestedMaxMessages;
            queues[index].maxMessageSize = requestedMaxMessageSize;
            result = (mqd_t) (index + MQFAKE_HANDLE_BASE);
        }
        else
        {
            errno = EMFILE;
        }

        lastOpenOflag = oflag;
        lastOpenMaxMessages = requestedMaxMessages;
        lastOpenMaxMessageSize = requestedMaxMessageSize;
    }

    SafeString_Copy(lastOpenName, sizeof(lastOpenName), name);
    if (openCallCount < MQFAKE_MAX_OPEN_HISTORY)
    {
        SafeString_Copy(openNameHistory[openCallCount], sizeof(openNameHistory[openCallCount]), name);
    }
    openCallCount++;
    return result;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name) -- POSIX API; parameter names differ from glibc internal names
int mq_close(mqd_t mqd)
{
    closeCallCount++;
    lastClosedMqd = mqd;
    struct MqFakeQueue* queue = MqFake_QueueFromMqd(mqd);
    if (queue != NULL)
    {
        queue->inUse = false;
    }
    return 0;
}

int mq_unlink(const char* name)
{
    unlinkCallCount++;
    SafeString_Copy(lastUnlinkName, sizeof(lastUnlinkName), name);
    return 0;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters) -- POSIX API; parameter names and signature differ from glibc internal names
int mq_send(mqd_t mqd, const char* msg, size_t msgLen, unsigned int msgPrio)
{
    (void) msgPrio;

    sendCallCount++;
    lastSendMqd = mqd;
    lastSendLen = msgLen;
    size_t copyLen = (msgLen < (sizeof(lastSendBufCopy) - 1U)) ? msgLen : (sizeof(lastSendBufCopy) - 1U);
    if (copyLen > 0U)
    {
        memcpy(lastSendBufCopy, msg, copyLen);
    }
    lastSendBufCopy[copyLen] = '\0';

    int result = -1;
    if (nextSendShouldFail)
    {
        nextSendShouldFail = false;
        errno = nextSendErrno;
    }
    else
    {
        struct MqFakeQueue* queue = MqFake_QueueFromMqd(mqd);
        if (queue == NULL)
        {
            errno = EBADF;
        }
        else if (msgLen > queue->maxMessageSize)
        {
            /* POSIX: mq_send must return -1/EMSGSIZE when msgLen exceeds the
             * queue's per-message size. Truncating instead would mask real-kernel
             * behaviour from tests. */
            errno = EMSGSIZE;
        }
        else if (queue->count >= (int) queue->maxMessages)
        {
            /* POSIX: with O_NONBLOCK and the queue at capacity, mq_send must
             * return -1/EAGAIN. Use the per-queue limit captured at mq_open,
             * not the fake's storage cap. */
            errno = EAGAIN;
        }
        else
        {
            if (msgLen > 0U)
            {
                memcpy(queue->messages[queue->tail], msg, msgLen);
            }
            queue->messageLens[queue->tail] = msgLen;
            queue->tail = (queue->tail + 1) % MQFAKE_MAX_MESSAGES_PER_QUEUE;
            queue->count++;
            result = 0;
        }
    }
    return result;
}

// NOLINTNEXTLINE(readability-inconsistent-declaration-parameter-name,bugprone-easily-swappable-parameters,readability-non-const-parameter) -- POSIX API; parameter names and signature differ from glibc internal names
ssize_t mq_receive(mqd_t mqd, char* msg, size_t msgLen, unsigned int* msgPrio)
{
    (void) msgPrio;

    receiveCallCount++;
    lastReceiveMqd = mqd;
    lastReceiveMaxLen = msgLen;

    ssize_t result = -1;
    if (nextReceiveShouldFail)
    {
        nextReceiveShouldFail = false;
        errno = nextReceiveErrno;
    }
    else
    {
        struct MqFakeQueue* queue = MqFake_QueueFromMqd(mqd);
        if (queue == NULL)
        {
            errno = EBADF;
        }
        else if (queue->count == 0)
        {
            errno = EAGAIN;
        }
        else
        {
            size_t pending = queue->messageLens[queue->head];
            if (pending > msgLen)
            {
                errno = EMSGSIZE;
            }
            else
            {
                if (pending > 0U)
                {
                    memcpy(msg, queues[(int) mqd - MQFAKE_HANDLE_BASE].messages[queue->head], pending);
                }
                queue->head = (queue->head + 1) % MQFAKE_MAX_MESSAGES_PER_QUEUE;
                queue->count--;
                result = (ssize_t) pending;
            }
        }
    }
    return result;
}
