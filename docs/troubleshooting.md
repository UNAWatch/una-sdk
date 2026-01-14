# Troubleshooting

Find solutions to common issues encountered during Una-Watch development.

## Build Issues

### Doxygen XML Parsing Error
**Problem**: `doxygenindex: Unable to parse xml file`
**Solution**: This is often caused by invalid characters (e.g., non-ASCII comments) or syntax errors in header files. Ensure all headers are UTF-8 encoded and free of Cyrillic or other special characters in comments.

### Missing `cbor.h`
**Problem**: Compiler error stating `cbor.h` not found.
**Solution**: Ensure you have initialized and updated submodules. The TinyCBOR library is required for CBOR operations.

## Deployment Issues

### OTA Update Fails
**Problem**: Update reaches 100% but the device does not reboot into the new version.
**Solution**: Check the CRC of the `.uapp` file. The kernel validates the signature and integrity before applying updates. Ensure the app version is higher than the currently installed version.

## Runtime Issues

### App Crashes on Start
**Problem**: App terminates immediately after launching.
**Solution**: Use the `ILogger` interface to check for error messages. Common causes include:
- Requesting a sensor that isn't present.
- Memory allocation failure (check `getHeapFree()`).
- Stack overflow in the Service process.

### UI is Unresponsive
**Problem**: Touch events aren't being registered.
**Solution**: Ensure your GUI process isn't blocking the main thread. All heavy processing should be moved to the Service process.

---

*Still having trouble? Check the [Community & Support](community-support.md) page.*
