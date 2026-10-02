# The VxWorks 6.4 runner's job dispatch. Dot-source, do not run.
# Written for Windows PowerShell 5.1.

Set-StrictMode -Version Latest

function Invoke-RunnerJob
    {
    param(
        [Parameter(Mandatory)] [hashtable] $Job,
        [Parameter(Mandatory)] [hashtable] $Actions
    )

    if ($Actions.ContainsKey($Job.type))
        {
        & $Actions[$Job.type] $Job.args
        }
    else
        {
        @{ Outcome = 'refused'; Summary = "unknown job '$($Job.type)'" }
        }
    }
