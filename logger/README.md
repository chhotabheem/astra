# Logger

## Building

Build from project root:
```bash
cd /app/astra
cmake -B build -S .
cmake --build build --target logger
```

## Running Tests

```bash
cd /app/astra/build/logger
ctest --output-on-failure
```
