' Rewrite <PlatformToolset> in build\*.vcxproj using UTF-8.
' Usage: cscript //nologo apply_platform_toolset.vbs v145 [buildDir]
Option Explicit

Dim fso, folder, file, text, toolset, buildDir, re, changed, count, path

If WScript.Arguments.Count < 1 Then
  WScript.Echo "Usage: apply_platform_toolset.vbs <toolset> [buildDir]"
  WScript.Quit 1
End If

toolset = WScript.Arguments(0)
If WScript.Arguments.Count >= 2 Then
  buildDir = WScript.Arguments(1)
Else
  buildDir = "build"
End If

Set fso = CreateObject("Scripting.FileSystemObject")
If Not fso.FolderExists(buildDir) Then
  WScript.Echo "Build directory not found: " & buildDir
  WScript.Quit 1
End If

Set re = New RegExp
re.Global = True
re.IgnoreCase = True
re.Pattern = "<PlatformToolset>[^<]*</PlatformToolset>"

count = 0
Set folder = fso.GetFolder(buildDir)
For Each file In folder.Files
  If LCase(fso.GetExtensionName(file.Name)) = "vcxproj" Then
    path = file.Path
    text = ReadTextFile(path)
    changed = re.Replace(text, "<PlatformToolset>" & toolset & "</PlatformToolset>")
    If changed <> text Then
      WriteTextFile path, changed
      count = count + 1
    End If
  End If
Next

WScript.Echo "Rewrote PlatformToolset=" & toolset & " in " & count & " vcxproj file(s)."
WScript.Quit 0

Function ReadTextFile(filePath)
  Dim stream
  Set stream = CreateObject("ADODB.Stream")
  stream.Type = 2
  stream.Charset = "utf-8"
  stream.Open
  stream.LoadFromFile filePath
  ReadTextFile = stream.ReadText
  stream.Close
End Function

Sub WriteTextFile(filePath, content)
  Dim stream
  Set stream = CreateObject("ADODB.Stream")
  stream.Type = 2
  stream.Charset = "utf-8"
  stream.Open
  stream.WriteText content
  stream.SaveToFile filePath, 2 ' adSaveCreateOverWrite
  stream.Close
End Sub