copy ..\Release\eduroam-config.exe .
"%ProgramFiles%\7-Zip\7z.exe" a archive.7z eduroam.xml globalsign.der eduroam-config.exe -mx

copy /b 7zS2.sfx + archive.7z eduroam.exe
del archive.7z