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
        try
            {
            & $Actions[$Job.type].Run $Job.args
            }
        catch
            {
            @{ Outcome = 'failed'; Summary = $_.Exception.Message }
            }
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
        $missing = @($Actions[$Job.type].Arguments | Where-Object { -not $Job.args.ContainsKey($_) })
        if ($unexpected.Count -gt 0)
            {
            "job '$($Job.type)' does not take $($unexpected -join ', ')"
            }
        elseif ($missing.Count -gt 0)
            {
            "job '$($Job.type)' needs $($missing -join ', ')"
            }
        else
            {
            $unsafe = @($Job.args.Keys | Where-Object { -not (Test-RunnerArgumentValue $Job.args[$_]) })
            if ($unsafe.Count -gt 0)
                {
                "job '$($Job.type)' has an unsafe value for $($unsafe -join ', ')"
                }
            }
        }
    }

# Values reach command lines, so none may start like an option or carry a quote
# or a separator.
function Test-RunnerArgumentValue
    {
    param([AllowEmptyString()] [string] $Value)

    $Value -cmatch '^[A-Za-z0-9][A-Za-z0-9._/:@ -]*\z'
    }

# In C#, because the TLS validation callback that uses it runs on threads
# where a PowerShell script block cannot.
if (-not ('SolidSyslogRunnerPinning' -as [type]))
    {
    Add-Type -TypeDefinition @'
using System;
using System.Net;
using System.Security.Cryptography.X509Certificates;

public static class SolidSyslogRunnerPinning
{
    private static string pinned;

    // Every HTTPS request this process makes then trusts only that certificate,
    // whoever signed it, and speaks TLS 1.2, which Windows PowerShell 5.1 may
    // not offer by default.
    public static void Install(string thumbprint)
    {
        pinned = thumbprint;
        ServicePointManager.SecurityProtocol = SecurityProtocolType.Tls12;
        ServicePointManager.ServerCertificateValidationCallback =
            (sender, certificate, chain, errors) => Matches(certificate, pinned);
    }

    public static bool Matches(X509Certificate certificate, string thumbprint)
    {
        return (certificate != null) &&
            string.Equals(new X509Certificate2(certificate).Thumbprint, thumbprint, StringComparison.OrdinalIgnoreCase);
    }
}
'@
    }

function Test-RunnerCertificate
    {
    param(
        [Parameter(Mandatory)] [System.Security.Cryptography.X509Certificates.X509Certificate] $Certificate,
        [Parameter(Mandatory)] [string] $Thumbprint
    )

    [SolidSyslogRunnerPinning]::Matches($Certificate, $Thumbprint)
    }

# ConvertFrom-Json in Windows PowerShell 5.1 gives objects, not hashtables.
function ConvertFrom-RunnerJobJson
    {
    param([Parameter(Mandatory)] [string] $Json)

    $parsed = ConvertFrom-Json $Json
    $arguments = @{}
    foreach ($property in $parsed.args.PSObject.Properties)
        {
        $arguments[$property.Name] = [string] $property.Value
        }
    @{ id = $parsed.id; type = $parsed.type; args = $arguments }
    }

# Sends a finished job's result, given as @{ Id; Body }. Gives it back if the
# send failed, so it can be sent again; a lost result leaves the job running on
# the service.
function Send-RunnerResult
    {
    param(
        [Parameter(Mandatory)] [hashtable] $Pending,
        [Parameter(Mandatory)] [scriptblock] $Send
    )

    try
        {
        $null = & $Send $Pending.Id $Pending.Body
        $kept = $null
        }
    catch
        {
        Write-Warning "Result of job $($Pending.Id) not sent, will retry: $($_.Exception.Message)"
        $kept = $Pending
        }
    $kept
    }
