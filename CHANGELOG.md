# Changelog

All notable changes to this project will be documented in this file.

## [2.1.3] - 2025-09-17

### 🔧 Fixed
- **Robust partial write handling** - StdPipe now properly handles large data transfers (multi-GB) with automatic retry logic, preventing data loss
- **Null pointer protection** - Added comprehensive parameter validation returning ERROR_INVALID_PARAMETER for safer API usage
- **Memory leak prevention** - Enhanced RAII handle management with UniqueHandle wrapper

### ⚡ Improved  
- **Performance** - Fixed incorrect std::move usage in Read() method
- **Simplified API** - Streamlined argument handling from std::vector to std::string
- **Reliability** - Added 49 comprehensive tests including 1MB stress testing

### 📦 Packaging
- **Modern NuGet support** - Replaced deprecated CoApp with standard NuGet toolchain and MSBuild integration

## [2.1.2] - Previous Release
- Bug fixes and stability improvements

---

**Full backward compatibility maintained** - No breaking changes, drop-in replacement for v2.1.2.