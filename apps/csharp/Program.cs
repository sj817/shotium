using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;

try
{
    if (args.Length != 3) throw new ArgumentException("usage: dotnet run -- <library-dir> <input.html> <output.png>");
    var directory = Path.GetFullPath(args[0]);
    var filename = OperatingSystem.IsWindows() ? "shotium.dll" : OperatingSystem.IsMacOS() ? "libshotium.dylib" : "libshotium.so";
    var library = NativeLibrary.Load(Path.Combine(directory, filename));
    // Keep mapped until process exit, including native TLS teardown.
    NativeLibrary.SetDllImportResolver(typeof(Shotium).Assembly, (name, _, _) => name == "shotium" ? library : IntPtr.Zero);
    var version = Shotium.shot_abi_version();
    if (version != 3) throw new InvalidOperationException($"C ABI mismatch: expected 3, got {version}");
    var status = Shotium.shot_engine_create(JsonSerializer.Serialize(new { resourceDir = directory }), out var engine, out var error);
    var message = Shotium.TakeText(error);
    if (status != 0) throw new InvalidOperationException($"create failed ({status}): {message}");
    try
    {
        var request = JsonSerializer.Serialize(new { file = Path.GetFullPath(args[1]), allowFileAccess = true, width = 720, height = 380, type = "png" });
        status = Shotium.shot_engine_capture(engine, request, out var image, out var stats, out error);
        try
        {
            if (stats != IntPtr.Zero) Console.WriteLine(Shotium.Text(stats));
            if (status != 0) throw new InvalidOperationException($"capture failed ({status}): {Shotium.Text(error)}");
            var bytes = Shotium.Bytes(image);
            File.WriteAllBytes(args[2], bytes);
            Console.WriteLine($"Wrote {args[2]} ({bytes.Length} bytes)");
        }
        finally
        {
            Shotium.shot_buffer_free(image);
            Shotium.shot_buffer_free(stats);
            Shotium.shot_buffer_free(error);
        }
    }
    finally { Shotium.shot_engine_destroy(engine); }
}
catch (Exception error) { Console.Error.WriteLine(error.Message); Environment.ExitCode = 1; }

static class Shotium
{
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int shot_abi_version();
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int shot_engine_create([MarshalAs(UnmanagedType.LPUTF8Str)] string options, out IntPtr engine, out IntPtr error);
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern int shot_engine_capture(IntPtr engine, [MarshalAs(UnmanagedType.LPUTF8Str)] string request, out IntPtr image, out IntPtr stats, out IntPtr error);
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void shot_engine_destroy(IntPtr engine);
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern IntPtr shot_buffer_data(IntPtr buffer);
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern nuint shot_buffer_size(IntPtr buffer);
    [DllImport("shotium", CallingConvention = CallingConvention.Cdecl)]
    internal static extern void shot_buffer_free(IntPtr buffer);

    internal static byte[] Bytes(IntPtr buffer)
    {
        if (buffer == IntPtr.Zero) return [];
        var bytes = new byte[checked((int)shot_buffer_size(buffer))];
        if (bytes.Length > 0) Marshal.Copy(shot_buffer_data(buffer), bytes, 0, bytes.Length);
        return bytes;
    }
    internal static string Text(IntPtr buffer) => Encoding.UTF8.GetString(Bytes(buffer)).TrimEnd('\0');
    internal static string TakeText(IntPtr buffer)
    {
        try { return Text(buffer); }
        finally { shot_buffer_free(buffer); }
    }
}
