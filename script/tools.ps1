Enter-PSSession -computer  192.168.20.60 -Credential dev

Invoke-Command -ComputerName  192.168.20.60 -Credential dev -FilePath C:\Users\TD\Source\Doodle\script\install_rpc_service.ps1

Exit-PSSession

New-Item -Path "D:\Doodle\data" -Name "dubuxiaoyao" -ItemType "SymbolicLink" -Value "\\192.168.10.250\public\DuBuXiaoYao_3"