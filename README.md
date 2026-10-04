# hashmonke
A high-performance file hash verification tool for Windows.

Hashmonke is a terminal UI tool for verifying file hashes.

Some file sharers distribute archives that include the MD5/SHA of the files inside.
Non-technical users appreciate having a tool that can test and identify which files fail to match their provided hash.
Many file sharers still rely on [QuickSFV](https://www.quicksfv.org/), which is Windows XP era software.

Hashmonke aims to replace QuickSFV for the above usecase. Goals:
- small binary size
- higher performance
- hardware maximization
- user friendliness
- modern Windows integration

## Features

### Formats

|format name|example|
|---|---|
|.sfv|`file.zip 1A2B3C4D`|
|.md5 (GNU)|`d41d8cd98f00b204e9800998ecf8427e *file.zip`|
|BSD|`MD5 (file.zip) = d41d8cd98f00b204e9800998ecf8427e`|

### Algorithms
- SFV CRC32
- MD5
- SHA1

## Technical Details
Hashmonke is built for 64-bit Windows, targeting the modern `x86-64-v2` microarchitecture level.

For hashing, we use algorithms adapted from [AWS-LC](https://github.com/aws/aws-lc) with assembly-level optimizations for common hashing functions. To maximize hardware utilization, Hashmonke dynamically scales worker threads to optimize hashing throughput.

TUI is rendered using the Windows Console API.

## Building

Requires CMake and Visual Studio (MSVC) with C/C++ tools.

```pwsh
cmake -B build
cmake --build build --config Release
```

Run tests:
```pwsh
ctest --test-dir build -C Release --output-on-failure
```
