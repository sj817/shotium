// Calls the prebuilt C ABI; this is an example, not a published Go binding.
package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
	"runtime"
	"strings"
	"unsafe"

	"github.com/ebitengine/purego"
)

func run() error {
	if len(os.Args) != 4 {
		return fmt.Errorf("usage: go run . <library-dir> <input.html> <output.png>")
	}
	dir, err := filepath.Abs(os.Args[1])
	if err != nil {
		return err
	}
	input, err := filepath.Abs(os.Args[2])
	if err != nil {
		return err
	}
	name := "libshotium.so"
	if runtime.GOOS == "windows" {
		name = "shotium.dll"
	}
	if runtime.GOOS == "darwin" {
		name = "libshotium.dylib"
	}
	handle, err := loadLibrary(filepath.Join(dir, name))
	if err != nil {
		return err
	}
	// Keep the library mapped until process exit, including native TLS teardown.
	var abi func() int32
	var create func(string, *uintptr, *uintptr) int32
	var capture func(uintptr, string, *uintptr, *uintptr, *uintptr) int32
	var destroy func(uintptr)
	var data func(uintptr) unsafe.Pointer
	var size func(uintptr) uintptr
	var free func(uintptr)
	purego.RegisterLibFunc(&abi, handle, "shot_abi_version")
	purego.RegisterLibFunc(&create, handle, "shot_engine_create")
	purego.RegisterLibFunc(&capture, handle, "shot_engine_capture")
	purego.RegisterLibFunc(&destroy, handle, "shot_engine_destroy")
	purego.RegisterLibFunc(&data, handle, "shot_buffer_data")
	purego.RegisterLibFunc(&size, handle, "shot_buffer_size")
	purego.RegisterLibFunc(&free, handle, "shot_buffer_free")
	if version := abi(); version != 3 {
		return fmt.Errorf("C ABI mismatch: expected 3, got %d", version)
	}
	read := func(buffer uintptr) []byte {
		if buffer == 0 || size(buffer) == 0 {
			return nil
		}
		// Native buffers remain valid until free; copy into Go-owned memory.
		return append([]byte(nil), unsafe.Slice((*byte)(data(buffer)), int(size(buffer)))...)
	}
	options, err := json.Marshal(map[string]any{"resourceDir": dir})
	if err != nil {
		return err
	}
	var engine, failure uintptr
	status := create(string(options), &engine, &failure)
	defer free(failure)
	if status != 0 {
		return fmt.Errorf("create failed (%d): %s", status, strings.TrimRight(string(read(failure)), "\x00"))
	}
	defer destroy(engine)
	request, err := json.Marshal(map[string]any{"file": input, "allowFileAccess": true, "width": 720, "height": 380, "type": "png"})
	if err != nil {
		return err
	}
	var image, stats, captureError uintptr
	status = capture(engine, string(request), &image, &stats, &captureError)
	defer free(image)
	defer free(stats)
	defer free(captureError)
	if stats != 0 {
		fmt.Println(strings.TrimRight(string(read(stats)), "\x00"))
	}
	if status != 0 {
		return fmt.Errorf("capture failed (%d): %s", status, strings.TrimRight(string(read(captureError)), "\x00"))
	}
	bytes := read(image)
	if err := os.WriteFile(os.Args[3], bytes, 0644); err != nil {
		return err
	}
	fmt.Printf("Wrote %s (%d bytes)\n", os.Args[3], len(bytes))
	return nil
}

func main() {
	if err := run(); err != nil {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}
}
