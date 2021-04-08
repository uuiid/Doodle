$gpu = Get-WmiObject win32_VideoController

$gpu.Caption

$cpu = Get-WmiObject -Class Win32_Processor

$cpu.Name

$ip4s =  Get-NetIPAddress -AddressFamily IPv4

foreach($ip4 in $ip4s){
    $ip4.IPAddress
}
Read-Host -Prompt "Press any key to continue"