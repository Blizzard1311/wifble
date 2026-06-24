# Tencent Cloud Deployment Steps

This updates the existing `cat1-ingest` service in place.

## 1. Replace `server.py`

On the server, overwrite:

- `/home/ubuntu/cat1-ingest/server.py`

with:

- [server.py](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/server/cat1_ingest/server.py)

## 2. Restart the service

```bash
sudo systemctl restart cat1-ingest
sudo systemctl status cat1-ingest --no-pager
```

Expected:

- `active (running)`

## 3. Add nginx routes

In your existing server block, add:

- `location /api/m5/cat1/latest`
- `location /api/m5/cat1/download`
- `location /m5/cat1`

Reference snippet:

- [nginx.snippet.conf](/Users/Kenneth/Documents/M5%20STACK/apps/heat-remote/server/cat1_ingest/nginx.snippet.conf)

Then run:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

## 4. Verify

```bash
curl -s http://127.0.0.1:3012/health
curl -s 'http://127.0.0.1:3012/api/m5/cat1/latest?limit=5'
curl -s -D - 'http://127.0.0.1:3012/api/m5/cat1/download?format=csv' | sed -n '1,12p'
curl -s -D - 'http://127.0.0.1:3012/m5/cat1' | sed -n '1,8p'
```

Public verification:

```bash
curl -s 'http://124.223.4.88/api/m5/cat1/latest?limit=5'
curl -s -D - 'http://124.223.4.88/api/m5/cat1/download?format=csv' | sed -n '1,12p'
curl -s -D - 'http://124.223.4.88/m5/cat1' | sed -n '1,8p'
```

## 5. What the page provides

- latest upload list
- recent heat summary
- device summary
- simple trend visualization
- JSONL download
- CSV download
