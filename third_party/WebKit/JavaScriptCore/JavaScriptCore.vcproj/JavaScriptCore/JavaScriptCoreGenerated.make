all:
    -xcopy /y/d/e/i "..\..\..\WebKitLibraries\win\tools" "$(WEBKITLIBRARIESDIR)\tools"
    set PATH=%PATH%;%SystemDrive%\cygwin\bin
    touch "$(WEBKITOUTPUTDIR)\buildfailed"
    bash build-generated-files.sh "$(WEBKITOUTPUTDIR)" "$(WEBKITLIBRARIESDIR)"
    -mkdir 2>NUL "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\APICast.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JavaScript.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSBase.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSContextRef.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSObjectRef.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSStringRef.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSStringRefCF.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSStringRefBSTR.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSValueRef.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JavaScriptCore.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    xcopy /y /d "..\..\API\JSRetainPtr.h" "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    -del "$(WEBKITOUTPUTDIR)\buildfailed"

clean:
    -del "$(WEBKITOUTPUTDIR)\buildfailed"
    -del /s /q "$(WEBKITOUTPUTDIR)\include\JavaScriptCore\JavaScriptCore"
    -del /s /q "$(WEBKITOUTPUTDIR)\obj\JavaScriptCore\DerivedSources"
