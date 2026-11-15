// CDDA Fork Launcher.exe: starts Launcher\cdda-launcher.ps1 with no console window.
// It exists so the launcher has a proper .exe with the launcher icon (Launcher\launcher.ico).
// Build it with build-exe.ps1 (uses the C# compiler that comes with Windows).
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Text;
using System.Windows.Forms;

[assembly: AssemblyTitle("CDDA Fork Launcher")]
[assembly: AssemblyProduct("CDDA Fork Launcher")]
[assembly: AssemblyDescription("Starts the CDDA Fork Launcher (Launcher\\cdda-launcher.ps1)")]
[assembly: AssemblyVersion("1.0.0.0")]
[assembly: AssemblyFileVersion("1.0.0.0")]

static class LauncherStarter
{
    [STAThread]
    static int Main(string[] args)
    {
        string root = AppDomain.CurrentDomain.BaseDirectory;
        string script = Path.Combine(root, "Launcher", "cdda-launcher.ps1");
        if (!File.Exists(script))
        {
            MessageBox.Show("Couldn't find the launcher script:\n\n" + script +
                "\n\nKeep this .exe next to the Launcher folder.", "CDDA Fork Launcher",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }

        string system = Environment.GetFolderPath(Environment.SpecialFolder.System);
        string powershell = Path.Combine(system, @"WindowsPowerShell\v1.0\powershell.exe");
        if (!File.Exists(powershell)) { powershell = "powershell.exe"; }

        var cmd = new StringBuilder();
        cmd.Append("-NoProfile -ExecutionPolicy Bypass -STA -WindowStyle Hidden -File ").Append(Quote(script));
        foreach (string a in args) { cmd.Append(' ').Append(Quote(a)); }

        var psi = new ProcessStartInfo(powershell, cmd.ToString());
        psi.UseShellExecute = false;
        psi.CreateNoWindow = true;
        psi.WorkingDirectory = root;
        try
        {
            Process.Start(psi);
        }
        catch (Exception e)
        {
            MessageBox.Show("Couldn't start PowerShell:\n\n" + e.Message, "CDDA Fork Launcher",
                MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        return 0;
    }

    // Quotes one argument for the Windows command line (handles spaces and trailing backslashes).
    static string Quote(string s)
    {
        if (s.Length > 0 && s.IndexOfAny(new[] { ' ', '\t', '"' }) < 0) { return s; }
        var b = new StringBuilder("\"");
        int slashes = 0;
        foreach (char c in s)
        {
            if (c == '\\') { slashes++; continue; }
            if (c == '"') { b.Append('\\', slashes * 2 + 1); }
            else { b.Append('\\', slashes); }
            slashes = 0;
            b.Append(c);
        }
        b.Append('\\', slashes * 2).Append('"');
        return b.ToString();
    }
}
