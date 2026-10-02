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

    $refusal = Get-RunnerJobRefusal -Job $Job -Actions $Actions
    if ($refusal)
        {
        @{ Outcome = 'refused'; Summary = $refusal }
        }
    else
        {
        & $Actions[$Job.type].Run $Job.args
        }
    }

# Why the job may not run, or nothing if it may.
function Get-RunnerJobRefusal
    {
    param(
        [Parameter(Mandatory)] [hashtable] $Job,
        [Parameter(Mandatory)] [hashtable] $Actions
    )

    if (-not $Actions.ContainsKey($Job.type))
        {
        "unknown job '$($Job.type)'"
        }
    else
        {
        $unexpected = @($Job.args.Keys | Where-Object { $Actions[$Job.type].Arguments -notcontains $_ })
        if ($unexpected.Count -gt 0)
            {
            "job '$($Job.type)' does not take $($unexpected -join ', ')"
            }
        }
    }
