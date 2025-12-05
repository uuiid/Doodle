
# wsl 配置 
"
# Settings apply across all Linux distros running on WSL 2
[wsl2]

networkingMode=mirrored

[experimental]

hostAddressLoopback=true
"

wsl --shutdown

sudo ln -s /etc/nginx/sites-available/doodle /etc/nginx/sites-enabled
sudo nano /etc/nginx/sites-available/doodle
"
server {
    listen 801;
    # listen 4431 ssl;
    server_name 192.168.0.181;

    # ssl_certificate     /opt/ssl/cert.crt;  
    # ssl_certificate_key  /opt/ssl/cert.key;

    location /api/ {
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_pass http://localhost:50025/api/;
        client_max_body_size 0;
        proxy_connect_timeout 6000s;
        proxy_send_timeout 6000s;
        proxy_read_timeout 6000s;
        send_timeout 6000s;
    }

    location /socket.io {
        proxy_http_version 1.1;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "Upgrade";
        proxy_pass http://localhost:50025/socket.io;
    }

    location / {
        autoindex on;
        root /mnt/d/kitsu/dist;
        try_files $uri $uri/ /index.html;
    }
}

"

"创建ssl证书"
sudo mkdir /opt/ssl
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 -keyout /opt/ssl/cert.key -out /opt/ssl/cert.crt
"调整权限"
# sudo chown -R zou:www-data /opt/ssl

nginx -s reload
sudo systemctl status nginx