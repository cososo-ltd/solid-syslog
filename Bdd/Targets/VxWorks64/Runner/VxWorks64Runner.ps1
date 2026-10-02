# The VxWorks 6.4 runner's job dispatch. Dot-source, do not run.
# Written for Windows PowerShell 5.1.
#
# Each action is @{ Arguments = <the names it takes>; Run = { param($Arguments) ... } },
# and Run returns @{ Outcome = 'succeeded' | 'failed'; Summary = <text> }.
# A job is run only by an action of its type: the actions are the fixed set.

Set-StrictMode -Version Latest

function Invoke-RunnerJob
    {
    param(
        [Parameter(Mandatory)] [hashtable] $Job,
        [Parameter(Mandatory)] [hashtable] $Actions
    )

    if ($Actions.ContainsKey($Job.type))
        {
        & $Actions[$Job.type].Run $Job.args
        }
    else
        {
        @{ Outcome = 'refused'; Summary = "unknown job '$($Job.type)'" }
        }
    }
