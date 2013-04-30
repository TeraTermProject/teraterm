
#
# Export Tera Term Menu registry to ini file.
#   with PowerShell
#
# Usage:
# PS>.\ttpmenu.ps1 > ttpmenu.ini
#

$TTMREG = "HKCU:\Software\ShinpeiTools\TTermMenu"
$TTMFILE = "ttpmenu.ini"



Function PrintEntry
{
	Param($obj)
	
	begin {
#		%{"{0}={1,8:x8}" -f $prop, $val}
	}
	
	process {
		$type = $obj.value.GetType().name;
#		Write-Host "$type"
		if ($type -eq "Int32" -or $type -eq "UInt32") {
			Write-Output ("{0}={1:x8}" -f $obj.property, $obj.value)
		} elseif ($type -eq "Byte[]") {
			Write-Output ("{0}={1}" -f $obj.property, [BitConverter]::ToString($obj.value).Replace("-", " "))
		} else {
			Write-Output ("{0}={1}" -f $obj.property, $obj.value)
		}
	}
	
	end {
	}
}


Function ExportIniFile
{
	Param(
	[Parameter(Mandatory=$true)]
	[string]$path)

	Push-Location
	Set-Location $path
	
	$s = Get-ItemProperty $path
	Write-Output ("[{0}]" -f $s.PSChildName);
	
	$hash = @{};
	$obj;

	Get-Item . |
	Select-Object -ExpandProperty property |
	ForEach-Object {
		New-Object psobject -Property @{
			"property"=$_;
	    	"value" = (Get-ItemProperty -Path . -Name $_).$_
    	}
	} |
	ForEach-Object {
		PrintEntry($_)
	}
#	%{"{0}={1,8:x8}" -f $_.property, $_.value}
#	Format-Table property, value -AutoSize
	Pop-Location
	
	Write-Output  "";
}


Function Main
{
	# 最初の設定情報を出力する
	ExportIniFile($TTMREG);


	# 各ホストの設定を出力する
	Push-Location
	$rootitem = Get-ItemProperty $TTMREG
	Set-Location $rootitem.PSPath;
	$items = Get-ChildItem . | ForEach-Object {Get-ItemProperty $_.PSPath};
	ForEach ($item in $items)
	{
	#	Write-Host ("{0}" -f $item.PSPath);
		ExportIniFile($item.PSPath);
	}
	Pop-Location
}


Main


