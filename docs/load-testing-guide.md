# HTTP/2 Load Testing Guide

## Before You Begin

**Requirements:**
- Docker container running with `tools/Dockerfile`

**Architecture:**
```
h2load → uri_shortener:8081 → Http2ClientPool → nginx:8080
```

---

## Quick Start

### Step 1: Start the stub backend

```bash
./tools/loadtest/setup-loadtest.sh
```

Expected output: nginx starts silently. Verify with:
```bash
curl http://localhost:8080/health   # Returns: {"status":"ok"}
```

### Step 2: Build the application

```bash
cmake --build --preset clang-release --target uri_shortener
```

### Step 3: Start the application

```bash
./build/clang-release/bin/uri_shortener tools/loadtest/uri-shortener-config.json
```

Verify with:
```bash
curl --http2-prior-knowledge http://localhost:8081/health   # Returns: {"status":"ok"}
```

### Step 4: Run load test

```bash
h2load -D120 -c10 --rps 1000 \
  -d tools/loadtest/shorten-request.json \
  -H "Content-Type: application/json" \
  http://localhost:8081/shorten
```

---

## Reference

### h2load Options

| Option | Description | Example |
|--------|-------------|---------|
| `-n` | Total requests | `-n1000` |
| `-c` | Concurrent clients | `-c10` |
| `-D` | Duration (seconds) | `-D60` |
| `--rps` | Requests/second per client | `--rps 1000` |
| `-d` | Request body file | `-d payload.json` |

### Benchmark Results

Tested with 10 clients × 1000 RPS for 2 minutes:

| Run | Requests | Success | RPS | Latency (mean) |
|-----|----------|---------|-----|----------------|
| 1 | 1,199,897 | 100% | 9,999 | 824µs |
| 2 | 1,177,486 | 100% | 9,812 | 959µs |

### Configuration Files

All files in `tools/loadtest/`:

| File | Purpose |
|------|---------|
| `nginx-stub.conf` | Stub backend (mock API responses) |
| `setup-loadtest.sh` | Starts nginx with stub config |
| `uri-shortener-config.json` | Application config |
| `shorten-request.json` | POST body for /shorten |

---

## Troubleshooting

**502/504 errors after many requests**
: nginx connection limit reached. Already fixed in `nginx-stub.conf` with `keepalive_requests 10000000`.

**Connection closed by peer**
: Http2Client auto-reconnects on EOF. No action needed.

**Low throughput**
: Use release build. Check CPU on nginx and uri_shortener.
