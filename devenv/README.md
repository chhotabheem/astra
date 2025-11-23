# Astra Development Environment

## Build the Docker Image
```bash
docker build --network=host -t astrabuilder:v1 -f Dockerfile .
```

## Run the Container
```bash
docker run -it --name astra --dns 8.8.8.8 --dns 8.8.4.4 -v $(pwd):/app/astra astrabuilder:v1 bash
```

## Fix DNS (If Needed)
If you're already inside the container and need to fix DNS:
```bash
echo "nameserver 8.8.8.8" > /etc/resolv.conf
echo "nameserver 8.8.4.4" >> /etc/resolv.conf
```
