# hashmonke
A hashing APE

Hashmonke is a terminal UI tool for verifying file hashes.

Some file sharers distribute archives that include the MD5/SHA of the files inside.
Non-technical users appreciate having a GUI tool that can test and identify which files fail to match their provided hash.
Many file sharers still rely on [QuickSFV](https://www.quicksfv.org/), which is Windows XP era software.

Hashmonke aims to replace QuickSFV for the above usecase. Goals:
- small binary size
- higher performance
- hardware maximization
- user friendliness
- portability

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
Hashmonke is an [actually portable executable (APE)](https://justine.lol/ape.html). It should run on a variety of OS/CPUs, making it suitable for distribution inside a file archive.

For hashing, we use files adapted from [AWS-LC](https://github.com/aws/aws-lc), which includes assembly level optimizations for common hashing functions. To maximize the hardware ability, Hashmonke adds threads one at a time until the total hash rate ceases to increase

TUI provided by [termbox2](https://github.com/termbox/termbox2). A TUI is preferred for a small cross platform GUI interface.
