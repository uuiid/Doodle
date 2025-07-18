sudo apt-get update 

sudo update-alternatives --install /usr/bin/python python /usr/bin/python3.10.12 1

sudo apt-get install --no-install-recommends -q -y \
    bzip2 \
    build-essential \
    ffmpeg \
    git \
    gcc \
    nginx \
    postgresql \
    postgresql-client \
    python3 \
    python3-dev \
    python3-pip \
    python3-venv \
    libjpeg-dev \
    libpq-dev \
    redis-server \
    software-properties-common \
    supervisor \
    wget

sudo apt-get clean
sudo rm -rf /var/lib/apt/lists/*
sudo useradd --home /opt/zou zou 
sudo mkdir /opt/zou
sudo mkdir /opt/zou/backups
sudo chown zou: /opt/zou/backups

sudo python3 -m venv /opt/zou/env
sudo /opt/zou/env/bin/python -m pip install --upgrade pip
sudo /opt/zou/env/bin/python -m pip install zou

sudo mkdir /opt/zou/previews
sudo chown -R zou:www-data /opt/zou/previews

sudo mkdir /opt/zou/tmp
sudo chown -R zou:www-data /opt/zou/tmp

sudo -u postgres psql -c 'create database zoudb;' -U postgres

sudo -u postgres psql

#\password postgres

source /opt/zou/env/bin/activate
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou init-db
sudo nano /etc/sysctl.conf
vm.overcommit_memory = 1

sudo useradd meilisearch 
echo "deb [trusted=yes] https://apt.fury.io/meilisearch/ /" | sudo tee /etc/apt/sources.list.d/fury.list
sudo apt update && sudo apt install meilisearch

sudo mkdir /opt/meilisearch
sudo chown -R meilisearch: /opt/meilisearch
sudo nano /etc/systemd/system/meilisearch.service

"
[Unit]
Description=Meilisearch search engine
After=network.target

[Service]
User=meilisearch
Group=meilisearch
WorkingDirectory=/opt/meilisearch
ExecStart=/usr/bin/meilisearch --master-key="QrcnaU9ZXCDJYjrt" --http-addr "127.0.0.1:7700"

[Install]
WantedBy=multi-user.target
"

sudo service meilisearch start

sudo mkdir /etc/zou

sudo nano /etc/zou/gunicorn.conf
"
accesslog = "/opt/zou/logs/gunicorn_access.log"
errorlog = "/opt/zou/logs/gunicorn_error.log"
workers = 13
threads = 8
worker_connections = 10000
worker_class = "gevent"
"

sudo mkdir /opt/zou/logs
sudo chown zou: /opt/zou/logs

sudo nano /etc/systemd/system/zou.service

"
[Unit]
Description=Gunicorn instance to serve the Zou API
After=network.target

[Service]
User=zou
Group=www-data
WorkingDirectory=/opt/zou
# Append DB_USERNAME=username DB_HOST=server when default values aren't used
# ffmpeg must be in PATH
Environment="DB_PASSWORD=euQVpMXeFz8k0A3lD0aj"
Environment="SECRET_KEY=22T0iwSHK7qkhdI6"
Environment="PATH=/opt/zou/env/bin:/usr/bin"
Environment="PREVIEW_FOLDER=/opt/zou/previews"
Environment="TMP_DIR=/opt/zou/tmp"
Environment="INDEXER_KEY=QrcnaU9ZXCDJYjrt"
Environment="ENABLE_JOB_QUEUE=True"

Environment="MAIL_SERVER=smtp.163.com"
Environment="MAIL_USERNAME=19975298467@163.com"
Environment="MAIL_PASSWORD=WIGOMKIGPOKFJKEB"
Environment="MAIL_USE_TLS=True"
Environment="MAIL_DEFAULT_SENDER=19975298467@163.com"
Environment="DOMAIN_NAME=192.168.40.181"
Environment="DOMAIN_PROTOCOL=http"

Environment="USER_LIMIT=400"

Environment="DEFAULT_LOCALE=zh"
Environment="DEFAULT_TIMEZONE=Asia/Shanghai"

RuntimeDirectory=zou
ExecStart=/opt/zou/env/bin/gunicorn -p /run/zou/zou.pid -c /etc/zou/gunicorn.conf -b 127.0.0.1:5000 zou.app:app

[Install]
WantedBy=multi-user.target
"

sudo nano /etc/zou/gunicorn-events.conf
"
accesslog = "/opt/zou/logs/gunicorn_events_access.log"
errorlog = "/opt/zou/logs/gunicorn_events_error.log"
workers = 1
worker_class = "geventwebsocket.gunicorn.workers.GeventWebSocketWorker"
"
sudo nano /etc/systemd/system/zou-events.service
"
[Unit]
Description=Gunicorn instance to serve the Zou Events API
After=network.target

[Service]
User=zou
Group=www-data
WorkingDirectory=/opt/zou
# Append DB_USERNAME=username DB_HOST=server when default values aren't used
Environment="PATH=/opt/zou/env/bin"
Environment="SECRET_KEY=22T0iwSHK7qkhdI6" # Same one than zou.service

RuntimeDirectory=zou
ExecStart=/opt/zou/env/bin/gunicorn -p /run/zou/zou-events.pid -c /etc/zou/gunicorn-events.conf -b 127.0.0.1:5001 zou.event_stream:app

[Install]
WantedBy=multi-user.target
"
sudo nano /etc/nginx/sites-available/zou
"
server {
    listen 80;
    listen 443 ssl;
    server_name 192.168.20.69;

    ssl_certificate     /opt/ssl/cert.crt;  
    ssl_certificate_key  /opt/ssl/cert.key;

    location /api {
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_pass http://localhost:5000/;
        client_max_body_size 500M;
        proxy_connect_timeout 600s;
        proxy_send_timeout 600s;
        proxy_read_timeout 600s;
        send_timeout 600s;
    }

    location /socket.io {
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_pass http://localhost:5001;
    }

    # location /api/file_exists {
    #     proxy_pass http://192.168.40.181:50021;
    # }
    #
    # location /api/doodle {
    #     proxy_pass http://192.168.40.181:50025;
    # }


    # location / {
    #     autoindex on;
    #     root  /opt/kitsu/dist;
    #     try_files $uri $uri/ /index.html;
    # }
}

"
sudo rm /etc/nginx/sites-enabled/default
sudo ln -s /etc/nginx/sites-available/zou /etc/nginx/sites-enabled

"创建ssl证书"
sudo mkdir /opt/ssl
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /opt/ssl/cert.key -out /opt/ssl/cert.crt
"调整权限"
sudo chown -R zou:www-data /opt/ssl


"启动队列"
sudo nano /etc/systemd/system/zou-jobs.service

"
[Unit]
Description=RQ Job queue to run asynchronous job from Zou
After=network.target

[Service]
User=zou
Group=www-data
WorkingDirectory=/opt/zou
Environment="DB_PASSWORD=euQVpMXeFz8k0A3lD0aj"
Environment="SECRET_KEY=22T0iwSHK7qkhdI6"
Environment="PATH=/opt/zou/env/bin:/usr/bin"
Environment="PREVIEW_FOLDER=/opt/zou/previews"
# Environment="FS_BACKEND=s3"
# Environment="FS_BUCKET_PREFIX=prefix"
# Environment="FS_S3_REGION=region"
# Environment="FS_S3_ENDPOINT=https://endpoint.url"
# Environment="FS_S3_ACCESS_KEY=XXX"
# Environment="FS_S3_SECRET_KEY=XXX"
ExecStart=/opt/zou/env/bin/rq worker -c zou.job_settings 

[Install]
WantedBy=multi-user.target

"


"轮换日志"

sudo nano /etc/logrotate.d/zou

"
/opt/zou/logs/gunicorn_access.log {
    daily
    missingok
    rotate 14
    notifempty
    nocompress
    size 100M
    create 644 zou zou
    postrotate
        kill -USR1 `cat /run/zou/zou.pid`
    endscript
}

/opt/zou/logs/gunicorn_error.log {
    daily
    missingok
    rotate 14
    nocompress
    size 100M
    notifempty
    create 644 zou zou
    postrotate
        kill -USR1 `cat /run/zou/zou.pid`
    endscript
}

/opt/zou/logs/gunicorn_events_access.log {
    daily
    missingok
    rotate 14
    nocompress
    size 100M
    notifempty
    create 644 zou zou
    postrotate
        kill -USR1 `cat /run/zou/zou-events.pid`
    endscript
}

/opt/zou/logs/gunicorn_events_error.log {
    daily
    missingok
    rotate 14
    nocompress
    size 100M
    notifempty
    create 644 zou zou
    postrotate
        kill -USR1 `cat /run/zou/zou-events.pid`
    endscript
}

"

logrotate /etc/logrotate.d/zou --debug

sudo systemctl enable zou-jobs
sudo service zou-jobs start


sudo systemctl enable zou
sudo systemctl enable zou-events
sudo systemctl start zou
sudo systemctl start zou-events
sudo systemctl restart zou-events
sudo systemctl restart nginx



"部署 Kitsu"

cd /opt/
sudo git clone -b build https://github.com/cgwire/kitsu
cd kitsu
git config --global --add safe.directory /opt/kitsu
sudo git config --global --add safe.directory /opt/kitsu
sudo git checkout build

初始化: 
cd /opt/zou/
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou create-admin --password 8jO6sJm5EYAZSuZ7wy3P 957714080@qq.com
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou init-data







sudo systemctl status zou
sudo systemctl status zou-events
sudo systemctl status zou-jobs
sudo systemctl status nginx
sudo systemctl status meilisearch


sudo systemctl restart meilisearch

更新界面

export http_proxy="socks5://192.168.20.89:10810/"&&export https_proxy="socks5://192.168.20.89:10810/"
export http_proxy="http://192.168.20.89:10810/"&&export https_proxy="http://192.168.20.89:10810/"

sudo git clone --branch master_sy https://github.com/uuiid/kitsu.git
git fetch origin master_sy && git checkout origin/master_sy
sudo cp -TRv /home/auto_light/dist /opt/kitsu/dist

更新node
cd /opt/test
curl -sL https://deb.nodesource.com/setup_20.x | sudo -E bash -

curl -sL https://deb.nodesource.com/setup_20.x -o /tmp/nodesource_setup.sh
sudo bash /tmp/nodesource_setup.sh

sudo apt-get install nodejs -y
sudo apt install nodejs -y



更新核心
sudo /opt/zou/env/bin/python -m pip install --index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple --upgrade zou
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou upgrade-db
sudo systemctl restart zou&&sudo systemctl restart zou-events

打开pg控制
sudo nano /etc/postgresql/14/main/postgresql.conf
listen_addresses = '*'                  # what IP address(es) to listen on;
sudo ufw allow 5432
sudo nano /etc/postgresql/14/main/pg_hba.conf
host    all             all             192.168.20.89/32         md5
sudo service postgresql restart

重置索引器
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj INDEXER_KEY=QrcnaU9ZXCDJYjrt /opt/zou/env/bin/zou reset-search-index




"迁移数据库"

source /opt/zou/env/bin/activate


DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou clear-db
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou reset-migrations
DB_PASSWORD=euQVpMXeFz8k0A3lD0aj /opt/zou/env/bin/zou upgrade-db

DB_PASSWORD=euQVpMXeFz8k0A3lD0aj \
SYNC_LOGIN="957714080@qq.com" \
SYNC_PASSWORD="8jO6sJm5EYAZSuZ7wy3P" \
zou sync-full --source http://192.168.40.182/api

DB_PASSWORD=euQVpMXeFz8k0A3lD0aj \
SYNC_LOGIN="957714080@qq.com" \
SYNC_PASSWORD="8jO6sJm5EYAZSuZ7wy3P" \
zou sync-full --source http://192.168.40.182/api --no-projects

DB_PASSWORD=euQVpMXeFz8k0A3lD0aj \
SYNC_LOGIN="957714080@qq.com" \
SYNC_PASSWORD="8jO6sJm5EYAZSuZ7wy3P" \
zou sync-full --source http://192.168.40.182/api --only-projects

DB_PASSWORD=euQVpMXeFz8k0A3lD0aj \
SYNC_LOGIN="957714080@qq.com" \
SYNC_PASSWORD="8jO6sJm5EYAZSuZ7wy3P" \
zou sync-full --source http://192.168.40.182/api --project AwesomeProject

DB_PASSWORD=euQVpMXeFz8k0A3lD0aj \
SYNC_LOGIN="957714080@qq.com" \
SYNC_PASSWORD="8jO6sJm5EYAZSuZ7wy3P" \
zou sync-full-files --source http://192.168.40.182/api


sudo cp -TRv /home/auto_light/previews /opt/zou/previews