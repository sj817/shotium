[CmdletBinding()]
param(
  [Parameter(Mandatory)][string]$Root,
  [Parameter(Mandatory)][ValidateRange(1, 65535)][int]$Port
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$rootPath = (Resolve-Path -LiteralPath $Root).Path.TrimEnd([IO.Path]::DirectorySeparatorChar)
$rootPrefix = $rootPath + [IO.Path]::DirectorySeparatorChar
$listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, $Port)

function Get-ContentType([string]$Path) {
  switch ([IO.Path]::GetExtension($Path).ToLowerInvariant()) {
    '.html' { 'text/html; charset=utf-8' }
    '.css' { 'text/css; charset=utf-8' }
    '.svg' { 'image/svg+xml' }
    '.png' { 'image/png' }
    '.jpg' { 'image/jpeg' }
    '.jpeg' { 'image/jpeg' }
    '.woff' { 'font/woff' }
    '.woff2' { 'font/woff2' }
    '.ttf' { 'font/ttf' }
    default { 'application/octet-stream' }
  }
}

function Write-Response {
  param(
    [Parameter(Mandatory)][IO.Stream]$Stream,
    [Parameter(Mandatory)][int]$Status,
    [Parameter(Mandatory)][string]$Reason,
    [Parameter(Mandatory)][string]$ContentType,
    [Parameter(Mandatory)][byte[]]$Body,
    [switch]$HeadOnly
  )
  $headers = "HTTP/1.1 $Status $Reason`r`nContent-Type: $ContentType`r`nContent-Length: $($Body.Length)`r`nCache-Control: no-store`r`nConnection: close`r`n`r`n"
  $headerBytes = [Text.Encoding]::ASCII.GetBytes($headers)
  $Stream.Write($headerBytes, 0, $headerBytes.Length)
  if (-not $HeadOnly -and $Body.Length) {
    $Stream.Write($Body, 0, $Body.Length)
  }
  $Stream.Flush()
}

$listener.Start()
try {
  Write-Output "LISTENING http://127.0.0.1:$Port/ root=$rootPath"
  while ($true) {
    $client = $listener.AcceptTcpClient()
    try {
      $client.ReceiveTimeout = 5000
      $client.SendTimeout = 5000
      $stream = $client.GetStream()
      $reader = [IO.StreamReader]::new($stream, [Text.Encoding]::ASCII, $false, 1024, $true)
      $requestLine = $reader.ReadLine()
      if (-not $requestLine) { continue }
      while ($true) {
        $line = $reader.ReadLine()
        if ($null -eq $line -or $line.Length -eq 0) { break }
      }

      $parts = $requestLine.Split(' ')
      if ($parts.Count -lt 2) {
        Write-Response -Stream $stream -Status 400 -Reason 'Bad Request' -ContentType 'text/plain' -Body ([Text.Encoding]::UTF8.GetBytes('bad request'))
        continue
      }
      $method = $parts[0].ToUpperInvariant()
      if ($method -notin @('GET', 'HEAD')) {
        Write-Response -Stream $stream -Status 405 -Reason 'Method Not Allowed' -ContentType 'text/plain' -Body ([Text.Encoding]::UTF8.GetBytes('method not allowed'))
        continue
      }

      $requestPath = [Uri]::UnescapeDataString($parts[1].Split('?')[0])
      if ($requestPath -eq '/__health') {
        Write-Response -Stream $stream -Status 200 -Reason 'OK' -ContentType 'text/plain' -Body ([Text.Encoding]::ASCII.GetBytes('ok')) -HeadOnly:($method -eq 'HEAD')
        continue
      }
      $relative = $requestPath.TrimStart('/').Replace('/', [IO.Path]::DirectorySeparatorChar)
      $candidate = [IO.Path]::GetFullPath((Join-Path $rootPath $relative))
      if (-not $candidate.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase) -or
          -not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        Write-Response -Stream $stream -Status 404 -Reason 'Not Found' -ContentType 'text/plain' -Body ([Text.Encoding]::UTF8.GetBytes('not found')) -HeadOnly:($method -eq 'HEAD')
        continue
      }

      $body = [IO.File]::ReadAllBytes($candidate)
      Write-Response -Stream $stream -Status 200 -Reason 'OK' -ContentType (Get-ContentType $candidate) -Body $body -HeadOnly:($method -eq 'HEAD')
    } catch {
      [Console]::Error.WriteLine($_.Exception.Message)
    } finally {
      $client.Dispose()
    }
  }
} finally {
  $listener.Stop()
}
