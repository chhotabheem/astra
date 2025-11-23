# Astra

## Building the Project

### Build Everything
```bash
cd /app/astra
cmake -B build -S .
cmake --build build
```

### Build Specific Target
```bash
cd /app/astra
cmake --build build --target logger
cmake --build build --target jsonformatter
cmake --build build --target httpserver
cmake --build build --target mongoclient
cmake --build build --target server
```

## Running Tests

### All Tests
```bash
cd /app/astra/build
ctest --output-on-failure
```

### Specific Module Tests
```bash
cd /app/astra/build/logger
ctest --output-on-failure

cd /app/astra/build/jsonformatter
ctest --output-on-failure
```