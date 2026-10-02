# Pester 3.4 tests for VxWorks64Runner.ps1, the runner's job dispatch.
# Written for Windows PowerShell 5.1:
#   Invoke-Pester -Script Bdd\Targets\VxWorks64\Runner\VxWorks64Runner.Tests.ps1

. (Join-Path $PSScriptRoot 'VxWorks64Runner.ps1')

Describe 'Invoke-RunnerJob' {
    $script:called = $false
    $actions = @{ 'build' = { param($Arguments) $script:called = $true; @{ Outcome = 'succeeded'; Summary = '' } } }

    It 'refuses a job of an unknown type, and runs nothing' {
        $script:called = $false
        $result = Invoke-RunnerJob -Job @{ type = 'format-disk'; args = @{} } -Actions $actions
        $result.Outcome | Should Be 'refused'
        $script:called | Should Be $false
    }
}
