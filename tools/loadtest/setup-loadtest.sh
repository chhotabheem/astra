#!/bin/sh
d=$(dirname "$0")
cp "$d/nginx-stub.conf" /etc/nginx/sites-enabled/
rm -f /etc/nginx/sites-enabled/default
nginx
