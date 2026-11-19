# Resolve the current user's Saved Games known folder (including redirection).
# Used only by Makefile.win at build time, never by the Rogue executable.
# Emits ONE UTF-8 path with forward slashes, no BOM, followed by a newline.
param([switch]$Create)
$ErrorActionPreference = 'Stop'
try {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;

public static class RogueWindowsKnownFolder {
    [DllImport("ole32.dll")]
    private static extern int CoInitializeEx(IntPtr pvReserved, uint dwCoInit);
    [DllImport("ole32.dll")]
    private static extern void CoUninitialize();
    [DllImport("ole32.dll")]
    private static extern void CoTaskMemFree(IntPtr pv);
    [DllImport("shell32.dll", ExactSpelling = true)]
    private static extern int SHGetKnownFolderPath(
        ref Guid rfid, uint dwFlags, IntPtr hToken, out IntPtr ppszPath);

    public static string GetSavedGamesPath() {
        Guid id = new Guid("4C5C32FF-BB9D-43B0-B5B4-2D72E54EAAA4");
        int init = CoInitializeEx(IntPtr.Zero, 2);
        if (init < 0 && init != unchecked((int)0x80010106))
            Marshal.ThrowExceptionForHR(init);
        IntPtr path = IntPtr.Zero;
        try {
            int hr = SHGetKnownFolderPath(ref id, 0x00004000, IntPtr.Zero, out path);
            Marshal.ThrowExceptionForHR(hr);
            if (path == IntPtr.Zero)
                throw new InvalidOperationException("Saved Games has no path.");
            return Marshal.PtrToStringUni(path);
        }
        finally {
            if (path != IntPtr.Zero) CoTaskMemFree(path);
            if (init >= 0) CoUninitialize();
        }
    }
}
'@
    $savedGames = [RogueWindowsKnownFolder]::GetSavedGamesPath()
    if ([string]::IsNullOrWhiteSpace($savedGames) -or
        -not [System.IO.Path]::IsPathRooted($savedGames)) {
        throw 'Windows returned an empty or non-absolute Saved Games path.'
    }
    $result = [System.IO.Path]::Combine($savedGames, 'Rogue').Replace('\', '/')
    if ($Create) {
        [System.IO.Directory]::CreateDirectory($result) | Out-Null
        exit 0
    }
    $bytes = ([System.Text.UTF8Encoding]::new($false)).GetBytes($result + "`n")
    $stdout = [Console]::OpenStandardOutput()
    $stdout.Write($bytes, 0, $bytes.Length)
    $stdout.Flush()
}
catch {
    [Console]::Error.WriteLine('Cannot resolve Windows Saved Games: ' + $_.Exception.Message)
    exit 1
}
