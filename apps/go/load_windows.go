package main

import "syscall"

func loadLibrary(file string) (uintptr, error) {
	handle, err := syscall.LoadLibrary(file)
	return uintptr(handle), err
}
