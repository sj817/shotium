use libloading::Library;
use serde_json::json;
use std::{
    env,
    error::Error,
    ffi::{c_char, c_void, CString},
    fs,
    mem::ManuallyDrop,
    path::PathBuf,
    ptr, slice,
};

type Handle = *mut c_void;
struct Api {
    _library: ManuallyDrop<Library>, // Keep mapped through native TLS teardown.
    create: unsafe extern "C" fn(*const c_char, *mut Handle, *mut Handle) -> i32,
    capture:
        unsafe extern "C" fn(Handle, *const c_char, *mut Handle, *mut Handle, *mut Handle) -> i32,
    destroy: unsafe extern "C" fn(Handle),
    data: unsafe extern "C" fn(Handle) -> *const u8,
    size: unsafe extern "C" fn(Handle) -> usize,
    free: unsafe extern "C" fn(Handle),
}
struct Buffer<'a>(&'a Api, Handle);
impl Buffer<'_> {
    fn bytes(&self) -> &[u8] {
        if self.1.is_null() {
            return &[];
        }
        // ABI 3 guarantees the pointer and length until this buffer is freed.
        unsafe {
            let size = (self.0.size)(self.1);
            if size == 0 {
                &[]
            } else {
                slice::from_raw_parts((self.0.data)(self.1), size)
            }
        }
    }
    fn text(&self) -> String {
        String::from_utf8_lossy(self.bytes()).trim_end_matches('\0').into()
    }
}
impl Drop for Buffer<'_> {
    fn drop(&mut self) {
        unsafe { (self.0.free)(self.1) }
    }
}
struct Engine<'a>(&'a Api, Handle);
impl Drop for Engine<'_> {
    fn drop(&mut self) {
        unsafe { (self.0.destroy)(self.1) }
    }
}

fn run() -> Result<(), Box<dyn Error>> {
    let args: Vec<_> = env::args_os().collect();
    if args.len() != 4 {
        return Err("usage: cargo run -- <library-dir> <input.html> <output.png>".into());
    }
    let directory = PathBuf::from(&args[1]).canonicalize()?;
    // Do not canonicalize input: a missing file should demonstrate capture errors/stats.
    let input = std::path::absolute(PathBuf::from(&args[2]))?;
    let name = if cfg!(windows) {
        "shotium.dll"
    } else if cfg!(target_os = "macos") {
        "libshotium.dylib"
    } else {
        "libshotium.so"
    };
    // Only a trusted, architecture-matching Release library should be loaded.
    let api = unsafe {
        let library = ManuallyDrop::new(Library::new(directory.join(name))?);
        let abi = library.get::<unsafe extern "C" fn() -> i32>(b"shot_abi_version\0")?;
        let version = abi();
        if version != 3 {
            return Err(format!("C ABI mismatch: expected 3, got {version}").into());
        }
        Api {
            create: *library.get(b"shot_engine_create\0")?,
            capture: *library.get(b"shot_engine_capture\0")?,
            destroy: *library.get(b"shot_engine_destroy\0")?,
            data: *library.get(b"shot_buffer_data\0")?,
            size: *library.get(b"shot_buffer_size\0")?,
            free: *library.get(b"shot_buffer_free\0")?,
            _library: library,
        }
    };
    let options = CString::new(json!({"resourceDir": directory}).to_string())?;
    let (mut handle, mut error) = (ptr::null_mut(), ptr::null_mut());
    let status = unsafe { (api.create)(options.as_ptr(), &mut handle, &mut error) };
    let error = Buffer(&api, error);
    if status != 0 {
        return Err(format!("create failed ({status}): {}", error.text()).into());
    }
    let engine = Engine(&api, handle);
    let request = CString::new(
        json!({"file": input, "allowFileAccess": true, "width": 720, "height": 380, "type": "png"})
            .to_string(),
    )?;
    let (mut image, mut stats, mut error) = (ptr::null_mut(), ptr::null_mut(), ptr::null_mut());
    let status =
        unsafe { (api.capture)(engine.1, request.as_ptr(), &mut image, &mut stats, &mut error) };
    let (image, stats, error) = (Buffer(&api, image), Buffer(&api, stats), Buffer(&api, error));
    if !stats.1.is_null() {
        println!("{}", stats.text());
    }
    if status != 0 {
        return Err(format!("capture failed ({status}): {}", error.text()).into());
    }
    fs::write(&args[3], image.bytes())?;
    println!("Wrote {} ({} bytes)", PathBuf::from(&args[3]).display(), image.bytes().len());
    Ok(())
}
fn main() {
    if let Err(error) = run() {
        eprintln!("{error}");
        std::process::exit(1);
    }
}
