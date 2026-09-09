//go:build linux || darwin

package main

import "github.com/ebitengine/purego"

func loadLibrary(file string) (uintptr, error) {
	return purego.Dlopen(file, purego.RTLD_NOW|purego.RTLD_LOCAL)
}
