rem %1 argument is used for graphics. It can be cg or cairo
rem %2 argument is used for network. It can be cf or curl

mkdir 2>NUL "%WebKitOutputDir%\include\WebCore"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\bindings"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\parser"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\runtime"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\masm"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\pcre"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\profiler"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wrec"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\text"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\unicode"
mkdir 2>NUL "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\unicode\icu"

xcopy /y /d "%ProjectDir%..\config.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%WebKitOutputDir%\obj\WebCore\DerivedSources\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\accessibility\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\accessibility\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\inspector\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\loader\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\loader\appcache\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\loader\archive\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\loader\archive\cf\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\loader\icon\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\history\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\history\cf\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\html\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\notifications\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\css\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\animation\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\cf\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\graphics\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\graphics\%1\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\graphics\transforms\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\graphics\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\graphics\opentype\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\text\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\text\transcoder\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\network\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\network\%2\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\network\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\sql\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\platform\cairo\cairo\src\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\bindings\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\bindings\js\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\page\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\page\animation\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\page\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\bridge\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\bridge\jsc\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\plugins\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\plugins\win\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\rendering\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\rendering\style\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\rendering\svg\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\editing\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\dom\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\xml\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\svg\animation\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\svg\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\storage\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\websockets\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\workers\*.h" "%WebKitOutputDir%\include\WebCore"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\bindings\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\bindings"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\parser\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\parser"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\runtime\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\runtime"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\masm\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\masm"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\pcre\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\pcre"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\profiler\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\profiler"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\wrec\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wrec"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\wtf\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\wtf\text\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\text"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\wtf\unicode\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\unicode"
xcopy /y /d "%ProjectDir%..\ForwardingHeaders\wtf\unicode\icu\*.h" "%WebKitOutputDir%\include\WebCore\ForwardingHeaders\wtf\unicode\icu"

if exist "%WebKitOutputDir%\buildfailed" del "%WebKitOutputDir%\buildfailed"
