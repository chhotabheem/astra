# JsonFormatter

## Building

Build from project root:
```bash
cd /app/astra
cmake -B build -S .
cmake --build build --target jsonformatter
```

## Running Tests

```bash
cd /app/astra/build/jsonformatter
ctest --output-on-failure
```
