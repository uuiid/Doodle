#Set-Item WSMan:\localhost\Client\TrustedHosts -Value '*'
#Set-Item WSMan:\localhost\Client\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Service\Auth\Basic -Value True
#Set-Item WSMan:\localhost\Client\AllowUnencrypted -Value True
#Set-Item WSMan:\localhost\Service\AllowUnencrypted -Value True

$Credential = New-Object -TypeName System.Management.Automation.PSCredential -ArgumentList auto_light,root
Enter-PSSession -ComputerName 192.168.40.181 -Credential $Credential -Authentication Basic