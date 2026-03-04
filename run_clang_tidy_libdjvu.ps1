Set-Location 'C:\Users\Volodymyr\DevelopNew\DjVuLibreExperimental'
$clang='c:\Users\Volodymyr\Develop\tools\llvm-mingw-20251118-ucrt-x86_64\bin\clang-tidy.exe'
$files=Get-ChildItem libdjvu -Filter *.cpp | ForEach-Object { $_.FullName }
$out='C:\Users\Volodymyr\DevelopNew\DjVuLibreExperimental\clang_tidy_libdjvu.txt'
if(Test-Path $out){Remove-Item $out -Force}
foreach($f in $files){
  "### FILE: $f" | Out-File -FilePath $out -Append -Encoding utf8
  & $clang $f --checks=-*,bugprone-*,performance-*,portability-* -- -std=c++17 -I. -Ilibdjvu 2>&1 | Out-File -FilePath $out -Append -Encoding utf8
}
'DONE'
(Get-Content $out | Measure-Object -Line).Lines
