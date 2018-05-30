@echo off
set RUSTFLAGS=-C target-feature=+crt-static
cargo build --release
