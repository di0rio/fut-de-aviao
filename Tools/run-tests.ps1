<#
.SYNOPSIS
	Builds and runs the standalone pure-C++ test suites under Tools/.

.DESCRIPTION
	Cada suite mora em Tools\<Nome>Tests\<Nome>Tests.cpp e compila junto com
	Source\FutebolAviao\Flight\<Nome>.cpp. As classes puras (FuelSystem,
	FlightPhysics, e o que vier depois) nao tem nenhum header da Unreal, entao
	dao pra compilar e rodar fora do editor com o cl.exe do Build Tools.

	NOTA: a linha "'vswhere.exe' is not recognized as an internal or external
	command" e ruido esperado do vcvars64.bat nesta maquina (aqui so tem o
	Build Tools instalado, nao o Visual Studio completo com vswhere.exe) e NAO
	indica falha. O script so reporta falha de verdade se a compilacao de
	alguma suite falhar ou se algum assert de teste disparar (exit code
	diferente de zero do .exe da suite).
#>

$RepoRoot = Split-Path -Parent $PSScriptRoot
$ToolsDir = $PSScriptRoot
$FlightDir = Join-Path $RepoRoot "Source\FutebolAviao\Flight"
$VcVars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"

if (-not (Test-Path $VcVars))
{
	Write-Host "vcvars64.bat nao encontrado em: $VcVars" -ForegroundColor Red
	exit 1
}

# Descoberta das suites: toda pasta Tools\<Nome>Tests\ que contenha um
# <Nome>Tests.cpp casado com Source\FutebolAviao\Flight\<Nome>.cpp entra na
# lista automaticamente - uma terceira suite so precisa seguir essa
# convencao de nomes, sem tocar neste script.
$Suffix = "Tests"
$Suites = @()
$OrphanFound = $false

Get-ChildItem -Path $ToolsDir -Directory | Where-Object { $_.Name.EndsWith($Suffix) } | ForEach-Object {
	$Name = $_.Name.Substring(0, $_.Name.Length - $Suffix.Length)
	$TestCpp = Join-Path $_.FullName "$Name$Suffix.cpp"
	$SourceCpp = Join-Path $FlightDir "$Name.cpp"

	if ((Test-Path $TestCpp) -and (Test-Path $SourceCpp))
	{
		$Suites += [PSCustomObject]@{
			Name = $Name
			Dir = $_.FullName
			TestCpp = $TestCpp
			SourceCpp = $SourceCpp
		}
	}
	elseif (Test-Path $TestCpp)
	{
		# Existe o teste mas nao o par em Source\Flight - suite orfa. Isso NAO
		# pode ser um skip silencioso: uma suite parar de rodar (fonte
		# renomeada/apagada) e pior do que nunca ter existido, porque da falsa
		# confianca. Reporta por nome e derruba o exit code.
		Write-Host "ORFAO: $TestCpp existe mas $SourceCpp nao foi encontrado - suite '$Name' NAO sera executada" -ForegroundColor Red
		$OrphanFound = $true
	}
	else
	{
		Write-Host "Ignorando $($_.Name): nao achei o par $TestCpp / $SourceCpp" -ForegroundColor Yellow
	}
}

if ($Suites.Count -eq 0 -and -not $OrphanFound)
{
	Write-Host "Nenhuma suite encontrada em $ToolsDir" -ForegroundColor Red
	exit 1
}

Write-Host "Suites encontradas: $($Suites.Name -join ', ')"

$Results = @()
$AnyFailed = $false

foreach ($Suite in $Suites)
{
	Write-Host ""
	Write-Host "=== $($Suite.Name) ==="

	$ExePath = Join-Path $Suite.Dir "$($Suite.Name)Tests.exe"

	# vcvars64.bat e cl.exe precisam rodar dentro do MESMO processo cmd pra
	# herdar as variaveis de ambiente x64 que o vcvarsall.bat monta - por
	# isso tudo vai num unico "cmd /c" encadeado com &&, igual ao comando na
	# mao que este script substitui.
	$CompileCmd = "`"$VcVars`" && cd /d `"$RepoRoot`" && cl /nologo /EHsc /std:c++17 /Fe:`"$ExePath`" `"$($Suite.TestCpp)`" `"$($Suite.SourceCpp)`""
	cmd /c $CompileCmd
	$CompileExitCode = $LASTEXITCODE

	if ($CompileExitCode -ne 0)
	{
		Write-Host "$($Suite.Name): FALHOU AO COMPILAR (exit $CompileExitCode)" -ForegroundColor Red
		$Results += [PSCustomObject]@{ Name = $Suite.Name; Status = "COMPILE FAILED" }
		$AnyFailed = $true
		continue
	}

	& $ExePath
	$RunExitCode = $LASTEXITCODE

	if ($RunExitCode -ne 0)
	{
		Write-Host "$($Suite.Name): TESTES FALHARAM (exit $RunExitCode)" -ForegroundColor Red
		$Results += [PSCustomObject]@{ Name = $Suite.Name; Status = "TESTS FAILED" }
		$AnyFailed = $true
	}
	else
	{
		Write-Host "$($Suite.Name): PASSOU" -ForegroundColor Green
		$Results += [PSCustomObject]@{ Name = $Suite.Name; Status = "PASSED" }
	}
}

# Limpeza dos .obj que o cl.exe solta no diretorio de trabalho (o /Fe so
# controla o destino do .exe, nao do .obj).
Get-ChildItem -Path $RepoRoot -Filter "*.obj" -File -ErrorAction SilentlyContinue | Remove-Item -Force

Write-Host ""
Write-Host "=== Resumo ==="
foreach ($Result in $Results)
{
	$Color = if ($Result.Status -eq "PASSED") { "Green" } else { "Red" }
	Write-Host "$($Result.Name): $($Result.Status)" -ForegroundColor $Color
}

if ($OrphanFound)
{
	Write-Host ""
	Write-Host "Encontrada suite orfa (teste sem fonte pareado) - ver mensagens ORFAO acima." -ForegroundColor Red
	exit 1
}

if ($AnyFailed)
{
	exit 1
}

exit 0
