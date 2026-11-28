#
# Export TeraTerm Menu registry to ini file.
#   with PowerShell
#
# Usage:
# PS>.\ttpmenu.ps1 > ttpmenu.ini
#

$TTMREG = "HKCU:\Software\ShinpeiTools\TTermMenu"
$TTMFILE = "ttpmenu.ini"
$changed = 0;

Function PrintSectionName
{
	Param([string]$name)

	if ($name.Contains(']')) {
		$org = $name
		# ] を削除。] 削除によりセクション名が重複する場合がある。
		$name = $name -replace '\]', ''
		Write-Output ("[{0}] ; <- [{1}]" -f $name, $org)
		$script:changed = 1
	} else {
		Write-Output ("[{0}]" -f $name)
	}
}

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
			$bytes = [System.BitConverter]::GetBytes($obj.value)
			$long = [System.BitConverter]::ToInt32($bytes, 0)
			Write-Output ("{0}={1}" -f $obj.property, $long)
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
		[string]$path
	)

	Push-Location
	Set-Location -LiteralPath $path

	$s = Get-ItemProperty -LiteralPath $path
	PrintSectionName $s.PSChildName

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
	Set-Location -LiteralPath $rootitem.PSPath;
	Get-ChildItem . |
	ForEach-Object {
		ExportIniFile($_.PSPath);
	}
	Pop-Location

	if ($script:changed -eq 1) {
		[Console]::Error.WriteLine("; ']' has been removed. check the section name.")
	}
}

Main
