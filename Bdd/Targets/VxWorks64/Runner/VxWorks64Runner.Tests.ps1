# Pester 3.4 tests for VxWorks64Runner.ps1, the runner's job dispatch.
# Written for Windows PowerShell 5.1:
#   Invoke-Pester -Script Bdd\Targets\VxWorks64\Runner\VxWorks64Runner.Tests.ps1

. (Join-Path $PSScriptRoot 'VxWorks64Runner.ps1')

Describe 'Test-RunnerCertificate' {
    # A public certificate only; its key was discarded when it was made.
    $certificate = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2(
        (Join-Path $PSScriptRoot 'TestData\pinning-test-certificate.pem'))
    $pinned = '789D71BF084F7F1BE52573811974F3BA93F3B9B9'

    It 'accepts the certificate whose thumbprint is pinned, in either case' {
        Test-RunnerCertificate -Certificate $certificate -Thumbprint $pinned | Should Be $true
        Test-RunnerCertificate -Certificate $certificate -Thumbprint $pinned.ToLowerInvariant() | Should Be $true
    }

    It 'rejects a certificate whose thumbprint is not pinned' {
        Test-RunnerCertificate -Certificate $certificate -Thumbprint '0000000000000000000000000000000000000000' | Should Be $false
    }
}

Describe 'Invoke-RunnerJob' {
    $script:called = $false
    $actions = @{ 'build' = @{ Arguments = @(); Run = { param($Arguments) $script:called = $true; @{ Outcome = 'succeeded'; Summary = '' } } } }

    It 'refuses a job of an unknown type, and runs nothing' {
        $script:called = $false
        $result = Invoke-RunnerJob -Job @{ type = 'format-disk'; args = @{} } -Actions $actions
        $result.Outcome | Should Be 'refused'
        $script:called | Should Be $false
    }

    It 'refuses a job with an argument its action does not take, and runs nothing' {
        $script:called = $false
        $result = Invoke-RunnerJob -Job @{ type = 'build'; args = @{ script = 'del *' } } -Actions $actions
        $result.Outcome | Should Be 'refused'
        $script:called | Should Be $false
    }

    It 'refuses a job missing an argument its action needs, and runs nothing' {
        $script:called = $false
        $checkout = @{ 'checkout' = @{ Arguments = @('ref'); Run = { param($Arguments) $script:called = $true } } }
        $result = Invoke-RunnerJob -Job @{ type = 'checkout'; args = @{} } -Actions $checkout
        $result.Outcome | Should Be 'refused'
        $script:called | Should Be $false
    }

    It 'refuses an argument value that could be read as an option or quoting, and runs nothing' {
        $checkout = @{ 'checkout' = @{ Arguments = @('ref'); Run = { param($Arguments) $script:called = $true } } }
        foreach ($value in @('--upload-pack=evil', 'main"; del *', "main'", 'main;x', ''))
            {
            $script:called = $false
            $result = Invoke-RunnerJob -Job @{ type = 'checkout'; args = @{ ref = $value } } -Actions $checkout
            $result.Outcome | Should Be 'refused'
            $script:called | Should Be $false
            }
    }

    It 'passes ordinary branch names, commits and markers to the action' {
        $checkout = @{ 'checkout' = @{ Arguments = @('ref'); Run = { param($Arguments) @{ Outcome = 'succeeded'; Summary = $Arguments.ref } } } }
        foreach ($value in @('feat/s41.03-vxworks-udp', '0cfb2e56', 'SolidSyslog VxWorks 6.4 BDD target: Core ran'))
            {
            $result = Invoke-RunnerJob -Job @{ type = 'checkout'; args = @{ ref = $value } } -Actions $checkout
            $result.Summary | Should Be $value
            }
    }

    It 'reports an action that throws as failed, with the error' {
        $result = Invoke-RunnerJob -Job @{ type = 'build'; args = @{} } -Actions @{
            'build' = @{ Arguments = @(); Run = { param($Arguments) throw 'No project at C:\x' } } }
        $result.Outcome | Should Be 'failed'
        $result.Summary | Should Be 'No project at C:\x'
    }

    It 'runs a known job and returns its result' {
        $result = Invoke-RunnerJob -Job @{ type = 'build'; args = @{} } -Actions @{
            'build' = @{ Arguments = @(); Run = { param($Arguments) @{ Outcome = 'succeeded'; Summary = 'Diagnostics: none' } } } }
        $result.Outcome | Should Be 'succeeded'
        $result.Summary | Should Be 'Diagnostics: none'
    }
}
