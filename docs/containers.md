# Container Images

## Images in use

| Image | Tag | Used by |
|---|---|---|
| `ghcr.io/davidcozens/cpputest` | `sha-18f19e1` | devcontainer (`gcc` service), all CI jobs except clang |
| `ghcr.io/davidcozens/cpputest-clang` | `sha-7eac3ab` | `clang` compose service, `build-linux-clang` CI job, `analyze-iwyu` CI job |
| `balabit/syslog-ng` | `latest` | `syslog-ng` service — BDD test oracle |
| `ghcr.io/davidcozens/behave` | `sha-3faff14` | `behave` service — Debian trixie + Python 3.12 + Behave for BDD scenarios |

## Docker Compose setup

The devcontainer uses Docker Compose (`.devcontainer/docker-compose.yml`).
VS Code connects to the `gcc` service (GCC). The `clang` service is on-demand only —
it starts when you explicitly run a command against it and stops when done.

The `syslog-ng` and `behave` services support BDD testing. The `gcc` service depends on
`syslog-ng`, so it starts automatically with the devcontainer. The `behave` service is
on-demand — it runs when BDD scenarios are executed. See [BDD testing](bdd.md) for details.

As cross-compilation targets are added, each gets its own service in the compose file,
following the same pattern.

## Running the clang build locally

From a host terminal (not inside the devcontainer):

```bash
docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --preset clang-debug

docker compose -f .devcontainer/docker-compose.yml run --rm clang \
    cmake --build --preset clang-debug --target junit
```

## Updating an image

When a new image tag is available:

1. Build and push the new image in the container image repo
2. Update the SHA tag in all files that reference it (see table below), plus `docs/containers.md`
3. Rebuild the devcontainer (`Ctrl+Shift+P` → "Dev Containers: Rebuild Container") and verify locally
4. Raise a PR — use `chore: bump container image to <sha>` as the title

| Image | Files to update |
|---|---|
| `cpputest` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml` |
| `cpputest-clang` | `.devcontainer/docker-compose.yml`, `.github/workflows/ci.yml` |
| `behave` | `.devcontainer/docker-compose.yml`, `ci/docker-compose.bdd.yml` |

All references to a given image must use the same tag. Never update one without the others.

## Switching to a different container as the devcontainer

Each service in `docker-compose.yml` sets a `BUILD_PRESET` environment variable that
VS Code tasks pick up automatically. This means a single change — the `service` in
`.devcontainer/devcontainer.json` — is all that is needed to switch environments.
Ctrl+Shift+B and all other tasks will use the correct preset for that container.

To work interactively in a different container:

1. In `.devcontainer/devcontainer.json`, change `"service": "gcc"` to the target service name
2. Rebuild the devcontainer (`Ctrl+Shift+P` → "Dev Containers: Rebuild Container")
3. Work normally — tasks adapt automatically via `BUILD_PRESET`

When done, revert `"service"` back to `"gcc"` and rebuild again.

| Service | Use case | `BUILD_PRESET` |
|---|---|---|
| `gcc` | Primary C/C++ development (default) | `debug` |
| `clang` | Clang-specific debugging / portability | `clang-debug` |
| `behave` | BDD scenario development (Python + Behave) | (none — cmake skipped) |
