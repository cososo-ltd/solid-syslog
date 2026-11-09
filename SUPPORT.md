# Support

Thanks for using SolidSyslog. Here is where to get help.

## Documentation first

- **Integration guides and API reference** live in [`docs/`](docs/) and on the
  [published documentation site](https://docs.cososo.co.uk/solid-syslog/).
- **The public headers** under `Core/Interface/` are the API boundary and are
  documented at the declaration.

## Questions, help, and bugs

- **Have a usage or integration question?** Ask in
  [Discussions Q&A](https://github.com/cososo-ltd/solid-syslog/discussions/categories/q-a).
  Ideas and show-and-tell posts are welcome in
  [Discussions](https://github.com/cososo-ltd/solid-syslog/discussions) too.
- **Found a bug or want a concrete feature?** Open an issue using the templates -
  a clear, reproducible report is the fastest route to a fix.
- Please search existing discussions and issues first - someone may already have
  asked.

## Upstream versions

Each platform pack builds against a third-party RTOS, network stack, file
system or crypto library. Supported means built and tested in CI against that
version. The pack's page under `docs/platforms/` states the minimum it needs
and the version it is tested against, and the documentation build holds that
version to the record each CI run leaves of what it compiled against.

From 1.0.0 every release follows the latest patch or minor release of each
upstream within the major version the pack supports. Such a bump is routine
and is taken as it appears. A minimum rises when the latest upstream release
requires it, and such a rise is listed in the release notes under *Before you
upgrade*; it is not a breaking change of the library's own API and does not by
itself change the major version
([release process](docs/release-process.md#what-is-a-breaking-change)).

A new major version of an upstream is platform work, not a bump. It is taken
up as an epic of its own, open to votes like any platform epic, and lands
either as a raised minimum or as a second pack where the existing pack must
keep serving the older major. Until it lands, the pack states the major it
supports. The free tier carries no date for it; a commercial maintenance
agreement is where dates live.

A pack is not supported past its upstream's own end of support. When an
upstream line leaves support, the pack that tracks it is retired at the
library's next major version.

A pack whose name carries a release of its platform applies the same policy
within that release, following its latest patch rather than a later release.

An older upstream version is supported only on an explicit requirement; raise
one in Discussions.

## Security issues

Do **not** report security vulnerabilities in public issues. Follow the
private reporting process in [SECURITY.md](SECURITY.md).

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the contribution model, including
how to get in touch about Platform-adapter contributions.

## Commercial support

Free-tier support is best-effort. For commercial licensing, guaranteed
response times, or paid support, enquire via the form at
[cososo.co.uk](https://www.cososo.co.uk/?service=solidsyslog#contact).
