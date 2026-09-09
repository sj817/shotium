import com.google.gson.Gson;
import com.sun.jna.*;
import com.sun.jna.ptr.PointerByReference;
import java.nio.charset.StandardCharsets;
import java.nio.file.*;
import java.util.Map;

public class ShotiumDemo {
    // size_t is pointer-sized, including 64-bit Windows where C long is 32-bit.
    public static class SizeT extends IntegerType {
        public SizeT() { super(Native.SIZE_T_SIZE, 0, true); }
    }
    public interface Api extends Library {
        int shot_abi_version();
        int shot_engine_create(String options, PointerByReference engine, PointerByReference error);
        int shot_engine_capture(Pointer engine, String request, PointerByReference image, PointerByReference stats, PointerByReference error);
        void shot_engine_destroy(Pointer engine);
        Pointer shot_buffer_data(Pointer buffer);
        SizeT shot_buffer_size(Pointer buffer);
        void shot_buffer_free(Pointer buffer);
    }
    // Keep JNA's proxy/library strongly reachable until JVM shutdown.
    private static Api api;
    private static byte[] bytes(Pointer buffer) {
        if (buffer == null) return new byte[0];
        int size = Math.toIntExact(api.shot_buffer_size(buffer).longValue());
        return size == 0 ? new byte[0] : api.shot_buffer_data(buffer).getByteArray(0, size);
    }
    private static String text(Pointer buffer) {
        return new String(bytes(buffer), StandardCharsets.UTF_8).replaceFirst("\u0000+$", "");
    }
    private static void run(String[] args) throws Exception {
        if (args.length != 3) throw new IllegalArgumentException("usage: ShotiumDemo <library-dir> <input.html> <output.png>");
        Path directory = Path.of(args[0]).toAbsolutePath().normalize();
        String name = Platform.isWindows() ? "shotium.dll" : Platform.isMac() ? "libshotium.dylib" : "libshotium.so";
        api = Native.load(directory.resolve(name).toString(), Api.class, Map.of(Library.OPTION_STRING_ENCODING, "UTF-8"));
        int version = api.shot_abi_version();
        if (version != 3) throw new IllegalStateException("C ABI mismatch: expected 3, got " + version);
        Gson json = new Gson();
        PointerByReference engine = new PointerByReference(), error = new PointerByReference();
        int status = api.shot_engine_create(json.toJson(Map.of("resourceDir", directory.toString())), engine, error);
        try {
            if (status != 0) throw new IllegalStateException("create failed (" + status + "): " + text(error.getValue()));
        } finally { api.shot_buffer_free(error.getValue()); }
        try {
            PointerByReference image = new PointerByReference(), stats = new PointerByReference();
            String request = json.toJson(Map.of("file", Path.of(args[1]).toAbsolutePath().normalize().toString(),
                "allowFileAccess", true, "width", 800, "height", 600, "type", "png"));
            status = api.shot_engine_capture(engine.getValue(), request, image, stats, error);
            try {
                if (stats.getValue() != null) System.out.println(text(stats.getValue()));
                if (status != 0) throw new IllegalStateException("capture failed (" + status + "): " + text(error.getValue()));
                byte[] data = bytes(image.getValue());
                Files.write(Path.of(args[2]), data);
                System.out.println("Wrote " + args[2] + " (" + data.length + " bytes)");
            } finally {
                api.shot_buffer_free(image.getValue());
                api.shot_buffer_free(stats.getValue());
                api.shot_buffer_free(error.getValue());
            }
        } finally { api.shot_engine_destroy(engine.getValue()); }
    }
    public static void main(String[] args) {
        try { run(args); }
        catch (Exception error) { System.err.println(error.getMessage()); System.exit(1); }
    }
}
