# Creating a New Component Repository

## Prerequisites

### Required tools

- [VS Code](https://code.visualstudio.com/) with the [Dev Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) extension
- Git
- An SSH key configured for GitHub
- [`gh` CLI](https://cli.github.com/) — optional, only needed for the automated setup commands in steps 10–12

### Platform support

| Platform | Status | Notes |
|---|---|---|
| Windows (WSL2) | Tested | All commands must be run inside WSL, not a Windows-native shell. Docker Desktop with the WSL2 backend is required. |
| Linux (x86_64) | Should work | Docker Engine + Docker Compose plugin. No known caveats. |
| macOS (Intel) | Untested | Docker Desktop for Mac. Container images are `linux/amd64` — likely works but not verified. |
| macOS (Apple Silicon) | Not supported | Container images are `linux/amd64` only. Rosetta emulation may be unreliable for sanitizer and debugger features. |

This process has only been tested on Windows with WSL2. If you try it on another platform, feedback is welcome —
please [open an issue](https://github.com/DavidCozens/CppUTestTemplate/issues).

---

### 1. Create a new repository on GitHub

Create an empty repository on GitHub for your component (e.g. `LedDriver`).
Do not initialise it with a README or any other files.

### 2. Clone the template

```bash
git clone git@github.com:DavidCozens/CppUTestTemplate.git LedDriver
cd LedDriver
```

### 3. Point the origin remote at the new repository

```bash
git remote set-url origin git@github.com:DavidCozens/LedDriver.git
```

### 4. Add the template as a named remote

This allows template updates to be pulled in later:

```bash
git remote add template git@github.com:DavidCozens/CppUTestTemplate.git
```

Verify your remotes:

```bash
git remote -v
# origin    git@github.com:DavidCozens/LedDriver.git (fetch)
# origin    git@github.com:DavidCozens/LedDriver.git (push)
# template  git@github.com:DavidCozens/CppUTestTemplate.git (fetch)
# template  git@github.com:DavidCozens/CppUTestTemplate.git (push)
```

### 5. Push to the new repository

```bash
git push -u origin main
```

### 6. Run the init script

```bash
bash scripts/init-component.sh LedDriver
```

This will:

- Rename the CMake project to `LedDriver`
- Replace the example source, header, and test files with a `LedDriver` stub
- Replace the BDD target binary in `Bdd/Targets/` with a `LedDriver` stub
- Update the BDD feature and steps to test the new BDD target
- Update the VS Code debugger launch configuration
- Update the README title
- Update the devcontainer name and workspace path in `.devcontainer/devcontainer.json`
- Update the volume mount path and working directory in `.devcontainer/docker-compose.yml`

### 7. Open in VS Code devcontainer

Open the cloned folder in VS Code. When prompted, select **Reopen in Container**
(or use `Ctrl+Shift+P` → "Dev Containers: Reopen in Container"). This starts the
`gcc` devcontainer service and makes cmake and the build toolchain available.

### 8. Build, verify the red bar, and make it pass

Press `Ctrl+Shift+B` to build and run the tests. Expect exactly one failing test
(`LedDriver.NeedsWork`) — this confirms the build and test harness are working.

Make the test pass before continuing. This is your first TDD cycle.

### 9. Commit and push the initialised project

Do this **before** configuring branch protection — the init script changes need to
land on `main` while direct pushes are still permitted.

```bash
git add -A
git commit -m "feat: initialise LedDriver component"
git push
```

### 10. Enable GitHub Pages for coverage reports

In the GitHub repository settings, under **Pages**:

- Set **Source** to `GitHub Actions`

Without this, the `deploy-coverage-pages` job will fail on every push to `main`.
The CI check jobs (`build-linux-gcc`, `build-linux-clang`, `sanitize-linux-gcc`,
`coverage-linux-gcc`, `analyze-tidy`, `analyze-cppcheck`, `analyze-format`) are independent
of Pages and will pass regardless.

> **Note:** GitHub Pages deployment requires a public repository, or a paid GitHub plan
> (Pro, Team, or Enterprise) for private repositories. On a free plan with a private repo,
> `deploy-coverage-pages` will fail — this does not affect the required CI status checks.

Alternatively, using the `gh` CLI:

```bash
gh api repos/OWNER/REPO/pages -X POST -f build_type=workflow
```

### 11. Allow GitHub Actions to create pull requests

This is required for the Release Please workflow to open its release PR automatically.

In the GitHub repository settings, under **Actions** → **General** → **Workflow permissions**:

- Select **Read and write permissions**
- Check **Allow GitHub Actions to create and approve pull requests**

Alternatively, using the `gh` CLI:

```bash
gh api repos/OWNER/REPO/actions/permissions/workflow \
  -X PUT \
  -f default_workflow_permissions=write \
  -F can_approve_pull_request_reviews=true
```

### 12. Configure branch protection on the new repository

In the GitHub repository settings, under **Branches** → **Add branch protection rule** for `main`:

- Require a pull request before merging
- Require all status checks to pass: `build-linux-gcc`, `build-linux-clang`, `build-windows-msvc`, `sanitize-linux-gcc`, `coverage-linux-gcc`, `analyze-tidy`, `analyze-cppcheck`, `analyze-format`, `integration-linux-openssl`, `bdd-linux-syslog-ng`, `bdd-windows-otel`
- Require squash merge only
- Enable automatic branch deletion after merge

Alternatively, using the `gh` CLI:

```bash
gh api repos/OWNER/REPO/branches/main/protection \
  -X PUT \
  --input - <<'EOF'
{
  "required_status_checks": {
    "strict": false,
    "contexts": ["build-linux-gcc", "build-linux-clang", "build-windows-msvc", "sanitize-linux-gcc", "coverage-linux-gcc", "analyze-tidy", "analyze-cppcheck", "analyze-format", "integration-linux-openssl", "bdd-linux-syslog-ng", "bdd-windows-otel"]
  },
  "enforce_admins": false,
  "required_pull_request_reviews": {
    "required_approving_review_count": 0
  },
  "restrictions": null
}
EOF
```
