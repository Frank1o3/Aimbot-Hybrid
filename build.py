"""
Smart setup.py that auto-builds C++ extensions when files change.
Works on Linux and Windows.
"""

import os
import sys
import subprocess
import hashlib
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name: str, sourcedir: str = ""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    """Custom build command that uses CMake"""

    def run(self):
        # Check if CMake is installed
        try:
            subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError("CMake must be installed to build extensions")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext: "CMakeExtension"):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        # Detect if we need to rebuild
        if not self.needs_rebuild(ext):
            print(f"✓ {ext.name} is up to date, skipping build")
            return

        print(f"Building {ext.name}...")

        # CMake config
        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
        ]

        # Build type
        cfg = "Debug" if self.debug else "Release"
        build_args = ["--config", cfg]

        # Platform-specific settings
        if sys.platform.startswith("win"):
            # Windows
            cmake_args += [
                f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{cfg.upper()}={extdir}",
            ]
            if sys.maxsize > 2**32:
                cmake_args += ["-A", "x64"]
            build_args += ["--", "/m"]
        else:
            # Linux/Mac
            cmake_args += [f"-DCMAKE_BUILD_TYPE={cfg}"]
            import multiprocessing

            build_args += ["--", f"-j{multiprocessing.cpu_count()}"]

        # Build directory
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)

        # Run CMake
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp
        )

        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, cwd=self.build_temp
        )

        # Save build hash
        self.save_build_hash(ext)
        print(f"✓ {ext.name} built successfully")

    def needs_rebuild(self, ext: "CMakeExtension") -> bool:
        """Check if C++ sources have changed since last build"""
        cpp_dir = Path(ext.sourcedir)

        # If no built library exists, rebuild
        ext_path = Path(self.get_ext_fullpath(ext.name))
        if not ext_path.exists():
            return True

        # Calculate hash of all C++ files
        current_hash = self.calculate_cpp_hash(cpp_dir)

        # Compare with saved hash
        hash_file = Path(self.build_temp) / ".cpp_hash"
        if not hash_file.exists():
            return True

        saved_hash = hash_file.read_text().strip()
        return current_hash != saved_hash

    def calculate_cpp_hash(self, cpp_dir: Path):
        """Calculate MD5 hash of all C++ source files"""
        hasher = hashlib.md5()

        # Include all .cpp, .hpp, .h files
        for pattern in ["**/*.cpp", "**/*.hpp", "**/*.h"]:
            for filepath in sorted(cpp_dir.glob(pattern)):
                if filepath.is_file():
                    hasher.update(filepath.read_bytes())

        # Also include CMakeLists.txt
        for cmake_file in cpp_dir.glob("**/CMakeLists.txt"):
            hasher.update(cmake_file.read_bytes())

        return hasher.hexdigest()

    def save_build_hash(self, ext: "CMakeExtension"):
        """Save hash of C++ files after successful build"""
        cpp_dir = Path(ext.sourcedir)
        current_hash = self.calculate_cpp_hash(cpp_dir)

        hash_file = Path(self.build_temp) / ".cpp_hash"
        hash_file.parent.mkdir(parents=True, exist_ok=True)
        hash_file.write_text(current_hash)


if __name__ == "__main__":
    setup(
        name="aimbot",
        version="0.1.0",
        ext_modules=[CMakeExtension("xwayland_capture", sourcedir="cpp")],
        cmdclass={"build_ext": CMakeBuild},
        zip_safe=False
    )