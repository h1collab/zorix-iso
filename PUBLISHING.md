# Publishing the ISO

The GitHub connector used to initialize this repository can write normal repository files, but it does not expose GitHub Release asset upload.

The ISO is 437,256,192 bytes, so it cannot be committed as a normal Git blob. Upload it as a Release asset instead.

On a machine with GitHub CLI authenticated as an account with write permission:

```bash
./scripts/publish-release.sh /path/to/assets
```

Expected ISO SHA-256:

`18eee8b5137c6eabcfeaa6f526bd9613d7f9c1eda20faf774bb4cb3d08fca793`

Do not claim the Release asset exists until GitHub reports a successful upload.
