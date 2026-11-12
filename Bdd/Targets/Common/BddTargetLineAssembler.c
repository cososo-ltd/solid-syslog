#include "BddTargetLineAssembler.h"

#include <stdbool.h>
#include <stddef.h>

static inline void LineAssembler_StartLine(struct BddTargetLineAssembler* self);
static inline bool LineAssembler_IsLineEnd(char received);
static inline bool LineAssembler_EndLine(struct BddTargetLineAssembler* self);
static inline void LineAssembler_Append(struct BddTargetLineAssembler* self, char received);

void BddTargetLineAssembler_Reset(struct BddTargetLineAssembler* self)
{
    LineAssembler_StartLine(self);
}

static inline void LineAssembler_StartLine(struct BddTargetLineAssembler* self)
{
    self->Length = 0U;
    self->Text[0] = '\0';
    self->Completed = false;
}

bool BddTargetLineAssembler_Accept(struct BddTargetLineAssembler* self, char received)
{
    bool completed = false;
    if (LineAssembler_IsLineEnd(received))
    {
        completed = LineAssembler_EndLine(self);
    }
    else
    {
        LineAssembler_Append(self, received);
    }
    return completed;
}

static inline bool LineAssembler_IsLineEnd(char received)
{
    return (received == '\r') || (received == '\n');
}

/* A line end that follows a completed line, or nothing at all, ends no line:
 * it is the second of a CR LF pair, or a blank line. */
static inline bool LineAssembler_EndLine(struct BddTargetLineAssembler* self)
{
    bool completed = (!self->Completed) && (self->Length > 0U);
    if (completed)
    {
        self->Completed = true;
    }
    return completed;
}

static inline void LineAssembler_Append(struct BddTargetLineAssembler* self, char received)
{
    if (self->Completed)
    {
        LineAssembler_StartLine(self);
    }
    if (self->Length < (size_t) BDDTARGET_LINE_ASSEMBLER_CAPACITY)
    {
        self->Text[self->Length] = received;
        self->Length++;
        self->Text[self->Length] = '\0';
    }
}
