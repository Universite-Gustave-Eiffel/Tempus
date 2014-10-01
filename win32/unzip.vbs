strZipFile = WScript.Arguments(0)                                      'name of zip file
outFolder = WScript.Arguments(1)                                       'destination folder of unzipped files
    
Set objShell = CreateObject( "Shell.Application" )
Set objSource = objShell.NameSpace(strZipFile).Items()
Set objTarget = objShell.NameSpace(outFolder)
intOptions = 256 'option 256 displays a progress dialog box but does not show the file names
objTarget.CopyHere objSource, intOptions