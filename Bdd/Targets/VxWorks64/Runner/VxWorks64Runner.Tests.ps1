# Pester 3.4 tests for VxWorks64Runner.ps1, the runner's job dispatch.
# Written for Windows PowerShell 5.1:
#   Invoke-Pester -Script Bdd\Targets\VxWorks64\Runner\VxWorks64Runner.Tests.ps1

. (Join-Path $PSScriptRoot 'VxWorks64Runner.ps1')

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

    It 'runs a known job and returns its result' {
        $result = Invoke-RunnerJob -Job @{ type = 'build'; args = @{} } -Actions @{
            'build' = @{ Arguments = @(); Run = { param($Arguments) @{ Outcome = 'succeeded'; Summary = 'Diagnostics: none' } } } }
        $result.Outcome | Should Be 'succeeded'
        $result.Summary | Should Be 'Diagnostics: none'
    }
}
