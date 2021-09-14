docker-compose.exe -f "docker-compose.mysql.yml" up -d --build
&"C:\Program Files\Docker\Docker\DockerCli.exe" -SwitchDaemon
docker-compose.exe -f "docker-compose.rpc.yml" up -d --build
&"C:\Program Files\Docker\Docker\DockerCli.exe" -SwitchDaemon