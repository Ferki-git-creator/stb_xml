# stb_xml - Ultra-Fast Minimal XML Parser

*stb_xml* is a single-header XML parser that's 193× faster than libxml2 while being 333× smaller (1.5KB vs 500KB). It achieves extreme performance through zero-copy parsing, zero allocations, and 6-byte tokens (vs libxml2's 40+ bytes per node). Perfect for embedded systems, games, and high-performance applications where minimal memory and maximum speed are critical. All benchmark results showing 8,700 MB/sec parsing speed are available in the `output_tests.txt` file.

## If you want to support my work

[![Buy Me a Coffee](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/ferki)