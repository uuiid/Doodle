Enter-PSSession -computer  192.168.20.60 -Credential dev

New-Item -Path "D:\Doodle\data" -Name "dubuxiaoyao" -ItemType "SymbolicLink" -Value "\\192.168.10.250\public\DuBuXiaoYao_3"