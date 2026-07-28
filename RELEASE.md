# Release

zen is distributed **via github only** — consumers install it with

    npm install github:akaoio/zen

There is no npm publish. Because a github install runs no build step,
**every generated artifact must be committed**: `zen.js`, `zen.min.js`,
`index.min.js`, and all touched `lib/*.min.js` / `*.wasm`.

## Flow

1. Rebuild artifacts and verify:

       npm run verify:release     # build:release && test:all

2. Bump `version` in `package.json`.
3. Commit everything (source + regenerated artifacts).
4. Tag with the bare version (matching existing tags like `1.0.31`):

       git tag 1.0.36
       git push origin main --tags

CI runs the test matrix on every push/PR; a pushed tag additionally
creates a github release.
