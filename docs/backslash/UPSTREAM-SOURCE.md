# Upstream source provenance

Backslash is based on the Windows Terminal source release identified below. Microsoft attribution, the MIT license and component notices are retained; Backslash changes are maintained separately.

- Official releases: https://github.com/microsoft/terminal/releases
- Release: Windows Terminal v1.25.2733.0, published 2026-10-02T20:27:27Z, prerelease=false. Verified as latest stable through the official releases page and GitHub latest-release API on preparation date.
- Release URL: https://github.com/microsoft/terminal/releases/tag/v1.25.2733.0
- Tag: v1.25.2733.0
- Annotated tag object: 643609c3f9e022c9fb44366a43f5ebb8567f9722
- Upstream commit: dc8ae365847d41f62d1ebca93815bb00959bc12f
- Official source archive: https://codeload.github.com/microsoft/terminal/zip/refs/tags/v1.25.2733.0
- Archive SHA-256: 1D2B9D701E66333D698412DBE4C8240E5239837B763E7DA79C8DEBE2B57CDAEA
- Metadata: https://api.github.com/repos/microsoft/terminal/releases/latest
- Tag resolution: https://api.github.com/repos/microsoft/terminal/git/tags/643609c3f9e022c9fb44366a43f5ebb8567f9722
- Tree inspection: https://api.github.com/repos/microsoft/terminal/git/trees/dc8ae365847d41f62d1ebca93815bb00959bc12f?recursive=1

## Dependencies and completeness

The official recursive commit tree was not truncated and contained zero gitlinks (mode 160000) and no .gitmodules. There are no pinned submodules to materialize in this release. Bundled source dependencies, oss content, attribution, LICENSE, NOTICE.md, NOTICE.html and component licenses were preserved from the archive.

The source snapshot does not include an offline build dependency bundle. NuGet dependencies are declared in the package manifests and restored during builds. vcpkg.json declares fmt 12.1.0, ms-gsl 4.2.2, jsoncpp 1.9.6, cli11 2.6.1 and cmark 0.31.1 with baseline 927f62e4b8838bd7e441e9c45103a16ffd75007e; building requires the documented Visual Studio/Windows SDK prerequisites and dependency restoration. Package-feed access is required when dependencies are not already cached.
